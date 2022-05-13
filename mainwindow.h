#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_refresh_button_clicked();
    void image_mouse_move(int x, int y);

    void on_resample_button_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
