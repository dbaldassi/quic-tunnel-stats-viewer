#include <iostream>
#include <fstream>

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>

#include "qlog_display.h"

#include "stats_line_chart.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

QlogDisplay::QlogDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info_widget)
    : DisplayBase(tab, legend), _info_widget(info_widget)
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
    map[StatKey::CWND] = std::make_tuple("Cwnd", nullptr, _chart_bitrate);
    map[StatKey::BYTES_IN_FLIGHT] = std::make_tuple("Bytes in flight", nullptr, _chart_bitrate);
    map[StatKey::RTT] = std::make_tuple("RTT", nullptr, _chart_rtt);

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

void QlogDisplay::parse_mvfst(const fs::path& path)
{
    std::ifstream qlog_file(path);
    auto qlog_data = json::parse(qlog_file);

    int64_t time_0 = -1;

    Info info{0,0};

    fs::path key = path.parent_path();

    for(auto& trace : qlog_data["traces"]) {
        for(auto& event : trace["events"]) {
            auto name = event["name"].get<std::string>();

            if(name == "recovery:metrics_updated") {
                try {
                    if(time_0 == -1) time_0 = event["time"].get<int64_t>();

                    int64_t time = event["time"].get<int64_t>() - time_0;
                    auto data = event["data"];

                    QPointF p_cwnd{time/1000000.f, data["congestion_window"].get<float>()};
                    QPointF p_bif{time/1000000.f, data["bytes_in_flight"].get<float>()};

                    add_point(key.c_str(), StatKey::CWND, p_cwnd);
                    add_point(key.c_str(), StatKey::BYTES_IN_FLIGHT, p_bif);
                } catch(...) { }

                try {
                    if(time_0 == -1) time_0 = event["time"].get<int64_t>();

                    int64_t time = event["time"].get<int64_t>() - time_0;
                    auto data = event["data"];

                    QPointF p_rtt{time/1000000.f, data["latest_rtt"].get<float>()};
                    add_point(key.c_str(), StatKey::RTT, p_rtt);

                } catch(...) {}
            }
            else if(name == "loss:packets_lost") {
                info.lost += event["data"]["lost_packets"].get<int>();
            }
            else if(name == "transport:packet_sent") {
                ++info.sent;
            }
        }
    }

    QTreeWidgetItem * item = new QTreeWidgetItem(_info_widget);
    item->setText(0, path.parent_path().filename().c_str());

    QTreeWidgetItem * loss = new QTreeWidgetItem(item);
    loss->setText(0, "Lost");
    loss->setText(1, QString::number(info.lost));

    QTreeWidgetItem * sent = new QTreeWidgetItem(item);
    sent->setText(0, "Sent");
    sent->setText(1, QString::number(info.sent));
}

void QlogDisplay::parse_quicgo(const fs::path& path)
{
    std::ifstream ifs(path);
    std::string line;

    Info info{0,0};

    fs::path key = path.parent_path();
    std::cout << key << "\n";

    while(std::getline(ifs, line)) {
        auto pos = line.find("{");

        auto line_json = json::parse(line.substr(pos));

        try {
            auto name = line_json["name"].get<std::string>();
            if(name == "recovery:metrics_updated") {
                auto data = line_json["data"];
                auto time = line_json["time"].get<float>() / 1000.f;

                if(data.contains("congestion_window")) {
                    QPointF p_cwnd{time, data["congestion_window"].get<float>()};
                    add_point(key.c_str(), StatKey::CWND, p_cwnd);
                }

                if(data.contains("bytes_in_flight")) {
                    QPointF p_bif{time, data["bytes_in_flight"].get<float>()};
                    add_point(key.c_str(), StatKey::BYTES_IN_FLIGHT, p_bif);
                }

                if(data.contains("latest_rtt")) {
                    QPointF p_rtt{time, data["latest_rtt"].get<float>()};
                    add_point(key.c_str(), StatKey::RTT, p_rtt);
                }
            }
            else if(name == "transport:packet_lost") {
                ++info.lost;
            }
            else if(name == "transport:packet_sent") {
                ++info.sent;
            }
        } catch(...) {}
    }

    QTreeWidgetItem * item = new QTreeWidgetItem(_info_widget);
    item->setText(0, path.parent_path().filename().c_str());

    QTreeWidgetItem * loss = new QTreeWidgetItem(item);
    loss->setText(0, "Lost");
    loss->setText(1, QString::number(info.lost));

    QTreeWidgetItem * sent = new QTreeWidgetItem(item);
    sent->setText(0, "Sent");
    sent->setText(1, QString::number(info.sent));
}

void QlogDisplay::load(const fs::path& p)
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

    auto impl = path.parent_path().filename().string();

    if(impl.starts_with("mvfst")) {
        std::cout << "Parsing mvfst file : " << path << std::endl;
        parse_mvfst(path);
    }
    else if(impl.starts_with("quicgo")) {
        std::cout << "Parsing quicgo file : " << path << std::endl;
        parse_quicgo(path);
    }
    else if(impl.starts_with("quiche")) {
        std::cout << "Parsing quiche file : " << path << std::endl;
        parse_quicgo(path);
    }
    else if(impl.starts_with("msquic")) {
        std::cout << "Parsing msquic file : " << path << std::endl;
        parse_quicgo(path);
    }

    add_serie(p.c_str(), StatKey::BYTES_IN_FLIGHT);
    add_serie(p.c_str(), StatKey::CWND);
    add_serie(p.c_str(), StatKey::RTT);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}
