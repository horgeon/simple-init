//
// Created by bigfoot on 2022/2/8.
//
#ifndef _MATH_H
#define _MATH_H

#ifdef __cplusplus
extern "C" {
#endif

#define __NEED_float_t
#define __NEED_double_t
#include <bits/alltypes.h>
#include <stdint.h>

/* Support non-nearest rounding mode.  */
#define WANT_ROUNDING 1
/* Support signaling NaNs.  */
#define WANT_SNAN 0

#define issignalingf_inline(x) 0
#define issignaling_inline(x) 0

#define EXP_TABLE_BITS 7
#define EXP_POLY_ORDER 5
#define EXP_USE_TOINT_NARROW 0
#define EXP2_POLY_ORDER 5
extern const struct exp_data {
	double invln2N;
	double shift;
	double negln2hiN;
	double negln2loN;
	double poly[4];/* Last four coefficients.  */
	double exp2_shift;
	double exp2_poly[EXP2_POLY_ORDER];
	uint64_t tab[2*(1<<EXP_TABLE_BITS)];
} __exp_data;

#define POW_LOG_TABLE_BITS 7
#define POW_LOG_POLY_ORDER 8
extern const struct pow_log_data{
	double ln2hi;
	double ln2lo;
	double poly[POW_LOG_POLY_ORDER-1];/* First coefficient is 1.  */
	/* Note: the pad field is unused,but allows slightly faster indexing.  */
	struct {double invc,pad,logc,logctail;}tab[1<<POW_LOG_TABLE_BITS];
} __pow_log_data;

#define LOG_TABLE_BITS 7
#define LOG_POLY_ORDER 6
#define LOG_POLY1_ORDER 12
extern const struct log_data {
	double ln2hi;
	double ln2lo;
	double poly[LOG_POLY_ORDER - 1]; /* First coefficient is 1.  */
	double poly1[LOG_POLY1_ORDER - 1];
	struct {
		double invc, logc;
	} tab[1 << LOG_TABLE_BITS];
#if !__FP_FAST_FMA
	struct {
		double chi, clo;
	} tab2[1 << LOG_TABLE_BITS];
#endif
} __log_data;

extern float comp__cosdf(double x);
extern float comp__sindf(double x);
extern float comp__tandf(double x,int odd);
extern double comp__sin(double,double,int);
extern double comp__cos(double,double);
extern double comp__tan(double,double,int);
extern double comp__expo2(double,double);
extern int comp__rem_pio2(double,double*);
extern int comp__rem_pio2f(float x,double*y);
extern int comp__rem_pio2_large(double*x,double*y,int e0,int nx,int prec);
extern float comp_acosf(float x);
extern float comp_atan2f(float y,float x);
extern float comp_ceilf(float x);
extern float comp_cosf(float x);
extern float comp_fabsf(float x);
extern float comp_floorf(float x);
extern float comp_fmodf(float x,float y);
extern float comp_sinf(float x);
extern double comp_sqrt(double x);
extern float comp_sqrtf(float x);
extern float comp_tanf(float x);
extern float comp_atanf(float x);
extern double comp_fabs(double x);
extern double comp_sqrt(double x);
extern double comp_pow(double x,double y);
extern double comp_scalbn(double x,int n);
extern double comp_floor(double x);
#define DBL_EPSILON 2.22044604925031308085e-16
#define __cosdf comp__cosdf
#define __sindf comp__sindf
#define __tandf comp__tandf
#define __rem_pio2f comp__rem_pio2f
#define __rem_pio2_large comp__rem_pio2_large
#define __math_xflow comp__math_xflow
#define __math_xflowf comp__math_xflowf
#define __math_uflow comp__math_uflow
#define __math_uflowf comp__math_uflowf
#define __math_oflow comp__math_oflow
#define __math_oflowf comp__math_oflowf
#define __math_invalid comp__math_invalid
#define __math_invalidf comp__math_invalidf
#define __math_divzero comp__math_divzero
#define tanf comp_tanf
#define atanf comp_atanf
#define atan2f comp_atan2f
#define ceilf comp_ceilf
#define acosf comp_acosf
#define cosf comp_cosf
#define fabs comp_fabs
#define fabsf comp_fabsf
#define floor comp_floor
#define floorf comp_floorf
#define fmodf comp_fmodf
#define sinf comp_sinf
#define sqrt comp_sqrt
#define sqrtf comp_sqrtf
#define pow comp_pow
#define scalbn comp_scalbn

#define asuint(f) ((union{float _f;uint32_t _i;}){f})._i
#define asfloat(i) ((union{uint32_t _i;float _f;}){i})._f
#define asuint64(f) ((union{double _f;uint64_t _i;}){f})._i
#define asdouble(i) ((union{uint64_t _i;double _f;}){i})._f
#define GET_FLOAT_WORD(w,d) do{(w)=asuint(d);}while(0)
#define SET_FLOAT_WORD(d,w) do{(d)=asfloat(w);}while(0)
#define GET_HIGH_WORD(hi,d) do{(hi)=asuint64(d)>>32;}while(0)
#define GET_LOW_WORD(lo,d) do{(lo)=(uint32_t)asuint64(d);}while(0)
#define EXTRACT_WORDS(hi,lo,d) do{uint64_t __u=asuint64(d);(hi)=__u>>32;(lo)=(uint32_t)__u;}while(0)
#define INSERT_WORDS(d,hi,lo) do{(d)=asdouble(((uint64_t)(hi)<<32)|(uint32_t)(lo));}while(0)
#define SET_HIGH_WORD(d,hi) INSERT_WORDS(d,hi,(uint32_t)asuint64(d))
#define SET_LOW_WORD(d,lo) INSERT_WORDS(d,asuint64(d)>>32,lo)

#if !defined(__DEFINED_float_t)
typedef float float_t;
#define __DEFINED_float_t
#endif

#if !defined(__DEFINED_double_t)
typedef double double_t;
#define __DEFINED_double_t
#endif

#define FORCE_EVAL(x) do{\
        if(sizeof(x)==sizeof(float))fp_force_evalf(x);\
        else if(sizeof(x)==sizeof(double))fp_force_eval(x);\
        else fp_force_evall(x);\
}while(0)

