#ifndef QLOGDISPLAY_H
#define QLOGDISPLAY_H

#include <QObject>

#include <QString>
#include <QColor>
#include <QObject>
#include <QLineSeries>
#include <QChart>

#include "display_base.h"

#include <filesystem>

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class StatsLineChart;
class StatsLineChartView;
class QVBoxLayout;
class QTreeWidget;

class QlogDisplay : public QObject, public DisplayBase
{
    Q_OBJECT

    enum StatKey : uint8_t
    {
        BYTES_IN_FLIGHT,
        CWND,
        RTT
    };

    struct Info {
        int lost;
        int sent;

        double mean_rtt;
        double variance_rtt;
    };

    StatsLineChart * _chart_bitrate, * _chart_rtt;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_rtt;

    void init_map(StatMap& map, bool signal) override;

    void add_info(const fs::path& path, const Info& info);
    void parse_mvfst(const fs::path& path);
    void parse_quicgo(const fs::path& path);
public:
    QlogDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info_widget);
    ~QlogDisplay() = default;

    void load(const fs::path& path) override;
};

#endif // QLOGDISPLAY_H
