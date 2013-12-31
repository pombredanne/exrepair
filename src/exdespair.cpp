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

#include "exdespair.h"

#define BUFFER_SIZE 32768

typedef struct ProductionRule8 {
  uchar left;
  uchar right;
} RULE8;

typedef struct ProductionRule16 {
  ushort left;
  ushort right;
} RULE16;

typedef struct ProductionRuleVar {
  CODE left;
  CODE right;
} RULEVar;

typedef struct EncodedString {
  uchar tchar;
  uint  len;
  uchar *string;
} ENC;

static
void readHeaders(uint *txt_len, uint *char_size, uint *seq_len, uchar **echar_table, uint **num_rules, FILE *input) {
  fread(txt_len, sizeof(uint), 1, input);
  fread(char_size, sizeof(uint), 1, input);
  fread(seq_len, sizeof(uint), 1, input);
  *echar_table = (uchar*)malloc(sizeof(uchar)*(*char_size));
  fread(*echar_table, sizeof(uchar), *char_size, input);
  *num_rules = (uint*)malloc(sizeof(uint)*(*char_size));
  fread(*num_rules, sizeof(uint), *char_size, input);
}

static
void runCodeDespair8F(FILE *input, FILE *output) {
  uint i, j;
  uint txt_len, char_size, seq_len, *num_rules;
  uchar *echar_table;
  uint t_num_rules;
  RULE8 **rule;
  ENC **enc;

  readHeaders(&txt_len, &char_size, &seq_len, &echar_table, &num_rules, input);

  t_num_rules = 0;
  for (i = 0; i < char_size; i++) {
    t_num_rules += num_rules[i];
  }

  rule    = (RULE8 **)malloc(sizeof(RULE8 *)*char_size);
  rule[0] = (RULE8 *)malloc(sizeof(RULE8)*t_num_rules);
  enc     = (ENC **)malloc(sizeof(ENC *)*char_size);
  enc[0]  = (ENC *)malloc(sizeof(ENC)*t_num_rules);

  for (i = 0, j = 0; i < char_size; i++) {
    rule[i] = rule[0] + j;
    enc[i]  = enc[0]  + j;
    j += num_rules[i];
  }

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      rule[i][j].left = j;
      rule[i][j].right = 0;
    }
    fread(rule[i]+char_size, sizeof(RULE8), num_rules[i]-char_size, input);
  }

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      enc[i][j].tchar = j;
      enc[i][j].len = 1;
      enc[i][j].string = (uchar*)malloc(sizeof(uchar)*2);
      enc[i][j].string[0] = echar_table[j];
      enc[i][j].string[1] = '\0';
    }
  }
  for (j = char_size; j < 256; j++) {
    for (i = 0; i < char_size; i++) {
      uchar l, r;
      PCODE p;
      if (j >= num_rules[i]) continue;
      l = rule[i][j].left; r = rule[i][j].right;
      p = enc[i][l].tchar;
      enc[i][j].len = enc[i][l].len + enc[p][r].len;
      enc[i][j].string = (uchar*)malloc(sizeof(uchar)*(enc[i][j].len+1));
      strcpy((char*)enc[i][j].string, (char*)enc[i][l].string);
      strcat((char*)enc[i][j].string, (char*)enc[p][r].string);
      enc[i][j].tchar = enc[p][r].tchar;
    }
  }

  free(echar_table);
  free(rule[0]); free(rule);

  printf("Expanding Compressed Text ...");
  fflush(stdout);
  {
    uchar rb[BUFFER_SIZE];
    uint  rb_len;
    uchar *rb_ptr, c;
    uchar wb[BUFFER_SIZE];
    uint  wb_pos;
    uchar *str, *wb_ptr;
    PCODE p = HEAD_PCHAR;

    wb_ptr = wb; wb_pos = 0;
    while ((rb_len = fread(rb, sizeof(uchar), BUFFER_SIZE, input)) > 0) {
      rb_ptr = rb;
      for (i = 0; i < rb_len; i++) {
	str = enc[p][c=*rb_ptr++].string;
	while (*str != '\0') {
	  *wb_ptr++ = *str++;
	  if (++wb_pos == BUFFER_SIZE) {
	    fwrite(wb, 1, BUFFER_SIZE, output);
	    wb_ptr = wb; wb_pos = 0;
	  }
	}
	p = enc[p][c].tchar;
      }
    }
    fwrite(wb, 1, wb_pos, output);
  }

  printf("Done!!\n");
  fflush(stdout);

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < num_rules[i]; j++) {
      free(enc[i][j].string);
    }
  }

  free(enc[0]); free(enc);
  free(num_rules);
}

