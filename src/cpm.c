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
#define DISPLAY

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

typedef struct patternMatchingMachine {
  uint num_state;
  uint num_code;
  uint plen;
  uint slen;
  void *GoTo;
  void *Output;
} PMM;

static bool encodePattern (uchar *pat, uint plen, DICT *dict);
static PMM  *mkPMM        (uchar *pat, uint plen, uint code_len, DICT *dict);
static DICT *mkDict       (FILE *input);
static uint runPMM8       (PMM *pmm, FILE *input);
static uint runPMM16      (PMM *pmm, FILE *input);
static uint runPMM9       (PMM *pmm, BITIN *bitin);
static uint runPMM10      (PMM *pmm, BITIN *bitin);
static uint runPMM11      (PMM *pmm, BITIN *bitin);
static uint runPMM12      (PMM *pmm, BITIN *bitin);
static uint runPMM13      (PMM *pmm, BITIN *bitin);
static uint runPMM14      (PMM *pmm, BITIN *bitin);
static uint runPMM15      (PMM *pmm, BITIN *bitin);
static void readRule8     (DICT *dict, FILE *input);
static void readRule16    (DICT *dict, FILE *input);
static void readRuleVar   (DICT *dict, uint code_len, BITIN *bitin);
static uint search8       (uchar *pat, FILE *input);
static uint search16      (uchar *pat, FILE *input);
static uint searchVar     (uchar *pat, uint code_len, FILE *input);

#ifdef DEBUG
static
void printPMM(PMM *pmm)
{
  uint i, j;
  STATE *GoTo = pmm->GoTo;
  uint *Output = pmm->Output;

  for (i = 0; i < pmm->num_state; i++) {
    for (j = 0; j < pmm->num_code; j++) {
      printf("GoTo[%d][%d] = %d\n", i, j, GoTo[i][j]);
    }
  }
  for (i = 0; i < pmm->num_state; i++) {
    for (j = 0; j < pmm->num_code; j++) {
      printf("Output[%d][%d] = %d\n", i, j, Output[i][j]);
    }
  }
}
#endif

#ifdef DISPLAY
static
void printTableSize(PMM *pmm)
{
  printf("Size of GoTo Table = %d (bytes)\n", 
	 (int)sizeof(STATE)*pmm->num_state*pmm->num_code);
  printf("Size of Output Table = %d (bytes)\n", 
	 (int)sizeof(uint)*pmm->num_state*pmm->num_code);
}
#endif

static
uint runPMM8(PMM *pmm, FILE *input)
{
  uint lg;
  STATE (*GoTo)[256] = pmm->GoTo;
  uint (*Output)[256] = pmm->Output;
  uint result = 0;
  STATE q, qt;
  uchar *buftop, *ptr;

  q = HEAD_PCHAR;
  buftop = (uchar *)malloc(sizeof(uchar)*INPUT_BUF_SIZE*2);
  while ((lg = fread(buftop, sizeof(uchar), INPUT_BUF_SIZE*2, input)) > 0) {
    ptr = buftop;
    do {
      if ((qt = GoTo[q][*ptr]) < 0) {
	result += Output[q][*ptr];
	qt = ~qt;
      }
      ptr++; q = qt;
    } while (--lg > 0);
  }
  free(buftop);
  return result;
}

static
uint runPMM16(PMM *pmm, FILE *input)
{
  uint lg;
  STATE (*GoTo)[65536] = pmm->GoTo;
  uint (*Output)[65536] = pmm->Output;
  uint result = 0;
  STATE q, qt;
  ushort *buftop, *ptr;

  q = HEAD_PCHAR;
  buftop = (ushort*)malloc(sizeof(ushort)*INPUT_BUF_SIZE);
  while ((lg = fread(buftop, sizeof(ushort), 
		     INPUT_BUF_SIZE, input)) > 0) {
    ptr = buftop;
    do {
      if ((qt = GoTo[q][*ptr]) < 0) {
	result += Output[q][*ptr];
	qt = ~qt;
      }
      ptr++;
      q = qt;
    } while (--lg > 0);
  }
  return result;
}

static
uint runPMM9(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[512] = pmm->GoTo;
  uint (*Output)[512] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 9);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
uint runPMM10(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[1024] = pmm->GoTo;
  uint (*Output)[1024] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 10);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
uint runPMM11(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[2048] = pmm->GoTo;
  uint (*Output)[2048] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 11);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
uint runPMM12(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[4096] = pmm->GoTo;
  uint (*Output)[4096] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 12);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
uint runPMM13(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[8192] = pmm->GoTo;
  uint (*Output)[8192] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 13);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
uint runPMM14(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[16384] = pmm->GoTo;
  uint (*Output)[16384] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 14);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
uint runPMM15(PMM *pmm, BITIN *bitin)
{
  CODE c;
  uint i;
  uint slen = pmm->slen;
  STATE (*GoTo)[32768] = pmm->GoTo;
  uint (*Output)[32768] = pmm->Output;
  uint result = 0;
  STATE q, qt;

  q = HEAD_PCHAR; i = 0;
  while (i < slen) {
    c = readBits(bitin, 15);
    if ((qt = GoTo[q][c]) < 0) {
      result += Output[q][c];
      qt = ~qt;
    }
    i++; q = qt;
  }
  return result;
}

