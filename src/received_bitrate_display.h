#ifndef RECEIVEDBITRATEDISPLAY_H
#define RECEIVEDBITRATEDISPLAY_H

#include <QString>
#include <QColor>
#include <QObject>
#include <QLineSeries>
#include <QChart>

#include <filesystem>

#include "display_base.h"

class QWidget;
class QListWidget;
class StatsLineChart;
class StatsLineChartView;
class QVBoxLayout;
class QTreeWidget;
class AllBitrateDisplay;

namespace fs = std::filesystem;

class ReceivedBitrateDisplay : public QObject, public DisplayBase
{
    Q_OBJECT

    enum StatKey : uint8_t
    {
        // bitrate.csv
        LINK,
        BITRATE,
        FPS,
        FRAME_DROPPED,
        FRAME_DECODED,
        FRAME_KEY_DECODED,
        FRAME_RENDERED,

        // stats_line.csv
        BITRATE_AVERAGE,
        BITRATE_QUARTILE_1,
        BITRATE_QUARTILE_3,
        BITRATE_MEDIAN,
        BITRATE_INTERQUARTILE,
        BITRATE_BOX,

        FPS_AVERAGE,
        FPS_QUARTILE_1,
        FPS_QUARTILE_3,
        FPS_MEDIAN,
        FPS_INTERQUARTILE,
        FPS_BOX,

        // quic.csv
        QUIC_SENT,

        NUM_KEY
    };

    struct Info
    {
        double mean = 0.;
        double variance = 0.;
    };

    StatsLineChart * _chart_bitrate, * _chart_fps;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_fps;
    std::array<Info, StatKey::NUM_KEY> _infos_array;

    static std::vector<QColor> colors;
    int current_color = 0;

    void init_map(StatMap& map, bool signal = true) override;

    void load_exp(const fs::path& p);
    void load_stat_line(const fs::path& p);

    template<typename T>
    bool get_stats(const fs::path& p, std::ifstream& ifs, std::vector<StatLinePoint<T>>& tab, StatKey key);

public:
    ReceivedBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info_widget);
    ~ReceivedBitrateDisplay() = default;

    void add_to_all(const fs::path& dir, AllBitrateDisplay* all);

    // load bitrate.csv, quic.csv
    void load(const fs::path& path) override;

    void save(const fs::path& dir) override;
    void on_keyboard_event(QKeyEvent * event);

    void set_geometry(float ratio_w, float ratio_h);
};

#endif // RECEIVEDBITRATEDISPLAY_H
