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
class AllBitrateDisplay;

template<typename T>
concept Processable = requires(T t) { t.process(std::declval<QTreeWidgetItem*>()); };

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
        RECEIVED_BITRATE,
        LOSS_ACCUMULATED,

        MEDIA_BOX,
        RTT_BOX,
        TARGET_BOX,

        MEDIA_INTERQUARTILE,
        RTT_INTERQUARTILE,
        TARGET_INTERQUARTILE,
    };

    struct Info
    {
        struct Stats
        {
            double mean = 0.;
            double variance = 0.;
            double var_coeff = 0.;
            uint64_t n = 0;
            QString name;

            explicit Stats(QString&& in_name) : name(std::move(in_name)) {}

            void update(double value) {
                ++n;
                mean += value;
                variance += (value * value);
            }

            void process(QTreeWidgetItem* root);
        };

        struct StatsLoss
        {
            int loss = 0;
            int sent = 0;
            QString name;

            explicit StatsLoss(QString&& in_name) : name(std::move(in_name)) {}

            void update(int v) {
                ++sent;
                loss += v;
            }

            void process(QTreeWidgetItem* root);
        };

        Stats rtt{"rtt"};
        Stats minrtt{"minrtt"};
        Stats target{"Target"};
        Stats media{"Media"};
        Stats rtx{"rtx"};
        Stats probing{"probing"};
        Stats total{"total"};
        Stats received{"received"};
        StatsLoss loss{"loss"};
    };

    void load_stat_line(const fs::path& p);

    template<typename T>
    bool get_stats(const fs::path& p, std::ifstream& ifs, std::vector<StatLinePoint<T>>& tab, StatKey key);

 private:

    StatsLineChart * _chart_bitrate, * _chart_rtt;
    StatsLineChartView* _chart_view_bitrate, * _chart_view_rtt;

    void init_map(StatMap& map, bool signal = true) override;

    template<Processable Stat>
    void process(QTreeWidgetItem* root, Stat& stat, const fs::path& p, StatKey key, const QList<QPointF>& points) {
        stat.process(root);
        add_point(p.c_str(), key, points);
        add_serie(p.c_str(), key);
    }

    template<Processable Stat>
    void process(const fs::path& p, StatKey key, QTreeWidgetItem* root, Stat& stat) {
        stat.process(root);
        add_serie(p.c_str(), key);
    }

    void load_average(const fs::path& path);
    void load_exp(const fs::path& path);

public:
    MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info);
    ~MedoozeDisplay() = default;

    void load(const fs::path& path) override;
    void save(const fs::path& dir) override;

    void add_to_all(const fs::path& dir, AllBitrateDisplay* all);
    void set_geometry(float ratio_w, float ratio_h);

    void on_keyboard_event(QKeyEvent* key);

signals:
    void on_loss_stats(const fs::path& path, int loss, int sent);
};

#endif // MEDOOZEDISPLAY_H
