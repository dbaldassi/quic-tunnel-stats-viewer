#ifndef MEDOOZEDISPLAY_H
#define MEDOOZEDISPLAY_H

#include <QObject>

#include <QString>
#include <QColor>
#include <QObject>
#include <QLineSeries>
#include <QChart>

#include <filesystem>

#include "display_base.h"

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class StatsLineChart;
class StatsLineChartView;
class QVBoxLayout;
class QTreeWidget;
class QTreeWidgetItem;

class MedoozeDisplay : public QObject, public DisplayBase
{
    Q_OBJECT

public:
    enum StatKey : uint8_t
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
        FBDELAY,
        TOTAL,
        RECEIVED_BITRATE
    };

private:

    struct Info
    {
        struct Stats
        {
            double mean = 0.;
            double variance = 0.;
            double var_coeff = 0.;
            uint64_t n = 0;
        };

        Stats rtt;
        Stats target;
        Stats media;
        Stats rtx;
        Stats probing;
        Stats total;
    };

    StatsLineChart * _chart_bitrate, * _chart_rtt;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_rtt;

    void init_map(StatMap& map, bool signal = true) override;
    void update_info(Info::Stats& s, double value);
    void process_info(QTreeWidgetItem * root, Info::Stats& s, const QString& name);

    void load_average(const fs::path& path);
    void load_exp(const fs::path& path);

public:
    MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info);
    ~MedoozeDisplay() = default;

    void load(const fs::path& path) override;
};

#endif // MEDOOZEDISPLAY_H
