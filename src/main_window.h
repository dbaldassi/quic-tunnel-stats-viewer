#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include "received_bitrate_display.h"
#include "medooze_display.h"

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

public slots:

    void on_exp_changed(QTreeWidgetItem* item, int column);
};

#endif // MAIN_WINDOW_H
