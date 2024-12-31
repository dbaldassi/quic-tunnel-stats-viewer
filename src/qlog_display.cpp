#include <iostream>
#include <fstream>

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QValueAxis>
#include <QLogValueAxis>

#include "qlog_display.h"
#include "stats_line_chart.h"
#include "csv_reader.h"

#include "all_bitrate.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

QlogDisplay::QlogDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info_widget)
    : DisplayBase(tab, legend, info_widget)
{
    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_rtt = create_chart();
    _chart_view_rtt = create_chart_view(_chart_rtt);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_rtt, 1);

    create_legend();
}

void QlogDisplay::on_keyboard_event(QKeyEvent* key)
{
    switch(key->key()) {
    case Qt::Key_1:
        if(_chart_view_rtt->isHidden()) _chart_view_rtt->show();
        else _chart_view_rtt->hide();
        break;
    case Qt::Key_2:
        if(_chart_view_bitrate->isHidden()) _chart_view_bitrate->show();
        else _chart_view_bitrate->hide();
        break;
    case Qt::Key_R: {
        for(auto& map : _path_keys) {
            // auto& map = _path_keys[p.c_str()];
            auto serie = static_cast<QLineSeries*>(std::get<StatsKeyProperty::SERIE>(map[StatKey::DISTRIBUTION]));
            auto w = new DistributionWidget(serie);
            w->show();
        }
    }
        break;
    }
}

void QlogDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::CWND] = std::make_tuple("Cwnd", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);
    map[StatKey::BYTES_IN_FLIGHT] = std::make_tuple("Bytes in flight", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);
    map[StatKey::RTT] = std::make_tuple("RTT", nullptr, _chart_rtt, ExpInfo{.stream = false}, false);
    map[StatKey::LOSS] = std::make_tuple("Loss", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);

    map[StatKey::CWND_BOX] = std::make_tuple("Cwnd box", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);
    map[StatKey::BYTES_IN_FLIGHT_BOX] = std::make_tuple("Bytes in flight box", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);
    map[StatKey::RTT_BOX] = std::make_tuple("RTT box", nullptr, _chart_rtt, ExpInfo{.stream = false}, false);

    map[StatKey::CWND_INTERQUARTILE] = std::make_tuple("Cwnd", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);
    map[StatKey::BYTES_IN_FLIGHT_INTERQUARTILE] = std::make_tuple("Bytes in flight", nullptr, _chart_bitrate, ExpInfo{.stream = false}, false);
    map[StatKey::RTT_INTERQUARTILE] = std::make_tuple("RTT", nullptr, _chart_rtt, ExpInfo{.stream = false}, false);

    if(signal) {
        connect(_legend, &QListWidget::itemChanged, this, [&map](QListWidgetItem* item) -> void {
            StatKey key = static_cast<StatKey>(item->data(1).toUInt());

            std::get<StatsKeyProperty::SHOW>(map[(uint8_t)key]) = item->checkState() == Qt::Checked;

            auto line = std::get<StatsKeyProperty::SERIE>(map[key]);
            if(line == nullptr) return;

            if(item->checkState() == Qt::Checked) line->show();
            else line->hide();
        });
    }
}

void QlogDisplay::set_makeup(const fs::path& path)
{
    const auto& map = _path_keys[path.c_str()];

    QFont font = _chart_bitrate->font();
    font.setPointSize(40);
    font.setBold(true);

    _chart_bitrate->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    _chart_bitrate->legend()->setFont(font);
    _chart_bitrate->legend()->detachFromChart();

    _chart_rtt->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    _chart_rtt->legend()->setFont(font);
    _chart_rtt->legend()->detachFromChart();

    for(const auto& [name, abs_serie, chart, info, show] : map) {
        auto* serie = dynamic_cast<QLineSeries*>(abs_serie);
        if(!serie) continue;

        auto pen = serie->pen();
        pen.setWidth(6);

        QColor color = get_color(info);
        pen.setColor(color);

        if(name == "Bytes in flight") {
            pen.setWidth(4);
            pen.setStyle(Qt::DashLine);
        }

        serie->setPen(pen);
    }

    auto setup_axes = [&font](auto&& axe,const std::string& name) {
        axe->setTitleText(name.c_str());

        font.setPointSize(40);
        axe->setTitleFont(font);

        font.setPointSize(36);
        axe->setLabelsFont(font);
        axe->setGridLineVisible(false);
    };

    auto axes = _chart_bitrate->axes(Qt::Horizontal);
    if(!axes.empty()) setup_axes(axes.front(), "Time (s)");

    axes = _chart_bitrate->axes(Qt::Vertical);
    if(!axes.empty()) setup_axes(axes.front(), "Packets (KBytes)");
    if(axes.size() == 2) setup_axes(axes.back(), "Loss");

    axes = _chart_rtt->axes(Qt::Horizontal);
    if(!axes.empty()) setup_axes(axes.front(), "Time (s)");

    axes = _chart_rtt->axes(Qt::Vertical);
    if(!axes.empty()) setup_axes(axes.front(), "RTT (ms)");
}


