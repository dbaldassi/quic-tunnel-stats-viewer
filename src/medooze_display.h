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


class MedoozeDisplay : public QObject, public DisplayBase
{
    Q_OBJECT

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
        FBDELAY
    };

    StatsLineChart * _chart_bitrate, * _chart_rtt;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_rtt;

    void init_map(StatMap& map, bool signal = true);
public:
    MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend);
    ~MedoozeDisplay() = default;

    void load(const fs::path& path) override;
};

#endif // MEDOOZEDISPLAY_H
