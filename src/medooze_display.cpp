#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>

#include "medooze_display.h"

#include "csv_reader.h"
#include "stats_line_chart.h"

MedoozeDisplay::MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info)
    : DisplayBase(tab, legend, info)
{
    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_rtt= create_chart();
    _chart_view_rtt = create_chart_view(_chart_rtt);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_rtt, 1);

    create_legend();
}

void MedoozeDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::BWE] = std::make_tuple("Bwe", nullptr, _chart_bitrate);
    map[StatKey::TARGET] = std::make_tuple("Target", nullptr, _chart_bitrate);
    map[StatKey::AVAILABLE_BITRATE] = std::make_tuple("Available bitrate", nullptr, _chart_bitrate);
    map[StatKey::RTT] = std::make_tuple("RTT", nullptr, _chart_rtt);
    map[StatKey::RTX] = std::make_tuple("RTX", nullptr, _chart_bitrate);
    map[StatKey::PROBING] = std::make_tuple("Probing", nullptr, _chart_bitrate);
    map[StatKey::MEDIA] = std::make_tuple("Media", nullptr, _chart_bitrate);
    map[StatKey::LOSS] = std::make_tuple("Loss", nullptr, _chart_rtt);
    map[StatKey::MINRTT] = std::make_tuple("Min rtt", nullptr, _chart_rtt);
    map[StatKey::TOTAL] = std::make_tuple("Total", nullptr, _chart_bitrate);
    map[StatKey::RECEIVED_BITRATE] = std::make_tuple("Received bitrate", nullptr, _chart_bitrate);
    map[StatKey::FBDELAY] = std::make_tuple("Feedback delay", nullptr, _chart_rtt);

    if(signal) {
        connect(_legend, &QListWidget::itemChanged, this, [&map](QListWidgetItem* item) -> void {
            StatKey key = static_cast<StatKey>(item->data(1).toUInt());

            auto line = std::get<StatsKeyProperty::SERIE>(map[key]);
            if(line == nullptr) return;

            if(item->checkState()) line->show();
            else line->hide();
        });
    }
}

void MedoozeDisplay::update_info(Info::Stats& s, double value)
{
    ++s.n;
    s.mean += value;
    s.variance += (value * value);
}

void MedoozeDisplay::process_info(QTreeWidgetItem * root, Info::Stats& s, const QString& name)
{
    s.mean /= s.n;
    s.variance = (s.variance / s.n) - (s.mean * s.mean);
    s.var_coeff = std::sqrt(s.variance) / s.mean;

    QTreeWidgetItem * item = new QTreeWidgetItem(root);
    item->setText(0, name);

    QTreeWidgetItem * mean_item = new QTreeWidgetItem(item);
    mean_item->setText(0, "mean");
    mean_item->setText(1, QString::number(s.mean));

    QTreeWidgetItem * variance_item = new QTreeWidgetItem(item);
    variance_item->setText(0, "variance");
    variance_item->setText(1, QString::number(s.variance));

    QTreeWidgetItem * coeff_var_item = new QTreeWidgetItem(item);
    coeff_var_item->setText(0, "variation coeff");
    coeff_var_item->setText(1, QString::number(s.var_coeff));
}

struct Accu
{
    int current_ts = -1;
    int sum = 0;
    MedoozeDisplay::StatKey key;
    static constexpr int base = 1000000;
    static constexpr int window = 200000;

    double factor = base / window;

    QList<QPointF> points;
    QList<QPoint> points_window;
    QPointF pt;

    uint64_t accumulated = 0;

    Accu(MedoozeDisplay::StatKey k) : pt{0., 0.}
    {
        key = k;

    }

    void accumulate(int time, int v)
    {
        while(!points_window.empty() && points_window.front().x() < (time  - window)) {
            accumulated -= points_window.front().y();
            points_window.pop_front();
        }

        accumulated += v;
        points_window.emplace_back(time, v);
        points.emplace_back(time/1000000., accumulated * factor / 1000.);
    }

    void add_value(double time, double v)
    {
        points.emplace_back(time, v);
    }

    void add_value(int ts, double time, double v)
    {
        if(ts != current_ts) {
            if(current_ts != -1) {
                pt.setX(pt.x() / sum);
                pt.setY(pt.y() / sum);

                points.push_back(pt);
            }

            current_ts = ts;
            sum = 0;

            pt.setX(0.);
            pt.setY(0.);
        }

        pt.setX(pt.x() + time);
        pt.setY(pt.y() + v);

        ++sum;
    }

    const QList<QPointF>& get_points()
    {
        std::sort(points.begin(), points.end(),
                  [](const auto& p1, const auto& p2) { return p1.x() < p2.x(); });

        return points;
    }
};

