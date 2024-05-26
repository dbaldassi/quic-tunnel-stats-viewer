#ifndef SENTLOSSDISPLAY_H
#define SENTLOSSDISPLAY_H

#include <QObject>
#include <QList>
#include <QPercentBarSeries>
#include <QLayout>
#include <QChartView>
#include <QValueAxis>

#include <filesystem>

namespace fs = std::filesystem;

class SentLossDisplay : public QObject
{
    Q_OBJECT

    // QList<QBarSet*> _bars;
    // QList<QBarSeries*> _series;

    QChart*  _chart;
    QChartView* _chart_view;
    QVBoxLayout* _layout;

    QValueAxis * _axis_y;

    QMap<QString, QPercentBarSeries*> _series;
public:
    explicit SentLossDisplay(QWidget* tab, QVBoxLayout* layout);
    ~SentLossDisplay();

public slots:
    void on_medooze_loss_stats(const fs::path& path, int loss, int sent);
    void on_quic_loss_stats(const fs::path& path, int loss, int sent);
};

#endif // SENTLOSSDISPLAY_H
