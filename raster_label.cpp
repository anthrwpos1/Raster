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
    QImage resized = _raster->scaled(new_size.width(), new_size.height(), Qt::KeepAspectRatioByExpanding);
    setPixmap(QPixmap::fromImage(resized));
    QLabel::resizeEvent(event);
}

void raster_label::calc_colormodel()
{
    double clri;
    double* LAB = new double[3000];
    delete[] RGB;
    RGB = new double[3000];
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
    imlab2rgb(LAB, 1000, RGB);
    delete[] LAB;
}

raster_label::raster_label(QWidget *parent, const Qt::WindowFlags &f) :
    QLabel(parent, f),
    _raster(new QImage(800,600,QImage::Format_RGB32)),
    _coef(-3.0),
    _points(new mandelbrot[800*600])
{}

void raster_label::recreate_raster(int new_x, int new_y)
{
    delete _raster;
    delete[] _points;
    _points = new mandelbrot[new_x * new_y];
    _raster = new QImage(new_x, new_y, QImage::Format_RGB32);
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
    if (_process) return;
    if (!RGB) calc_colormodel();
    qint32 iw = _raster->width();
    qint32 ih = _raster->height();
    double xcp = _ccp.real();//координаты центральной точки
    double ycp = _ccp.imag();
    double msst = exp(-log(2)*_coef);//массштабный коэффициент

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
            _points[index++] ={0, {cx, cy}, 0, 0};
        }
    }
    _process = new calc_threads_manager_thread(_points, iw * ih, _max_steps, 8);
    connect(_process, &calc_threads_manager_thread::image_refresh, this, &raster_label::image_refresh);
    connect(_process, &calc_threads_manager_thread::job_done, this, &raster_label::process_done);
    _threads_manager_active_ptr = _process->get_active_ptr();
    _process->start();
    emit calculation_started();
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

bool raster_label::is_calculations_in_progress()
{
    return _process != nullptr;
}

const QImage *raster_label::get_raster()
{
    return _raster;
}

//находит внутренние координаты модели по координатам в окне
std::unique_ptr<double[]> raster_label::get_coord(int x, int y)
{
    double iw = _raster->width();
    double ih = _raster->height();
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
    delete _raster;
    delete[] _points;
    delete[] RGB;
}

void raster_label::image_refresh()
{
    //размеры растра
    qint32 iw = _raster->width();
    qint32 ih = _raster->height();
    //число пикселей вычисленных за данное число шагов
    int* ds = new int[_max_steps]{0};
    //кумулятивная сумма ds.
    int* ids = new int[_max_steps + 1];
    for (int i = 0; i < ih * iw; ++i)
    {
        ds[(int) std::floor(_points[i].steps)] += 1;//(points[i].steps > 0);
    }
    ids[0] = 0;
    for (int i = 1; i < _max_steps + 1; ++i) ids[i] = ids[i-1] + ds[i-1];
    double ids_end = ids[_max_steps];
    if(ids_end == 0) ids_end = 1;
    delete[] _clrindx;
    //"гистограммная раскраска": цвет распределяется равномерно по всем пикселям
    _clrindx = new double[iw*ih];
    for (int i = 0; i < ih * iw; ++i)
    {
        int fk = std::floor(_points[i].steps);
        double dk = _points[i].steps - fk;
        _clrindx[i]=(ids[fk]+dk*ds[fk])/ids_end;
    }

    int i = 0;
    for (qint32 line = 0; line < ih; ++line)
    {
        QRgb* lineptr = reinterpret_cast<QRgb*>(_raster->scanLine(line));
        for (qint32 pix = 0; pix < iw; ++pix)
        {
            color3 pcolor = interp1(RGB, 1000, _clrindx[i]);
            *lineptr++ = qRgb(pcolor._c1, pcolor._c2, pcolor._c3);
            ++i;
        }
    }
    QImage resized = _raster->scaled(width(), height(), Qt::KeepAspectRatioByExpanding);
    setPixmap(QPixmap::fromImage(resized));
    delete[] ids;
    delete[] ds;
}

void raster_label::interrupt_calculation()
{
    if (_threads_manager_active_ptr)
    {
        *_threads_manager_active_ptr = 0;
        _process->wait();
        delete _process;
        _process = nullptr;
        emit calculation_finished();
    }
}

void raster_label::process_done()
{
    delete _process;
    _process = nullptr;
    emit calculation_finished();
}


void calc_thread::run()
{
    if (!_active) return;
    if (!_index_map) return;
    if (!_index_mutex) return;
    while(*_active)
    {
        _index_mutex->lock();
        int first_index = *_index_map;
        if (*_index_map < _max_index) *_index_map += 1000;
        _index_mutex->unlock();
        if (first_index >= _max_index) break;
        int last_index = (first_index + 1000);
        if (last_index > _max_index) last_index = _max_index;
        for (int i = first_index; i < last_index; ++i)
        {
            _points[i].process(_mmax);
        }
    }
}

calc_thread::calc_thread(mandelbrot *points, QMutex *index_mutex,
                         int *index_map, int max_index, int *active, int mmax, int num) :
    _points(points),
    _index_mutex(index_mutex),
    _index_map(index_map),
    _max_index(max_index),
    _active(active),
    _mmax(mmax),
    _num(num)
{}

calc_threads_manager_thread::calc_threads_manager_thread(mandelbrot *points, int max_index, int mmax, int threadnum) :
    _points(points), _max_index(max_index), _mmax(mmax), _threadnum(threadnum)
{

}

int *calc_threads_manager_thread::get_active_ptr()
{
    return &_active;
}

void calc_threads_manager_thread::run()
{
    _active = 1;
    QMutex index_mutex;
    int index_map;
    std::vector<std::unique_ptr<calc_thread>> threads;
    for (int i = 0; i < _threadnum; ++i) threads.push_back(std::unique_ptr<calc_thread>(new calc_thread(_points, &index_mutex, &index_map, _max_index, &_active, _mmax, i)));
    for (auto& t : threads) t->start();
    while(_active)
    {
        index_mutex.lock();
        if (index_map >= _max_index)
        {
            index_mutex.unlock();
            break;
        }
        index_mutex.unlock();
        if (!_active) break;
        msleep(500);
        emit image_refresh();
    }
    for (auto& t : threads) t->wait();
    emit job_done();
}