#ifndef fp_force_evall
#define fp_force_evall fp_force_evall
static inline void fp_force_evall(long double x){volatile long double y;y=x;(void)y;}
#endif

#ifndef fp_force_evalf
#define fp_force_evalf fp_force_evalf
static inline void fp_force_evalf(float x){volatile float y;y=x;(void)y;}
#endif

#ifndef fp_force_eval
#define fp_force_eval fp_force_eval
static inline void fp_force_eval(double x){volatile double y;y=x;(void)y;}
#endif

#ifndef fp_barrierf
#define fp_barrierf fp_barrierf
static inline float fp_barrierf(float x){volatile float y=x;return y;}
#endif

#ifndef fp_barrier
#define fp_barrier fp_barrier
static inline double fp_barrier(double x){volatile double y=x;return y;}
#endif

#ifndef fp_barrierl
#define fp_barrierl fp_barrierl
static inline long double fp_barrierl(long double x){volatile long double y=x;return y;}
#endif

static inline float eval_as_float(float x){float y=x;return y;}
static inline double eval_as_double(double x){double y=x;return y;}
static inline double comp__math_invalid(double x){return(x-x)/(x-x);}
static inline float comp__math_invalidf(float x){return(x-x)/(x-x);}
static inline double comp__math_xflow(uint32_t sign,double y){return eval_as_double(fp_barrier(sign?-y:y)*y);}
static inline float comp__math_xflowf(uint32_t sign,float y){return eval_as_float(fp_barrierf(sign?-y:y)*y);}
static inline double comp__math_uflow(uint32_t sign){return comp__math_xflow(sign,0x1p-767);}
static inline float comp__math_uflowf(uint32_t sign){return comp__math_xflowf(sign,0x1p-95f);}
static inline float comp__math_oflowf(uint32_t sign){return comp__math_xflowf(sign,0x1p97f);}
static inline double comp__math_oflow(uint32_t sign){return comp__math_xflow(sign,0x1p769);}
static inline double comp__math_divzero(uint32_t sign){return fp_barrier(sign?-1.0:1.0)/0.0;}

