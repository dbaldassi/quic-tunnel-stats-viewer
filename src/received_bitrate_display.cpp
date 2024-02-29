#include "received_bitrate_display.h"

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>

#include "csv_reader.h"
#include "stats_line_chart.h"



ReceivedBitrateDisplay::ReceivedBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend)
    : DisplayBase(tab, legend)
{
    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_fps = create_chart();
    _chart_view_fps = create_chart_view(_chart_fps);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_fps, 1);

    create_legend();
}

void ReceivedBitrateDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::LINK] = std::make_tuple("link", nullptr, _chart_bitrate);
    map[StatKey::BITRATE] = std::make_tuple("bitrate", nullptr, _chart_bitrate);
    map[StatKey::FPS] = std::make_tuple("fps", nullptr, _chart_fps);
    map[StatKey::FRAME_DROPPED] = std::make_tuple("frame dropped", nullptr, nullptr);
    map[StatKey::FRAME_DECODED] = std::make_tuple("frame decoded", nullptr, nullptr);
    map[StatKey::FRAME_KEY_DECODED] = std::make_tuple("frame key decoded", nullptr, nullptr);
    map[StatKey::FRAME_RENDERED] = std::make_tuple("frame rendered", nullptr, nullptr);
    map[StatKey::QUIC_SENT] = std::make_tuple("quic sent bitrate", nullptr, _chart_bitrate);

    if(signal) {
        connect(_legend, &QListWidget::itemChanged, this, [&map](QListWidgetItem* item) -> void {
            StatKey key = static_cast<StatKey>(item->data(1).toUInt());

            auto line = std::get<StatsKeyProperty::SERIE>(map[(uint8_t)key]);
            if(line == nullptr) return;

            if(item->checkState()) line->show();
            else line->hide();
        });
    }
}

// bitrate.csv : time, bitrate, link, fps, frame_dropped, frame_decoded, frame_keydecoded, frame_rendered
// quic.csv : time, bitrate
void ReceivedBitrateDisplay::load(const fs::path& p)
{
    fs::path path = p / "bitrate.csv";

    using BitrateReader = CsvReaderTypeRepeat<',', int, 8>;

    create_serie(p, StatKey::BITRATE);
    create_serie(p, StatKey::LINK);
    create_serie(p, StatKey::FPS);

    for(auto &it : BitrateReader(path)) {
        const auto& [time, bitrate, link, fps, frame_dropped, frame_decoded, frame_key_decoded, frame_rendered] = it;

        QPoint p_bitrate(time, bitrate), p_fps(time, fps), p_link(time, link);

        add_point(p.c_str(), StatKey::BITRATE, p_bitrate);
        add_point(p.c_str(), StatKey::LINK, p_link);
        add_point(p.c_str(), StatKey::FPS, p_fps);
    }

    add_serie(p.c_str(), StatKey::BITRATE);
    add_serie(p.c_str(), StatKey::LINK);
    add_serie(p.c_str(), StatKey::FPS);

    _chart_bitrate->createDefaultAxes();
    _chart_fps->createDefaultAxes();
}
