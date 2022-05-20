#include "raster_label.h"

//пишет в строке состояния текущие координаты
void raster_label::mouseMoveEvent(QMouseEvent *event)
{
    auto coords = get_coord(event->x(), event->y());
    emit mouse_move("coord x = " + QString::number(coords[0]) + ", coord y = " + QString::number(coords[1]));
    QLabel::mouseMoveEvent(event);
}

//выставляет координаты клика в поля ввода коордтинат
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

//вычисляет раскраску
void raster_label::calc_colormodel()
{
    double clri;
    std::unique_ptr<double[]> LAB(new double[3000]);
    RGB.reset(new double[3000]);
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
    imlab2rgb(LAB.get(), 1000, RGB.get());
}

raster_label::raster_label(QWidget *parent, const Qt::WindowFlags &f, int width, int height, double coef) :
    QLabel(parent, f),
    _raster(new QImage(width,height,QImage::Format_RGB32)),
    _coef(coef),
    _points(new mandelbrot[width*height])
{}

//изменяет размеры растра
void raster_label::recreate_raster(int new_x, int new_y)
{
    _points.reset(new mandelbrot[new_x * new_y]);
    _raster.reset(new QImage(new_x, new_y, QImage::Format_RGB32));
    recalculate();
}

//выбирает путем линейной интерполяции цвет из массива цветов RGB так,
//что параметру clrindex = 0 соответствует нулевой цвет, параметру clrindex = 1 - последний
color3 interp1(double* RGB, int rgb_size, double clrindex)
{
    int t = floor(clrindex);
    int even = t % 2;
    double saw = ((clrindex - t) * (1-2*even)+even) * rgb_size;
    int lowerindex = floor(saw);
    double dindex = saw - lowerindex;
    if (lowerindex >= rgb_size - 1) return {RGB[rgb_size - 1], RGB[rgb_size * 2 - 1], RGB[rgb_size * 3 - 1]};
    double r = RGB[lowerindex] * (1 - dindex) + RGB[lowerindex + 1] * dindex;
    lowerindex += rgb_size;
    double g = RGB[lowerindex] * (1 - dindex) + RGB[lowerindex + 1] * dindex;
    lowerindex += rgb_size;
    double b = RGB[lowerindex] * (1 - dindex) + RGB[lowerindex + 1] * dindex;
    return {r, g, b};
}

//запускает процесс вычисления точек модели
void raster_label::recalculate()
{
    if (_process) return;
    if (!RGB) calc_colormodel();
    qint32 iw = _raster->width();
    qint32 ih = _raster->height();
    double xcp = _ccp.real();//координаты центральной точки
    double ycp = _ccp.imag();
    double msst = exp(-log(2)*_coef);//масштабный коэффициент 1 единица - увеличение в 2 раза

    int index = 0;
    double pixel_size = msst / sqrt(ih*ih+iw*iw);
    double x_corner = xcp - iw * pixel_size / 2.0;
    double y_corner = ycp + ih * pixel_size / 2.0;
    //набиваем точки в массив
    for (int iy = 0; iy < ih; ++iy)
    {
        double cy = y_corner - iy * pixel_size;
        for (int ix = 0; ix < iw; ++ix)
        {
            double cx = x_corner + ix * pixel_size;
            if (julia_mode) _points[index++] = {{cx, cy},   julia_shift,    0,  0};     //режим вычисления Жюлиашек
            else            _points[index++] = {0,          {cx, cy},       0,  0};     //режим вычисления Мандельброток
        }
    }
    _process.reset(new calc_threads_manager_thread(_points.get(), iw * ih, _max_steps, _threadnum));//создаем вычислительный поток
    connect(_process.get(), &calc_threads_manager_thread::image_refresh, this, &raster_label::image_refresh);//привязываем его события
    connect(_process.get(), &calc_threads_manager_thread::job_done, this, &raster_label::process_done);
    _threads_manager_active_ptr = _process->get_active_ptr();//получаем указатель на стоп-кран
    _process->start();
    emit calculation_started();
}

void raster_label::set_zoom(double coef)
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

void raster_label::set_julia_mode(bool do_set)
{
    julia_mode = do_set;
}

void raster_label::set_julia_shift(double x, double y)
{
    julia_shift = {x, y};
}

