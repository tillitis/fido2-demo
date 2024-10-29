#ifndef  _MATH_H_

#define  _MATH_H_

# ifndef INFINITY
#  define INFINITY (__builtin_inff())
# endif

# ifndef NAN
#  define NAN (__builtin_nanf(""))
# endif

extern double ldexp (double, int);

#endif /* _MATH_H_ */
