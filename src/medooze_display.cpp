#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>

#include "medooze_display.h"

#include "csv_reader.h"
#include "stats_line_chart.h"

StatsLineChart * MedoozeDisplay::create_chart()
{
    auto chart = new StatsLineChart();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->createDefaultAxes();
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart;
}

StatsLineChartView * MedoozeDisplay::create_chart_view(QChart* chart)
{
    auto chart_view = new StatsLineChartView(chart, _tab);
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart_view;
}


MedoozeDisplay::MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend)
    : _tab(tab), _legend(legend)
{
    // _map[StatKey::FEEDBACK_TS] = std::make_tuple("link", QColorConstants::Black, nullptr);
    // _map[StatKey::TWCC_NUM] = std::make_tuple("bitrate", QColorConstants::Red, nullptr);
    // _map[StatKey::FEEDBACK_NUM] = std::make_tuple("fps", QColorConstants::Red, nullptr);
    // _map[StatKey::PACKET_SIZE] = std::make_tuple("frame dropped", QColorConstants::DarkBlue, nullptr);
    // _map[StatKey::SENT_TIME] = std::make_tuple("frame decoded", QColorConstants::DarkGreen, nullptr);
    // _map[StatKey::RECEIVED_TS] = std::make_tuple("frame key decoded", QColorConstants::Magenta, nullptr);
    // _map[StatKey::DELTA_SENT] = std::make_tuple("frame rendered", QColorConstants::DarkYellow, nullptr);
    // _map[StatKey::DELTA_RECV] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);
    // _map[StatKey::DELTA] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);
    _map[StatKey::BWE] = std::make_tuple("Bwe", QColorConstants::Yellow, nullptr);
    _map[StatKey::TARGET] = std::make_tuple("Target", QColorConstants::Red, nullptr);
    _map[StatKey::AVAILABLE_BITRATE] = std::make_tuple("Available bitrate", QColorConstants::Green, nullptr);
    _map[StatKey::RTT] = std::make_tuple("RTT", QColorConstants::DarkYellow, nullptr);
    // _map[StatKey::FLAG] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);
    _map[StatKey::RTX] = std::make_tuple("RTX", QColorConstants::DarkYellow, nullptr);
    _map[StatKey::PROBING] = std::make_tuple("Probing", QColorConstants::Magenta, nullptr);
    _map[StatKey::MEDIA] = std::make_tuple("Media", QColorConstants::Blue, nullptr);
    _map[StatKey::LOSS] = std::make_tuple("Loss", QColorConstants::Green, nullptr);
    _map[StatKey::MINRTT] = std::make_tuple("Min rtt", QColorConstants::DarkMagenta, nullptr);


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

void MedoozeDisplay::create_legend()
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

void MedoozeDisplay::create_serie(const fs::path&p, StatKey key)
{
    auto serie = new QLineSeries;
    serie->setColor(std::get<StatsKeyProperty::COLOR>(_map[key]));
    serie->setName(std::get<StatsKeyProperty::NAME>(_map[key]));
    std::get<StatsKeyProperty::SERIE>(_map[key]) = serie;

    _path_keys[p.c_str()].push_back(serie);
}
#include <iostream>
void MedoozeDisplay::load(const fs::path& p)
{
    // fs::path path = p / "bitrate.csv";

    fs::path path;
    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{p}) {
        if(dir_entry.path().filename().string().starts_with("quic-relay-") && dir_entry.path().extension() == ".csv") {
            path = dir_entry.path();
            break;
        }
    }

    std::cout << path.string() << std::endl;

    using MedoozeReader = CsvReaderTypeRepeat<'|', int, 17>;

    // create_serie(p, StatKey::BITRATE);
    create_serie(p, StatKey::MEDIA);
    create_serie(p, StatKey::RTX);
    create_serie(p, StatKey::PROBING);
    create_serie(p, StatKey::RTT);
    create_serie(p, StatKey::MINRTT);

    bool start = true;
    QPoint p_media, p_rtx, p_probing;
    QPointF p_rtt, p_minrtt;

    for(auto &it : MedoozeReader(path)) {
        const auto& [fb_ts, twcc_num, fb_num, packet_size, sent_time, recv_ts, delta_sent, delta_recv, delta,
                     bwe, target, available_bitrate, rtt, minrtt, flag, rtx, probing] = it;

        // QPoint p_bitrate(time, bitrate), p_fps(time, fps), p_link(time, link);

        int timestamp = sent_time / 1000000;

        if(start || timestamp != p_media.x()) {
            if(!start) {
                add_point(StatKey::MEDIA, p_media);
                add_point(StatKey::RTX, p_rtx);
                add_point(StatKey::PROBING, p_probing);
            }

            start = false;
            p_media.setX(timestamp);
            p_media.setY(0);

            p_probing.setX(timestamp);
            p_probing.setY(0);

            p_rtx.setX(timestamp);
            p_rtx.setY(0);
        }

        p_media.setY(p_media.y() + ((rtx == 0 && probing == 0) ? packet_size * 8 : 0));
        p_probing.setY(p_probing.y() + ((rtx == 0 && probing == 1) ? packet_size * 8 : 0));
        p_rtx.setY(p_rtx.y() + ((rtx == 1 && probing == 0) ? packet_size * 8 : 0));

        p_rtt.setX(sent_time / 1000000.f);
        p_rtt.setY(rtt);

        p_minrtt.setX(sent_time / 1000000.f);
        p_minrtt.setY(minrtt);

        add_point(StatKey::RTT, p_rtt);
        add_point(StatKey::MINRTT, p_minrtt);
    }

    add_serie(StatKey::MEDIA, _chart_bitrate);
    add_serie(StatKey::RTX, _chart_bitrate);
    add_serie(StatKey::PROBING, _chart_bitrate);

    add_serie(StatKey::RTT, _chart_rtt);
    add_serie(StatKey::MINRTT, _chart_rtt);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}

void MedoozeDisplay::unload(const fs::path& path)
{
    auto vec = _path_keys[path.c_str()];

    for(auto s : vec) {
        _chart_bitrate->removeSeries(s);
        _chart_rtt->removeSeries(s);

        delete s;
    }

    _path_keys[path.c_str()] = QVector<QLineSeries*>{};
}