void QlogDisplay::add_info(const fs::path& path, const Info& info)
{
    QTreeWidgetItem * item = new QTreeWidgetItem(_info);
    item->setText(0, path.parent_path().filename().c_str());

    QTreeWidgetItem * loss = new QTreeWidgetItem(item);
    loss->setText(0, "Lost");
    loss->setText(1, QString::number(info.lost));

    QTreeWidgetItem * sent = new QTreeWidgetItem(item);
    sent->setText(0, "Sent");
    sent->setText(1, QString::number(info.sent));

    if(info.sent != 0) {
        double p = 100. * static_cast<double>(info.lost) / static_cast<double>(info.sent);
        QTreeWidgetItem * percent = new QTreeWidgetItem(item);
        percent->setText(0, "loss percent");
        percent->setText(1, QString::number(p));
    }

    QTreeWidgetItem * rtt = new QTreeWidgetItem(item);
    rtt->setText(0, "RTT mean");
    rtt->setText(1, QString::number(info.mean_rtt));

    QTreeWidgetItem * variance = new QTreeWidgetItem(item);
    variance->setText(0, "RTT variance");
    variance->setText(1, QString::number(info.variance_rtt));
}

void QlogDisplay::parse_mvfst(const fs::path& path)
{
    std::ifstream qlog_file(path);
    auto qlog_data = json::parse(qlog_file);

    int64_t time_0 = -1;
    uint64_t sum = 0;

    Info info{};
    memset(&info, 0, sizeof(info));

    fs::path key = path.parent_path();

    for(auto& trace : qlog_data["traces"]) {
        for(auto& event : trace["events"]) {
            auto name = event["name"].get<std::string>();

            if(time_0 == -1) time_0 = event["time"].get<int64_t>();
            int64_t time = event["time"].get<int64_t>() - time_0;

            if(name == "recovery:metrics_updated") {
                try {
                    auto data = event["data"];

                    QPointF p_cwnd{time/1000000.f, data["congestion_window"].get<float>() / 1000.f};
                    QPointF p_bif{time/1000000.f, data["bytes_in_flight"].get<float>() / 1000.f};
                    QPointF p_distrib{time/1000000.f, p_cwnd.y() / p_bif.y()};

                    add_point(key.c_str(), StatKey::CWND, p_cwnd);
                    add_point(key.c_str(), StatKey::BYTES_IN_FLIGHT, p_bif);
                    add_point(key.c_str(), StatKey::DISTRIBUTION, p_distrib);
                } catch(...) { }

                try {
                    // if(time_0 == -1) time_0 = event["time"].get<int64_t>();
                    // int64_t time = event["time"].get<int64_t>() - time_0;

                    auto data = event["data"];

                    float rtt = data["latest_rtt"].get<float>();
                    QPointF p_rtt{time/1000000.f, rtt};
                    add_point(key.c_str(), StatKey::RTT, p_rtt);

                    info.mean_rtt += rtt;
                    info.variance_rtt += (rtt * rtt);

                    ++sum;
                } catch(...) {}
            }
            else if(name == "loss:packets_lost") {
                add_point(key.c_str(), StatKey::LOSS, QPointF(time / 1000000.f, info.lost));
                info.lost += event["data"]["lost_packets"].get<int>();
                add_point(key.c_str(), StatKey::LOSS, QPointF(time / 1000000.f, info.lost));
            }
            else if(name == "transport:packet_sent") {
                ++info.sent;
            }
        }
    }

    info.mean_rtt /= sum;
    info.variance_rtt = (info.variance_rtt / sum) - (info.mean_rtt * info.mean_rtt);

    add_info(path, info);

    emit on_loss_stats(key, info.lost, info.sent);
}