static
void runCodeDespair16F(FILE *input, FILE *output) {
  uint i, j;
  uint txt_len, char_size, seq_len, *num_rules;
  uchar *echar_table;
  uint t_num_rules;
  RULE16 **rule;
  ENC **enc;

  readHeaders(&txt_len, &char_size, &seq_len, &echar_table, &num_rules, input);

  t_num_rules = 0;
  for (i = 0; i < char_size; i++) {
    t_num_rules += num_rules[i];
  }

  rule    = (RULE16**)malloc(sizeof(RULE16*)*char_size);
  rule[0] = (RULE16*)malloc(sizeof(RULE16)*t_num_rules);
  enc     = (ENC**)malloc(sizeof(ENC*)*char_size);
  enc[0]  = (ENC*)malloc(sizeof(ENC)*t_num_rules);

  for (i = 0, j = 0; i < char_size; i++) {
    rule[i] = rule[0] + j;
    enc[i]  = enc[0]  + j;
    j += num_rules[i];
  }

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      rule[i][j].left = j;
      rule[i][j].right = 0;
    }
    fread(rule[i]+char_size, sizeof(RULE16), num_rules[i]-char_size, input);
  }

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      enc[i][j].tchar = j;
      enc[i][j].len = 1;
      enc[i][j].string = (uchar*)malloc(sizeof(uchar)*2);
      enc[i][j].string[0] = echar_table[j];
      enc[i][j].string[1] = '\0';
    }
  }
  for (j = char_size; j < 65536; j++) {
    for (i = 0; i < char_size; i++) {
      ushort l, r;
      PCODE p;
      if (j >= num_rules[i]) continue;
      l = rule[i][j].left; r = rule[i][j].right;
      p = enc[i][l].tchar;
      enc[i][j].len = enc[i][l].len + enc[p][r].len;
      enc[i][j].string = (uchar*)malloc(sizeof(uchar)*(enc[i][j].len + 1));
      strcpy((char*)enc[i][j].string, (char*)enc[i][l].string);
      strcat((char*)enc[i][j].string, (char*)enc[p][r].string);
      enc[i][j].tchar = enc[p][r].tchar;
    }
  }
  free(echar_table);
  free(rule[0]);
  free(rule);

  printf("Expanding Compressed Text ...");
  fflush(stdout);
  {
    ushort rb[BUFFER_SIZE];
    uint  rb_len;
    ushort *rb_ptr, c;
    uchar wb[BUFFER_SIZE];
    uint  wb_pos;
    uchar *str, *wb_ptr;
    PCODE p = HEAD_PCHAR;

    wb_ptr = wb; wb_pos = 0;
    while ((rb_len = fread(rb, sizeof(ushort), BUFFER_SIZE, input)) > 0) {
      rb_ptr = rb;
      for (i = 0; i < rb_len; i++) {
	str = enc[p][c=*rb_ptr++].string;
	while (*str != '\0') {
	  *wb_ptr++ = *str++;
	  if (++wb_pos == BUFFER_SIZE) {
	    fwrite(wb, 1, BUFFER_SIZE, output);
	    wb_ptr = wb; wb_pos = 0;
	  }
	}
	p = enc[p][c].tchar;
      }
    }
    fwrite(wb, 1, wb_pos, output);
  }
  printf("Done!!\n");
  fflush(stdout);
  
  for (i = 0; i < char_size; i++) {
    for (j = 0; j < num_rules[i]; j++) {
      free(enc[i][j].string);
    }
  }

  free(enc[0]);
  free(enc);
  free(num_rules);
}

