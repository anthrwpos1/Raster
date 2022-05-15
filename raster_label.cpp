#include "raster_label.h"
#include <iostream>

struct color3
{
    double _c1, _c2, _c3;
};

color3 rgb2xyz(const color3& in)
{
    return {in._c1 * 0.4123955889674142 + in._c2 * 0.3575834307637148 + in._c3 * 0.1804926473817016,
                in._c1 * 0.2125862307855955 + in._c2 * 0.7151703037034108 + in._c3 * 0.07220049864333621,
                in._c1 * 0.01929721549174694 + in._c2 * 0.1191838645808485 + in._c3 * 0.9504971251315798};
}

double af(double t)
{
    return t * t * t * (t > 6/29) + (3 * 36/841 * (t-4/29)) * (t <= 6/29);
}

void imlab2xyz(double* array, int shift, double* output_array)
{
    int shift2 = shift * 2;
    for (int i = 0; i < shift; ++i)
    {
        double L = (array[i] + 16)/116;
        output_array[i] = af(L+array[i+shift]/500.0);
        output_array[i + shift] = af(L);
        output_array[i + shift2] = af(L-array[i+shift2]/200.0);
    }
}

void imxyz2rgb(double* array, int shift, double* output_array)
{
    int shift2 = shift * 2;
    for (int i = 0; i < shift; ++i)
    {
        double x = array[i];
        double y = array[i+shift];
        double z = array[i+shift2];
        output_array[i] = x * 3.2406 + y * -1.5372 + z * -0.4986;
        output_array[i + shift] = x * -0.9689 + y * 1.8758 + z * 0.0415;
        output_array[i + shift2] = x * 0.0557 + y * -0.204 + z * 1.057;
    }
    for (int i = 0; i < shift * 3; ++i)
    {
        double linear = output_array[i];
        double nonlinear = ((linear*12.92)*(linear<=0.0031308)+((1+0.055)*exp(log(linear)*(1/2.4))-0.055)*(linear>0.0031308));
        nonlinear = nonlinear * (nonlinear > 0) * (nonlinear < 1) + (nonlinear > 1);
        output_array[i] = nonlinear*255.0;
    }
}

void imlab2rgb(double* array, int shift, double* output_array)
{
    color3 white65 = rgb2xyz({1, 1, 1});
    double* temp = new double[shift * 3];
    imlab2xyz(array,shift, temp);
    int shift2 = shift * 2;
    for (int i = 0; i < shift; ++i) temp[i] *= white65._c1;
    for (int i = 0; i < shift; ++i) temp[i + shift] *= white65._c2;
    for (int i = 0; i < shift; ++i) temp[i + shift2] *= white65._c3;
    imxyz2rgb(temp, shift, output_array);
    delete[] temp;
}

template<class T>
void list_filter (std::list<T>& list,const std::vector<bool>& elements_to_leave)
{
    auto list_iterator = list.begin();
    int i = 0;
    while ((list_iterator != list.end()) && i < elements_to_leave.size()) {
        if (elements_to_leave[i])
        {
            list_iterator = list.erase(list_iterator);
        }
        else
            ++list_iterator;
        ++i;
    }
}

void raster_label::mouseMoveEvent(QMouseEvent *event)
{
    int index = event->y()*raster->width() + event->x();
    if (index < raster->width()*raster->height())
    {
        double point = clrindx[index];
        emit mouse_move(QString::number(point));
    }
    QLabel::mouseMoveEvent(event);
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
    points(new mandelbrot_point[800*600])
{}

void raster_label::recreate_raster(int new_x, int new_y)
{
    delete raster;
    delete[] points;
    points = new mandelbrot_point[new_x * new_y];
    raster = new QImage(new_x, new_y, QImage::Format_RGB32);
    recalculate();
}

void calcuclate_points(mandelbrot_point* points, int from, int to, int stepmax)
{
    for (int i = from; i < to; ++i) points[i].process(stepmax);
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
    complex ccp = 0.0;
    double xcp = ccp.real();//координаты центральной точки
    double ycp = ccp.imag();
    double msst = 2;//массштабный коэффициент
    int mmax = 1000;//предельное число итераций (1000..1000000)

    //вектор раскраски
    double* clri = new double[1000];
    double* LAB = new double[3000];
    for (int i = 0; i < 1000; ++i)
    {
        clri[i] = (i+1)/1000.0;
        LAB[i] = clri[i] * clri[i] * 100.0;
        double C;
        if (i < 990) C = (i + 1) / 10.0;
        else C = (999 - i) * 10.0;
        LAB[i+1000] = C * cos(2*M_PI*-3 + 0.2857);
        LAB[i+2000] = C * sin(2*M_PI*-3 + 0.2857);
    }
    double* RGB = new double[3000];
    imlab2rgb(LAB, 1000, RGB);
    delete[] LAB;

    int index = 0;
    double diagonal = sqrt(ih*ih+iw*iw);
    for (int iy = 0; iy < ih; ++iy)
    {
        double cy = ycp + (0.5 - iy / diagonal)*msst;
        for (int ix = 0; ix < iw; ++ix)
        {
            double cx = xcp + (ix / diagonal - 0.5)*msst;
            points[index++] ={0, {cx, cy}, 0, 0};
        }
    }
    for (int i = 0; i < ih * iw; ++i) points[i].process(mmax);
    int* ds = new int[mmax]{0};
    int* ids = new int[mmax];
    for (int i = 0; i < ih * iw; ++i) ++ds[(int) std::floor(points[i].steps)];
    ids[0] = 0;
    ds[0] = 0;
    for (int i = 1; i < mmax; ++i) ids[i] = ids[i-1] + ds[i];
    double ids_end = ids[mmax - 1];
    delete[] clrindx;
    clrindx = new double[iw*ih];
    for (int i = 0; i < ih * iw; ++i)
    {
        int fk = std::floor(points[i].steps);
        double dk = points[i].steps - fk;
        //clrindx[i]=(ids[fk]+dk*ds[fk])/ids_end;
        clrindx[i] = points[i].steps / 10.0;
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
    delete[] clri;
}

void raster_label::set_coef(double coef)
{
    _coef = coef;
}

raster_label::~raster_label()
{
    delete raster;
    delete[] points;
}

void mandelbrot_point::process(int mmax)
{
    double abs2;
    while(step < mmax)
    {
        z = z * z + c;
        ++step;
        abs2 = z.real() * z.real() + z.imag() * z.imag();
        if (abs2 > 1e10) break;
    }
    if (step == mmax) steps = 0;
    else
    {
        steps = step-log(abs2/1e20)/log(1e10);
    }
}