void QlogDisplay::parse_quicgo(const fs::path& path)
{
    std::ifstream ifs(path);
    std::string line;

    Info info{};
    uint64_t sum = 0;
    memset(&info, 0, sizeof(info));

    fs::path key = path.parent_path();

    while(std::getline(ifs, line)) {
        auto pos = line.find("{");

        auto line_json = json::parse(line.substr(pos));

        try {
            auto name = line_json["name"].get<std::string>();
            auto time = line_json["time"].get<float>() / 1000.f;

            if(name == "recovery:metrics_updated") {
                auto data = line_json["data"];

                QPointF p_cwnd;
                if(data.contains("congestion_window")) {
                    p_cwnd = QPointF{time, data["congestion_window"].get<float>() / 1000.};
                    add_point(key.c_str(), StatKey::CWND, p_cwnd);
                }

                if(data.contains("bytes_in_flight")) {
                    QPointF p_bif{time, data["bytes_in_flight"].get<float>() / 1000.};
                    add_point(key.c_str(), StatKey::BYTES_IN_FLIGHT, p_bif);

                    QPointF p_distrib{time/1000000.f, p_cwnd.y() / p_bif.y()};
                    add_point(key.c_str(), StatKey::DISTRIBUTION, p_distrib);
                }

                if(data.contains("latest_rtt")) {
                    float rtt = data["latest_rtt"].get<float>();
                    QPointF p_rtt{time, rtt};
                    add_point(key.c_str(), StatKey::RTT, p_rtt);
                    info.mean_rtt += rtt;
                    info.variance_rtt += (rtt * rtt);

                    ++sum;
                }
                else if(data.contains("smoothed_rtt")) {
                    float rtt = data["smoothed_rtt"].get<float>();
                    QPointF p_rtt{time, rtt};
                    add_point(key.c_str(), StatKey::RTT, p_rtt);
                    info.mean_rtt += rtt;
                    info.variance_rtt += (rtt * rtt);

                    ++sum;
                }

                if(data.contains("lost_packets")) {
                    add_point(key.c_str(), StatKey::LOSS, QPointF(time, info.lost));
                    info.lost = data["lost_packets"].get<int>();
                    add_point(key.c_str(), StatKey::LOSS, QPointF(time, info.lost));
                }
                if(data.contains("total_send_packets")) {
                    info.sent = data["total_send_packets"].get<int>();
                }
            }
            else if(name == "transport:packet_lost" || name == "recovery:packet_lost" ) {
                add_point(key.c_str(), StatKey::LOSS, QPointF(time, info.lost));
                ++info.lost;
                add_point(key.c_str(), StatKey::LOSS, QPointF(time, info.lost));
            }
            else if(name == "transport:packet_sent") {
                ++info.sent;
            }
        } catch(...) {}
    }

    info.mean_rtt /= sum;
    info.variance_rtt = (info.variance_rtt / sum) - (info.mean_rtt * info.mean_rtt);

    add_info(path, info);

    emit on_loss_stats(key, info.lost, info.sent);
}

void QlogDisplay::load_exp(const fs::path& p)
{
    fs::path path;
    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{p}) {
        if(dir_entry.path().extension() == ".qlog") {
            path = dir_entry.path();
            break;
        }
    }

    if(path.empty()) {
        std::cout << "No qlog file found" << std::endl;
        return;
    }

    create_serie(p, StatKey::BYTES_IN_FLIGHT);
    create_serie(p, StatKey::CWND);
    create_serie(p, StatKey::RTT);
    create_serie(p, StatKey::LOSS);
    create_serie(p, StatKey::DISTRIBUTION);

    auto impl = path.parent_path().filename().string();

    if(path.filename().string().starts_with("mvfst") || impl.starts_with("mvfst")) {
        std::cout << "Parsing mvfst file : " << path << std::endl;
        parse_mvfst(path);
    }
    else if(path.filename().string().starts_with("quicgo") || impl.starts_with("quicgo")) {
        std::cout << "Parsing quicgo file : " << path << std::endl;
        parse_quicgo(path);
    }
    else if(path.filename().string().starts_with("quiche") || impl.starts_with("quiche")) {
        std::cout << "Parsing quiche file : " << path << std::endl;
        parse_quicgo(path);
    }
    else if(path.filename().string().starts_with("msquic") || impl.starts_with("msquic")) {
        std::cout << "Parsing msquic file : " << path << std::endl;
        parse_quicgo(path);
    }

    auto& map = _path_keys[p.c_str()];

    for(auto it : map.keys()) {
        auto info = std::get<StatsKeyProperty::INFO>(map[it]);
        info.stream = false;
        std::get<StatsKeyProperty::INFO>(map[it]) = info;
    }

    add_serie(p.c_str(), StatKey::BYTES_IN_FLIGHT);
    add_serie(p.c_str(), StatKey::CWND);
    add_serie(p.c_str(), StatKey::RTT);
    add_serie(p.c_str(), StatKey::LOSS);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();

    // auto& map = _path_keys[p.c_str()];
    /*auto* serie = std::get<StatsKeyProperty::SERIE>(map[StatKey::LOSS]);

    auto loss_axis = new QValueAxis();
    _chart_bitrate->addAxis(loss_axis, Qt::AlignRight);

    const auto& points = serie->points();
    if(!points.empty()) {
        const auto& point = points.back();
        loss_axis->setRange(0,  point.y());
    }

    auto axis = serie->attachedAxes();
    serie->detachAxis(axis.back());
    serie->attachAxis(loss_axis);*/
}

