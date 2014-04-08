/* 
 *  Copyright (c) 2011-2012 Shirou Maruyama
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

#include "cont.h"

void getContext(uint mchar_size, uint cont_len, PCODE id, uchar *context)
{
  int i;
  uint div;
  uint mod;

  for (i = cont_len - 1; i >= 0; i--) {
    div = pow(mchar_size, i);
    mod = id / div;
    context[i] = mod;
    id -= div*mod;
  }
}

PCODE getContextBegID(uint mchar_size, uint cont_len, 
		      uchar *cont_suffix, uint suffix_len)
{
  int i;
  PCODE id = 0;

#ifdef DEBUG
  if (suffix_len > cont_len) {
    fprintf(stderr, "Exception error in getContextBegID.¥n");
    exit(1);
  }
#endif
  for (i = suffix_len-1; i >= 0; i--) {
    id += cont_suffix[i] * pow(mchar_size, i + cont_len - suffix_len);
  }
  return id;
}

PCODE getContextEndID(uint mchar_size, uint cont_len, 
		      uchar *cont_suffix, uint suffix_len)
{
  int i;
  PCODE id = 0;

#ifdef DEBUG
  if (suffix_len > cont_len) {
    fprintf(stderr, "Exception error in getContextEndID.¥n");
    exit(1);
  }
#endif
  for (i = suffix_len-1; i >= 0; i--) {
    id += cont_suffix[i] * pow(mchar_size, i + cont_len - suffix_len);
  }
  id += pow(mchar_size, cont_len - suffix_len) - 1;
  return id;
}

PCODE getContextID(uint mchar_size, uint cont_len, uchar *cont)
{
  return getContextBegID(mchar_size, cont_len, cont, cont_len);
}

uint getContextRangeSize(uint mchar_size, uint cont_len, uint suffix_len)
{
  return (uint)pow(mchar_size, (cont_len - suffix_len));
}
