#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>

#include "received_bitrate_display.h"

namespace Ui {
class MainWindow;
}

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
};

#endif // MAIN_WINDOW_H
