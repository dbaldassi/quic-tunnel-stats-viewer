#ifndef DISPLAYBASE_H
#define DISPLAYBASE_H

#include <QString>
#include <QMap>
#include <QChart>
#include <QLineSeries>
#include <QBoxPlotSeries>

#include <filesystem>
#include <fstream>
#include <optional>

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class QLineSeries;
class QSeries;
class StatsLineChart;
class StatsLineChartView;
class QTreeWidget;

class DisplayBase
{

protected:

    enum class QuicImpl {
        MVFST,
        QUICGO,
        MSQUIC,
        QUICHE,
        UDP,
        NONE
    };

    enum class CCAlgo {
        BBR,
        CUBIC,
        NEWRENO,
        COPA,
        NONE
    };

    struct ExpInfo {
        bool     stream = true;
        QuicImpl impl;
        CCAlgo   cc;
        bool editable = true;
        bool should_be_black = false;

        QString impl_str;
        QString cc_str;

        std::optional<QColor> color;
    };

    using StatMap = QMap<uint8_t, std::tuple<QString, QAbstractSeries*, QChart*, ExpInfo>>;
    QMap<QString, StatMap> _path_keys;

    QWidget     * _tab;
    QListWidget * _legend;
    QTreeWidget * _info;

    bool _display_impl = true;

    enum StatsKeyProperty : uint8_t
    {
        NAME,
        SERIE,
        CHART,
        INFO,
        NUM_KEY
    };

    template<typename T>
    double find_median(const std::vector<T>& values, int begin, int end)
    {
        int count = end - begin;
        if (count % 2) {
            return (double)values.at(count / 2 + begin);
        } else {
            qreal right = values.at(count / 2 + begin);
            qreal left = values.at(count / 2 - 1 + begin);
            return (right + left) / 2.0;
        }
    }

    template<typename T>
    void add_point(const QString& path, uint8_t key, const T& point)
    {
        auto map = _path_keys[path];
        auto s = static_cast<QLineSeries*>(std::get<StatsKeyProperty::SERIE>(map[key]));
        *s << point;
    }

    template<typename T>
    void add_point(const QString& path, uint8_t key, const QString& label, const std::vector<T>& values)
    {
        auto map = _path_keys[path];
        auto serie = static_cast<QBoxPlotSeries*>(std::get<StatsKeyProperty::SERIE>(map[key]));

        int count = values.size();
        auto box = new QBoxSet(label);
        box->setValue(QBoxSet::LowerExtreme, values.front());
        box->setValue(QBoxSet::UpperExtreme, values.back());
        box->setValue(QBoxSet::Median, find_median(values, 0, count));
        box->setValue(QBoxSet::LowerQuartile, find_median(values, 0, count / 2));
        box->setValue(QBoxSet::UpperQuartile, find_median(values, count / 2 + (count % 2), count));
        serie->append(box);
    }

    template<typename T>
    struct StatLinePoint {
        int time;
        std::vector<T> values;
    };

    template<typename T>
    bool get_csv_line(std::ifstream& ifs, std::vector<StatLinePoint<T>>& pts)
    {
        std::string line_str;

        if(!std::getline(ifs, line_str)) return false;
        std::replace(line_str.begin(), line_str.end(), ',', ' ');
        std::istringstream iss(line_str);

        iss >> pts.back().time;
        for(T val; iss >> val;) pts.back().values.push_back(val);
        std::sort(pts.back().values.begin(), pts.back().values.end());

        return true;
    }

    template<typename T>
    double get_average(const std::vector<T>& values)
    {
        T sum = std::accumulate(values.begin(), values.end(), 0);
        return (sum / (double)values.size());
    }

    template<typename T>
    double get_interquartile_average(const std::vector<T>& values)
    {
        int count = values.size();

        double first = find_median(values, 0, count / 2);
        double third = find_median(values, count / 2 + (count % 2), count);
        int n = 0;

        T sum = std::accumulate(values.begin(), values.end(), 0, [first, third, &n](T acc, T v) {
            if(v >= first && v <= third) {
                n += 1;
                return acc + v;
            }
            return acc;
        });

        return (sum / (double)n);
    }

    template<typename Serie = QLineSeries>
    void add_serie(const QString& path, uint8_t key, QAbstractAxis* x_axis = nullptr, QAbstractAxis* y_axis = nullptr)
    {
        const auto& map = _path_keys[path];
        auto* serie = static_cast<Serie*>(std::get<StatsKeyProperty::SERIE>(map[key]));
        auto* chart = std::get<StatsKeyProperty::CHART>(map[key]);
        auto info = std::get<StatsKeyProperty::INFO>(map[key]);

        if(chart) {
            chart->addSeries(serie);

            if constexpr(!std::is_same_v<Serie, QLineSeries>) {
                return;
            }

            QFont font = chart->font();
            font.setPointSize(44);
            font.setBold(true);

            chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
            chart->legend()->setFont(font);
            chart->legend()->detachFromChart();

            // qInfo() << "geo ; " << chart->geometry();
            // chart->legend()->setGeometry(chart->geometry());

            auto pen = serie->pen();
            pen.setWidth(8);
            if(info.stream) pen.setStyle(Qt::DotLine);
            if(info.color.has_value()) pen.setColor(*info.color);

            serie->setPen(pen);

            if(info.should_be_black) {
                auto pen = serie->pen();
                pen.setColor(Qt::black);
                pen.setStyle(Qt::SolidLine);
                serie->setPen(pen);
            }

            if(x_axis) {
                serie->attachAxis(x_axis);
                serie->attachAxis(y_axis);
            }
        }
    }

    template<typename Serie=QLineSeries>
    Serie* create_serie(const fs::path&p, uint8_t key)
    {
        auto serie = new Serie;

        QString name;
        QTextStream stream(&name);

        std::stringstream exp_name(p.filename().string());

        StatMap& map = _path_keys[p.c_str()];
        if(map.empty()) {
            init_map(map);
            set_info(p);
        }

        // stream << std::get<StatsKeyProperty::NAME>(map[key]);

        auto info = std::get<StatsKeyProperty::INFO>(map[key]);
        if(_display_impl && info.editable) {
            stream << info.impl_str << " " << (info.stream ? info.cc_str : "dgrams");
            serie->setName(name);
        }
        else {
            stream << std::get<StatsKeyProperty::NAME>(map[key]);
            serie->setName(name);
        }

        std::get<StatsKeyProperty::SERIE>(map[key]) = serie;

        return serie;
    }

    static ExpInfo get_info(const fs::path& p);

    void create_legend();
    StatsLineChart * create_chart();
    StatsLineChartView * create_chart_view(QChart* chart);
    // void create_serie(const fs::path&p, uint8_t key);

    virtual void init_map(StatMap& map, bool signal = true) = 0;

    void set_info(const fs::path& path);

public:
    DisplayBase(QWidget* tab, QListWidget* legend, QTreeWidget* info);

    virtual void load(const fs::path& path) = 0;
    virtual void unload(const fs::path& path);

    virtual void save(const fs::path& dir) = 0;
};

#endif // DISPLAYBASE_H
