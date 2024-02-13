#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <climits>
#include <vector>
#include <bitset>
#include <complex>
#include <chrono>

#include <QTime>
#include <QDebug>

#include <benchmark/benchmark.h>

#include <vector>
#include <cassert>
#include <cmath>

std::vector<double> linspace(double a, double b, int n) {
    /*
     * Function which returns a std::vector containing n linearly-spaced
     * points in the interval [a, b], inclusive.
     * Args:
     *      - a: Lower bound of an interval in double precision
     *      - b: Upper bound of an interval in double precision
     *      - n: Positive integer number of linearly-spaced values to return.
     * Returns:
     *      - A vector of n double precision values within [a, b]
     */
    assert(a < b && n > 0);
    std::vector<double> points;
    int iter = 0;
    double d = b - a;
    for (int i = 0; i <= n - 2; i++) {
        points.insert(points.begin() + iter, a + (i * d) / (floor(n) - 1));
        iter++;
    }
    // We also need to insert the upper bound at the end, or the result is incomplete.
    points.insert(points.begin() + iter, b);
    return points;
}

template <typename Function>
std::vector<double> rel_error(Function f, Function F, double a, double b, int n) {
    /*
     * Benchmark function for assessing relative error of two functions over n values in
     * an interval [a, b].
     * Args:
     *     - f: Benchmark function to be compared against.
     *     - F: Approximation of the true function.
     *     - a: Left boundary of the interval. Must be less than b.
     *     - b: Right boundary of the interval. Must be greater than a.
     *     - n: The number of values in the interval to consider.
     * Returns:
     *     - A vector matching the input precision consisting of the relative errors
     *     f and F at every x evaluated from the interval [a, b]
     */
    std::vector<double> x = linspace(a, b, n);
    std::vector<double> control;
    std::vector<double> test;
    for (auto i : x) {
        control.push_back(f(i));
        test.push_back(F(i));
    }
    std::vector<double> errors;
    errors.reserve(control.size());
    for (std::size_t i = 0; i < control.size(); i++) {
        errors.push_back(abs(control[i] - test[i]));// / abs(control[i]));
    }
    return errors;
}

double mean(std::vector<double> nums) {
    /*
     * Basic function which returns the mean of double precision
     * values contained in the input vector. Returns the mean as
     * a double precision number.
     */
    return std::accumulate(nums.begin(), nums.end(), 0) / nums.size();
}

double var(std::vector<double> nums) {
    /*
     * Basic function which returns the variance of double
     * precision values contained in the input vector. Returns
     * the variance as a double precision number.
     */
    std::vector<double> square_vec;
    square_vec.reserve(nums.size());
    for (auto i : nums) {
        square_vec.push_back(i * i);
    }
    return mean(square_vec) - pow(mean(nums), 2);
}

double median(std::vector<double> nums) {
    /*
     * Basic function which returns the median of double
     * precision values contained in the input vector. Returns
     * the median as a double precision number.
     */
    std::sort(nums.begin(), nums.end());
    if (nums.size() % 2 == 0) {
        return nums[(int)(nums.size() / 2) - 1] + nums[(int)(nums.size() / 2)];
    }
    else {
        return nums[(int)(nums.size() / 2)];
    }
}

double expa( double x ) {

#ifdef __LITTLE_ENDIAN
#define __HI(x) *(1+(int*)&x)
#define __LO(x) *(int*)&x
#define __HIp(x) *(1+(int*)x)
#define __LOp(x) *(int*)x
#else
#define __HI(x) *(int*)&x
#define __LO(x) *(1+(int*)&x)
#define __HIp(x) *(int*)x
#define __LOp(x) *(1+(int*)x)
#endif

    static const double
        one = 1.0,
        halF[2] = {0.5,-0.5,},
        huge    = 1.0e+300,
        twom1000= 9.33263618503218878990e-302,     /* 2**-1000=0x01700000,0*/
        o_threshold=  7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
        u_threshold= -7.45133219101941108420e+02,  /* 0xc0874910, 0xD52D3051 */
        ln2HI[2]   ={ 6.93147180369123816490e-01,  /* 0x3fe62e42, 0xfee00000 */
                 -6.93147180369123816490e-01,},/* 0xbfe62e42, 0xfee00000 */
        ln2LO[2]   ={ 1.90821492927058770002e-10,  /* 0x3dea39ef, 0x35793c76 */
                 -1.90821492927058770002e-10,},/* 0xbdea39ef, 0x35793c76 */
        invln2 =  1.44269504088896338700e+00, /* 0x3ff71547, 0x652b82fe */
        P1   =  1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
        P2   = -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
        P3   =  6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
        P4   = -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
        P5   =  4.13813679705723846039e-08; /* 0x3E663769, 0x72BEA4D0 */

    double y,hi,lo,c,t;
    int k,xsb;
    unsigned hx;

    hx  = __HI(x);  /* high word of x */
    xsb = (hx>>31)&1;       /* sign bit of x */
    hx &= 0x7fffffff;       /* high word of |x| */

    /* argument reduction */
    if(hx > 0x3fd62e42) {       /* if  |x| > 0.5 ln2 */
        if(hx < 0x3FF0A2B2) {   /* and |x| < 1.5 ln2 */
        hi = x-ln2HI[xsb]; lo=ln2LO[xsb]; k = 1-xsb-xsb;
        } else {
        k  = (int)(invln2*x+halF[xsb]);
        t  = k;
        hi = x - t*ln2HI[0];    /* t*ln2HI is exact here */
        lo = t*ln2LO[0];
        }
        x  = hi - lo;
    }
    else if(hx < 0x3e300000)  { /* when |x|<2**-28 */
        if(huge+x>one) return one+x;/* trigger inexact */
    }
    else k = 0;

    /* x is now in primary range */
    t  = x*x;
    c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
    if(k==0)    return one-((x*c)/(c-2.0)-x);
    else        y = one-((x*c)/(c-2.0)+lo-hi);
    if(k >= -1021) {
        __HI(y) += (k<<20); /* add k to y's exponent */
        return y;
    } else {
        __HI(y) += ((k+1000)<<20);/* add k to y's exponent */
        return y*twom1000;
    }
}

