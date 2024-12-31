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
    StatMap& map = _path_keys["legend"];
    init_map(map);

    for(auto it = map.cbegin(); it != map.cend(); ++it) {
        QListWidgetItem * item = new QListWidgetItem(_legend);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Unchecked);
        item->setText(std::get<StatsKeyProperty::NAME>(it.value()));
        item->setData(1, static_cast<uint8_t>(it.key()));
    }
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

    _path_keys.remove(path.c_str());
}

void DisplayBase::set_info(const fs::path& path)
{
    auto info = get_info(path);

    auto& map = _path_keys[path.c_str()];

    // qInfo() << info.stream << " " << info.cc << " " << info.impl;

    for(auto& it : map) {
        auto info_tmp = std::get<StatsKeyProperty::INFO>(it);
        if(info_tmp.editable) std::get<StatsKeyProperty::INFO>(it) = info;
    }
}

DisplayBase::ExpInfo DisplayBase::get_info(const fs::path& path)
{
    ExpInfo info;

    for(auto& it : path) {
        if(it == "streams" || it == "stream") {
            info.stream = true;
        }
        else if(it == "dgrams" || it == "dgram") {
            info.stream = false;
        }
        else if(it == "mvfst") {
            info.impl = QuicImpl::MVFST;
            info.impl_str = it.c_str();
        }
        else if(it == "quicgo") {
            info.impl = QuicImpl::QUICGO;
            info.impl_str = it.c_str();
        }
        else if(it == "quiche") {
            info.impl = QuicImpl::QUICHE;
            info.impl_str = it.c_str();
        }
        else if(it == "msquic") {
            info.impl = QuicImpl::MSQUIC;
            info.impl_str = it.c_str();
        }
        else if(it == "udp") {
            info.impl = QuicImpl::UDP;
            info.impl_str = it.c_str();
        }
        else if(it == "bbr") {
            info.cc = CCAlgo::BBR;
            info.cc_str = it.c_str();
        }
        else if(it == "newreno") {
            info.cc = CCAlgo::NEWRENO;
            info.cc_str = it.c_str();
        }
        else if(it == "none") {
            info.cc = CCAlgo::NONE;
            info.cc_str = it.c_str();
        }
        else if(it == "copa") {
            info.cc = CCAlgo::COPA;
            info.cc_str = it.c_str();
        }
        else if(it == "cubic") {
            info.cc = CCAlgo::CUBIC;
            info.cc_str = it.c_str();
        }
    }

    return info;
}


QColor DisplayBase::get_color(const DisplayBase::ExpInfo& info)
{
    switch(info.impl) {
    case QuicImpl::MVFST:
        switch(info.cc) {
        case CCAlgo::BBR:
            return Qt::green;
        case CCAlgo::CUBIC:
            return Qt::darkCyan;
        case CCAlgo::NEWRENO:
            return Qt::darkBlue;
        case CCAlgo::COPA:
            return Qt::darkMagenta;
        case CCAlgo::NONE:
            return Qt::darkGray;
        }
        break;
    case QuicImpl::MSQUIC:
        switch(info.cc) {
        case CCAlgo::BBR:
            return Qt::darkRed;
        case CCAlgo::CUBIC:
            return Qt::darkYellow;
        default:
            break;
        }
        break;
    case QuicImpl::QUICGO:
        return QColor(255,128,0);
    case QuicImpl::UDP:
        return QColor(255,0,196);
    case QuicImpl::QUICHE:
        break;
    default:
        return Qt::black;
    }

    return Qt::black;
}
