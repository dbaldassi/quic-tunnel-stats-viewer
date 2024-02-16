#include "stats_line_chart.h"

StatsLineChartView::StatsLineChartView(QChart *chart, QWidget *parent)
    : QChartView(chart, parent)
{
    setRubberBand(QChartView::RectangleRubberBand);
}

void StatsLineChartView::keyPressEvent(QKeyEvent *event)
{
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


StatsLineChart::StatsLineChart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
    : QChart(QChart::ChartTypeCartesian, parent, wFlags)
{
    grabGesture(Qt::PanGesture);
    grabGesture(Qt::PinchGesture);
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

