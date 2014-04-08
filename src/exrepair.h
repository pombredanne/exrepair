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

#ifndef CREPAIR_H
#define CREPAIR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "basics.h"
#include "bits.h"
#include "cont.h"

typedef struct Rule {
  uint num_rules;
  CODE *left;
  CODE *right;
  uint buff_size;
} RULE;

typedef struct Dictionary {
  uint txt_len;
  uint code_len;
  uint char_size;
  uchar *echar_table;
  uint mchar_size;
  uchar *mchar_table;
  uint cont_len;
  uint num_contexts;
  RULE **rule;
  uint seq_len;
  CODE *comp_seq;
} DICT;

DICT *RunCodeRepair(FILE *input, uint code_len, uint cont_len, uint mchar_size);
void DestructDict(DICT *dict);
void OutputCompTxt(DICT *dict, FILE *output);

#endif /* CREPAIR_H */
