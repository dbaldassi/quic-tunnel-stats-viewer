#include "received_bitrate_display.h"

#include <QTabWidget>
#include <QListWidget>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QTreeWidgetItem>

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

void ReceivedBitrateDisplay::init_map(StatMap& map, bool signal)
{
    map[StatKey::LINK] = std::make_tuple("link", nullptr, _chart_bitrate, ExpInfo{false, QuicImpl::NONE, CCAlgo::NONE, false, true});
    map[StatKey::BITRATE] = std::make_tuple("bitrate", nullptr, _chart_bitrate, ExpInfo{});
    map[StatKey::FPS] = std::make_tuple("fps", nullptr, _chart_fps, ExpInfo{});
    map[StatKey::FRAME_DROPPED] = std::make_tuple("frame dropped", nullptr, nullptr, ExpInfo{});
    map[StatKey::FRAME_DECODED] = std::make_tuple("frame decoded", nullptr, nullptr, ExpInfo{});
    map[StatKey::FRAME_KEY_DECODED] = std::make_tuple("frame key decoded", nullptr, nullptr, ExpInfo{});
    map[StatKey::FRAME_RENDERED] = std::make_tuple("frame rendered", nullptr, nullptr, ExpInfo{});
    map[StatKey::QUIC_SENT] = std::make_tuple("quic sent bitrate", nullptr, _chart_bitrate, ExpInfo{});

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

        /*QTreeWidgetItem * quic_mean = new QTreeWidgetItem(item);
        quic_mean->setText(0, "QUIC sent mean");
        quic_mean->setText(1, QString::number(_infos_array[StatKey::QUIC_SENT].mean));

        QTreeWidgetItem * quic_variance = new QTreeWidgetItem(item);
        quic_variance->setText(0, "QUIC sent variance");
        quic_variance->setText(1, QString::number(_infos_array[StatKey::QUIC_SENT].variance));*/
    }

    _chart_bitrate->createDefaultAxes();
    _chart_fps->createDefaultAxes();

    QFont font1, font2;
    font1.setPointSize(40);
    font2.setPointSize(36);
    font1.setBold(true);
    font2.setBold(true);

    auto axe = _chart_bitrate->axes(Qt::Horizontal);
    if(!axe.empty()) {
        axe.front()->setTitleText("Time (s)");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
        axe.front()->setGridLineVisible(false);
    }

    axe = _chart_bitrate->axes(Qt::Vertical);
    if(!axe.empty()) {
        axe.front()->setTitleText("Bitrate (kbps)");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
        axe.front()->setGridLineVisible(false);
    }

    axe = _chart_fps->axes(Qt::Horizontal);
    if(!axe.empty()) {
        axe.front()->setTitleText("Time (s)");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
    }

    axe = _chart_fps->axes(Qt::Vertical);
    if(!axe.empty()) {
        axe.front()->setTitleText("FPS");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
    }

    _chart_view_fps->hide();

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