void QlogDisplay::load_average(const fs::path& p)
{
    fs::path file = p / "qlog.csv";

    if(!fs::exists(file)) return; // in case of udp

    using QlogReader = CsvReaderTypeRepeat<',', double, 4>;

    create_serie(p, StatKey::RTT);

    Info info;

    int sum = 0;

    for(const auto& it : QlogReader(file)) {
        const auto& [ts, rtt, loss, sent] = it;

        QPointF point{ts, rtt / 1000.};

        add_point(p.c_str(), StatKey::RTT, point);

        info.mean_rtt += rtt;
        info.variance_rtt += (rtt * rtt);

        info.lost = loss;
        info.sent = sent;
        ++sum;
    }

    info.mean_rtt /= sum;
    info.variance_rtt = (info.variance_rtt / sum) - (info.mean_rtt * info.mean_rtt);

    add_info(p, info);

    add_serie(p.c_str(), StatKey::RTT);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();

    emit on_loss_stats(p, info.lost, info.sent);
}

template<typename T>
bool QlogDisplay::get_stats(const fs::path& p, std::ifstream& ifs, std::vector<StatLinePoint<T>>& tab, StatKey key)
{
    tab.emplace_back(StatLinePoint<T>{});
    if(!get_csv_line(ifs, tab)) {
        tab.pop_back();
        return false;
    }

    // if(tab.back().values.size() < 4) return true;
    const auto& map = _path_keys[p.c_str()];
    const auto& info = std::get<StatsKeyProperty::INFO>(map[key]);

    if((key == StatKey::CWND || key == StatKey::BYTES_IN_FLIGHT)) {
        for(int i = 0; i < tab.back().values.size(); ++i) tab.back().values[i] /= 1000.;
    }

    if(tab.back().values.size() < 4) {
        auto time = tab.back().time;
        tab.pop_back();

        if(tab.size() > 0) {
            tab.push_back(tab.back());
            tab.back().time = time;
        }
    }

    QPointF avg{(double)tab.back().time, get_average(tab.back().values)};

    StatKey key_box, key_inter;
    switch(key) {
    case StatKey::CWND:
        key_box = StatKey::CWND_BOX;
        key_inter = StatKey::CWND_INTERQUARTILE;
        break;
    case StatKey::BYTES_IN_FLIGHT:
        key_box = StatKey::BYTES_IN_FLIGHT_BOX;
        key_inter = StatKey::BYTES_IN_FLIGHT_INTERQUARTILE;
        break;
    case StatKey::RTT:
        key_box = StatKey::RTT_BOX;
        key_inter = StatKey::RTT_INTERQUARTILE;
        break;
    default:
        return false;
    }

    QPointF inter{(double)tab.back().time, get_interquartile_average(tab.back().values)};
    add_point(p.c_str(), key_inter, inter);
    add_point(p.c_str(), key, avg);
    add_point(p.c_str(), key_box, QString::number(tab.back().time), tab.back().values);

    return true;
}