#ifndef predict_true
#ifdef __GNUC__
#define predict_true(x) __builtin_expect(!!(x),1)
#define predict_false(x) __builtin_expect(x,0)
#else
#define predict_true(x) (x)
#define predict_false(x) (x)
#endif
#endif

#if 100*__GNUC__+__GNUC_MINOR__ >= 303
#define NAN       __builtin_nanf("")
#define INFINITY  __builtin_inff()
#else
#define NAN       (0.0f/0.0f)
#define INFINITY  1e5000f
#endif

#define HUGE_VALF INFINITY
#define HUGE_VAL  ((double)INFINITY)
#define HUGE_VALL ((long double)INFINITY)

#define MATH_ERRNO  1
#define MATH_ERREXCEPT 2
#define math_errhandling 2

#define FP_ILOGBNAN (-1-0x7fffffff)
#define FP_ILOGB0 FP_ILOGBNAN

#define FP_NAN       0
#define FP_INFINITE  1
#define FP_ZERO      2
#define FP_SUBNORMAL 3
#define FP_NORMAL    4

#ifdef __FP_FAST_FMA
#define FP_FAST_FMA 1
#endif

#ifdef __FP_FAST_FMAF
#define FP_FAST_FMAF 1
#endif

#ifdef __FP_FAST_FMAL
#define FP_FAST_FMAL 1
#endif

int __fpclassify(double);
int __fpclassifyf(float);
int __fpclassifyl(long double);

static __inline unsigned __FLOAT_BITS(float __f)
{
	union {float __f; unsigned __i;} __u;
	__u.__f = __f;
	return __u.__i;
}
static __inline unsigned long long __DOUBLE_BITS(double __f)
{
	union {double __f; unsigned long long __i;} __u;
	__u.__f = __f;
	return __u.__i;
}

#define fpclassify(x) ( \
	sizeof(x) == sizeof(float) ? __fpclassifyf(x) : \
	sizeof(x) == sizeof(double) ? __fpclassify(x) : \
	__fpclassifyl(x) )

#define isinf(x) ( \
	sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) == 0x7f800000 : \
	sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL>>1) == 0x7ffULL<<52 : \
	__fpclassifyl(x) == FP_INFINITE)

#define isnan(x) ( \
	sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) > 0x7f800000 : \
	sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL>>1) > 0x7ffULL<<52 : \
	__fpclassifyl(x) == FP_NAN)

#define isnormal(x) ( \
	sizeof(x) == sizeof(float) ? ((__FLOAT_BITS(x)+0x00800000) & 0x7fffffff) >= 0x01000000 : \
	sizeof(x) == sizeof(double) ? ((__DOUBLE_BITS(x)+(1ULL<<52)) & -1ULL>>1) >= 1ULL<<53 : \
	__fpclassifyl(x) == FP_NORMAL)

#define isfinite(x) ( \
	sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) < 0x7f800000 : \
	sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL>>1) < 0x7ffULL<<52 : \
	__fpclassifyl(x) > FP_INFINITE)

int __signbit(double);
int __signbitf(float);
int __signbitl(long double);

#define signbit(x) ( \
	sizeof(x) == sizeof(float) ? (int)(__FLOAT_BITS(x)>>31) : \
	sizeof(x) == sizeof(double) ? (int)(__DOUBLE_BITS(x)>>63) : \
	__signbitl(x) )

#define isunordered(x,y) (isnan((x)) ? ((void)(y),1) : isnan((y)))

#define __ISREL_DEF(rel, op, type) \
static __inline int __is##rel(type __x, type __y) \
{ return !isunordered(__x,__y) && __x op __y; }

