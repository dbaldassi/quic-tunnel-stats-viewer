#ifndef STATSLINECHART_H
#define STATSLINECHART_H

#include <QChart>
#include <QChartView>
#include <QGestureEvent>

class StatsLineChartView : public QChartView
{
    bool _is_touching = false;
public:
    explicit StatsLineChartView(QWidget * parent = nullptr) : QChartView(parent) {}
    StatsLineChartView(QChart *chart, QWidget *parent = nullptr);

protected:
    bool viewportEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
};

class StatsLineChart : public QChart
{
public:
    explicit StatsLineChart(QGraphicsItem * parent = nullptr,  Qt::WindowFlags wFlags = {});

protected:
    bool sceneEvent(QEvent *event) override;

private:
    bool gestureEvent(QGestureEvent *event);
};

#endif // STATSLINECHART_H