void QlogDisplay::load_stats_line(const fs::path& p)
{
    fs::path file = p / "stats_line_qlog.csv";

    std::ifstream ifs(file.c_str());
    if(!ifs.is_open()) return; // in case of datagrams

    create_serie(p, StatKey::CWND);
    create_serie(p, StatKey::BYTES_IN_FLIGHT);
    create_serie(p, StatKey::RTT);

    create_serie(p, StatKey::CWND_INTERQUARTILE);
    create_serie(p, StatKey::RTT_INTERQUARTILE);
    create_serie(p, StatKey::BYTES_IN_FLIGHT_INTERQUARTILE);

    create_serie<QBoxPlotSeries>(p, StatKey::CWND_BOX);
    create_serie<QBoxPlotSeries>(p, StatKey::BYTES_IN_FLIGHT_BOX);
    create_serie<QBoxPlotSeries>(p, StatKey::RTT_BOX);

    std::vector<StatLinePoint<double>> cwnd;
    std::vector<StatLinePoint<double>> bif;
    std::vector<StatLinePoint<double>> rtt;

    while(!ifs.eof()) {
        if(!get_stats(p, ifs, cwnd, StatKey::CWND)) break;
        get_stats(p, ifs, bif, StatKey::BYTES_IN_FLIGHT);
        get_stats(p, ifs, rtt, StatKey::RTT);
    }

    add_serie(p.c_str(), StatKey::CWND);
    add_serie(p.c_str(), StatKey::BYTES_IN_FLIGHT);
    add_serie(p.c_str(), StatKey::RTT);

    add_serie<QBoxPlotSeries>(p.c_str(), StatKey::CWND_INTERQUARTILE);
    add_serie<QBoxPlotSeries>(p.c_str(), StatKey::BYTES_IN_FLIGHT_INTERQUARTILE);
    add_serie<QBoxPlotSeries>(p.c_str(), StatKey::RTT_INTERQUARTILE);

    // add_serie<QBoxPlotSeries>(p.c_str(), StatKey::CWND_BOX);
    // add_serie<QBoxPlotSeries>(p.c_str(), StatKey::BYTES_IN_FLIGHT_BOX);
    // add_serie<QBoxPlotSeries>(p.c_str(), StatKey::RTT_BOX);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}

void QlogDisplay::load(const fs::path& p)
{
    if(p.filename().string() == "average") load_stats_line(p); // load_average(p);
    else load_exp(p);

    set_makeup(p);
    // _chart_view_rtt->hide();
}

void QlogDisplay::save(const fs::path& dir)
{
    auto bitrate_filename = dir / "qlog_cwnd.png";
    _chart_view_bitrate->grab().save(bitrate_filename.c_str(), "PNG");

    auto rtt_filename = dir / "qlog_rtt.png";
    _chart_view_rtt->grab().save(rtt_filename.c_str(), "PNG");
}

void QlogDisplay::add_to_all(const fs::path& dir, AllBitrateDisplay* all)
{
    auto& map = _path_keys[dir.c_str()];
    // all->add_stats(dir, AllBitrateDisplay::CWND, map[CWND]);
    // all->add_stats(dir, AllBitrateDisplay::BYTES_IN_FLIGHT, map[BYTES_IN_FLIGHT]);
    // all->add_stats(dir, AllBitrateDisplay::QUIC_RTT, map[RTT]);
    // all->add_stats(dir, AllBitrateDisplay::QUIC_LOSS, map[LOSS]);
}

void QlogDisplay::set_geometry(float ratio_w, float ratio_h)
{
    auto g = _chart_view_bitrate->geometry();

    if(ratio_w == ratio_h) {
        if(g.width() > g.height()) g.setWidth(g.height());
        else g.setHeight((g.width()));
    }
    else {
        g.setHeight(g.width() * ratio_h);
    }

    _chart_view_bitrate->setGeometry(g);
}

int count_above(QList<QPointF> list, float coeff)
{
    int c = 0;

    for(auto& pt : list) {
        if(pt.y() >= coeff) ++c;
    }

    return c;
}

DistributionWidget::DistributionWidget(QLineSeries* s, QWidget* parent) : QWidget(parent)
{
    std::array<float, 10> coeffs{1000, 100, 50, 20, 10, 8, 5, 2, 1, 0.5};

    QLineSeries* serie = new QLineSeries(s);
    QList<QPointF> pts = s->points();

    for(float coeff : coeffs) {
        int count = count_above(pts, coeff);
        QPointF pt(coeff, count * 100. / pts.size());

        qInfo() << coeff << " " << count << " " << pts.size();

        auto pts_tmp = serie->points();
        if(!pts_tmp.empty()) {
            auto p = pts_tmp.last();
            p.setX(coeff);
            serie->append(p);
        }
        else {
            serie->append(QPointF(coeff, 0));
        }

        serie->append(pt);
    }

    QChart *chart = new StatsLineChart();
    // chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->addSeries(serie);
    // chart->createDefaultAxes();
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    chart->setTitle("cwnd above bytes in flight repartition");

    auto axisX = new QLogValueAxis;
    axisX->setTitleText("Coeff");
    axisX->setLabelFormat("%i");
    axisX->setBase(10.0);
    axisX->setReverse(true);
    // axisX->
    chart->addAxis(axisX, Qt::AlignBottom);
    serie->attachAxis(axisX);

    auto axisY = new QValueAxis;
    axisY->setTitleText("Percent");
    axisY->setLabelFormat("%g");
    axisY->setTickCount(serie->count());
    axisY->setMinorTickCount(-1);
    chart->addAxis(axisY, Qt::AlignLeft);
    serie->attachAxis(axisY);

    QChartView *chart_view = new StatsLineChartView(chart);
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(chart_view);

    setLayout(layout);
}

void DistributionWidget::closeEvent(QCloseEvent* event)
{
    deleteLater();
}
