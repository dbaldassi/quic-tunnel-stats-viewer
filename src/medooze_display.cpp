#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>

#include "medooze_display.h"

#include "csv_reader.h"
#include "stats_line_chart.h"

MedoozeDisplay::MedoozeDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend)
    : DisplayBase(tab, legend)
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
    // _map[StatKey::FEEDBACK_TS] = std::make_tuple("link", QColorConstants::Black, nullptr);
    // _map[StatKey::TWCC_NUM] = std::make_tuple("bitrate", QColorConstants::Red, nullptr);
    // _map[StatKey::FEEDBACK_NUM] = std::make_tuple("fps", QColorConstants::Red, nullptr);
    // _map[StatKey::PACKET_SIZE] = std::make_tuple("frame dropped", QColorConstants::DarkBlue, nullptr);
    // _map[StatKey::SENT_TIME] = std::make_tuple("frame decoded", QColorConstants::DarkGreen, nullptr);
    // _map[StatKey::RECEIVED_TS] = std::make_tuple("frame key decoded", QColorConstants::Magenta, nullptr);
    // _map[StatKey::DELTA_SENT] = std::make_tuple("frame rendered", QColorConstants::DarkYellow, nullptr);
    // _map[StatKey::DELTA_RECV] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);
    // _map[StatKey::DELTA] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);
    // _map[StatKey::FLAG] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);
    map[StatKey::BWE] = std::make_tuple("Bwe", nullptr, _chart_bitrate);
    map[StatKey::TARGET] = std::make_tuple("Target", nullptr, _chart_bitrate);
    map[StatKey::AVAILABLE_BITRATE] = std::make_tuple("Available bitrate", nullptr, _chart_bitrate);
    map[StatKey::RTT] = std::make_tuple("RTT", nullptr, _chart_rtt);
    map[StatKey::RTX] = std::make_tuple("RTX", nullptr, _chart_bitrate);
    map[StatKey::PROBING] = std::make_tuple("Probing", nullptr, _chart_bitrate);
    map[StatKey::MEDIA] = std::make_tuple("Media", nullptr, _chart_bitrate);
    map[StatKey::LOSS] = std::make_tuple("Loss", nullptr, _chart_rtt);
    map[StatKey::MINRTT] = std::make_tuple("Min rtt", nullptr, _chart_rtt);

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

void MedoozeDisplay::load(const fs::path& p)
{
    fs::path path;
    for (auto const& dir_entry : std::filesystem::recursive_directory_iterator{p}) {
        if(dir_entry.path().filename().string().starts_with("quic-relay-") && dir_entry.path().extension() == ".csv") {
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

    bool start = true;
    QPoint p_media, p_rtx, p_probing;
    QPointF p_rtt, p_minrtt;

    for(auto &it : MedoozeReader(path)) {
        const auto& [fb_ts, twcc_num, fb_num, packet_size, sent_time, recv_ts, delta_sent, delta_recv, delta,
                     bwe, target, available_bitrate, rtt, minrtt, flag, rtx, probing] = it;

        int timestamp = sent_time / 1000000;

        if(start || timestamp != p_media.x()) {
            if(!start) {
                add_point(p.c_str(), StatKey::MEDIA, p_media);
                add_point(p.c_str(), StatKey::RTX, p_rtx);
                add_point(p.c_str(), StatKey::PROBING, p_probing);
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

        add_point(p.c_str(), StatKey::RTT, p_rtt);
        add_point(p.c_str(), StatKey::MINRTT, p_minrtt);
    }

    add_serie(p.c_str(), StatKey::MEDIA);
    add_serie(p.c_str(), StatKey::RTX);
    add_serie(p.c_str(), StatKey::PROBING);

    add_serie(p.c_str(), StatKey::RTT);
    add_serie(p.c_str(), StatKey::MINRTT);

    _chart_bitrate->createDefaultAxes();
    _chart_rtt->createDefaultAxes();
}
