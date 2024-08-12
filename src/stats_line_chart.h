#ifndef STATSLINECHART_H
#define STATSLINECHART_H

#include <QChart>
#include <QChartView>
#include <QGestureEvent>
#include <functional>

class StatsLineChartView : public QChartView
{
    bool _is_touching = false;
    std::vector<std::function<bool(QKeyEvent*)>> _key_event;

public:
    explicit StatsLineChartView(QWidget * parent = nullptr) : QChartView(parent) {}
    StatsLineChartView(QChart *chart, QWidget *parent = nullptr);

    void add_keyboard_event(std::function<bool(QKeyEvent*)> event) { _key_event.push_back(event); }

protected:
    bool viewportEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
};

class StatsLineChart : public QChart
{
public:
    explicit StatsLineChart(QGraphicsItem * parent = nullptr,  Qt::WindowFlags wFlags = {});

protected:
    bool sceneEvent(QEvent *event) override;
    // void resizeEvent(QResizeEvent* event) override;

private:
    bool gestureEvent(QGestureEvent *event);
};

#endif // STATSLINECHART_H
