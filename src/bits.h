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

#ifndef BITS_H
#define BITS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "basics.h"

typedef struct bit_output64 {
  FILE *output;
  uint  emplen;
  ulong bitbuf;
  ulong *bufpos;
  ulong *buftop;
  ulong *bufend;
} BITOUT;

BITOUT *createBitout(FILE *output);
void writeBits(BITOUT *bitout, ulong symbol, uint writeBitLen);
void flushBitout(BITOUT *bitout);


typedef struct bit_output32 {
  FILE *output;
  uint  emplen;
  uint bitbuf;
  uint *bufpos;
  uint *buftop;
  uint *bufend;
} BITOUT32;

BITOUT32 *createBitout32(FILE *output);
void writeBits32(BITOUT32 *bitout, uint symbol, uint writeBitLen);
void flushBitout32(BITOUT32 *bitout);


typedef struct bit_input64 {
  FILE *input;
  uint  bitlen;
  ulong bitbuf;
  ulong *bufpos;
  ulong *buftop;
  ulong *bufend;
} BITIN;

BITIN *createBitin(FILE *input);
ulong readBits(BITIN *bitin, uint readBitLen);


typedef struct bit_input32 {
  FILE *input;
  uint  bitlen;
  uint bitbuf;
  uint *bufpos;
  uint *buftop;
  uint *bufend;
} BITIN32;

BITIN32 *createBitin32(FILE *input);
uint readBits32(BITIN32 *bitin, uint readBitLen);

#endif
