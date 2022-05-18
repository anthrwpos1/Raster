#include "raster_label.h"

void raster_label::mouseMoveEvent(QMouseEvent *event)
{
    auto coords = get_coord(event->x(), event->y());
    //int index = y*raster->width() + x;
    //if (index < raster->width()*raster->height())
    //{
    //double point = clrindx[index];
    emit mouse_move("coord x = " + QString::number(coords[0]) + ", coord y = " + QString::number(coords[1]));
    //}
    QLabel::mouseMoveEvent(event);
}


void raster_label::mousePressEvent(QMouseEvent *event)
{
    auto coords = get_coord(event->x(), event->y());
    _ccp = complex{coords[0], coords[1]};
    emit mouse_pressed(coords[0], coords[1]);
}

void raster_label::resizeEvent(QResizeEvent *event)
{
    QSize new_size = event->size();
    QImage resized = raster->scaled(new_size.width(), new_size.height(), Qt::KeepAspectRatioByExpanding);
    setPixmap(QPixmap::fromImage(resized));
    QLabel::resizeEvent(event);
}

raster_label::raster_label(QWidget *parent, const Qt::WindowFlags &f) :
    QLabel(parent, f),
    raster(new QImage(800,600,QImage::Format_RGB32)),
    _coef(1.0),
    points(new mandelbrot[800*600])
{}

void raster_label::recreate_raster(int new_x, int new_y)
{
    delete raster;
    delete[] points;
    points = new mandelbrot[new_x * new_y];
    raster = new QImage(new_x, new_y, QImage::Format_RGB32);
    recalculate();
}

color3 interp1(double* RGB, int rgb_size, double clrindex)
{
    int t = floor(clrindex);
    int even = t % 2;
    double saw = ((clrindex - t) * (1-2*even)+even) * rgb_size;
    int lowerindex = floor(saw);
    double dindex = saw - lowerindex;
    if (lowerindex >= rgb_size) return {RGB[rgb_size - 1], RGB[rgb_size * 2 - 1], RGB[rgb_size * 3 - 1]};
    double r = RGB[lowerindex] * (1 - dindex) + RGB[lowerindex + 1] * dindex;
    lowerindex += rgb_size;
    double g = RGB[lowerindex] * (1 - dindex) + RGB[lowerindex + 1] * dindex;
    lowerindex += rgb_size;
    double b = RGB[lowerindex] * (1 - dindex) + RGB[lowerindex + 1] * dindex;
    return {r, g, b};
}

void raster_label::recalculate()
{
    qint32 iw = raster->width();
    qint32 ih = raster->height();
    double xcp = _ccp.real();//координаты центральной точки
    double ycp = _ccp.imag();
    double msst = exp(-log(2)*_coef);//массштабный коэффициент

    //вектор раскраски
    double clri;
    double* LAB = new double[3000];
    for (int i = 0; i < 1000; ++i)
    {
        clri = (i+1)/1000.0;
        LAB[i] = clri * clri * 100.0;
        double C;
        if (i < 990) C = (i + 1) / 10.0;
        else C = (999 - i) * 10.0;
        LAB[i+1000] = C * sin(2*M_PI*-3*clri + 0.2857);
        LAB[i+2000] = C * cos(2*M_PI*-3*clri + 0.2857);
    }
    double* RGB = new double[3000];
    imlab2rgb(LAB, 1000, RGB);
    delete[] LAB;

    int index = 0;
    double pixel_size = msst / sqrt(ih*ih+iw*iw);
    double x_corner = xcp - iw * pixel_size / 2.0;
    double y_corner = ycp + ih * pixel_size / 2.0;
    for (int iy = 0; iy < ih; ++iy)
    {
        double cy = y_corner - iy * pixel_size;
        for (int ix = 0; ix < iw; ++ix)
        {
            double cx = x_corner + ix * pixel_size;
            points[index++] ={0, {cx, cy}, 0, 0};
        }
    }
    for (int i = 0; i < ih * iw; ++i) points[i].process(_max_steps);
    int* ds = new int[_max_steps]{0};
    int* ids = new int[_max_steps + 1];
    for (int i = 0; i < ih * iw; ++i) ds[(int) std::floor(points[i].steps)] += 1;//(points[i].steps > 0);
    ids[0] = 0;
    for (int i = 1; i < _max_steps + 1; ++i) ids[i] = ids[i-1] + ds[i-1];
    double ids_end = ids[_max_steps];
    if(ids_end == 0) ids_end = 1;
    delete[] clrindx;
    clrindx = new double[iw*ih];
    for (int i = 0; i < ih * iw; ++i)
    {
        int fk = std::floor(points[i].steps);
        double dk = points[i].steps - fk;
        clrindx[i]=(ids[fk]+dk*ds[fk])/ids_end;
        //clrindx[i] = points[i].steps / 10.0;
    }

    int i = 0;
    for (qint32 line = 0; line < ih; ++line)
    {
        QRgb* lineptr = reinterpret_cast<QRgb*>(raster->scanLine(line));
        for (qint32 pix = 0; pix < iw; ++pix)
        {
            color3 pcolor = interp1(RGB, 1000, clrindx[i]);
            *lineptr++ = qRgb(pcolor._c1, pcolor._c2, pcolor._c3);
            ++i;
        }
    }
    QImage resized = raster->scaled(width(), height(), Qt::KeepAspectRatioByExpanding);
    setPixmap(QPixmap::fromImage(resized));
    delete[] ids;
    delete[] ds;
    delete[] RGB;
}

void raster_label::set_coef(double coef)
{
    _coef = coef;
}

void raster_label::set_steps(double steps)
{
    _max_steps = steps;
}

void raster_label::set_coords(double xcp, double ycp)
{
    _ccp = {xcp, ycp};
}

const QImage *raster_label::get_raster()
{
    return raster;
}

//находит внутренние координаты модели по координатам в окне
std::unique_ptr<double[]> raster_label::get_coord(int x, int y)
{
    double iw = raster->width();
    double ih = raster->height();
    double dissize_x = width() / iw;
    double dissize_y = height() / ih;
    double zoom = std::max(dissize_x, dissize_y);
    double msst = exp(-log(2)*_coef);
    double pixel_size = msst / sqrt(ih*ih+iw*iw);
    double x_corner = _ccp.real() - iw * pixel_size / 2.0;
    double y_center = _ccp.imag();
    pixel_size /= zoom;
    std::unique_ptr<double[]> out(new double[2]);
    out[0] = x_corner + x * pixel_size;
    out[1] = y_center - (y - height() / 2.0) * pixel_size;
    return out;
}

raster_label::~raster_label()
{
    delete raster;
    delete[] points;
}
