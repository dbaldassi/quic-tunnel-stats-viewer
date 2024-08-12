#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QValueAxis>

#include "medooze_display.h"

#include "csv_reader.h"
#include "stats_line_chart.h"
#include "all_bitrate.h"

MedoozeDisplay::MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info)
    : DisplayBase(tab, legend, info)
{
    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_rtt= create_chart();
    _chart_view_rtt = create_chart_view(_chart_rtt);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_rtt, 1);

    _display_impl = false;
    create_legend();
}

void MedoozeDisplay::on_keyboard_event(QKeyEvent* key)
{
    if(key->key() == Qt::Key_1) {
        if(_chart_view_rtt->isHidden()) _chart_view_rtt->show();
        else _chart_view_rtt->hide();
    }
    else if(key->key() == Qt::Key_2) {
        if(_chart_view_bitrate->isHidden()) _chart_view_bitrate->show();
        else _chart_view_bitrate->hide();
    }
}

void MedoozeDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::BWE] = std::make_tuple("Bwe", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::TARGET] = std::make_tuple("Target", nullptr, _chart_bitrate, ExpInfo{});
    // map[StatKey::AVAILABLE_BITRATE] = std::make_tuple("Available bitrate", nullptr, _chart_bitrate);
    map[StatKey::RTT] = std::make_tuple("RTT", nullptr, _chart_rtt, ExpInfo{});
    map[StatKey::RTX] = std::make_tuple("RTX", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::PROBING] = std::make_tuple("Probing", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::MEDIA] = std::make_tuple("Media", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::LOSS] = std::make_tuple("Loss", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::MINRTT] = std::make_tuple("Min rtt", nullptr, _chart_rtt, ExpInfo{});
    map[StatKey::TOTAL] = std::make_tuple("Total", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::RECEIVED_BITRATE] = std::make_tuple("Received bitrate", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::FBDELAY] = std::make_tuple("Feedback delay", nullptr, _chart_rtt, ExpInfo{});
    map[StatKey::LOSS_ACCUMULATED] = std::make_tuple("Loss accumulated", nullptr, _chart_rtt, ExpInfo{});

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

void MedoozeDisplay::Info::Stats::process(QTreeWidgetItem* root)
{
    mean /= n;
    variance = (variance / n) - (mean * mean);
    var_coeff = std::sqrt(variance) / mean;

    QTreeWidgetItem * item = new QTreeWidgetItem(root);
    item->setText(0, name);

    QTreeWidgetItem * mean_item = new QTreeWidgetItem(item);
    mean_item->setText(0, "mean");
    mean_item->setText(1, QString::number(mean));

    QTreeWidgetItem * variance_item = new QTreeWidgetItem(item);
    variance_item->setText(0, "variance");
    variance_item->setText(1, QString::number(variance));

    QTreeWidgetItem * coeff_var_item = new QTreeWidgetItem(item);
    coeff_var_item->setText(0, "variation coeff");
    coeff_var_item->setText(1, QString::number(var_coeff));
}

void MedoozeDisplay::Info::StatsLoss::process(QTreeWidgetItem* root)
{
    double percent = static_cast<double>(loss) * 100. / static_cast<double>(sent);

    QTreeWidgetItem * item = new QTreeWidgetItem(root);
    item->setText(0, name);

    QTreeWidgetItem * loss_item = new QTreeWidgetItem(item);
    loss_item->setText(0, "loss");
    loss_item->setText(1, QString::number(loss));

    QTreeWidgetItem * sent_item = new QTreeWidgetItem(item);
    sent_item->setText(0, "sent");
    sent_item->setText(1, QString::number(sent));

    QTreeWidgetItem * percent_item = new QTreeWidgetItem(item);
    percent_item->setText(0, "Percent");
    percent_item->setText(1, QString::number(percent));
}

template<typename T>
concept MedoozeStat = requires(T t) { t.update({}); };

template<MedoozeStat Stat>
struct Accu
{
    int current_ts = -1;
    int sum = 0;
    MedoozeDisplay::StatKey key;
    static constexpr int base = 1000000;
    static constexpr int window = 200000;

    Stat* stats;

    double factor = base / window;

    QList<QPointF> points;
    QList<QPoint> points_window;
    QPointF pt;

    uint64_t accumulated = 0;

    Accu(MedoozeDisplay::StatKey k,  Stat* in_stats = nullptr) : pt{0., 0.}, stats(in_stats)
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

