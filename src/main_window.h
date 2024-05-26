#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include "received_bitrate_display.h"
#include "medooze_display.h"
#include "qlog_display.h"
#include "sent_loss_display.h"

namespace Ui {
class MainWindow;
}

class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

    std::string _stats_dir;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void set_stats_dir(std::string dir);
    void load();

private:
    Ui::MainWindow *ui;

    std::unique_ptr<ReceivedBitrateDisplay> _recv_display;
    std::unique_ptr<MedoozeDisplay> _medooze_display;
    std::unique_ptr<QlogDisplay> _qlog_display;
    std::unique_ptr<SentLossDisplay> _sent_loss_display;

public slots:

    void on_exp_changed(QTreeWidgetItem* item, int column);
    void on_screenshot();
};

#endif // MAIN_WINDOW_H
