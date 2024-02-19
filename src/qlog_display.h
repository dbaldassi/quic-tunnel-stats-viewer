#ifndef QLOGDISPLAY_H
#define QLOGDISPLAY_H

#include <QObject>

#include <QString>
#include <QColor>
#include <QObject>
#include <QLineSeries>
#include <QChart>

#include <filesystem>

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class StatsLineChart;
class StatsLineChartView;
class QVBoxLayout;

class QlogDisplay : public QObject
{
    Q_OBJECT

    enum class StatKey : uint8_t
    {
        BYTES_IN_FLIGHT,
        CWND,
        RTT
    };

    enum StatsKeyProperty : uint8_t
    {
        NAME,
        COLOR,
        SERIE
    };

    QWidget     * _tab;
    QListWidget * _legend;

    StatsLineChart * _chart_bitrate, * _chart_rtt;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_rtt;

    QMap<StatKey, std::tuple<QString, QColor, QLineSeries*>> _map;
    QMap<QString, QVector<QLineSeries*>> _path_keys;

    void create_legend();
    StatsLineChart * create_chart();
    StatsLineChartView * create_chart_view(QChart* chart);
    void create_serie(const fs::path&p, StatKey key);

    template<typename T>
    void add_point(StatKey key, const T& point)
    {
        auto s = std::get<StatsKeyProperty::SERIE>(_map[key]);
        *s << point;
    }

    inline void add_serie(StatKey key, QChart* chart)
    {
        chart->addSeries(std::get<StatsKeyProperty::SERIE>(_map[key]));
    }

    void parse_mvfst(const fs::path& path);
public:
    QlogDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend);
    ~QlogDisplay() = default;

    void load(const fs::path& path);
    void unload(const fs::path& path);
};

#endif // QLOGDISPLAY_H
