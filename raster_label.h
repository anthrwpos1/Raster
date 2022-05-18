#ifndef RASTER_LABEL_H
#define RASTER_LABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <memory>
#include "mandelbrot.h"

class raster_label : public QLabel
{
    Q_OBJECT;

    QImage* raster;
    double _coef;
    int _max_steps = 1000;
    mandelbrot* points;
    double* clrindx = nullptr;
    complex _ccp;
public:
    raster_label(QWidget *parent = nullptr, const Qt::WindowFlags &f = Qt::WindowFlags());
    void recreate_raster(int new_x, int new_y);
    void recalculate();
    void set_coef(double coef);
    void set_steps(double steps);
    void set_coords(double xcp, double ycp);
    std::unique_ptr<double[]> get_coord(int x, int y);
    ~raster_label();
Q_SIGNALS:
    void mouse_move(QString msg);
    void mouse_pressed(double xcp, double ycp);

    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // RASTER_LABEL_H