const QImage *raster_label::get_raster()
{
    return _raster.get();
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

//обновляем картинку
void raster_label::image_refresh()
{
    //размеры растра
    qint32 iw = _raster->width();
    qint32 ih = _raster->height();
    //число пикселей вычисленных за данное число шагов
    std::unique_ptr<int[]> ds (new int[_max_steps]{0});
    //кумулятивная сумма ds.
    std::unique_ptr<int[]> ids(new int[_max_steps + 1]);
    for (int i = 0; i < ih * iw; ++i)
    {
        //считаем, сколько пикселей ушли за данное число шагов
        ds[(int) std::floor(_points[i].steps)] += (_points[i].steps > 0);
    }
    ids[0] = 0;
    //сколько пикселей ушло за менее чем данное число шагов
    for (int i = 1; i < _max_steps + 1; ++i) ids[i] = ids[i-1] + ds[i-1];
    double ids_end = ids[_max_steps];
    if(ids_end == 0) ids_end = 1;
    //"гистограммная раскраска": цвет распределяется равномерно по всем пикселям
    _clrindx.reset(new double[iw*ih]);
    for (int i = 0; i < ih * iw; ++i)
    {
        //целая часть числа шагов
        int fk = std::floor(_points[i].steps);
        //дробная часть
        double dk = _points[i].steps - fk;
        //выбираем цветовой индекс согласно числу шагов с тем, чтобы обеспечить равномерное распределение
        //для этого берем целое число шагов и смотрим сколько пикселей есть до этого уровня
        //После чего берем дробное число шагов и добавляем к этому числу число пикселей на следующем шаге
        //умноженное на это дробное число. Результат делим на общее число учетных пикселей и получаем
        //равномерно распределенное множество пикселей в диапазоне от -1 до 1.
        _clrindx[i]=(ids[fk]+dk*ds[fk])/ids_end;
    }
    //Набиваем растр
    int i = 0;
    for (qint32 line = 0; line < ih; ++line)
    {
        QRgb* lineptr = reinterpret_cast<QRgb*>(_raster->scanLine(line));
        for (qint32 pix = 0; pix < iw; ++pix)
        {
            color3 pcolor = interp1(RGB.get(), 1000, _clrindx[i]);
            *lineptr++ = qRgb(pcolor._c1, pcolor._c2, pcolor._c3);
            ++i;
        }
    }
    QImage resized = _raster->scaled(width(), height(), Qt::KeepAspectRatioByExpanding);
    setPixmap(QPixmap::fromImage(resized));
}

//прерываем вычисления: дергаем "стоп-кран", ждем пока остановится, удаляем процесс.
void raster_label::interrupt_calculation()
{
    if (_threads_manager_active_ptr)
    {
        *_threads_manager_active_ptr = 0;
        _process->wait();
        _process.reset();
        emit calculation_finished();
    }
}

void raster_label::process_done()
{
    _process.reset();
    emit calculation_finished();
}

//Вычислительный процесс
void calc_thread::run()
{
    if (!_active) return;
    if (!_index_map) return;
    if (!_index_mutex) return;
    while(*_active)
    {
        _index_mutex->lock();
        int first_index = *_index_map;//смотрим первый непосчитанный индекс
        if (*_index_map < _max_index) *_index_map += _single_task_size;//увеличиваем, всё что в этом диапазоне - наше.
        _index_mutex->unlock();
        if (first_index >= _max_index) break;//если первый непосчитанный индекс за пределами массива - значит всё уже посчитано
        int last_index = (first_index + _single_task_size);
        if (last_index > _max_index) last_index = _max_index;//убеждаемся что все входят в пределы массива
        for (int i = first_index; i < last_index; ++i)
        {
            _points[i].process(_mmax);//считаем
        }
    }
}

calc_thread::calc_thread(mandelbrot *points, QMutex *index_mutex,
                         int *index_map, int max_index, int *active, int mmax, int num) :
    QThread(),
    _points(points),
    _index_mutex(index_mutex),
    _index_map(index_map),
    _max_index(max_index),
    _active(active),
    _mmax(mmax),
    _num(num)
{}

calc_threads_manager_thread::calc_threads_manager_thread(mandelbrot *points, int max_index, int mmax, int threadnum) :
    QThread(), _points(points), _max_index(max_index), _mmax(mmax), _threadnum(threadnum)
{

}

int *calc_threads_manager_thread::get_active_ptr()
{
    return &_active;
}

//поток с потоками
void calc_threads_manager_thread::run()
{
    _active = 1;//стоп-кран
    QMutex index_mutex;
    int index_map = 0;//номер первой непосчитанной точки
    std::vector<std::unique_ptr<calc_thread>> threads;//потоки. Сделаны таким хитрым способом, чтобы не делать "конструктор по умолчанию"
    //набиваем
    for (int i = 0; i < _threadnum; ++i) threads.push_back(std::unique_ptr<calc_thread>(new calc_thread(_points, &index_mutex, &index_map, _max_index, &_active, _mmax, i)));
    //запускаем
    for (auto& t : threads) t->start();
    while(_active)
    {
        index_mutex.lock();
        //смотрим есть всё посчитано - выходим из цикла
        if (index_map >= _max_index)
        {
            index_mutex.unlock();
            break;
        }
        index_mutex.unlock();
        //ждем немного
        msleep(500);
        //посылаем команду обновить картинку.
        //todo: сделать так, чтобы дождаться обновления картинки и не бомбить событиями пересчета если картинка считается дольше полу секунды.
        emit image_refresh();
    }
    //дожидаемся пока все закончат
    //todo: дождаться еще и пересчета картинки
    for (auto& t : threads) t->wait();
    emit job_done();
}
