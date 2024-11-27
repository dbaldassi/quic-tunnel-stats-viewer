#ifndef QLOGDISPLAY_H
#define QLOGDISPLAY_H

#include <QObject>

#include <QString>
#include <QColor>
#include <QObject>
#include <QLineSeries>
#include <QChart>
#include <QWidget>

#include "display_base.h"

#include <filesystem>

namespace fs = std::filesystem;

class QWidget;
class QListWidget;
class StatsLineChart;
class StatsLineChartView;
class QVBoxLayout;
class QTreeWidget;
class AllBitrateDisplay;


class DistributionWidget : public QWidget
{
public:
    explicit DistributionWidget(QLineSeries* serie, QWidget * parent = nullptr);
    ~DistributionWidget() = default;

    void closeEvent(QCloseEvent* event) override;
};

class QlogDisplay : public QObject, public DisplayBase
{
    Q_OBJECT

    enum StatKey : uint8_t
    {
        BYTES_IN_FLIGHT,
        CWND,
        RTT,
        LOSS,
        DISTRIBUTION,

        CWND_BOX,
        RTT_BOX,
        BYTES_IN_FLIGHT_BOX,

        CWND_INTERQUARTILE,
        RTT_INTERQUARTILE,
        BYTES_IN_FLIGHT_INTERQUARTILE,

        NUM_KEYS
    };

    struct Info {
        int lost = 0;
        int sent = 0;

        double mean_rtt = 0;
        double variance_rtt = 0;
    };

    StatsLineChart * _chart_bitrate, * _chart_rtt;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_rtt;

    void init_map(StatMap& map, bool signal) override;

    void add_info(const fs::path& path, const Info& info);
    void parse_mvfst(const fs::path& path);
    void parse_quicgo(const fs::path& path);

    void load_average(const fs::path& path);
    void load_exp(const fs::path& path);
    void load_stats_line(const fs::path& path);

    template<typename T>
    bool get_stats(const fs::path& p, std::ifstream& ifs, std::vector<StatLinePoint<T>>& tab, StatKey key);

public:
    QlogDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info_widget);
    ~QlogDisplay() = default;

    void load(const fs::path& path) override;
    void save(const fs::path& dir) override;

    void add_to_all(const fs::path& dir, AllBitrateDisplay* all);
    void set_geometry(float ratio_w, float ratio_h);

    void on_keyboard_event(QKeyEvent* event);

signals:

    void on_loss_stats(const fs::path& path, int loss, int sent);
};

#endif // QLOGDISPLAY_H
