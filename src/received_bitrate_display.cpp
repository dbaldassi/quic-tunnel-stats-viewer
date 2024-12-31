#include "received_bitrate_display.h"

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QBoxPlotSeries>

#include "csv_reader.h"
#include "stats_line_chart.h"
#include "all_bitrate.h"

std::vector<QColor> ReceivedBitrateDisplay::colors = { Qt::blue, Qt::yellow, Qt::darkCyan, Qt::darkRed, Qt::blue,
                                                      Qt::green, Qt::yellow, Qt::darkCyan, Qt::darkRed };

ReceivedBitrateDisplay::ReceivedBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info)
    : DisplayBase(tab, legend, info)
{
    _chart_bitrate = create_chart();
    _chart_view_bitrate = create_chart_view(_chart_bitrate);

    _chart_fps = create_chart();
    _chart_view_fps = create_chart_view(_chart_fps);

    layout->addWidget(_chart_view_bitrate, 1);
    layout->addWidget(_chart_view_fps, 1);

    create_legend();
}

void ReceivedBitrateDisplay::on_keyboard_event(QKeyEvent* key)
{
    if(key->key() == Qt::Key_1) {
        if(_chart_view_fps->isHidden()) _chart_view_fps->show();
        else _chart_view_fps->hide();
    }
    else if(key->key() == Qt::Key_2) {
        if(_chart_view_bitrate->isHidden()) _chart_view_bitrate->show();
        else _chart_view_bitrate->hide();
    }
}

void ReceivedBitrateDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::LINK] = std::make_tuple("link", nullptr, _chart_bitrate, ExpInfo{false, QuicImpl::NONE, CCAlgo::NONE, false, true}, false);
    map[StatKey::BITRATE] = std::make_tuple("bitrate", nullptr, _chart_bitrate, ExpInfo{}, false);
    map[StatKey::FPS] = std::make_tuple("fps", nullptr, _chart_fps, ExpInfo{}, false);
    map[StatKey::FRAME_DROPPED] = std::make_tuple("frame dropped", nullptr, nullptr, ExpInfo{}, false);
    map[StatKey::FRAME_DECODED] = std::make_tuple("frame decoded", nullptr, nullptr, ExpInfo{}, false);
    map[StatKey::FRAME_KEY_DECODED] = std::make_tuple("frame key decoded", nullptr, nullptr, ExpInfo{}, false);
    map[StatKey::FRAME_RENDERED] = std::make_tuple("frame rendered", nullptr, nullptr, ExpInfo{}, false);
    map[StatKey::QUIC_SENT] = std::make_tuple("quic sent bitrate", nullptr, _chart_bitrate, ExpInfo{}, false);

    map[StatKey::BITRATE_INTERQUARTILE] = std::make_tuple("bitrate interquartile", nullptr, _chart_bitrate, ExpInfo{}, false);
    map[StatKey::BITRATE_BOX] = std::make_tuple("bitrate box", nullptr, _chart_bitrate, ExpInfo{}, false);
    map[StatKey::FPS_INTERQUARTILE] = std::make_tuple("fps interquartile", nullptr, _chart_fps, ExpInfo{}, false);
    map[StatKey::FPS_BOX] = std::make_tuple("fps box", nullptr, _chart_fps, ExpInfo{}, false);

    if(signal) {
        connect(_legend, &QListWidget::itemChanged, this, [&map](QListWidgetItem* item) -> void {
            StatKey key = static_cast<StatKey>(item->data(1).toUInt());

            std::get<StatsKeyProperty::SHOW>(map[key]) = item->checkState() == Qt::Checked;

            auto line = std::get<StatsKeyProperty::SERIE>(map[key]);
            if(line == nullptr) return;

            if(item->checkState() == Qt::Checked) line->show();
            else line->hide();
        });
    }
}

