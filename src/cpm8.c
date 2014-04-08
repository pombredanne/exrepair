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

#include "cpm.h"

//#define DEBUG
//#define DISPLAY

#define INPUT_BUF_SIZE 32768

typedef short STATE;

typedef struct Rule {
  CODE left;
  CODE right;
} RULE;

typedef struct Dictionary {
  uint txt_len;
  uint char_size;
  uint seq_len;
  uchar *echar_table;
  short *char_table;
  uint *num_rules;
  RULE **rule;
} DICT;

#define MAX_CHARSIZE 256
#define MAX_PATLEN 32

static uint num_state;
static uint num_code;
static STATE GoTo[MAX_CHARSIZE+MAX_PATLEN][MAX_CHAR_SIZE];
static uint Output[MAX_CHAR_SIZE+MAX_PATLEN][MAX_CHAR_SIZE];

static bool encodePattern (uchar *pat, uint plen, DICT *dict);
static void mkPMM         (uchar *pat, uint plen, uint code_len, DICT *dict);
static DICT *mkDict       (FILE *input);
static uint runPMM8       (FILE *input);
static void readRule8     (DICT *dict, FILE *input);

static
uint runPMM8(FILE *input)
{
  uint lg;
  uint result = 0;
  STATE q, qt;
  uchar *buftop, *ptr;

  q = HEAD_PCHAR;
  buftop = (uchar*)malloc(sizeof(uchar)*INPUT_BUF_SIZE*2);
  ptr = buftop;
  while ((lg = fread(buftop, sizeof(uchar), INPUT_BUF_SIZE*2, input)) > 0) {
    ptr = buftop;
    do {
      if ((qt = GoTo[q][*ptr]) < 0) {
	result += Output[q][*ptr];
	qt = ~qt;
      }
      ++ptr;
      q = qt;
    } while (--lg > 0);
  }
  free(buftop);
  return result;
}

static
void mkPMM(uchar *pat, uint plen, uint code_len, DICT *dict)
{
  uint char_size = dict->char_size;
  RULE **d = dict->rule;
  uint *num_rules = dict->num_rules;
  STATE *failure;

  num_code = MAX_CHARSIZE;
  num_state = char_size+plen-1;
  failure = (STATE*)calloc((plen+1), sizeof(STATE));

  // create failure function.
  {
    uint i, j;
    for (i = 0; i < plen; i++) {
      failure[i] = -1;
    }
    i = -1;
    for (j = 0; j < plen; j++) {
      while (i != -1 && pat[j] != pat[i]) {
	i = failure[i];
      }
      i++;
      if (pat[j+1] == pat[i]) {
	failure[j+1] = failure[i];
      }
      else {
	failure[j+1] = i;
      }
    }
  }
 
  {
    uint s, q, p;
    CODE c, lc, rc;

    for (s = 0; s < MAX_CHARSIZE+MAX_PATLEN; s++) {
      for (c = 0; c < MAX_CHARSIZE; c++) {
	GoTo[s][c] = 0;
	Output[s][c] = 0;
      }
    }

    // for q0 and terminal symbols
    for (c = 0; c < char_size; c++) {
      for (s = 0; s < char_size; s++) {
	if (c == pat[0]) {
	  GoTo[s][c] = char_size;
	}
	else {
	  GoTo[s][c] = c;
	}
	if (c == pat[0] && plen == 1) {
	  GoTo[s][c] = c;
	  Output[s][c] = 1;
	}
      }
    }
    
    // for q(1~final) and terminal symbols
    for (c = 0; c < char_size; c++) {
      for (q = 1; q < plen; q++) {
	p = q;
	while (p != -1 && pat[p] != c) {
	  p = failure[p];
	}
	s = q + char_size - 1;
	if (p == -1) {
	  GoTo[s][c] = c; //go initial state.
	}
	else {
	  GoTo[s][c] = p + char_size; //go next state.
	}
	if (p+1 == plen) { //if accepted state,
	  GoTo[s][c] = c;
	  Output[s][c] = 1;
	}
      }
    }
  
    // for q(0~final) and nonterminal symbols
    for (c = char_size; c < num_code; c++) {
      for (s = 0; s < num_state; s++) {
	if (s < char_size) {
	  q = s;
	}
	else {
	  q = pat[s-char_size];
	}      
	if (num_rules[q] <= c) {
	  continue;
	}
	
	lc = d[q][c].left;
	rc = d[q][c].right;
	p = GoTo[s][lc];
	GoTo[s][c] = GoTo[p][rc];
	Output[s][c] =  Output[s][lc];
	Output[s][c] += Output[p][rc];
      }
    }

    //optimize
    for (s = 0; s < num_state; s++) {
      for (c = 0; c < num_code; c++) {
	if (Output[s][c] > 0) {
	  GoTo[s][c] = ~GoTo[s][c];
	}
      }
    }
  }

  free(failure);
}

