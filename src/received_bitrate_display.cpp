#include "received_bitrate_display.h"

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>

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

ReceivedBitrateDisplay::ReceivedBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend)
    : _tab(tab), _legend(legend)
{
    create_legend();

    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_fps = create_chart();
    _chart_view_fps = create_chart_view(_chart_fps);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_fps, 1);

    _tab->grabGesture(Qt::PanGesture);
    _tab->grabGesture(Qt::PinchGesture);
}

void ReceivedBitrateDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::LINK] = std::make_tuple("link", nullptr, ChartKey::BITRATE);
    map[StatKey::BITRATE] = std::make_tuple("bitrate", nullptr, ChartKey::BITRATE);
    map[StatKey::FPS] = std::make_tuple("fps", nullptr, ChartKey::FPS);
    map[StatKey::FRAME_DROPPED] = std::make_tuple("frame dropped", nullptr, ChartKey::NONE);
    map[StatKey::FRAME_DECODED] = std::make_tuple("frame decoded", nullptr, ChartKey::NONE);
    map[StatKey::FRAME_KEY_DECODED] = std::make_tuple("frame key decoded", nullptr, ChartKey::NONE);
    map[StatKey::FRAME_RENDERED] = std::make_tuple("frame rendered", nullptr, ChartKey::NONE);
    map[StatKey::QUIC_SENT] = std::make_tuple("quic sent bitrate", nullptr, ChartKey::BITRATE);

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

void ReceivedBitrateDisplay::create_legend()
{
    StatMap map;
    init_map(map, false);

    for(auto it = map.cbegin(); it != map.cend(); ++it) {
        QListWidgetItem * item = new QListWidgetItem(_legend);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
        item->setText(std::get<StatsKeyProperty::NAME>(it.value()));
        item->setData(1, static_cast<uint8_t>(it.key()));
    }
}

void ReceivedBitrateDisplay::create_serie(const fs::path&p, StatKey key)
{
    auto serie = new QLineSeries;

    QString name;
    QTextStream stream(&name);

    std::stringstream exp_name(p.filename().string());

    StatMap& map = _path_keys[p.c_str()];
    if(map.empty()) init_map(map);

    stream << std::get<StatsKeyProperty::NAME>(map[key]) << " "
           << "(";

    for(int i = 0; i < 3; ++i) {
        std::string line;
        std::getline(exp_name, line, '_');

        if(i != 0) stream << "_";

        stream << line.c_str();
    }

    stream << ")";

    serie->setName(name);
    std::get<StatsKeyProperty::SERIE>(map[key]) = serie;
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

    add_serie(p.c_str(), StatKey::BITRATE, _chart_bitrate);
    add_serie(p.c_str(), StatKey::LINK, _chart_bitrate);
    add_serie(p.c_str(), StatKey::FPS, _chart_fps);

    _chart_bitrate->createDefaultAxes();
    _chart_fps->createDefaultAxes();
}

void ReceivedBitrateDisplay::unload(const fs::path& path)
{
    auto map = _path_keys[path.c_str()];

    for(auto& it : map) {
        auto s = std::get<StatsKeyProperty::SERIE>(it);
        auto chart = std::get<StatsKeyProperty::CHART>(it);

        switch(chart) {
        case ChartKey::BITRATE:
            _chart_bitrate->removeSeries(s);
            break;
        case ChartKey::FPS:
            _chart_fps->removeSeries(s);
            break;
        case ChartKey::NONE: break;
        }

        delete s;

        std::get<StatsKeyProperty::SERIE>(it) = nullptr;
    }
}