void MedoozeDisplay::load_exp(const fs::path& p)
{
    fs::path path;
    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{p}) {
        if(dir_entry.path().filename().string().starts_with("quic-relay-") && dir_entry.path().extension() == ".csv"
            || dir_entry.path().filename().string() == "medooze.csv") {
            path = dir_entry.path();
            break;
        }
    }

    using MedoozeReader = CsvReaderTypeRepeat<'|', int, 17>;

    // create_serie(p, StatKey::BITRATE);
    create_serie(p, StatKey::MEDIA);
    create_serie(p, StatKey::RTX);
    create_serie(p, StatKey::PROBING);
    create_serie(p, StatKey::RTT);
    create_serie(p, StatKey::MINRTT);
    create_serie(p, StatKey::TARGET);
    create_serie(p, StatKey::TOTAL);
    // create_serie(p, StatKey::RECEIVED_BITRATE);

    Info info;

    QList<Accu> accu;
    accu.emplace_back(StatKey::MEDIA);
    accu.emplace_back(StatKey::RTX);
    accu.emplace_back(StatKey::PROBING);
    accu.emplace_back(StatKey::RTT);
    accu.emplace_back(StatKey::MINRTT);
    accu.emplace_back(StatKey::TARGET);
    accu.emplace_back(StatKey::TOTAL);

    for(auto &it : MedoozeReader(path)) {
        const auto& [fb_ts, twcc_num, fb_num, packet_size, sent_time, recv_ts, delta_sent, delta_recv, delta,
                     bwe, target, available_bitrate, rtt, minrtt, flag, rtx, probing] = it;

        double timestamp = sent_time / 1000000.;

        for(auto& acc : accu) {
            switch(acc.key) {
            case StatKey::MEDIA:
                acc.accumulate(sent_time, ((rtx == 0 && probing == 0) ? packet_size * 8 : 0));
                break;
            case StatKey::RTX:
                acc.accumulate(sent_time, ((rtx == 1 && probing == 0) ? packet_size * 8 : 0));
                break;
            case StatKey::PROBING:
                acc.accumulate(sent_time, ((rtx == 0 && probing == 1) ? packet_size * 8 : 0));
                break;
            case StatKey::RTT:
                acc.add_value(fb_ts, timestamp, rtt);
                break;
            case StatKey::MINRTT:
                acc.add_value(fb_ts, timestamp, ((minrtt == 0 || rtt < minrtt) ? rtt : minrtt));
                break;
            case StatKey::TARGET:
                acc.add_value(fb_ts, timestamp, target);
                break;
            case StatKey::TOTAL:
                acc.accumulate(sent_time, packet_size * 8);
                break;
            default:
                break;
            }
        }

        /*update_info(info.media, point.y());
        update_info(info.rtx, point.y());
        update_info(info.probing, point.y());
        update_info(info.total, point.y());
        update_info(info.rtt, rtt);*/
    }

    for(auto& acc : accu) {
        add_point(p.c_str(), acc.key, acc.get_points());
    }

    QTreeWidgetItem * item = new QTreeWidgetItem(_info);
    item->setText(0, path.parent_path().filename().c_str());

    process_info(item, info.media, "Media");
    process_info(item, info.rtx, "Rtx");
    process_info(item, info.probing, "Probing");
    process_info(item, info.total, "Total");
    process_info(item, info.rtt, "Rtt");

    add_serie(p.c_str(), StatKey::MEDIA);
    add_serie(p.c_str(), StatKey::RTX);
    add_serie(p.c_str(), StatKey::PROBING);
    add_serie(p.c_str(), StatKey::TOTAL);
    add_serie(p.c_str(), StatKey::RTT);
    add_serie(p.c_str(), StatKey::MINRTT);
    // add_serie(p.c_str(), StatKey::TARGET);
    // add_serie(p.c_str(), StatKey::RECEIVED_BITRATE);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}

void MedoozeDisplay::load_average(const fs::path& p)
{
    fs::path medooze_file = p / "medooze.csv";

    using MedoozeReader = CsvReaderTypeRepeat<',', double, 9>;

    create_serie(p, StatKey::MEDIA);
    create_serie(p, StatKey::RTX);
    create_serie(p, StatKey::PROBING);
    create_serie(p, StatKey::RTT);
    create_serie(p, StatKey::MINRTT);
    create_serie(p, StatKey::TOTAL);
    create_serie(p, StatKey::FBDELAY);
    create_serie(p, StatKey::RECEIVED_BITRATE);
    create_serie(p, StatKey::TARGET);

    QPointF point;

    for(auto &it : MedoozeReader(medooze_file)) {
        const auto& [ts, media, rtx, probing, recv, fb_delay, target, minrtt, rtt] = it;

        double timestamp = ts / 1000000.;
        // std::cout << ts << " " << timestamp << std::endl;
        point.setX(timestamp);

        point.setY(media);
        add_point(p.c_str(), StatKey::MEDIA, point);

        point.setY(rtx);
        add_point(p.c_str(), StatKey::RTX, point);

        point.setY(probing);
        add_point(p.c_str(), StatKey::PROBING, point);

        point.setY(media + rtx + probing);
        add_point(p.c_str(), StatKey::TOTAL, point);

        point.setY(recv);
        add_point(p.c_str(), StatKey::RECEIVED_BITRATE, point);

        point.setY(target);
        add_point(p.c_str(), StatKey::TARGET, point);

        point.setY(rtt);
        add_point(p.c_str(), StatKey::RTT, point);

        point.setY(minrtt);
        add_point(p.c_str(), StatKey::MINRTT, point);

        point.setY(fb_delay);
        add_point(p.c_str(), StatKey::FBDELAY, point);
    }

    add_serie(p.c_str(), StatKey::MEDIA);
    add_serie(p.c_str(), StatKey::RTX);
    add_serie(p.c_str(), StatKey::PROBING);
    add_serie(p.c_str(), StatKey::TOTAL);
    add_serie(p.c_str(), StatKey::RTT);
    add_serie(p.c_str(), StatKey::MINRTT);
    add_serie(p.c_str(), StatKey::TARGET);
    add_serie(p.c_str(), StatKey::RECEIVED_BITRATE);
    add_serie(p.c_str(), StatKey::FBDELAY);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}

void MedoozeDisplay::load(const fs::path& p)
{
    if(p.filename().string() == "average") load_average(p);
    else load_exp(p);
}
