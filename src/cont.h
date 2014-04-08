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

#ifndef GETID_H
#define GETID_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "basics.h"


void getContext(uint mchar_size, uint cont_len, PCODE id, uchar *context);
PCODE getContextBegID(uint mchar_size, uint cont_len, 
		      uchar *cont_suffix, uint suffix_len);
PCODE getContextEndID(uint mchar_size, uint cont_len, 
		      uchar *cont_suffix, uint suffix_len);
PCODE getContextID(uint mchar_size, uint cont_len, uchar *cont_suffix);
uint getContextRangeSize(uint mchar_size, uint cont_len, uint suffix_len);


#endif /* GETID_H */
