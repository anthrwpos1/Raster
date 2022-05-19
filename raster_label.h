#ifndef RASTER_LABEL_H
#define RASTER_LABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QThread>
#include <QMutex>
#include <memory>
#include "mandelbrot.h"

//вычислительный поток. Берет ближайшую свободную 1000 точек и вычисляет их.
class calc_thread : public QThread
{
    Q_OBJECT;

    mandelbrot* _points;//точечки изображения
    QMutex* _index_mutex;//мутех на доступ к переменной хранящей индекс первой непосчитанной точки
    int* _index_map;//индекс первой непосчитанной точки
    int _max_index;//всего число точек
    int* _active;//переменная для прерывания процесса вычисления
    int _mmax;//число итераций
    int _num;//порядковый номер потока (для отладки)
public:
    calc_thread(mandelbrot *points, QMutex *index_mutex, int *index_map, int max_index, int *active, int mmax, int num);
    // QThread interface
protected:
    void run() override;
};

//поток обслуживания потоков. Запускает нужное число потоков, отслеживает выполнение работы, прерывает работу, периодически сигнал перерисовать картинку.
class calc_threads_manager_thread : public QThread
{
    Q_OBJECT;

    mandelbrot* _points;
    int _max_index;
    int _mmax;
    int _threadnum;
    int _active = 0;
public:
    calc_threads_manager_thread(mandelbrot* points, int max_index, int mmax, int threadnum);
    int* get_active_ptr();

Q_SIGNALS:
    void image_refresh();
    void job_done();

    // QThread interface
protected:
    void run() override;
};

class raster_label : public QLabel
{
    Q_OBJECT;

    QImage* _raster;
    double _coef;
    int _max_steps = 1000;
    mandelbrot* _points;
    double* _clrindx = nullptr;
    complex _ccp = 0;
    double* RGB = nullptr;
    calc_threads_manager_thread* _process = nullptr;
    int* _threads_manager_active_ptr = nullptr;
    bool julia_mode = false;
    complex julia_shift = 0;
    void calc_colormodel();
public:
    raster_label(QWidget *parent = nullptr, const Qt::WindowFlags &f = Qt::WindowFlags());
    void recreate_raster(int new_x, int new_y);
    void recalculate();
    void interrupt_calculation();
    void set_zoom(double coef);
    void set_steps(double steps);
    void set_coords(double xcp, double ycp);
    bool is_calculations_in_progress();
    void set_julia_mode(bool);
    void set_julia_shift(double x, double y);
    const QImage* get_raster();
    std::unique_ptr<double[]> get_coord(int x, int y);
    ~raster_label();

public Q_SLOTS:
    void image_refresh();//слоты для обновления картинки
    void process_done();

Q_SIGNALS:
    void mouse_move(QString msg);//сигнал движения мышкой, пишет в статусбаре координаты
    void mouse_pressed(double xcp, double ycp);//сигнал нажатия, записывает координаты клика в поля ввода координат
    void calculation_started();//сигнал начала вычислений, для смены названия кнопки "обновить" на "прервать"
    void calculation_finished();

    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // RASTER_LABEL_H
