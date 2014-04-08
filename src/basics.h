/* 
 *  Copyright (c) 2011 Shirou Maruyama
 * 
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 * 
 *   1. Redistributions of source code must retain the above Copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above Copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 */

#ifndef BASICS_H
#define BASICS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>

#ifndef LOG2
# define LOG2(X) (log((double)(X))/log((double)2))
#endif
#define MAX_CHAR_SIZE 256
#define DUMMY_CODE UINT_MAX
#define DUMMY_POS UINT_MAX
#define HEAD_PCODE 0

#define INT_BITS  32
#define LONG_BITS 64

#ifndef uchar
typedef unsigned char uchar;
#endif
#ifndef uint
typedef unsigned int uint;
#endif
#ifndef ushort
typedef unsigned short ushort;
#endif
#ifndef ulong
typedef unsigned long ulong;
#endif
#ifndef bool
typedef _Bool bool;
#endif
#ifndef CODE
typedef unsigned int CODE;
#endif
#ifndef PCODE
typedef unsigned int PCODE;
#endif

#ifndef true
# define false 0
# define true  1
#endif

#endif /* BASICS_H */
