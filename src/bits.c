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


#include "bits.h"

//#define DEBUG

#define INLINE __inline
#define BITIN_BUF_LEN 32768 /* BITIN_BUF_LEN*sizeof(ulong) bytes */
#define BITIN32_BUF_LEN 65536 /* BITIN32_BUF_LEN*sizeof(uint) bytes */
#define BITOUT_BUF_LEN 32768 /* BITOUT_BUF_LEN*sizeof(ulong) bytes */
#define BITOUT32_BUF_LEN 65536 /* BITIN32_BUF_LEN*sizeof(uint) bytes */

/*
 * BITOUT Functions for 64bits version.
 */

BITOUT *createBitout(FILE *output) {
  BITOUT *b = (BITOUT*)malloc(sizeof(BITOUT));

  b->output = output;
  b->emplen = LONG_BITS;
  b->bitbuf = 0;
  b->buftop = (ulong*)calloc(BITOUT_BUF_LEN+1, sizeof(ulong));
  b->bufpos = b->buftop;
  b->bufend = b->buftop + BITOUT_BUF_LEN;
  return b;
}

INLINE
void writeBits(BITOUT *b, ulong x, uint wblen) {
  uint s;

#ifdef DEBUG
  if (wblen > LONG_BITS) {
    fprintf(stderr, 
	    "Error: length of write bits (%d) is longer than %d.\n", 
	    wblen, LONG_BITS);
    exit (1);
  }
  if (wblen == 0) {
    return;
  }
#endif

  if (wblen < b->emplen) {
    b->emplen -= wblen;
    b->bitbuf |= x << b->emplen;
  }
  else {
    s = wblen - b->emplen;
    b->bitbuf |= x >> s;
    *b->bufpos++ = b->bitbuf;
    b->emplen = LONG_BITS - s;
    if (b->emplen != LONG_BITS) {
      b->bitbuf = x << b->emplen;
    }
    else {
      b->bitbuf = 0;
    }

    if (b->bufpos == b->bufend) {
      fwrite(b->buftop, sizeof(ulong), 
	     BITOUT_BUF_LEN, b->output);
      memset(b->buftop, 0, sizeof(ulong)*BITOUT_BUF_LEN);
      b->bufpos = b->buftop;
    }
  }
}

void flushBitout(BITOUT *b)
{
  uint n;
  if (b->emplen != LONG_BITS) {
    *(b->bufpos) = b->bitbuf;
    b->bufpos++;
  }
  n = fwrite(b->buftop, sizeof(ulong), 
	     b->bufpos - b->buftop, b->output);
  memset(b->buftop, 0, sizeof(ulong)*BITOUT_BUF_LEN);
  b->bufpos = b->buftop;
  b->bitbuf = 0;
  b->emplen = LONG_BITS;
}

void destructBitout(BITOUT *b)
{
  if (b == NULL) return;
  if (b->buftop != NULL) free(b->buftop);
  free(b);
}

/************** END BITOUT ********************/


/*
 * BITOUT Functions for 32bits version.
 */

BITOUT32 *createBitout32(FILE *output) {
  BITOUT32 *b = (BITOUT32*)malloc(sizeof(BITOUT32));

  b->output = output;
  b->emplen = INT_BITS;
  b->bitbuf = 0;
  b->buftop = (uint*)calloc(BITOUT32_BUF_LEN+1, sizeof(uint));
  b->bufpos = b->buftop;
  b->bufend = b->buftop + BITOUT32_BUF_LEN;
  return b;
}

INLINE
void writeBits32(BITOUT32 *b, uint x, uint wblen) {
  uint s;

#ifdef DEBUG
  if (wblen > INT_BITS) {
    fprintf(stderr, "Error: length of write bits (%d) is longer than %d.\n", 
	    wblen, INT_BITS);
    exit (1);
  }
  if (wblen == 0) {
    return;
  }
#endif

  if (wblen < b->emplen) {
    b->emplen -= wblen;
    b->bitbuf |= x << b->emplen;
  }
  else {
    s = wblen - b->emplen;
    b->bitbuf |= x >> s;
    *b->bufpos++ = b->bitbuf;
    b->emplen = INT_BITS - s;
    if (b->emplen != INT_BITS) {
      b->bitbuf = x << b->emplen;
    }
    else {
      b->bitbuf = 0;
    }
 
    if (b->bufpos == b->bufend) {
      fwrite(b->buftop, sizeof(uint), 
	     BITOUT32_BUF_LEN, b->output);
      memset(b->buftop, 0, sizeof(uint)*BITOUT32_BUF_LEN);
      b->bufpos = b->buftop;
    }
  }
}

