#ifndef DISPLAYBASE_H
#define DISPLAYBASE_H

#include <QString>
#include <QMap>
#include <QChart>
#include <QLineSeries>

#include <filesystem>

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class QLineSeries;
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
    };

    using StatMap = QMap<uint8_t, std::tuple<QString, QLineSeries*, QChart*, ExpInfo>>;
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
    void add_point(const QString& path, uint8_t key, const T& point)
    {
        auto map = _path_keys[path];
        auto s = std::get<StatsKeyProperty::SERIE>(map[key]);
        *s << point;
    }

    inline void add_serie(const QString& path, uint8_t key, QAbstractAxis* x_axis = nullptr, QAbstractAxis* y_axis = nullptr)
    {
        const auto& map = _path_keys[path];
        auto* serie = std::get<StatsKeyProperty::SERIE>(map[key]);
        auto* chart = std::get<StatsKeyProperty::CHART>(map[key]);
        auto info = std::get<StatsKeyProperty::INFO>(map[key]);

        if(chart) {
            chart->addSeries(serie);

            QFont font = chart->font();
            font.setPointSize(20);

            chart->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
            chart->legend()->setFont(font);
\
            if(info.stream) {
                auto pen = serie->pen();
                pen.setStyle(Qt::DotLine);
                serie->setPen(pen);
            }

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

    static ExpInfo get_info(const fs::path& p);

    void create_legend();
    StatsLineChart * create_chart();
    StatsLineChartView * create_chart_view(QChart* chart);
    void create_serie(const fs::path&p, uint8_t key);

    virtual void init_map(StatMap& map, bool signal = true) = 0;

    void set_info(const fs::path& path);

public:
    DisplayBase(QWidget* tab, QListWidget* legend, QTreeWidget* info);

    virtual void load(const fs::path& path) = 0;
    virtual void unload(const fs::path& path);

    virtual void save(const fs::path& dir) = 0;
};

#endif // DISPLAYBASE_H
