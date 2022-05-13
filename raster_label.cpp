#include "raster_label.h"

void raster_label::mouseMoveEvent(QMouseEvent *event)
{
    emit mouse_move(event->x(), event->y());
    QLabel::mouseMoveEvent(event);
}

void raster_label::resizeEvent(QResizeEvent *event)
{
    QSize new_size = event->size();
    QImage resized = raster->scaled(new_size.width(), new_size.height(), Qt::IgnoreAspectRatio);
    setPixmap(QPixmap::fromImage(resized));
    QLabel::resizeEvent(event);
}

raster_label::raster_label(QWidget *parent, const Qt::WindowFlags &f) : QLabel(parent, f), raster(new QImage(800,600,QImage::Format_RGB32)), _coef(1.0)
{}

void raster_label::recreate_raster(int new_x, int new_y)
{
    delete raster;
    raster = new QImage(new_x, new_y, QImage::Format_RGB32);
    recalculate();
}

void raster_label::recalculate()
{
    qint32 iw = raster->width();
    qint32 ih = raster->height();
    double coef = sqrt(iw*iw+ih*ih)*_coef;
    for (qint32 line = 0; line < ih; ++line)
    {
        double y = 0.5 - (double) line / (double) ih;
        QRgb* lineptr = reinterpret_cast<QRgb*>(raster->scanLine(line));
        for (qint32 pix = 0; pix < iw; ++pix)
        {
            double x = (double) pix / (double) iw - 0.5;
            int z = (int)((sin((y * y + x * x) * 2.0 * coef) + 1) / 2.0 * 255.0);
            *lineptr++ = qRgb(z, z, z);
        }
    }
    QImage resized = raster->scaled(width(), height(), Qt::IgnoreAspectRatio);
    setPixmap(QPixmap::fromImage(resized));
}

void raster_label::set_coef(double coef)
{
    _coef = coef;
}

raster_label::~raster_label()
{
    delete raster;
}
