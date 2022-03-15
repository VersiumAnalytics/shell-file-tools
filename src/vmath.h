// Copyright 2014 Versium Analytics, Inc.
// License : BSD 3 clause
/*
   Most of the routines in here are simply wrappers for the C fctn in all
   lower-case, but have error checking built into.
*/
#ifndef __VMATH_H__
#define __VMATH_H__

#include <stdlib.h>

#define L_UNKNOWN       (0)
#define L_DIVISOR       (1)
#define L_QUADRES       (2)
#define L_NONRES        (3)

#define I_COMPOSITE     (0)
#define I_PRIME         (1)

#define RM_TRIALS       (25)

/* Integer Math Functions */
int64_t   ModExp        (int64_t A, int64_t X, int64_t N);
int64_t   GCD           (int64_t X, int64_t Y);
int64_t   Legendre      (int64_t A, int64_t X);
int64_t   ISqrt         (int64_t X);
int64_t   FactorSmTD    (int64_t X);
int64_t   FactorPollard (int64_t X);
int64_t   RabinMiller   (int64_t Trials, int64_t X);
int64_t   VMathRandom   (void);
int64_t   IMulMod       (int64_t x, int64_t y, int64_t mod);
int64_t   NextPrime     (int64_t CurPrime);

#endif /* __KMATH_H__ */
