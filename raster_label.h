#ifndef RASTER_LABEL_H
#define RASTER_LABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <cmath>
#include <complex>

using complex = std::complex<double>;

struct mandelbrot_point
{
    complex z;
    complex c;
    int step;
    double steps;
    void process(int mmax);
};

class raster_label : public QLabel
{
    Q_OBJECT;

    QImage* raster;
    double _coef;
    mandelbrot_point* points;
    double* clrindx = nullptr;
public:
    raster_label(QWidget *parent = nullptr, const Qt::WindowFlags &f = Qt::WindowFlags());
    void recreate_raster(int new_x, int new_y);
    void recalculate();
    void set_coef(double coef);
    ~raster_label();
Q_SIGNALS:
    void mouse_move(QString msg);

    // QWidget interface
protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};

#endif // RASTER_LABEL_H