void ReceivedBitrateDisplay::set_makeup(const fs::path& path)
{
    const auto& map = _path_keys[path.c_str()];

    QFont font = _chart_bitrate->font();
    font.setPointSize(40);
    font.setBold(true);

    _chart_bitrate->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    _chart_bitrate->legend()->setFont(font);
    _chart_bitrate->legend()->detachFromChart();

    _chart_fps->legend()->setMarkerShape(QLegend::MarkerShapeFromSeries);
    _chart_fps->legend()->setFont(font);
    _chart_fps->legend()->detachFromChart();

    for(const auto& [name, abs_serie, chart, info, show] : map) {
        auto* serie = dynamic_cast<QLineSeries*>(abs_serie);
        if(!serie) continue;

        auto pen = serie->pen();
        pen.setWidth(6);
        if(!info.stream) pen.setStyle(Qt::DashLine);

        QColor color = get_color(info);
        pen.setColor(color);

        if(name == "link") {
            pen.setWidth(8);
            pen.setColor(Qt::black);
            pen.setStyle(Qt::SolidLine);
        }
        else if(name == "quic sent bitrate") {
            pen.setStyle(Qt::DashDotDotLine);
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
    if(!axes.empty()) setup_axes(axes.front(), "Bitrate (kbps)");

    axes = _chart_fps->axes(Qt::Horizontal);
    if(!axes.empty()) setup_axes(axes.front(), "Time (s)");

    axes = _chart_fps->axes(Qt::Vertical);
    if(!axes.empty()) setup_axes(axes.front(), "FPS");
}

void ReceivedBitrateDisplay::load_exp(const fs::path& p)
{
    fs::path path = p / "bitrate.csv";

    using BitrateReader = CsvReaderTypeRepeat<',', int, 8>;
    using QuicSentReader = CsvReaderTypeRepeat<',', double, 2>;

    if(_path_keys.empty()) create_serie(p, StatKey::LINK);

    create_serie(p, StatKey::BITRATE);
    create_serie(p, StatKey::FPS);
    create_serie(p, StatKey::QUIC_SENT);

    auto& map = _path_keys[p.c_str()];
    auto info = std::get<StatsKeyProperty::INFO>(map[StatKey::QUIC_SENT]);
    info.color = colors[current_color];
    std::get<StatsKeyProperty::INFO>(map[StatKey::QUIC_SENT]) = info;
    info = std::get<StatsKeyProperty::INFO>(map[StatKey::BITRATE]);
    info.color = colors[current_color];
    std::get<StatsKeyProperty::INFO>(map[StatKey::BITRATE]) = info;

    ++current_color;
    current_color = current_color % colors.size();

    // std::get<StatsKeyProperty::NAME>(_path_keys[p.c_str()][StatKey::BITRATE]) = p.c_str();

    uint64_t sum = 0;

    /*for(auto &it : BitrateReader(path)) {
        const auto& [time, bitrate, link, fps, frame_dropped, frame_decoded, frame_key_decoded, frame_rendered] = it;
        QPoint p_bitrate(time, bitrate), p_fps(time, fps), p_link(time, link);
        if(_path_keys.size() == 1)  add_point(p.c_str(), StatKey::LINK, p_link);
    }*/

    for(auto &it : BitrateReader(path)) {
        const auto& [time, bitrate, link, fps, frame_dropped, frame_decoded, frame_key_decoded, frame_rendered] = it;

        QPoint p_bitrate(time, bitrate), p_fps(time, fps), p_link(time, link);

        if(_path_keys.size() == 1)  add_point(p.c_str(), StatKey::LINK, p_link);
        add_point(p.c_str(), StatKey::BITRATE, p_bitrate);
        add_point(p.c_str(), StatKey::FPS, p_fps);

        _infos_array[StatKey::BITRATE].mean += bitrate;
        _infos_array[StatKey::BITRATE].variance += (bitrate * bitrate);
        _infos_array[StatKey::FPS].mean += fps;
        _infos_array[StatKey::FPS].variance += (fps * fps);

        ++sum;
    }

    _infos_array[StatKey::BITRATE].mean /= sum;
    _infos_array[StatKey::BITRATE].variance = (_infos_array[StatKey::BITRATE].variance / sum) - ( _infos_array[StatKey::BITRATE].mean *  _infos_array[StatKey::BITRATE].mean);
    _infos_array[StatKey::FPS].mean /= sum;
    _infos_array[StatKey::FPS].variance = (_infos_array[StatKey::FPS].variance / sum) - ( _infos_array[StatKey::FPS].mean *  _infos_array[StatKey::FPS].mean);

    if(_path_keys.size() == 1) add_serie(p.c_str(), StatKey::LINK);
    add_serie(p.c_str(), StatKey::BITRATE);
    add_serie(p.c_str(), StatKey::FPS);

    QTreeWidgetItem * item = new QTreeWidgetItem(_info);
    item->setText(0, path.parent_path().filename().c_str());

    QTreeWidgetItem * bitrate_mean = new QTreeWidgetItem(item);
    bitrate_mean->setText(0, "RTC Bitrate mean");
    bitrate_mean->setText(1, QString::number(_infos_array[StatKey::BITRATE].mean));

    QTreeWidgetItem * bitrate_variance = new QTreeWidgetItem(item);
    bitrate_variance->setText(0, "RTC Bitrate variance");
    bitrate_variance->setText(1, QString::number(_infos_array[StatKey::BITRATE].variance));

    QTreeWidgetItem * fps_mean = new QTreeWidgetItem(item);
    fps_mean->setText(0, "FPS mean");
    fps_mean->setText(1, QString::number(_infos_array[StatKey::FPS].mean));

    QTreeWidgetItem * fps_variance= new QTreeWidgetItem(item);
    fps_variance->setText(0, "FPS variance");
    fps_variance->setText(1, QString::number(_infos_array[StatKey::FPS].variance));

    fs::path quiccsv = p / "quic.csv";

    if(fs::exists(quiccsv)) {

        for(auto& it : QuicSentReader(quiccsv)) {
            const auto& [time, bitrate ] = it;
            QPointF p_bitrate{time, bitrate * 8. / 1000.};

            add_point(p.c_str(), StatKey::QUIC_SENT, p_bitrate);

            _infos_array[StatKey::QUIC_SENT].mean += p_bitrate.y();
            _infos_array[StatKey::QUIC_SENT].variance += (p_bitrate.y() * p_bitrate.y());
        }

        add_serie(p.c_str(), StatKey::QUIC_SENT);

        _infos_array[StatKey::QUIC_SENT].mean /= sum;
        _infos_array[StatKey::QUIC_SENT].variance = (_infos_array[StatKey::QUIC_SENT].variance / sum) - ( _infos_array[StatKey::QUIC_SENT].mean *  _infos_array[StatKey::QUIC_SENT].mean);

        QTreeWidgetItem * quic_mean = new QTreeWidgetItem(item);
        quic_mean->setText(0, "QUIC sent mean");
        quic_mean->setText(1, QString::number(_infos_array[StatKey::QUIC_SENT].mean));

        QTreeWidgetItem * quic_variance = new QTreeWidgetItem(item);
        quic_variance->setText(0, "QUIC sent variance");
        quic_variance->setText(1, QString::number(_infos_array[StatKey::QUIC_SENT].variance));
    }
}

template<typename T>
bool ReceivedBitrateDisplay::get_stats(const fs::path& p, std::ifstream& ifs, std::vector<StatLinePoint<T>>& tab, StatKey key)
{
    tab.emplace_back(StatLinePoint<T>{});
    if(!get_csv_line(ifs, tab)) {
        tab.pop_back();
        return false;
    }

    StatKey key_box, key_inter;
    switch(key) {
    case StatKey::BITRATE:
        key_box = StatKey::BITRATE_BOX;
        key_inter= StatKey::BITRATE_INTERQUARTILE;
        break;
    case StatKey::FPS:
        key_box = StatKey::FPS_BOX;
        key_inter = StatKey::FPS_INTERQUARTILE;
        break;
    case StatKey::LINK: {
        QPoint pt{(int)tab.back().time, tab.back().values.front()};
        add_point(p.c_str(), key, pt);
        return true;
    }
    default:
        return false;
    }

    QPointF avg{(double)tab.back().time, get_average(tab.back().values)};
    QPointF inter{(double)tab.back().time, get_interquartile_average(tab.back().values)};

    add_point(p.c_str(), key, avg);
    add_point(p.c_str(), key_inter, inter);
    add_point(p.c_str(), key_box, QString::number(tab.back().time), tab.back().values);

    return true;
}

void ReceivedBitrateDisplay::load_stat_line(const fs::path& p)
{
    fs::path file = p / "bitrate_line.csv";

    std::ifstream ifs(file.c_str());
    if(!ifs.is_open()) {
        throw std::runtime_error("Could not open csv file with provided path : " + file.string());
    }

    // to have link only one time
    // == 1 becasue there should already be legend hence not empty
    if(_path_keys.size() == 1) create_serie(p, StatKey::LINK);

    create_serie(p, StatKey::BITRATE);
    create_serie(p, StatKey::BITRATE_INTERQUARTILE);
    create_serie<QBoxPlotSeries>(p, StatKey::BITRATE_BOX);

    create_serie(p, StatKey::FPS);
    create_serie(p, StatKey::FPS_INTERQUARTILE);
    create_serie<QBoxPlotSeries>(p, StatKey::FPS_BOX);

    // set_makeup(p);

    std::vector<StatLinePoint<int>> bitrate;
    std::vector<StatLinePoint<int>> fps;
    std::vector<StatLinePoint<int>> link;

    while(!ifs.eof()) {
        if(!get_stats(p, ifs, link, StatKey::LINK)) break;
        get_stats(p, ifs, bitrate, StatKey::BITRATE);
        get_stats(p, ifs, fps, StatKey::FPS);
    }

    add_serie(p.c_str(), StatKey::LINK);
    add_serie(p.c_str(), StatKey::BITRATE);
    add_serie(p.c_str(), StatKey::BITRATE_INTERQUARTILE);
    add_serie(p.c_str(), StatKey::FPS);
    add_serie(p.c_str(), StatKey::FPS_INTERQUARTILE);
    // add_serie<QBoxPlotSeries>(p.c_str(), StatKey::BITRATE_BOX);
    // add_serie<QBoxPlotSeries>(p.c_str(), StatKey::FPS_BOX);
}

// bitrate.csv : time, bitrate, link, fps, frame_dropped, frame_decoded, frame_keydecoded, frame_rendered
// quic.csv : time, bitrate
void ReceivedBitrateDisplay::load(const fs::path& p)
{
    if(p.filename().string() == "average") load_stat_line(p);
    else load_exp(p);

    _chart_bitrate->createDefaultAxes();
    _chart_fps->createDefaultAxes();

    set_makeup(p);

    // _chart_view_fps->hide();
    // _chart_view_bitrate->setGeometry(0,0,1,1);
}

void ReceivedBitrateDisplay::save(const fs::path& dir)
{
    auto bitrate_filename = dir / "received_bitrate.png";
    _chart_view_bitrate->grab().save(bitrate_filename.c_str(), "PNG");

    auto fps_filename = dir / "received_fps.png";
    _chart_view_fps->grab().save(fps_filename.c_str(), "PNG");
}

void ReceivedBitrateDisplay::add_to_all(const fs::path& dir, AllBitrateDisplay* all)
{
    auto& map = _path_keys[dir.c_str()];
    // all->add_stats(dir, AllBitrateDisplay::LINK, map[LINK]);
    // all->add_stats(dir, AllBitrateDisplay::BITRATE, map[BITRATE]);
    // all->add_stats(dir, AllBitrateDisplay::QUIC_SENT, map[QUIC_SENT]);
}

void ReceivedBitrateDisplay::set_geometry(float ratio_w, float ratio_h)
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
