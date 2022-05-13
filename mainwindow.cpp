#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->image->recalculate();
    connect(ui->image, &raster_label::mouse_move, this, &MainWindow::image_mouse_move);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_refresh_button_clicked()
{
    ui->image->set_coef(ui->coef->value());
    ui->image->recreate_raster(ui->size_x->value(), ui->size_y->value());
}

void MainWindow::image_mouse_move(int x, int y)
{
    ui->statusbar->showMessage(QString::number(x) + "/" + QString::number(y), 3000);
}


void MainWindow::on_resample_button_clicked()
{
    QSize size = ui->image->size();
    ui->size_x->setValue(size.width());
    ui->size_y->setValue(size.height());
    ui->image->set_coef(ui->coef->value());
    ui->image->recreate_raster(ui->size_x->value(), ui->size_y->value());
}