void flushBitout32(BITOUT32 *b)
{
  uint n;
  if (b->emplen != INT_BITS) {
    *(b->bufpos) = b->bitbuf;
    b->bufpos++;
  }
  n = fwrite(b->buftop, sizeof(uint), 
	     b->bufpos - b->buftop, b->output);
  memset(b->buftop, 0, sizeof(uint)*BITOUT32_BUF_LEN);
  b->bufpos = b->buftop;
  b->bitbuf = 0;
  b->emplen = INT_BITS;
}

void destructBitout32(BITOUT32 *b)
{
  if (b == NULL) return;
  if (b->buftop != NULL) free(b->buftop);
  free(b);
}

/************* END BITOUT32 ***************/


/*
 * BITIN Functions for 64bits version.
 */

BITIN *createBitin(FILE *input) {
  BITIN *b = (BITIN*)malloc(sizeof(BITIN));

  b->input = input;
  b->bitlen = 0;
  b->bitbuf = 0;
  b->buftop = (ulong*)calloc(BITIN_BUF_LEN, sizeof(ulong));
  b->bufpos = b->bufend = b->buftop;

  return b;
}

INLINE
ulong readBits(BITIN *b, const uint rblen) {
  ulong x;
  uint s, n;
  
#ifdef DEBUG
  if (rblen > LONG_BITS) {
    fprintf(stderr, 
	    "Error: length of read bits (%d) is longer than %d.\n", 
	    rblen, LONG_BITS);
    exit (1);
  }
  if (rblen == 0) {
    return 0;
  }
#endif

  if (rblen < b->bitlen) {
    x = b->bitbuf >> (LONG_BITS - rblen);
    b->bitbuf <<= rblen;
    b->bitlen -= rblen;
  }
  else {
    if (b->bufpos == b->bufend) {
      n = fread(b->buftop, sizeof(ulong), 
		BITIN_BUF_LEN, b->input);
      b->bufpos = b->buftop;
      b->bufend = b->buftop + n;
      if (b->bufend < b->buftop) {
	fprintf(stderr, 
		"Error: new bits buffer was not loaded.\n");
	exit(1);
      }
    }

    s = rblen - b->bitlen;
    x = b->bitbuf >> (LONG_BITS - b->bitlen - s);
    b->bitbuf = *b->bufpos++;
    b->bitlen = LONG_BITS - s;

    if (s != 0) {
      x |= b->bitbuf >> b->bitlen;
      b->bitbuf <<= s;
    }
  }
  return x;
}

void destructBitin(BITIN *b)
{
  if (b == NULL) return;
  if (b->buftop != NULL) free(b->buftop);
  free(b);
}

/********** END BITIN ************/


/*
 * BITIN Functions for 32bits version.
 */

BITIN32 *createBitin32(FILE *input) {
  BITIN32 *b = (BITIN32*)malloc(sizeof(BITIN32));

  b->input = input;
  b->bitlen = 0;
  b->bitbuf = 0;
  b->buftop = (uint*)calloc(BITIN32_BUF_LEN, sizeof(uint));
  b->bufpos = b->bufend = b->buftop;

  return b;
}

INLINE
uint readBits32(BITIN32 *b, const uint rblen) {
  uint x;
  uint s, n;
  
#ifdef DEBUG
  if (rblen > INT_BITS) {
    fprintf(stderr, 
	    "Error: length of read bits (%d) is longer than %d.\n", 
	    rblen, INT_BITS);
    exit (1);
  }
  if (rblen == 0) {
    return 0;
  }
#endif

  if (rblen < b->bitlen) {
    x = b->bitbuf >> (INT_BITS - rblen);
    b->bitbuf <<= rblen;
    b->bitlen -= rblen;
  }
  else {
    if (b->bufpos == b->bufend) {
      n = fread(b->buftop, sizeof(uint), 
		BITIN32_BUF_LEN, b->input);
      b->bufpos = b->buftop;
      b->bufend = b->buftop + n;
      if (b->bufend < b->buftop) {
	fprintf(stderr, 
		"Error: new bits buffer was not loaded.\n");
	exit(1);
      }
    }

    s = rblen - b->bitlen;
    x = b->bitbuf >> (INT_BITS - b->bitlen - s);
    b->bitbuf = *b->bufpos++;
    b->bitlen = INT_BITS - s;

    if (s != 0) {
      x |= b->bitbuf >> b->bitlen;
      b->bitbuf <<= s;
    }
  }
  return x;
}

void destructBitin32(BITIN32 *b)
{
  if (b == NULL) return;
  if (b->buftop != NULL) free(b->buftop);
  free(b);
}

/************* END BITIN32 **************/
