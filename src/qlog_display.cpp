#include <iostream>
#include <fstream>

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>

#include "qlog_display.h"

#include "stats_line_chart.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

StatsLineChart * QlogDisplay::create_chart()
{
    auto chart = new StatsLineChart();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->createDefaultAxes();
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart;
}

StatsLineChartView * QlogDisplay::create_chart_view(QChart* chart)
{
    auto chart_view = new StatsLineChartView(chart, _tab);
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart_view;
}


QlogDisplay::QlogDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend)
    : _tab(tab), _legend(legend)
{
    _map[StatKey::CWND] = std::make_tuple("Cwnd", QColorConstants::Red, nullptr);
    _map[StatKey::BYTES_IN_FLIGHT] = std::make_tuple("Bytes in flight", QColorConstants::Blue, nullptr);

    _map[StatKey::RTT] = std::make_tuple("RTT", QColorConstants::DarkYellow, nullptr);

    create_legend();

    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_rtt= create_chart();
    _chart_view_rtt = create_chart_view(_chart_rtt);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_rtt, 1);

    _tab->grabGesture(Qt::PanGesture);
    _tab->grabGesture(Qt::PinchGesture);
}

void QlogDisplay::create_legend()
{
    for(auto it = _map.cbegin(); it != _map.cend(); ++it) {
        QListWidgetItem * item = new QListWidgetItem(_legend);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
        item->setForeground(QBrush(std::get<StatsKeyProperty::COLOR>(it.value())));
        item->setText(std::get<StatsKeyProperty::NAME>(it.value()));
        item->setData(1, static_cast<uint8_t>(it.key()));
    }

    connect(_legend, &QListWidget::itemChanged, this, [this](QListWidgetItem* item) -> void {
        StatKey key = static_cast<StatKey>(item->data(1).toUInt());

        auto line = std::get<StatsKeyProperty::SERIE>(_map[key]);
        if(line == nullptr) return;

        if(item->checkState()) line->show();
        else line->hide();
    });
}

void QlogDisplay::create_serie(const fs::path&p, StatKey key)
{
    auto serie = new QLineSeries;
    serie->setColor(std::get<StatsKeyProperty::COLOR>(_map[key]));
    serie->setName(std::get<StatsKeyProperty::NAME>(_map[key]));
    std::get<StatsKeyProperty::SERIE>(_map[key]) = serie;

    _path_keys[p.c_str()].push_back(serie);
}

void QlogDisplay::parse_mvfst(const fs::path& path)
{
    std::ifstream qlog_file(path);
    auto qlog_data = json::parse(qlog_file);

    int64_t time_0 = -1;

    for(auto& trace : qlog_data["traces"]) {
        for(auto& event : trace["events"]) {
            auto name = event["name"].get<std::string>();

            // std::cout << name << std::endl;
            if(name != "recovery:metrics_updated") continue;

            try {
                if(time_0 == -1) time_0 = event["time"].get<int64_t>();

                int64_t time = event["time"].get<int64_t>() - time_0;
                auto data = event["data"];

                QPointF p_cwnd{time/1000000.f, data["congestion_window"].get<float>()};
                QPointF p_bif{time/1000000.f, data["bytes_in_flight"].get<float>()};

                // std::cout << p_cwnd.x() << " " << p_cwnd.y() << std::endl;

                add_point(StatKey::CWND, p_cwnd);
                add_point(StatKey::BYTES_IN_FLIGHT, p_bif);
            } catch(...) { }

            try {
                if(time_0 == -1) time_0 = event["time"].get<int64_t>();

                int64_t time = event["time"].get<int64_t>() - time_0;
                auto data = event["data"];

                QPointF p_rtt{time/1000000.f, data["latest_rtt"].get<float>()};
                add_point(StatKey::RTT, p_rtt);

            } catch(...) {}
        }
    }
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


    if(path.parent_path().filename().string().starts_with("mvfst")) {
        std::cout << "Parsing mvfst file : " << path << std::endl;
        parse_mvfst(path);
    }

    add_serie(StatKey::BYTES_IN_FLIGHT, _chart_bitrate);
    add_serie(StatKey::CWND, _chart_bitrate);

    add_serie(StatKey::RTT, _chart_rtt);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}

void QlogDisplay::unload(const fs::path& path)
{
    auto vec = _path_keys[path.c_str()];

    for(auto s : vec) {
        _chart_bitrate->removeSeries(s);
        _chart_rtt->removeSeries(s);

        delete s;
    }

    _path_keys[path.c_str()] = QVector<QLineSeries*>{};
}
