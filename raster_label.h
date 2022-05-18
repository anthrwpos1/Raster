#ifndef RASTER_LABEL_H
#define RASTER_LABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QThread>
#include <QMutex>
#include <memory>
#include "mandelbrot.h"

class calc_thread : public QThread
{
    Q_OBJECT;

    mandelbrot* _points;
    QMutex* _index_mutex;
    int* _index_map;
    int _max_index;
    int* _active;
    int _mmax;
    int _num;
public:
    calc_thread(mandelbrot *points, QMutex *index_mutex, int *index_map, int max_index, int *active, int mmax, int num);
    // QThread interface
protected:
    void run() override;
};

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

    void calc_colormodel();
public:
    raster_label(QWidget *parent = nullptr, const Qt::WindowFlags &f = Qt::WindowFlags());
    void recreate_raster(int new_x, int new_y);
    void recalculate();
    void interrupt_calculation();
    void set_coef(double coef);
    void set_steps(double steps);
    void set_coords(double xcp, double ycp);
    bool is_calculations_in_progress();
    const QImage* get_raster();
    std::unique_ptr<double[]> get_coord(int x, int y);
    ~raster_label();

public Q_SLOTS:
    void image_refresh();
    void process_done();

Q_SIGNALS:
    void mouse_move(QString msg);
    void mouse_pressed(double xcp, double ycp);
    void calculation_started();
    void calculation_finished();

    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // RASTER_LABEL_H
