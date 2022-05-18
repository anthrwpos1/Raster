#include "mandelbrot.h"

/*
 * todo
 * - [пофикшено] разобраться с цветом
 *  - найти почему остаются черные точечки
 * - [пофикшено] гистограммная раскраска
 * - многоступенчатая генерация
 * - многопоточная генерация
 * - генерация в реальном времени
 * - возможность сохранения
 * - отдельный класс раскраски, возможно с возможностью задавать
 * - псевдо-3д мандельбротка
 *
 * */
void mandelbrot::process(int mmax)
{
    double abs2 = 0;
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
        steps = step-log(abs2/1e20) / 23.025850929940457;
    }
}

//матрица преобразования цвета из линейного уровня яркости пикселей (0..1) в уровни возбуждения фоторецепторов человека
color3 rgb2xyz(const color3& in)
{
    return {    in._c1 * 0.4123955889674142     + in._c2 * 0.3575834307637148   + in._c3 * 0.1804926473817016,
                in._c1 * 0.2125862307855955     + in._c2 * 0.7151703037034108   + in._c3 * 0.07220049864333621,
                in._c1 * 0.01929721549174694    + in._c2 * 0.1191838645808485   + in._c3 * 0.9504971251315798};
}

//психоколориметрическая функция
double pscol_functuin(double t)
{
    return t * t * t * (t > 0.20689655172413793) + (0.12841854934601665 * (t-0.13793103448275862)) * (t <= 0.20689655172413793);
}

//преобразование цвета из психоколориметрической модели CIELab в относительные уровни возбуждения фоторецепторов (без учета выбора точки белого)
void imlab2xyz(double* array, int shift, double* output_array)
{
    int shift2 = shift * 2;
    for (int i = 0; i < shift; ++i)
    {
        double L = (array[i] + 16)/116;
        output_array[i] =           pscol_functuin(L+array[i+shift]/500.0);
        output_array[i + shift] =   pscol_functuin(L);
        output_array[i + shift2] =  pscol_functuin(L-array[i+shift2]/200.0);
    }
}

//преобразование цвета из абсолютных уровней возбуждения фоторецепторов (с учетом выбора точки белого) в нелинейный цветовой код пикселей (0..255)
void imxyz2rgb(double* array, int shift, double* output_array)
{
    int shift2 = shift * 2;
    for (int i = 0; i < shift; ++i)
    {
        double x = array[i];
        double y = array[i+shift];
        double z = array[i+shift2];
        //матрица преобразования цвета из уровней возбуждения фоторецепторов человека в линейный уровень яркости пикселей (0..1)
        output_array[i] =           x * 3.2406      + y * -1.5372   + z * -0.4986;
        output_array[i + shift] =   x * -0.9689     + y * 1.8758    + z * 0.0415;
        output_array[i + shift2] =  x * 0.0557      + y * -0.204    + z * 1.057;
    }
    //преобразуем линейные уровни яркости пикселей (0..1) в нелинейный цветовой код (0..255) согласно стандарту CIE sRGB
    for (int i = 0; i < shift * 3; ++i)
    {
        double linear = output_array[i];
        double nonlinear = ((linear*12.92)*(linear<=0.0031308)+((1+0.055)*exp(log(linear)*(1/2.4))-0.055)*(linear>0.0031308));
        nonlinear = nonlinear * (nonlinear > 0) * (nonlinear < 1) + (nonlinear > 1);
        output_array[i] = nonlinear*255.0;
    }
}

//преобразование цвета из психоколориметрической модели CIELab напрямую в нелинейный цветовой код пикселей (0..255)
void imlab2rgb(double* array, int shift, double* output_array)
{
    color3 white65 = rgb2xyz({1, 1, 1});//используемый белый
    double* temp = new double[shift * 3];
    imlab2xyz(array,shift, temp);//переводим в относителный XYZ
    int shift2 = shift * 2;
    //переводим относительный XYZ в абсолютный,
    //выбрав точку белого так, чтобы цвет с кодом [255,255,255] считался белым (имел координаты XYZ = [1,1,1]).
    for (int i = 0; i < shift; ++i) temp[i]             *= white65._c1;
    for (int i = 0; i < shift; ++i) temp[i + shift]     *= white65._c2;
    for (int i = 0; i < shift; ++i) temp[i + shift2]    *= white65._c3;
    //переводим из объективного XYZ в код пикселей.
    imxyz2rgb(temp, shift, output_array);
    delete[] temp;
}
