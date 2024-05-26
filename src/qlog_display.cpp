#include <iostream>
#include <fstream>

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QValueAxis>

#include "qlog_display.h"
#include "stats_line_chart.h"
#include "csv_reader.h"

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

void QlogDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::CWND] = std::make_tuple("Cwnd", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::BYTES_IN_FLIGHT] = std::make_tuple("Bytes in flight", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::RTT] = std::make_tuple("RTT", nullptr, _chart_rtt, ExpInfo{});
    map[StatKey::LOSS] = std::make_tuple("Loss", nullptr, _chart_bitrate, ExpInfo{});

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

                    QPointF p_cwnd{time/1000000.f, data["congestion_window"].get<float>()};
                    QPointF p_bif{time/1000000.f, data["bytes_in_flight"].get<float>()};

                    add_point(key.c_str(), StatKey::CWND, p_cwnd);
                    add_point(key.c_str(), StatKey::BYTES_IN_FLIGHT, p_bif);
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


                if(data.contains("congestion_window")) {
                    QPointF p_cwnd{time, data["congestion_window"].get<float>()};
                    add_point(key.c_str(), StatKey::CWND, p_cwnd);
                }

                if(data.contains("bytes_in_flight")) {
                    QPointF p_bif{time, data["bytes_in_flight"].get<float>()};
                    add_point(key.c_str(), StatKey::BYTES_IN_FLIGHT, p_bif);
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

    add_serie(p.c_str(), StatKey::BYTES_IN_FLIGHT);
    add_serie(p.c_str(), StatKey::CWND);
    add_serie(p.c_str(), StatKey::RTT);
    add_serie(p.c_str(), StatKey::LOSS);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();

    const auto& map = _path_keys[p.c_str()];
    auto* serie = std::get<StatsKeyProperty::SERIE>(map[StatKey::LOSS]);

    auto loss_axis = new QValueAxis();
    _chart_bitrate->addAxis(loss_axis, Qt::AlignRight);

    const auto& points = serie->points();
    if(!points.empty()) {
        const auto& point = points.back();
        loss_axis->setRange(0,  point.y());
    }

    auto axis = serie->attachedAxes();
    serie->detachAxis(axis.back());
    serie->attachAxis(loss_axis);
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

void QlogDisplay::load(const fs::path& p)
{
    if(p.filename().string() == "average") load_average(p);
    else load_exp(p);

    QFont font1, font2;
    font1.setPointSize(18);
    font2.setPointSize(15);

    auto axe = _chart_bitrate->axes(Qt::Horizontal);
    if(!axe.empty()) {
        axe.front()->setTitleText("Time (s)");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
    }

    axe = _chart_bitrate->axes(Qt::Vertical);
    if(!axe.empty()) axe.front()->setTitleText("Bytes");
    if(axe.size() == 2) axe.back()->setTitleText("Loss");

    for(auto& it : axe) {
        it->setTitleFont(font1);
        it->setLabelsFont(font2);
    }

    axe = _chart_rtt->axes(Qt::Horizontal);
    if(!axe.empty()) {
        axe.front()->setTitleText("Time (s)");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
    }

    axe = _chart_rtt->axes(Qt::Vertical);
    if(!axe.empty()) axe.front()->setTitleText("Time (ms)");
    for(auto& it : axe) {
        it->setTitleFont(font1);
        it->setLabelsFont(font2);
    }

}

void QlogDisplay::save(const fs::path& dir)
{
    auto bitrate_filename = dir / "qlog_cwnd.png";
    _chart_view_bitrate->grab().save(bitrate_filename.c_str(), "PNG");

    auto rtt_filename = dir / "qlog_rtt.png";
    _chart_view_rtt->grab().save(rtt_filename.c_str(), "PNG");
}
