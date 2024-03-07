#include "display_base.h"
#include "stats_line_chart.h"

#include <QTabWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QHeaderView>

DisplayBase::DisplayBase(QWidget* tab, QListWidget* legend, QTreeWidget* info)
    : _tab(tab), _legend(legend), _info(info)
{
    _tab->grabGesture(Qt::PanGesture);
    _tab->grabGesture(Qt::PinchGesture);

    info->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    info->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
}

StatsLineChart * DisplayBase::create_chart()
{
    auto chart = new StatsLineChart();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->createDefaultAxes();
    chart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart;
}

StatsLineChartView * DisplayBase::create_chart_view(QChart* chart)
{
    auto chart_view = new StatsLineChartView(chart, _tab);
    chart_view->setRenderHint(QPainter::Antialiasing);
    chart_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    return chart_view;
}

void DisplayBase::create_legend()
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

void DisplayBase::create_serie(const fs::path&p, uint8_t key)
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

void DisplayBase::unload(const fs::path& path)
{
    auto map = _path_keys[path.c_str()];

    for(auto& it : map) {
        auto s = std::get<StatsKeyProperty::SERIE>(it);
        auto* chart = std::get<StatsKeyProperty::CHART>(it);

        if(chart && s) {
            chart->removeSeries(s);
            delete s;
        }

        std::get<StatsKeyProperty::SERIE>(it) = nullptr;
    }

    auto item = _info->findItems(path.filename().c_str(), Qt::MatchExactly);

    for(auto* it: item) {
        _info->removeItemWidget(it, 0);
        delete it;
    }
}
