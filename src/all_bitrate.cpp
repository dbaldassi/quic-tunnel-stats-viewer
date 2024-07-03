#include "all_bitrate.h"
#include "stats_line_chart.h"

#include <QListWidgetItem>
#include <QValueAxis>

std::vector<QColor> AllBitrateDisplay::colors = { Qt::blue, Qt::darkYellow/*, Qt::red*/, Qt::darkRed, Qt::red, Qt::darkCyan, Qt::darkMagenta };
int AllBitrateDisplay::current_color = 0;

AllBitrateDisplay::AllBitrateDisplay(QWidget* tab, QVBoxLayout* layout, QListWidget* legend, QTreeWidget* info)
    : DisplayBase(tab, legend, info)
{
    _chart = create_chart();
    _chart_view = create_chart_view(_chart);

    layout->addWidget(_chart_view, 1);

    _display_impl = false;
}

AllBitrateDisplay::~AllBitrateDisplay()
{}

void AllBitrateDisplay::init_map(StatMap& map, bool signal)
{}

void AllBitrateDisplay::create_legend(const fs::path& p, bool signal)
{
    if(signal) {
        connect(_legend, &QListWidget::itemChanged, this, [this, p](QListWidgetItem* item) -> void {
            StatKey key = static_cast<StatKey>(item->data(1).toUInt());

            auto& map = _path_keys[p.c_str()];
            auto line = std::get<StatsKeyProperty::SERIE>(map[key]);
            if(line == nullptr) return;

            if(item->checkState()) line->show();
            else line->hide();
        });
    }

    auto& map = _path_keys[p.c_str()];
    for(auto it = map.cbegin(); it != map.cend(); ++it) {
        QListWidgetItem * item = new QListWidgetItem(_legend);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
        item->setText(std::get<StatsKeyProperty::NAME>(it.value()));
        item->setData(1, static_cast<uint8_t>(it.key()));
    }
}

void AllBitrateDisplay::add_stats(const fs::path& path, StatKey key, std::tuple<QString, QLineSeries*, QChart*, ExpInfo> s)
{
    auto& map = _path_keys[path.c_str()];
    map[key] = s;

    // qInfo() << "Numa : " << std::get<StatsKeyProperty::NAME>(s);

    std::get<StatsKeyProperty::CHART>(map[key]) = _chart;

    auto info = std::get<StatsKeyProperty::INFO>(s);
    info.stream = false; // continuous line
    info.color = colors[current_color++];

    std::get<StatsKeyProperty::INFO>(map[key]) = info;

    if(key == QUIC_LOSS) {
        std::get<StatsKeyProperty::NAME>(map[key]) = "Quic Loss";
    }
    else if(key == MEDOOZE_LOSS) {
        std::get<StatsKeyProperty::NAME>(map[key]) = "Medooze Loss";
    }
    else if(key == TOTAL) {
        std::get<StatsKeyProperty::NAME>(map[key]) = "Medooze sent";
    }
    else {
        std::get<StatsKeyProperty::NAME>(map[key]) = std::get<StatsKeyProperty::NAME>(s);
    }

    create_serie(path, key);

    auto old_serie = std::get<StatsKeyProperty::SERIE>(s);

    auto pts = old_serie->points();
    for(auto pt : pts) {
        if(key == StatKey::CWND) pt.setY(pt.y() * 8. / 1000.);
        add_point(path.c_str(), key, pt);
    }
}

void AllBitrateDisplay::load(const fs::path& path)
{
    create_legend(path);

    auto& map =  _path_keys[path.c_str()];
    const auto& keys = map.keys();

    for(auto key : keys) {
        add_serie(path.c_str(), key);
    }

    _chart->createDefaultAxes();

    auto loss_axis = new QValueAxis();
    _chart->addAxis(loss_axis, Qt::AlignRight);

    auto* quic_loss_serie = std::get<StatsKeyProperty::SERIE>(map[StatKey::QUIC_LOSS]);
    auto* medooze_loss_serie = std::get<StatsKeyProperty::SERIE>(map[StatKey::MEDOOZE_LOSS]);

    const auto& quic_pts = quic_loss_serie->points();
    const auto& medooze_pts = medooze_loss_serie->points();

    if(!quic_pts.empty() && !medooze_pts.empty()) {
        const auto& pt1 = quic_pts.back();
        const auto& pt2 = medooze_pts.back();

        loss_axis->setRange(0,  std::max(pt1.y(), pt2.y()));
    }

    auto axis = quic_loss_serie->attachedAxes();
    quic_loss_serie->detachAxis(axis.back());
    quic_loss_serie->attachAxis(loss_axis);

    axis = medooze_loss_serie->attachedAxes();
    medooze_loss_serie->detachAxis(axis.back());
    medooze_loss_serie->attachAxis(loss_axis);

    QFont font1, font2;
    font1.setPointSize(36);
    font2.setPointSize(36);
    font1.setBold(true);
    font2.setBold(true);

    auto axe = _chart->axes(Qt::Horizontal);
    if(!axe.empty()) {
        axe.front()->setTitleText("Time (s)");
        axe.front()->setTitleFont(font1);
        axe.front()->setLabelsFont(font2);
        axe.front()->setGridLineVisible(false);
    }

    axe = _chart->axes(Qt::Vertical);
    if(!axe.empty()) {
        axe.front()->setTitleText("Bitrate (kbps)");
        axe.back()->setTitleText("Losses");

        for(auto& a : axe) {
            a->setTitleFont(font1);
            a->setLabelsFont(font2);
            a->setGridLineVisible(false);
        }
    }
}

void AllBitrateDisplay::unload(const fs::path& path)
{
    DisplayBase::unload(path);
}

void AllBitrateDisplay::save(const fs::path& path)
{

}

void AllBitrateDisplay::set_geometry(float ratio_w, float ratio_h)
{
    auto g = _chart_view->geometry();

    if(ratio_w == ratio_h) {
        if(g.width() > g.height()) g.setWidth(g.height());
        else g.setHeight((g.width()));
    }
    else {
        g.setHeight(g.width() * ratio_h);
    }

    _chart_view->setGeometry(g);
}