        if(key == MedoozeDisplay::StatKey::LOSS) {
            points.emplace_back(time/1000000., accumulated);
            stats->update(v);
        }
        else {
            points.emplace_back(time/1000000., accumulated * factor / 1000.);
            stats->update(points.back().y());
        }
    }

    void add_value(double time, double v)
    {
        points.emplace_back(time, v);
        stats->update(v);
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
    create_serie(p, StatKey::RECEIVED_BITRATE);
    create_serie(p, StatKey::LOSS);
    create_serie(p, StatKey::LOSS_ACCUMULATED);

    Info info;

    auto accu_media = Accu<Info::Stats>(StatKey::MEDIA, &info.media);
    auto accu_rtx = Accu<Info::Stats>(StatKey::RTX, &info.rtx);
    auto accu_probing = Accu<Info::Stats>(StatKey::PROBING, &info.probing);
    auto accu_rtt = Accu<Info::Stats>(StatKey::RTT, &info.rtt);
    auto accu_minrtt = Accu<Info::Stats>(StatKey::MINRTT, &info.minrtt);
    auto accu_target = Accu<Info::Stats>(StatKey::TARGET, &info.target);
    auto accu_total = Accu<Info::Stats>(StatKey::TOTAL, &info.total);
    auto accu_loss = Accu<Info::StatsLoss>(StatKey::LOSS, &info.loss);
    auto accu_received = Accu<Info::Stats>(StatKey::RECEIVED_BITRATE, &info.received);

    for(auto &it : MedoozeReader(path)) {
        const auto& [fb_ts, twcc_num, fb_num, packet_size, sent_time, recv_ts, delta_sent, delta_recv, delta,
                     bwe, target, available_bitrate, rtt, minrtt, flag, rtx, probing] = it;

        double timestamp = sent_time / 1000000.;

        accu_media.accumulate(sent_time, ((rtx == 0 && probing == 0) ? packet_size * 8 : 0));
        accu_rtx.accumulate(sent_time, ((rtx == 1 && probing == 0) ? packet_size * 8 : 0));
        accu_probing.accumulate(sent_time, ((rtx == 0 && probing == 1) ? packet_size * 8 : 0));
        accu_rtt.add_value(timestamp, rtt);
        accu_minrtt.add_value(timestamp, ((minrtt == 0 || rtt < minrtt) ? rtt : minrtt));
        accu_target.add_value(timestamp, target);
        accu_total.accumulate(sent_time, packet_size * 8);
        accu_loss.accumulate(sent_time, ((sent_time > 0 && recv_ts == 0) ? 1 : 0));
        accu_received.accumulate(sent_time, ((sent_time > 0 && recv_ts == 0) ? 0 : packet_size * 8));

        QPointF loss_pt(timestamp, info.loss.loss);
        add_point(p.c_str(), StatKey::LOSS_ACCUMULATED, loss_pt);
    }

    auto& map = _path_keys[p.c_str()];
    for(auto it : map.keys()) {
        auto info = std::get<StatsKeyProperty::INFO>(map[it]);
        info.stream = false;
        std::get<StatsKeyProperty::INFO>(map[it]) = info;
    }


    QTreeWidgetItem * item = new QTreeWidgetItem(_info);
    item->setText(0, path.parent_path().filename().c_str());

    process(item, info.media, p.c_str(), StatKey::MEDIA, accu_media.get_points());
    process(item, info.rtx, p.c_str(), StatKey::RTX, accu_rtx.get_points());
    process(item, info.probing, p.c_str(), StatKey::PROBING, accu_probing.get_points());
    process(item, info.total, p.c_str(), StatKey::TOTAL, accu_total.get_points());
    process(item, info.rtt, p.c_str(), StatKey::RTT, accu_rtt.get_points());
    process(item, info.minrtt, p.c_str(), StatKey::MINRTT, accu_minrtt.get_points());
    process(item, info.loss, p.c_str(), StatKey::LOSS, accu_loss.get_points());
    process(item, info.received, p.c_str(), StatKey::RECEIVED_BITRATE, accu_received.get_points());
    // add_serie(p.c_str(), StatKey::LOSS_ACCUMULATED);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();

    auto loss_axis = new QValueAxis();

    auto it = std::max_element(accu_loss.points.begin(), accu_loss.points.end(),
                               [](const auto& p1, const auto& p2) { return p1.y() < p2.y();});

    if(it != accu_loss.points.end()) {
        auto max_loss = it->y();
        loss_axis->setRange(0, max_loss * 2);
    }

    _chart_bitrate->addAxis(loss_axis, Qt::AlignRight);

    // const auto& map = _path_keys[p.c_str()];
    auto* serie = std::get<StatsKeyProperty::SERIE>(map[StatKey::LOSS]);

    auto axis = serie->attachedAxes();
    serie->detachAxis(axis.back());
    serie->attachAxis(loss_axis);

    emit on_loss_stats(p, info.loss.loss, info.loss.sent);
}