double pow2( int x ) {
    union {
        double retval;
        struct {
            uint64_t mantissa :52;
            uint64_t exponent :11;
            uint64_t sign :1;
        } parts;
    } u;
    u.retval = 1.0;
    u.parts.exponent += x;
    return u.retval;
}

double expc( double x ) {

    double x0 = abs( x );
    int k = ceil( ( x0 / M_LN2 ) - 0.5 );
    double r = x0 - ( k * M_LN2 );
    double tk = 1.0;
    double tn = 1.0;
    for ( int i = 1; i < 14; i++ ) {
        tk *= r / i;
        tn += tk;
    }
    tn = tn * pow2( k );
    if ( x < 0 )
        return 1 / tn;
    return tn;
}

double expp( double x ) {

    return expf( float( x ) );
}

/**
 *
 */
class Benchmark : public benchmark::Fixture {
public:
    Benchmark() {
    }

protected:
};

BENCHMARK_DEFINE_F( Benchmark, EXP0 ) ( benchmark::State& state ) {
    volatile double e = 1;
    for ( auto _ : state ) {
        for ( double i = 0; i < 1; i += 0.00001 )
            e += std::exp( i * state.range() );
    }
}

BENCHMARK_DEFINE_F( Benchmark, EXPP ) ( benchmark::State& state ) {
    volatile double e = 1;
    for ( auto _ : state ) {
        for ( double i = 0; i < 1; i += 0.00001 )
            e += expp( i * state.range() );
    }
}

BENCHMARK_DEFINE_F( Benchmark, EXPA ) ( benchmark::State& state ) {
    volatile double e = 1;
    for ( auto _ : state ) {
        for ( double i = 0; i < 1; i += 0.00001 )
            e += expa( i * state.range() );
    }
}

#define TEST_VALUE -100
BENCHMARK_REGISTER_F( Benchmark, EXP0 )->Arg( TEST_VALUE );
BENCHMARK_REGISTER_F( Benchmark, EXPP )->Arg( TEST_VALUE );
BENCHMARK_REGISTER_F( Benchmark, EXPA )->Arg( TEST_VALUE );

int main( int argc, char** argv ) {
    std::cout << std::fixed << std::setprecision( 20 );

    if ( 0 ) {
        double a = -100.0;
        double b = 0.0;
        int n = 10000;
        std::vector<double> errors = rel_error( std::exp, expp, a, b, n);
        std::cout << "Max relative error: " << *std::max_element(errors.begin(), errors.end()) << " " << std::distance( errors.begin(), std::max_element(errors.begin(), errors.end()) ) << std::endl;
        std::cout << "Min relative error: " << *std::min_element(errors.begin(), errors.end()) << std::endl;
        std::cout << "Avg relative error: " << mean(errors) << std::endl;
        std::cout << "Med relative error: " << median(errors) << std::endl;
        std::cout << "Var relative error: " << var(errors) << std::endl;
        double s = 0;
        for (auto i : errors) {
            if (i > 5e-9) {
                s += 1;
            }
        }
        std::cout << s / errors.size() * 100 << " percent of the values have less than 8 digits of precision. " << 5e-9 << std::endl;
        return 0;
    }

    if ( 0 ) {
        std::cout << "exp:  " << std::exp( TEST_VALUE ) << "\n";
        std::cout << "expa: " << expa( TEST_VALUE ) << "\n";
        std::cout << "expc: " << expc( TEST_VALUE ) << "\n";
        std::cout << "expp: " << expp( TEST_VALUE ) << "\n";
        return 0;
    }

    benchmark::Initialize( &argc, argv );
    if ( benchmark::ReportUnrecognizedArguments( argc, argv ) )
        return 1;

    benchmark::RunSpecifiedBenchmarks();
    return 0;
}
