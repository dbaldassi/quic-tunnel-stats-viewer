#ifndef ALL_BITRATE_H
#define ALL_BITRATE_H

#include <QObject>
#include <QVBoxLayout>

#include "display_base.h"

class AllBitrateDisplay : public QObject, public DisplayBase
{
    StatsLineChart * _chart;
    StatsLineChartView * _chart_view;

    void init_map(StatMap& map, bool signal = true) override;

    static std::vector<QColor> colors;
    static int current_color;

public:

    enum StatKey {
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

        BYTES_IN_FLIGHT,
        CWND,
        QUIC_RTT,
        QUIC_LOSS,

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
        MEDOOZE_RTT,
        FLAG,
        RTX,
        PROBING,

        MEDIA,
        MEDOOZE_LOSS,
        MINRTT,
        FBDELAY,
        TOTAL,
        RECEIVED_BITRATE,

        NUM_KEY
    };

    AllBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info);
    ~AllBitrateDisplay();

    void add_stats(const fs::path&, StatKey key, std::tuple<QString, QLineSeries*, QChart*, ExpInfo> s);

    void create_legend(const fs::path& p, bool signal = true);

    void load(const fs::path& path) override;
    void unload(const fs::path& path) override;
    void save(const fs::path& path) override;

    void set_geometry(float ratio_w, float ratio_h);
};

#endif // ALL_BITRATE_H
