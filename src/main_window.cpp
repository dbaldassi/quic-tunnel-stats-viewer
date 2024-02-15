#include "main_window.h"
#include "ui_main_window.h"

#include <iostream>
#include <filesystem>

#include <QStack>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _recv_display = std::make_unique<ReceivedBitrateDisplay>(ui->recv_tab, ui->legend_list);
}

void MainWindow::set_stats_dir(std::string dir)
{
    _stats_dir = std::move(dir);
}

void MainWindow::load()
{
    auto menu = ui->exp_menu;

    menu->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    QStack<QTreeWidgetItem*> items;
    int prev_depth = -1;

    // Create tree menu to choose experiment
    for(auto it = fs::recursive_directory_iterator{_stats_dir}; it != fs::recursive_directory_iterator(); ++it) {
        QTreeWidgetItem * item = nullptr;

        if(!it->is_directory()) continue;

        if(it.depth() == prev_depth) items.pop();
        else if(prev_depth != -1 && it.depth() < prev_depth) {
            for(int i = it.depth(); i <= prev_depth; ++i) items.pop();
        }

        if(it.depth() == 0) item = new QTreeWidgetItem(menu);
        else item = new QTreeWidgetItem(items.top());

        item->setFlags(Qt::ItemIsUserCheckable);
        item->setCheckState(0, Qt::Unchecked);
        item->setText(0, it->path().filename().c_str());
        item->setDisabled(false);

        items.push(item);
        prev_depth = it.depth();
    }

    _recv_display->load(fs::path{_stats_dir} / "mvfst/streams/mvfst_none_stream_Thu_Jan_25_16:23:21_2024/");
}

MainWindow::~MainWindow()
{
    delete ui;
}