void MedoozeDisplay::load_average(const fs::path& p)
{
    fs::path medooze_file = p / "medooze.csv";

    using MedoozeReader = CsvReaderTypeRepeat<',', double, 10>;

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

    Info info;

    for(auto &it : MedoozeReader(medooze_file)) {
        const auto& [ts, media, rtx, probing, recv, fb_delay, target, minrtt, rtt, loss] = it;

        double timestamp = ts / 1000000.;
        point.setX(timestamp);

        point.setY(media / 1000.);
        add_point(p.c_str(), StatKey::MEDIA, point);
        info.media.update(media / 1000.);

        point.setY(rtx / 1000.);
        add_point(p.c_str(), StatKey::RTX, point);
        info.rtx.update(rtx / 1000.);

        point.setY(probing / 1000.);
        add_point(p.c_str(), StatKey::PROBING, point);
        info.probing.update(probing / 1000.);

        point.setY((media + rtx + probing) / 1000.);
        add_point(p.c_str(), StatKey::TOTAL, point);
        info.total.update(point.y());

        point.setY(recv/ 1000.);
        add_point(p.c_str(), StatKey::RECEIVED_BITRATE, point);
        info.received.update(recv/ 1000.);

        point.setY(target / 1000.);
        add_point(p.c_str(), StatKey::TARGET, point);
        info.target.update(target / 1000.);

        point.setY(rtt);
        add_point(p.c_str(), StatKey::RTT, point);
        info.rtt.update(rtt);

        point.setY(minrtt);
        add_point(p.c_str(), StatKey::MINRTT, point);
        info.minrtt.update(minrtt);

        point.setY(fb_delay);
        add_point(p.c_str(), StatKey::FBDELAY, point);

        info.loss.sent++;
        info.loss.loss = loss;
    }

    QTreeWidgetItem * item = new QTreeWidgetItem(_info);
    item->setText(0, medooze_file.parent_path().filename().c_str());

    process(p.c_str(), StatKey::MEDIA, item, info.media);
    process(p.c_str(), StatKey::RTX, item, info.rtx);
    process(p.c_str(), StatKey::PROBING, item, info.probing);
    process(p.c_str(), StatKey::TOTAL, item, info.total);
    process(p.c_str(), StatKey::RTT, item, info.rtt);
    process(p.c_str(), StatKey::MINRTT, item, info.minrtt);
    process(p.c_str(), StatKey::TARGET, item, info.target);
    process(p.c_str(), StatKey::RECEIVED_BITRATE, item, info.received);
    add_serie(p.c_str(), StatKey::FBDELAY);
    info.loss.process(item);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();

    emit on_loss_stats(p, info.loss.loss, info.loss.sent);
}

void MedoozeDisplay::load(const fs::path& p)
{
    if(p.filename().string() == "average") load_average(p);
    else load_exp(p);

    QFont font1, font2;
    font1.setPointSize(40);
    font2.setPointSize(36);
    font1.setBold(true);
    font2.setBold(true);

    auto axe = _chart_bitrate->axes(Qt::Horizontal);
    axe.front()->setTitleText("Time (s)");
    axe.front()->setTitleFont(font1);
    axe.front()->setLabelsFont(font2);
    axe.front()->setGridLineVisible(false);

    axe = _chart_bitrate->axes(Qt::Vertical);
    axe.front()->setTitleText("Bitrate (kbps)");
    axe.front()->setTitleFont(font1);
    axe.front()->setLabelsFont(font2);
    axe.front()->setGridLineVisible(false);

    if(axe.size() == 2) {
        axe.back()->setTitleText("Loss");
        axe.back()->setTitleFont(font1);
        axe.back()->setLabelsFont(font2);
        axe.back()->setGridLineVisible(false);
    }

    axe = _chart_rtt->axes(Qt::Horizontal);
    axe.front()->setTitleText("Time (s)");
    axe.front()->setTitleFont(font1);
    axe.front()->setLabelsFont(font2);
    axe.front()->setGridLineVisible(false);

    axe = _chart_rtt->axes(Qt::Vertical);
    axe.front()->setTitleText("Time (ms)");
    axe.front()->setTitleFont(font1);
    axe.front()->setLabelsFont(font2);
    axe.front()->setGridLineVisible(false);

    _chart_view_bitrate->hide();
}

void MedoozeDisplay::save(const fs::path& dir)
{
    auto bitrate_filename = dir / "medooze_sent.png";
    _chart_view_bitrate->grab().save(bitrate_filename.c_str(), "PNG");

    auto rtt_filename = dir / "medooze_rtt.png";
    _chart_view_rtt->grab().save(rtt_filename.c_str(), "PNG");
}

void MedoozeDisplay::add_to_all(const fs::path& dir, AllBitrateDisplay* all)
{
    auto& map = _path_keys[dir.c_str()];
    // all->add_stats(dir, AllBitrateDisplay::TARGET, map[TARGET]);
    // all->add_stats(dir, AllBitrateDisplay::PROBING, map[PROBING]);
    // all->add_stats(dir, AllBitrateDisplay::MEDIA, map[MEDIA]);
    // all->add_stats(dir, AllBitrateDisplay::TOTAL, map[TOTAL]);
    // all->add_stats(dir, AllBitrateDisplay::RTX, map[RTX]);
    all->add_stats(dir, AllBitrateDisplay::MEDOOZE_RTT, map[RTT]);
    // all->add_stats(dir, AllBitrateDisplay::MEDOOZE_LOSS, map[LOSS]);
    // all->add_stats(dir, AllBitrateDisplay::MEDOOZE_LOSS, map[LOSS_ACCUMULATED]);
}

void MedoozeDisplay::set_geometry(float ratio_w, float ratio_h)
{
    auto g = _chart_view_rtt->geometry();

    if(ratio_w == ratio_h) {
        if(g.width() > g.height()) g.setWidth(g.height());
        else g.setHeight((g.width()));
    }
    else {
        g.setHeight(g.width() * ratio_h);
    }

    _chart_view_rtt->setGeometry(g);
}