static
PMM *mkPMM(uchar *pat, uint plen, uint code_len, DICT *dict)
{
  uint char_size = dict->char_size;
  RULE **d = dict->rule;
  uint num_code = pow(2, code_len);
  uint *num_rules = dict->num_rules;
  uint num_state = char_size+plen-1;
  STATE (*GoTo)[num_code];
  uint (*Output)[num_code];
  STATE *failure;
  PMM *pmm;

  failure = (STATE*)malloc(sizeof(STATE)*(plen+1));
  GoTo = calloc(num_state*num_code, sizeof(STATE));
  Output = calloc(num_state*num_code, sizeof(uint));

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

  pmm = (PMM*)malloc(sizeof(PMM));
  pmm->num_state = num_state;
  pmm->num_code = num_code;
  pmm->plen = plen;
  pmm->slen = dict->seq_len;
  pmm->GoTo = GoTo;
  pmm->Output = Output;

  free(failure);

  return pmm;
}

static
void destructPMM(PMM *pmm)
{
  free(pmm->GoTo);
  free(pmm->Output);
  free(pmm);
}

static
bool encodePattern(uchar *pat, uint plen, DICT *dict)
{
  uint i;
  short c;

  for (i = 0; i < plen; i++) {
    if ((c = dict->char_table[pat[i]]) == -1) {
      return false; //including not existed character.
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
void readRule16(DICT *dict, FILE *input)
{
  uint i, j;
  const uint num_code = 65536;
  RULE **rule = dict->rule;
  uint *num_rules = dict->num_rules;
  uint char_size = dict->char_size;
  ushort *buf = (ushort*)malloc(sizeof(ushort)*2*num_code);
  ushort *buftop = buf;

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      rule[i][j].left = j;
      rule[i][j].right = 0;
    }
    fread(buf, sizeof(ushort), 2*(num_rules[i]-char_size), input);
    for (j = char_size; j < num_rules[i]; j++) {
      rule[i][j].left  = *buf++;
      rule[i][j].right = *buf++;
    }
    buf = buftop;
  }
  free(buftop);
}

static
void readRuleVar(DICT *dict, uint code_len, BITIN *bitin)
{
  uint i, j;
  RULE **rule = dict->rule;
  uint *num_rules = dict->num_rules;
  uint char_size = dict->char_size;

  for (i = 0; i < char_size; i++) {
    for (j = 0; j < char_size; j++) {
      rule[i][j].left = j;
      rule[i][j].right = 0;
    }
    for (j = char_size; j < num_rules[i]; j++) {
      rule[i][j].left  = readBits(bitin, code_len);
      rule[i][j].right = readBits(bitin, code_len);
    }
  }
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

  dict->char_table = (short*)calloc(256, sizeof(short));
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

static
uint search8(uchar *pat, FILE *input)
{
  DICT *dict;
  PMM *pmm;
  uint plen = strlen((char*)pat);
  uint ret;

  dict = mkDict(input);
  readRule8(dict, input);
  if (encodePattern(pat, plen, dict) == false) {
    destructDict(dict);
    return 0;
  }
  pmm = mkPMM(pat, plen, 8, dict);
#ifdef DISPLAY
  printTableSize(pmm);
#endif
  destructDict(dict);
  ret = runPMM8(pmm, input);
  destructPMM(pmm);

  return ret;
}

static
uint search16(uchar *pat, FILE *input)
{
  DICT *dict;
  PMM *pmm;
  uint plen = strlen((char*)pat);
  uint ret;

  dict = mkDict(input);
  readRule16(dict, input);
  if (encodePattern(pat, plen, dict) == false) {
    destructDict(dict);
    return 0;
  }
  pmm = mkPMM(pat, plen, 16, dict);
#ifdef DISPLAY
  printTableSize(pmm);
#endif
  destructDict(dict);
  ret = runPMM16(pmm, input);
  destructPMM(pmm);

  return ret;
}

static
uint searchVar(uchar *pat, uint code_len, FILE *input)
{
  DICT *dict;
  PMM *pmm;
  uint plen = strlen((char*)pat);
  uint ret;
  BITIN *bitin;

  dict = mkDict(input);
  bitin = createBitin(input);
  if (encodePattern(pat, plen, dict) == false) {
    destructDict(dict);
    return 0;
  }
  readRuleVar(dict, code_len, bitin);
  pmm = mkPMM(pat, plen, code_len, dict);
#ifdef DISPLAY
  printTableSize(pmm);
#endif
  destructDict(dict);
  switch(code_len) {
  case 9:
    ret = runPMM9(pmm, bitin);
    break;
  case 10:
    ret = runPMM10(pmm, bitin);
    break;
  case 11:
    ret = runPMM11(pmm, bitin);
    break;
  case 12:
    ret = runPMM12(pmm, bitin);
    break;
  case 13:
    ret = runPMM13(pmm, bitin);
    break;
  case 14:
    ret = runPMM14(pmm, bitin);
    break;
  case 15:
    ret = runPMM15(pmm, bitin);
    break;
  default:
    fprintf(stderr, "exception error.\n");
    exit(1);
    break;
  }
  destructPMM(pmm);

  return ret;
}

uint Search(uchar *pat, FILE *input)
{
  uint ret;
  uint code_len;
  uint pat_len = strlen((char*)pat);
  uchar *newpat = (uchar*)malloc(sizeof(uchar)*(pat_len+1));
  strcpy((char*)newpat, (char*)pat);

  fread(&code_len, sizeof(uint), 1, input);

  switch(code_len) {
  case 8:
    ret = search8(newpat, input);
    break;
  case 16:
    ret = search16(newpat, input);
    break;
  default:
    ret = searchVar(newpat, code_len, input);
    break;
  }
  free(newpat);
  return ret;
}
