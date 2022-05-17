#ifndef MANDELBROT_H
#define MANDELBROT_H

#include <complex>

using complex = std::complex<double>;

struct color3
{
    double _c1, _c2, _c3;
};

class mandelbrot
{
public:
    complex z;
    complex c;
    int step;
    double steps;
    void process(int mmax);
};

void imlab2rgb(double* array, int shift, double* output_array);

#endif // MANDELBROT_H