__ISREL_DEF(lessf, <, float_t)
__ISREL_DEF(less, <, double_t)
__ISREL_DEF(lessl, <, long double)
__ISREL_DEF(lessequalf, <=, float_t)
__ISREL_DEF(lessequal, <=, double_t)
__ISREL_DEF(lessequall, <=, long double)
__ISREL_DEF(lessgreaterf, !=, float_t)
__ISREL_DEF(lessgreater, !=, double_t)
__ISREL_DEF(lessgreaterl, !=, long double)
__ISREL_DEF(greaterf, >, float_t)
__ISREL_DEF(greater, >, double_t)
__ISREL_DEF(greaterl, >, long double)
__ISREL_DEF(greaterequalf, >=, float_t)
__ISREL_DEF(greaterequal, >=, double_t)
__ISREL_DEF(greaterequall, >=, long double)

#define __tg_pred_2(x, y, p) ( \
	sizeof((x)+(y)) == sizeof(float) ? p##f(x, y) : \
	sizeof((x)+(y)) == sizeof(double) ? p(x, y) : \
	p##l(x, y) )

#define isless(x, y)            __tg_pred_2(x, y, __isless)
#define islessequal(x, y)       __tg_pred_2(x, y, __islessequal)
#define islessgreater(x, y)     __tg_pred_2(x, y, __islessgreater)
#define isgreater(x, y)         __tg_pred_2(x, y, __isgreater)
#define isgreaterequal(x, y)    __tg_pred_2(x, y, __isgreaterequal)

double      acos(double);
float       acosf(float);
long double acosl(long double);

double      acosh(double);
float       acoshf(float);
long double acoshl(long double);

double      asin(double);
float       asinf(float);
long double asinl(long double);

double      asinh(double);
float       asinhf(float);
long double asinhl(long double);

double      atan(double);
float       atanf(float);
long double atanl(long double);

double      atan2(double, double);
float       atan2f(float, float);
long double atan2l(long double, long double);

double      atanh(double);
float       atanhf(float);
long double atanhl(long double);

double      cbrt(double);
float       cbrtf(float);
long double cbrtl(long double);

double      ceil(double);
float       ceilf(float);
long double ceill(long double);

double      copysign(double, double);
float       copysignf(float, float);
long double copysignl(long double, long double);

double      cos(double);
float       cosf(float);
long double cosl(long double);

double      cosh(double);
float       coshf(float);
long double coshl(long double);

double      erf(double);
float       erff(float);
long double erfl(long double);

double      erfc(double);
float       erfcf(float);
long double erfcl(long double);

double      exp(double);
float       expf(float);
long double expl(long double);

double      exp2(double);
float       exp2f(float);
long double exp2l(long double);

double      expm1(double);
float       expm1f(float);
long double expm1l(long double);

double      fabs(double);
float       fabsf(float);
long double fabsl(long double);

double      fdim(double, double);
float       fdimf(float, float);
long double fdiml(long double, long double);

double      floor(double);
float       floorf(float);
long double floorl(long double);

double      fma(double, double, double);
float       fmaf(float, float, float);
long double fmal(long double, long double, long double);

double      fmax(double, double);
float       fmaxf(float, float);
long double fmaxl(long double, long double);

double      fmin(double, double);
float       fminf(float, float);
long double fminl(long double, long double);

double      fmod(double, double);
float       fmodf(float, float);
long double fmodl(long double, long double);

double      frexp(double, int *);
float       frexpf(float, int *);
long double frexpl(long double, int *);

double      hypot(double, double);
float       hypotf(float, float);
long double hypotl(long double, long double);

int         ilogb(double);
int         ilogbf(float);
int         ilogbl(long double);

double      ldexp(double, int);
float       ldexpf(float, int);
long double ldexpl(long double, int);

double      lgamma(double);
float       lgammaf(float);
long double lgammal(long double);

long long   llrint(double);
long long   llrintf(float);
long long   llrintl(long double);

