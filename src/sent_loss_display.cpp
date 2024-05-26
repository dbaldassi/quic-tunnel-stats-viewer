#include "sent_loss_display.h"

#include <QChart>
#include <QBarSet>


SentLossDisplay::SentLossDisplay(QWidget* tab, QVBoxLayout* layout) : _layout(layout)
{
    _chart = new QChart();
    _chart->setAnimationOptions(QChart::SeriesAnimations);

    _chart_view = new QChartView(_chart, tab);
    _chart_view->setRenderHint(QPainter::Antialiasing);

    _layout->addWidget(_chart_view, 1);
}

SentLossDisplay::~SentLossDisplay()
{

}

void SentLossDisplay::on_medooze_loss_stats(const fs::path& path, int loss, int sent)
{
    if(!_series.contains(path.c_str())) {
        _series[path.c_str()] = new QPercentBarSeries();

        _axis_y = new QValueAxis();
        _chart->addAxis(_axis_y, Qt::AlignLeft);
    }

    auto serie = _series[path.c_str()];

    QBarSet * _medooze_sent_set = new QBarSet("Medooze sent");
    QBarSet * _medooze_loss_set = new QBarSet("Medooze loss");
    *_medooze_sent_set << sent;
    *_medooze_loss_set << loss;

    serie->append(_medooze_sent_set);
    serie->append(_medooze_loss_set);

    _chart->removeSeries(serie);
    _chart->addSeries(serie);
    // _chart->addSeries(serie);

    serie->detachAxis(_axis_y);
    serie->attachAxis(_axis_y);
}

void SentLossDisplay::on_quic_loss_stats(const fs::path& path, int loss, int sent)
{
    if(!_series.contains(path.c_str())) {
        _series[path.c_str()] = new QPercentBarSeries();

        _axis_y = new QValueAxis();
        _chart->addAxis(_axis_y, Qt::AlignLeft);
    }

    auto serie = _series[path.c_str()];

    QBarSet * _quic_sent_set = new QBarSet("Quic sent");
    QBarSet * _quic_loss_set = new QBarSet("Quic loss");
    *_quic_sent_set << sent;
    *_quic_loss_set << loss;

    serie->append(_quic_sent_set);
    serie->append(_quic_loss_set);

    _chart->removeSeries(serie);
    _chart->addSeries(serie);
    // _chart->addSeries(serie);

    serie->detachAxis(_axis_y);
    serie->attachAxis(_axis_y);

    // QBarSet * _quic_sent_set = new QBarSet("Quic sent");
    // QBarSet * _quic_loss_set = new QBarSet("Quic sent");
}
