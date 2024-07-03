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

public:
    ReceivedBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info_widget);
    ~ReceivedBitrateDisplay() = default;

    void add_to_all(const fs::path& dir, AllBitrateDisplay* all);

    // load bitrate.csv, quic.csv
    void load(const fs::path& path) override;

    void save(const fs::path& dir) override;

    void set_geometry(float ratio_w, float ratio_h);
};

#endif // RECEIVEDBITRATEDISPLAY_H
