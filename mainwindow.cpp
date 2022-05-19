#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->image, &raster_label::mouse_move, this, &MainWindow::image_mouse_move);
    connect(ui->image, &raster_label::mouse_pressed, this, &MainWindow::image_mouse_pressed);
    connect(ui->image, &raster_label::calculation_started, this, &MainWindow::image_calculations_begin);
    connect(ui->image, &raster_label::calculation_finished, this, &MainWindow::image_calculations_end);
    ui->image->recalculate();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_refresh_button_clicked()
{
    if (ui->image->is_calculations_in_progress())
    {
        ui->image->interrupt_calculation();
    }
    else
    {
        ui->image->set_zoom(ui->coef->value());
        ui->image->set_steps(ui->steps->value());
        ui->image->set_coords(ui->coord_x->value(), ui->coord_y->value());
        ui->image->recreate_raster(ui->size_x->value(), ui->size_y->value());
    }
}

void MainWindow::image_mouse_move(QString msg)
{
    ui->statusbar->showMessage(msg, 3000);
}

void MainWindow::image_mouse_pressed(double x, double y)
{
    ui->coord_x->setValue(x);
    ui->coord_y->setValue(y);
}

void MainWindow::image_calculations_begin()
{
    ui->refresh_button->setText("Прервать");
}

void MainWindow::image_calculations_end()
{
    ui->refresh_button->setText("Обновить");
}


void MainWindow::on_resample_button_clicked()
{
    QSize size = ui->image->size();
    ui->size_x->setValue(size.width());
    ui->size_y->setValue(size.height());
    ui->image->set_zoom(ui->coef->value());
    ui->image->recreate_raster(ui->size_x->value(), ui->size_y->value());
}


void MainWindow::on_save_image_clicked()
{
    QString str = QFileDialog::getSaveFileName(nullptr, "Choose filename");
    const QImage* raster = ui->image->get_raster();
    if (!raster->save(str))
    {
        QMessageBox* err = new QMessageBox("error", "Не удалось сохранить.", QMessageBox::Critical, QMessageBox::Ok, 0, 0);
        err->exec();
        delete err;
    }
}

