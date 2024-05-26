#include "main_window.h"
#include "ui_main_window.h"

#include <filesystem>

#include <QStack>
// #include <thread>

namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _recv_display = std::make_unique<ReceivedBitrateDisplay>(ui->recv_tab, ui->recv_chart_layout, ui->legend_recv, ui->received_info);
    _medooze_display = std::make_unique<MedoozeDisplay>(ui->medooze_tab, ui->medooze_chart_layout, ui->legend_medooze, ui->medooze_info);
    _qlog_display = std::make_unique<QlogDisplay>(ui->qlog_tab, ui->qlog_chart_layout, ui->legend_qlog, ui->qlog_info);
    _sent_loss_display = std::make_unique<SentLossDisplay>(ui->sent_loss_tab, ui->loss_chart_layout);

    connect(ui->actionscreenshot, &QAction::triggered, this, &MainWindow::on_screenshot);
    connect(_qlog_display.get(), &QlogDisplay::on_loss_stats, _sent_loss_display.get(), &SentLossDisplay::on_quic_loss_stats);
    connect(_medooze_display.get(), &MedoozeDisplay::on_loss_stats, _sent_loss_display.get(), &SentLossDisplay::on_medooze_loss_stats);
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

    connect(menu, &QTreeWidget::itemChanged, this, &MainWindow::on_exp_changed);
}

void MainWindow::on_exp_changed(QTreeWidgetItem* item, int column)
{
    if(item->childCount() > 0) return;

    fs::path path = _stats_dir;

    QStack<QTreeWidgetItem*> stack;
    QTreeWidgetItem* curr = item;

    while((curr = dynamic_cast<QTreeWidgetItem*>(curr->parent()))) {
        stack.push(curr);
    }

    while(!stack.empty()) {
        path /= stack.pop()->text(0).toStdString();
    }

    path /= item->text(0).toStdString();

    if(item->checkState(0) == Qt::Checked) {
        /*std::thread([this, path]() { _recv_display->load(path); }).detach();
        std::thread([this, path]() { _medooze_display->load(path); }).detach();
        std::thread([this, path]() { _qlog_display->load(path); }).detach();*/

        _recv_display->load(path);
        _medooze_display->load(path);
        _qlog_display->load(path);
    }
    else {
        _recv_display->unload(path);
        _medooze_display->unload(path);
        _qlog_display->unload(path);
    }
}

void MainWindow::on_screenshot()
{
    fs::path dir = fs::temp_directory_path() / "tunnel_figures";

    if(!fs::exists(dir)) fs::create_directory(dir);

    _recv_display->save(dir);
    _medooze_display->save(dir);
    _qlog_display->save(dir);
}

MainWindow::~MainWindow()
{
    delete ui;
}
