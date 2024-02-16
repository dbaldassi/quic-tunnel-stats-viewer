#include "received_bitrate_display.h"

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>

#include <iostream>

#include "csv_reader.h"
#include "stats_line_chart.h"


StatsLineChart * ReceivedBitrateDisplay::create_chart()
{
    auto chart = new StatsLineChart();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->createDefaultAxes();
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart;
}

StatsLineChartView * ReceivedBitrateDisplay::create_chart_view(QChart* chart)
{
    auto chart_view = new StatsLineChartView(chart, _tab);
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart_view;
}

ReceivedBitrateDisplay::ReceivedBitrateDisplay(QWidget* tab, QListWidget* legend)
    : _tab(tab), _legend(legend)
{
    _map[StatKey::LINK] = std::make_tuple("link", QColorConstants::Black, nullptr);
    _map[StatKey::BITRATE] = std::make_tuple("bitrate", QColorConstants::Red, nullptr);
    _map[StatKey::FPS] = std::make_tuple("fps", QColorConstants::Red, nullptr);
    _map[StatKey::FRAME_DROPPED] = std::make_tuple("frame dropped", QColorConstants::DarkBlue, nullptr);
    _map[StatKey::FRAME_DECODED] = std::make_tuple("frame decoded", QColorConstants::DarkGreen, nullptr);
    _map[StatKey::FRAME_KEY_DECODED] = std::make_tuple("frame key decoded", QColorConstants::Magenta, nullptr);
    _map[StatKey::FRAME_RENDERED] = std::make_tuple("frame rendered", QColorConstants::DarkYellow, nullptr);
    _map[StatKey::QUIC_SENT] = std::make_tuple("quic sent bitrate", QColorConstants::Green, nullptr);

    create_legend();

    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_fps = create_chart();
    _chart_view_fps = create_chart_view(_chart_fps);

    _tab->layout()->addWidget(_chart_view_bitrate);
    _tab->layout()->addWidget(_chart_view_fps);

    _tab->grabGesture(Qt::PanGesture);
    _tab->grabGesture(Qt::PinchGesture);
}

void ReceivedBitrateDisplay::create_legend()
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

void ReceivedBitrateDisplay::create_serie(StatKey key)
{
    auto serie = new QLineSeries;
    serie->setColor(std::get<StatsKeyProperty::COLOR>(_map[key]));
    serie->setName(std::get<StatsKeyProperty::NAME>(_map[key]));
    std::get<StatsKeyProperty::SERIE>(_map[key]) = serie;
}

// bitrate.csv : time, bitrate, link, fps, frame_dropped, frame_decoded, frame_keydecoded, frame_rendered
// quic.csv : time, bitrate
void ReceivedBitrateDisplay::load(const fs::path& p)
{
    fs::path path = p / "bitrate.csv";

    using BitrateReader = CsvReaderTypeRepeat<',', int, 8>;

    create_serie(StatKey::BITRATE);
    create_serie(StatKey::LINK);
    create_serie(StatKey::FPS);

    for(auto &it : BitrateReader(path)) {
        const auto& [time, bitrate, link, fps, frame_dropped, frame_decoded, frame_key_decoded, frame_rendered] = it;

        QPoint p_bitrate(time, bitrate), p_fps(time, fps), p_link(time, link);

        add_point(StatKey::BITRATE, p_bitrate);
        add_point(StatKey::LINK, p_link);
        add_point(StatKey::FPS, p_fps);
    }

    add_serie(StatKey::BITRATE, _chart_bitrate);
    add_serie(StatKey::LINK, _chart_bitrate);
    add_serie(StatKey::FPS, _chart_fps);

    _chart_bitrate->createDefaultAxes();
    _chart_fps->createDefaultAxes();
}

void ReceivedBitrateDisplay::unload(const fs::path& path)
{

}