long long   llround(double);
long long   llroundf(float);
long long   llroundl(long double);

double      log(double);
float       logf(float);
long double logl(long double);

double      log10(double);
float       log10f(float);
long double log10l(long double);

double      log1p(double);
float       log1pf(float);
long double log1pl(long double);

double      log2(double);
float       log2f(float);
long double log2l(long double);

double      logb(double);
float       logbf(float);
long double logbl(long double);

long        lrint(double);
long        lrintf(float);
long        lrintl(long double);

long        lround(double);
long        lroundf(float);
long        lroundl(long double);

double      modf(double, double *);
float       modff(float, float *);
long double modfl(long double, long double *);

double      nan(const char *);
float       nanf(const char *);
long double nanl(const char *);

double      nearbyint(double);
float       nearbyintf(float);
long double nearbyintl(long double);

double      nextafter(double, double);
float       nextafterf(float, float);
long double nextafterl(long double, long double);

double      nexttoward(double, long double);
float       nexttowardf(float, long double);
long double nexttowardl(long double, long double);

double      pow(double, double);
float       powf(float, float);
long double powl(long double, long double);

double      remainder(double, double);
float       remainderf(float, float);
long double remainderl(long double, long double);

double      remquo(double, double, int *);
float       remquof(float, float, int *);
long double remquol(long double, long double, int *);

double      rint(double);
float       rintf(float);
long double rintl(long double);

double      round(double);
float       roundf(float);
long double roundl(long double);

double      scalbln(double, long);
float       scalblnf(float, long);
long double scalblnl(long double, long);

double      scalbn(double, int);
float       scalbnf(float, int);
long double scalbnl(long double, int);

double      sin(double);
float       sinf(float);
long double sinl(long double);

double      sinh(double);
float       sinhf(float);
long double sinhl(long double);

double      sqrt(double);
float       sqrtf(float);
long double sqrtl(long double);

double      tan(double);
float       tanf(float);
long double tanl(long double);

double      tanh(double);
float       tanhf(float);
long double tanhl(long double);

double      tgamma(double);
float       tgammaf(float);
long double tgammal(long double);

double      trunc(double);
float       truncf(float);
long double truncl(long double);
#undef  MAXFLOAT
#define MAXFLOAT        3.40282346638528859812e+38F
#define M_E             2.7182818284590452354   /* e */
#define M_LOG2E         1.4426950408889634074   /* log_2 e */
#define M_LOG10E        0.43429448190325182765  /* log_10 e */
#define M_LN2           0.69314718055994530942  /* log_e 2 */
#define M_LN10          2.30258509299404568402  /* log_e 10 */
#define M_PI            3.14159265358979323846  /* pi */
#define M_PI_2          1.57079632679489661923  /* pi/2 */
#define M_PI_4          0.78539816339744830962  /* pi/4 */
#define M_1_PI          0.31830988618379067154  /* 1/pi */
#define M_2_PI          0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI      1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2         1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2       0.70710678118654752440  /* 1/sqrt(2) */
extern int signgam;
double      j0(double);
double      j1(double);
double      jn(int, double);
double      y0(double);
double      y1(double);
double      yn(int, double);
#define HUGE            3.40282346638528859812e+38F
double      drem(double, double);
float       dremf(float, float);
int         finite(double);
int         finitef(float);
double      scalb(double, double);
float       scalbf(float, float);
double      significand(double);
float       significandf(float);
double      lgamma_r(double, int*);
float       lgammaf_r(float, int*);
float       j0f(float);
float       j1f(float);
float       jnf(int, float);
float       y0f(float);
float       y1f(float);
float       ynf(int, float);
long double lgammal_r(long double, int*);
void        sincos(double, double*, double*);
void        sincosf(float, float*, float*);
void        sincosl(long double, long double*, long double*);
double      exp10(double);
float       exp10f(float);
long double exp10l(long double);
double      pow10(double);
float       pow10f(float);
long double pow10l(long double);
#ifdef __cplusplus
}
#endif
#endif