static
void runCodeDespairVarF(uint code_len, FILE *input, FILE *output) {
  uint lim_code = pow(2, code_len);
  uint i, j;
  uint txt_len, char_size, seq_len, *num_rules;
  uchar *echar_table;
  uint t_num_rules;
  RULEVar **rule;
  ENC **enc;
  BITIN *bitin;

  readHeaders(&txt_len, &char_size, &seq_len, &echar_table, &num_rules, input);

  t_num_rules = 0;
  for (i = 0; i < char_size; i++) {
    t_num_rules += num_rules[i];
  }

  rule    = (RULEVar **)malloc(sizeof(RULEVar *)*char_size);
  rule[0] = (RULEVar *)malloc(sizeof(RULEVar)*t_num_rules);
  enc     = (ENC **)malloc(sizeof(ENC *)*char_size);
  enc[0]  = (ENC *)malloc(sizeof(ENC)*t_num_rules);

  for (i = 0, j = 0; i < char_size; i++) {
    rule[i] = rule[0] + j;
    enc[i]  = enc[0]  + j;
    j += num_rules[i];
  }

  bitin = createBitin(input);
  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      rule[i][j].left  = j; rule[i][j].right = 0;
    }
    for (j = char_size; j < num_rules[i]; j++) {
      rule[i][j].left  = (CODE)readBits(bitin, code_len);
      rule[i][j].right = (CODE)readBits(bitin, code_len);
    }
  }

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      enc[i][j].tchar = j;
      enc[i][j].len = 1;
      enc[i][j].string = (uchar*)malloc(sizeof(uchar)*(1+1));
      enc[i][j].string[0] = echar_table[j];
      enc[i][j].string[1] = '\0';
    }
  }

  for (j = char_size; j < lim_code; j++) {
    for (i = 0; i < char_size; i++) {
      CODE l, r;
      PCODE p;
      if (j >= num_rules[i]) continue;
      l = rule[i][j].left; r = rule[i][j].right;
      p = enc[i][l].tchar;
      enc[i][j].len = enc[i][l].len + enc[p][r].len;
      enc[i][j].string = (uchar*)malloc(sizeof(uchar)*(enc[i][j].len+1));
      strcpy((char*)enc[i][j].string, (char*)enc[i][l].string);
      strcat((char*)enc[i][j].string, (char*)enc[p][r].string);
      enc[i][j].tchar = enc[p][r].tchar;
    }
  }

  free(echar_table);
  free(rule[0]);
  free(rule);

  printf("Expanding Compressed Text ...");
  fflush(stdout);
  {
    CODE  c;
    uchar wb[BUFFER_SIZE];
    uint  wb_pos;
    uchar *wb_ptr, *str;
    PCODE p = HEAD_PCHAR;

    wb_ptr = wb; wb_pos = 0;
    for (i = 0; i < seq_len; i++) {
      c = (CODE)readBits(bitin, code_len);
      str = enc[p][c].string;
      while (*str != '\0') {
	*wb_ptr++ = *str++;
	if (++wb_pos == BUFFER_SIZE) {
	  fwrite(wb, 1, BUFFER_SIZE, output);
	  wb_ptr = wb; wb_pos = 0;
	}
      }
      p = enc[p][c].tchar;
    }
    fwrite(wb, 1, wb_pos, output);
  }
  printf("Done!!\n");
  fflush(stdout);
  
  for (i = 0; i < char_size; i++) {
    for (j = 0; j < num_rules[i]; j++) {
      free(enc[i][j].string);
    }
  }
  free(enc[0]);
  free(enc);
  free(num_rules);
}

bool RunCodeDespair(FILE *input, FILE *output)
{
  uint codelen;
  
  fread(&codelen, sizeof(uint), 1, input);
  printf("code length = %d\n", codelen);

  if (8 > codelen || 16 < codelen) {
    return false;
  }

  switch (codelen) {
  case 8: 
    runCodeDespair8F(input, output); 
    break;
  case 16:
    runCodeDespair16F(input, output);
    break;
  default:
    runCodeDespairVarF(codelen, input, output);
    break;
  }
  return true;
}
