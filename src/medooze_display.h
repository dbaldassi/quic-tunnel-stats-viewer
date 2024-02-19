#ifndef MEDOOZEDISPLAY_H
#define MEDOOZEDISPLAY_H

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


class MedoozeDisplay : public QObject
{
    Q_OBJECT

    enum class StatKey : uint8_t
    {
        // Medooze file
        FEEDBACK_TS,
        TWCC_NUM,
        FEEDBACK_NUM,
        PACKET_SIZE,
        SENT_TIME,
        RECEIVED_TS,
        DELTA_SENT,
        DELTA_RECV,
        DELTA,
        BWE,
        TARGET,
        AVAILABLE_BITRATE,
        RTT,
        FLAG,
        RTX,
        PROBING,

        // calculated
        MEDIA,
        LOSS,
        MINRTT,
        FBDELAY
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

public:
    MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend);
    ~MedoozeDisplay() = default;

    void load(const fs::path& path);
    void unload(const fs::path& path);
};

#endif // MEDOOZEDISPLAY_H
