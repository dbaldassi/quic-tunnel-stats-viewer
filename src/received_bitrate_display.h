#ifndef RECEIVEDBITRATEDISPLAY_H
#define RECEIVEDBITRATEDISPLAY_H

#include <QString>
#include <QColor>
#include <QObject>
#include <QLineSeries>
#include <QChart>

#include <filesystem>

class QWidget;
class QListWidget;
class StatsLineChart;
class StatsLineChartView;

namespace fs = std::filesystem;

class ReceivedBitrateDisplay : public QObject
{
    Q_OBJECT

    enum class StatKey : uint8_t
    {
        // bitrate.csv
        LINK,
        BITRATE,
        FPS,
        FRAME_DROPPED,
        FRAME_DECODED,
        FRAME_KEY_DECODED,
        FRAME_RENDERED,

        // quic.csv
        QUIC_SENT
    };

    enum StatsKeyProperty : uint8_t
    {
        NAME,
        COLOR,
        SERIE
    };

    QWidget     * _tab;
    QListWidget * _legend;

    StatsLineChart * _chart_bitrate, * _chart_fps;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_fps;

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
    ReceivedBitrateDisplay(QWidget* tab, QListWidget* legend);
    ~ReceivedBitrateDisplay() = default;

    // load bitrate.csv, quic.csv
    void load(const fs::path& path);
    void unload(const fs::path& path);
};

#endif // RECEIVEDBITRATEDISPLAY_H