static
bool encodePattern(uchar *pat, uint plen, DICT *dict)
{
  uint i;
  short c;

  for (i = 0; i < plen; i++) {
    if ((c = dict->char_table[pat[i]]) == -1) {
      return false; //including no existing characters.
    }
    pat[i] = (uchar)c;
  }
  return true;
}

static
void readRule8(DICT *dict, FILE *input)
{
  uint i, j;
  const uint num_code = 256;
  RULE **rule = dict->rule;
  uint *num_rules = dict->num_rules;
  uint char_size = dict->char_size;
  uchar *buf = (uchar*)malloc(sizeof(uchar)*2*num_code);
  uchar *buftop = buf;

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      rule[i][j].left = j;
      rule[i][j].right = 0;
    }
    fread(buf, sizeof(uchar), 2*(num_rules[i]-char_size), input);
    for (j = char_size; j < num_rules[i]; j++) {
      rule[i][j].left  = *buf++;
      rule[i][j].right = *buf++;
    }
    buf = buftop;
  }
  free(buftop);
}

static
DICT *mkDict(FILE *input) 
{
  uint i, j;
  DICT *dict = (DICT*)malloc(sizeof(DICT));
  uint t_num_rules;

  fread(&dict->txt_len, sizeof(uint), 1, input);
  fread(&dict->char_size, sizeof(uint), 1, input);
  fread(&dict->seq_len, sizeof(uint), 1, input);
  dict->echar_table = (uchar*)malloc(sizeof(uchar)*(dict->char_size));
  fread(dict->echar_table, sizeof(uchar), dict->char_size, input);
  dict->num_rules = (uint*)malloc(sizeof(uint)*(dict->char_size));
  fread(dict->num_rules, sizeof(uint), dict->char_size, input);

#ifdef DEBUG
  printf("txt_len = %d\n", dict->txt_len);
  printf("char_size = %d\n", dict->char_size);
  printf("seq_len = %d\n", dict->seq_len);
  for (i = 0; i < dict->char_size; i++) {
    printf("num_rules[%d] = %d\n", i, dict->num_rules[i]);
  }
#endif

  dict->char_table = (short*)malloc(256*sizeof(short));
  for (i = 0; i < 256; i++) {
    dict->char_table[i] = -1;
  }
  for (i = 0; i < dict->char_size; i++) {
    dict->char_table[dict->echar_table[i]] = i;
  }

  t_num_rules = 0;
  for (i = 0; i < dict->char_size; i++) {
    t_num_rules += dict->num_rules[i];
  }

  dict->rule = (RULE**)malloc(sizeof(RULE*)*dict->char_size);
  dict->rule[0] = (RULE*)malloc(sizeof(RULE)*t_num_rules);
  j = 0;
  for (i = 0; i < dict->char_size; i++) {
    dict->rule[i] = dict->rule[0] + j;
    j += dict->num_rules[i];
  }
  return dict;
}

static
void destructDict(DICT *dict)
{
  free(dict->echar_table);
  free(dict->char_table);
  free(dict->rule[0]);
  free(dict->rule);
  free(dict);
}

uint Search8(uchar *pat, FILE *input)
{
  uint ret;
  uint code_len;  
  DICT *dict;
  uint plen = strlen((char*)pat);

  if (plen > MAX_PATLEN) {
    fprintf(stderr, "Error: A given pattern is longer than %d\n", MAX_PATLEN);
    exit(1);
  }

  fread(&code_len, sizeof(uint), 1, input);

  if (code_len != 8) {
    fprintf(stderr, "Error: this program runs on 8bits encoded text only.\n");
    exit(1);
  }

  dict = mkDict(input);
  readRule8(dict, input);
  if (encodePattern(pat, plen, dict) == false) {
    destructDict(dict);
    return 0;
  }
  mkPMM(pat, plen, 8, dict);
  destructDict(dict);
  ret = runPMM8(input);

  return ret;
}
