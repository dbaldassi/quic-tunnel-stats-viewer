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
class QVBoxLayout;

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
        SERIE,
        CHART
    };

    enum ChartKey
    {
        BITRATE,
        FPS,
        NONE
    };

    QWidget     * _tab;
    QListWidget * _legend;

    StatsLineChart * _chart_bitrate, * _chart_fps;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_fps;

    using StatMap = QMap<StatKey, std::tuple<QString, QLineSeries*, ChartKey>>;
    QMap<QString, StatMap> _path_keys;

    void create_legend();
    StatsLineChart * create_chart();
    StatsLineChartView * create_chart_view(QChart* chart);
    void create_serie(const fs::path&p, StatKey key);

    template<typename T>
    void add_point(const QString& path, StatKey key, const T& point)
    {
        auto map = _path_keys[path];
        auto s = std::get<StatsKeyProperty::SERIE>(map[key]);
        *s << point;
    }

    inline void add_serie(const QString& path, StatKey key, QChart* chart)
    {
        auto map = _path_keys[path];
        chart->addSeries(std::get<StatsKeyProperty::SERIE>(map[key]));
    }

    void init_map(StatMap& map, bool signal = true);

public:
    ReceivedBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend);
    ~ReceivedBitrateDisplay() = default;

    // load bitrate.csv, quic.csv
    void load(const fs::path& path);
    void unload(const fs::path& path);
};

#endif // RECEIVEDBITRATEDISPLAY_H
