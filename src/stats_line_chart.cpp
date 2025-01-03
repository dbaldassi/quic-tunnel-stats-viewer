#include <QGraphicsLayout>
#include "stats_line_chart.h"

StatsLineChartView::StatsLineChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setRubberBand(QChartView::RectangleRubberBand);
}

void StatsLineChartView::keyPressEvent(QKeyEvent *event)
{
    for(auto&& ev : _key_event)  ev(event);

    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

bool StatsLineChartView::viewportEvent(QEvent *event)
{
    if (event->type() == QEvent::TouchBegin) {
        // By default touch events are converted to mouse events. So
        // after this event we will get a mouse event also but we want
        // to handle touch events as gestures only. So we need this safeguard
        // to block mouse events that are actually generated from touch.
        _is_touching = true;

        chart()->setAnimationOptions(QChart::NoAnimation);
    }

    return QChartView::viewportEvent(event);
}

void StatsLineChartView::mousePressEvent(QMouseEvent *event)
{
    if (_is_touching) return;
    QChartView::mousePressEvent(event);
}

void StatsLineChartView::mouseMoveEvent(QMouseEvent *event)
{
    if (_is_touching) return;
    QChartView::mouseMoveEvent(event);
}

void StatsLineChartView::mouseReleaseEvent(QMouseEvent *event)
{
    if (_is_touching) _is_touching = false;

    chart()->setAnimationOptions(QChart::SeriesAnimations);

    QChartView::mouseReleaseEvent(event);
}

void StatsLineChartView::resizeEvent(QResizeEvent* event)
{
    QChart * c = chart();

    if(!c) return;

    auto geometry = c->geometry();
    // geometry.setHeight(geometry.width() * 0.7);
    // QChartView::setGeometry(geometry.x(), geometry.y(), geometry.width(), geometry.height() * 0.7);

    c->legend()->setGeometry(geometry);
    c->setPlotArea(QRectF());
    QChartView::resizeEvent(event);
    // QChartView::setGeometry(geometry.x(), geometry.y(), geometry.width(), geometry.height() * 0.7);

    auto area = chart()->plotArea();
    area.setRect(area.x(), area.y() + 200, area.width(), area.height() - 200);
    c->setPlotArea(area);

    auto axes = chart()->axes(Qt::Vertical);
    if(!axes.empty()) {
        axes.front()->setVisible(false);
        axes.front()->setVisible(true);
    }

    // QChartView::resizeEvent(event);
}

StatsLineChart::StatsLineChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(QChart::ChartTypeCartesian, parent, wFlags)
{
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);

    layout()->setContentsMargins(0, 0, 0, 0);
    setBackgroundRoundness(0);
}

bool StatsLineChart::sceneEvent(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestureEvent(static_cast<QGestureEvent *>(event));
    return QChart::event(event);
}

bool StatsLineChart::gestureEvent(QGestureEvent *event)
{
    if (QGesture *gesture = event->gesture(Qt::PanGesture)) {
        auto pan = static_cast<QPanGesture *>(gesture);
        QChart::scroll(-(pan->delta().x()), pan->delta().y());
    }

    if (QGesture *gesture = event->gesture(Qt::PinchGesture)) {
        auto pinch = static_cast<QPinchGesture *>(gesture);
        if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
            QChart::zoom(pinch->scaleFactor());
    }

    return true;
}

