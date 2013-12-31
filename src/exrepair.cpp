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


#include <assert.h>
#include "exrepair.h" 

//#define DEBUG
#define DISPLAY
#define INLINE __inline

#define THRESHOLD 3
#define INIT_DICTIONARY_SIZE (256*1024)
#define DICTIONARY_SCALING_FACTOR (1.25)
#define INIT_HASH_NUM 12
#define LOAD_FACTOR 1.0

static const uint primes[] = {
  /* 0*/  8 + 3,
  /* 1*/  16 + 3,
  /* 2*/  32 + 5,
  /* 3*/  64 + 3,
  /* 4*/  128 + 3,
  /* 5*/  256 + 27,
  /* 6*/  512 + 9,
  /* 7*/  1024 + 9,
  /* 8*/  2048 + 5,
  /* 9*/  4096 + 3,
  /*10*/  8192 + 27,
  /*11*/  16384 + 43,
  /*12*/  32768 + 3,
  /*13*/  65536 + 45,
  /*14*/  131072 + 29,
  /*15*/  262144 + 3,
  /*16*/  524288 + 21,
  /*17*/  1048576 + 7,
  /*18*/  2097152 + 17,
  /*19*/  4194304 + 15,
  /*20*/  8388608 + 9,
  /*21*/  16777216 + 43,
  /*22*/  33554432 + 35,
  /*23*/  67108864 + 15,
  /*24*/  134217728 + 29,
  /*25*/  268435456 + 3,
  /*26*/  536870912 + 11,
  /*27*/  1073741824 + 85,
	  0
};

typedef struct Sequence_block {
  CODE  code;   //code in block
  PCODE pchar;  //previous character
  uint  next;   //position of next occurrence
  uint  prev;   //position of previous occurrence
} SEQ;

typedef struct Pair_info {
  CODE  left;    //left code
  CODE  right;   //right code
  PCODE pchar;   //previous character
  uint  freq;    //#occurences
  uint  f_pos;   //position of left-most occurrence
  uint  b_pos;   //position of (current) right-most occurence
  struct Pair_info *h_next; //for hash chain
  struct Pair_info *p_next; //for priority queue
  struct Pair_info *p_prev; //for priority queue
} PAIR;

typedef struct Priority_queue {
  uint num_pairs; //#recorded pairs
  uint h_num;     //primes[h_num] represents len of h_entry
  PAIR **h_entry; //for hash entry
  uint p_max;     //length of p_head (square root of text length)
  uint mp_pos;    //current position of maximum non-empty queue
  PAIR **p_head;  //heads of priority queue
} PQUE;

typedef struct CodeRePair_data_structures {
  uint txt_len;     //length of text
  uint char_size;   //size of initial alphabet
  CODE *char_table; //maps original chara to encoded chara.
  SEQ  *seq;        //sequence
  PQUE **p_que;     //pointers for priority queues.
} CRDS;

static CRDS *createCRDS      (FILE *input);
static void destructCRDS     (CRDS *crds);
static PAIR *locatePair      (CRDS *crds, PCODE pchar, CODE left, CODE right);
static void rehash           (CRDS *crds, PCODE pchar);
static void insertPair       (CRDS *crds, PAIR *target);
static void removePair       (CRDS *crds, PAIR *target);
static void incrementPair    (CRDS *crds, PAIR *target);
static void decrementPair    (CRDS *crds, PAIR *target);
static PAIR *createPair      (CRDS *crds, PCODE pchar, 
			      CODE left, CODE right, uint f_pos);
static void destructPair     (CRDS *crds, PAIR *target);
static void deletePQ         (CRDS *crds, uint p_pos, PCODE pchar);
static void initCRDS         (CRDS *crds);
static PAIR *getMaxPair      (CRDS *crds, PCODE pchar);
static uint leftPos          (CRDS *crds, uint pos);
static uint rightPos         (CRDS *crds, uint pos);
static void removeLink       (CRDS *crds, uint target_pos);
static void updateBlock      (CRDS *crds, CODE new_code, uint target_pos);
static uint replacePairs     (CRDS *crds, PAIR *max_pair, CODE new_code);
static void copyCompSeq      (CRDS *crds, DICT *dict);
static DICT *createDict      (CRDS *crds, uint code_len);

static int comparePair       (const void *a, const void *b); //for qsort
static double calCompRatio   (uint txt_len, uint char_size, uint seq_len, 
			      uint op_num_rules, uint code_len, bool print);
static CODE addNewPair       (DICT *dict, CODE new_code, PAIR *max_pair);
static void outputCompTxt8   (DICT *dict, FILE *output);
static void outputCompTxt16  (DICT *dict, FILE *output);
static void outputCompTxtVar (DICT *dict, FILE *output);

#if true
#define hash_val(N, A, B) ((A)*(B))%(primes[(N)])
#else
static INLINE
uint hash_val(uint h_num, CODE left, CODE right) {
  return (left * right) % primes[h_num];
}
#endif

static INLINE
PAIR *locatePair(CRDS *crds, PCODE pchar, CODE left, CODE right) {
  PQUE *p_que = crds->p_que[pchar];
  uint h  = hash_val(p_que->h_num, left, right);
  PAIR *p = p_que->h_entry[h];

  while (p != NULL) {
    if (p->left  == left && 
	p->right == right) {
      return p;
    }
    p = p->h_next;
  }
  return NULL;
}

static
void rehash(CRDS *crds, PCODE pchar)
{
  PAIR *p, *q;
  PQUE *p_que = crds->p_que[pchar];
  uint i, h_num, h;
  
  h_num = ++(p_que->h_num);
  p_que->h_entry = 
    (PAIR **)realloc(p_que->h_entry, sizeof(PAIR *) * primes[h_num]);
  for (i = 0; i < primes[h_num]; i++) {
    p_que->h_entry[i] = NULL;
  }
  for (i = 1; ; i++) {
    if (i == p_que->p_max) i = 0;
    p = p_que->p_head[i];
    while (p != NULL) {
      p->h_next = NULL;
      h = hash_val(h_num, p->left, p->right);
      q = p_que->h_entry[h];
      p_que->h_entry[h] = p;
      p->h_next = q;
      p = p->p_next;
    }
    if (i == 0) break;
  }
}

static INLINE
void insertPair(CRDS *crds, PAIR *target)
{
  PAIR *tmp;
  PCODE pchar = target->pchar;
  uint p_num  = target->freq;
  PQUE *p_que = crds->p_que[pchar];

  if (p_num >= p_que->p_max) {
    p_num = 0;
  }

  tmp = p_que->p_head[p_num];
  p_que->p_head[p_num] = target;
  target->p_prev = NULL;
  target->p_next = tmp;
  if (tmp != NULL) {
    tmp->p_prev = target;
  }
}

static INLINE
void removePair(CRDS *crds, PAIR *target)
{
  CODE pchar  = target->pchar;
  PQUE *p_que = crds->p_que[pchar];
  uint p_num  = target->freq;

  if (p_num >= p_que->p_max) {
    p_num = 0;
  }
  
  if (target->p_prev == NULL) {
    p_que->p_head[p_num] = target->p_next;
    if (target->p_next != NULL) {
      target->p_next->p_prev = NULL;
    }
  }
  else {
    target->p_prev->p_next = target->p_next;
    if (target->p_next != NULL) {
      target->p_next->p_prev = target->p_prev;
    }
  }
}

static INLINE
void destructPair(CRDS *crds, PAIR *target)
{
  PQUE *p_que = crds->p_que[target->pchar];
  uint  h = hash_val(p_que->h_num, target->left, target->right);
  PAIR *p = p_que->h_entry[h];
  PAIR *q = NULL;

  removePair(crds, target);

  while (p != NULL) {
    if (p->pchar == target->pchar && 
	p->left  == target->left  && 
	p->right == target->right) {
      break;
    }
    q = p;
    p = p->h_next;
  }

  assert(p != NULL);

  if (q == NULL) {
    p_que->h_entry[h] = p->h_next;
  }
  else {
    q->h_next = p->h_next;
  }
  free(target);
  p_que->num_pairs--;
}

static INLINE
void incrementPair(CRDS *crds, PAIR *target)
{
  PQUE *p_que = crds->p_que[target->pchar];
  if (target->freq >= p_que->p_max) {
    target->freq++;
    return;
  }
  removePair(crds, target);
  target->freq++;
  insertPair(crds, target);
}

static INLINE
void decrementPair(CRDS *crds, PAIR *target)
{
  PQUE *p_que = crds->p_que[target->pchar];

  if (target->freq > p_que->p_max) {
    target->freq--;
    return;
  }
  
  if (target->freq == 1) {
    //destructPair(crds, target);
  }
  else {
    removePair(crds, target);
    target->freq--;
    insertPair(crds, target);
  }
}

static INLINE
PAIR *createPair(CRDS *crds, PCODE pchar, CODE left, CODE right, uint f_pos)
{
  PAIR *pair = (PAIR *)malloc(sizeof(PAIR));
  uint h;
  PAIR *q;
  PQUE *p_que = crds->p_que[pchar];

  pair->pchar  = pchar;
  pair->left   = left;
  pair->right  = right;
  pair->freq   = 1;
  pair->f_pos  = pair->b_pos  = f_pos;
  pair->p_prev = pair->p_next = NULL;

  p_que->num_pairs++;

  if (p_que->num_pairs >= primes[p_que->h_num]) {
    rehash(crds, pchar);
  }

  h = hash_val(p_que->h_num, left, right);
  q = p_que->h_entry[h];
  p_que->h_entry[h] = pair;
  pair->h_next = q;

  insertPair(crds, pair);

  return pair;
}

static
void deletePQ(CRDS *crds, uint p_pos, PCODE pchar)
{
  PQUE *p_que = crds->p_que[pchar];
  PAIR *pair  = p_que->p_head[p_pos];
  PAIR *q;

  p_que->p_head[p_pos] = NULL;
  while (pair != NULL) {
    q = pair->p_next;
    destructPair(crds, pair);
    pair = q;
  }
}

static
void initCRDS(CRDS *crds)
{
  uint i;
  SEQ *seq = crds->seq;
  uint size_w = crds->txt_len;
  PCODE P;
  CODE A, B;
  PAIR *pair;

  for (i = 0; i < size_w - 1; i++) {
    P = seq[i].pchar;
    A = seq[i].code;
    B = seq[i+1].code;
    if ((pair = locatePair(crds, P, A, B)) == NULL) {
      pair = createPair(crds, P, A, B, i);
    }
    else {
      seq[i].prev = pair->b_pos;
      seq[i].next = DUMMY_POS;
      seq[pair->b_pos].next = i;
      pair->b_pos = i;
      incrementPair(crds, pair);
    }
  }
  for (i = 0; i < crds->char_size; i++) {
    deletePQ(crds, 1, i);
  }
}

static 
CRDS *createCRDS(FILE *input)
{
  uint size_w;
  uint i, j;
  SEQ  *seq;
  uint char_size;
  bool check_table[MAX_CHAR_SIZE];
  CODE *char_table;
  PQUE **p_que;
  uint p_max;
  CRDS *crds;

  fseek(input,0,SEEK_END);
  size_w = ftell(input);
  rewind(input);
  seq = (SEQ *)malloc(sizeof(SEQ)*size_w);
  char_table = (CODE *)malloc(sizeof(CODE) * MAX_CHAR_SIZE);

  for (i = 0; i < MAX_CHAR_SIZE; i++) {
    check_table[i] = false;
    char_table[i] = DUMMY_CODE;
  }

  char_size = 0;
  {
    CODE c;
    uint i = 0;
    while ((c = getc(input)) != EOF) {
      seq[i].code = c;
      seq[i].next = DUMMY_POS;
      seq[i].prev = DUMMY_POS;
      if (check_table[c] == false) {
	check_table[c] = true;
	char_size++;
      }
      i++;
    }
  }

  for (i = 0, j = 0; i < MAX_CHAR_SIZE; i++) {
    if (check_table[i] == true) {
      char_table[i] = (CODE)j++;
    }
  }

  assert(char_size == j);

  {
    PCODE pchar = HEAD_PCHAR;
    uint i = 0; 
    while (i < size_w) {
      seq[i].pchar = pchar;
      seq[i].code = pchar = char_table[seq[i].code];
      i++;
    }
  }

  p_max = (uint)ceil(sqrt((double)size_w));

  p_que = (PQUE **)malloc(sizeof(PQUE *)*char_size);
  for (i = 0; i < char_size; i++) {
    p_que[i] = (PQUE *)malloc(sizeof(PQUE));
    p_que[i]->h_entry = 
      (PAIR **)malloc(sizeof(PAIR *) * primes[INIT_HASH_NUM]);
    for (j = 0; j < primes[INIT_HASH_NUM]; j++) {
      p_que[i]->h_entry[j] = NULL;
    }
    p_que[i]->p_head = 
      (PAIR **)malloc(sizeof(PAIR *) * p_max);
    for (j = 0; j < p_max; j++) {
      p_que[i]->p_head[j] = NULL;
    }
    p_que[i]->h_num = INIT_HASH_NUM;
    p_que[i]->mp_pos = 0;
    p_que[i]->p_max = p_max;
    p_que[i]->num_pairs = 0;
  }

  crds = (CRDS *)malloc(sizeof(CRDS));
  crds->txt_len    = size_w;
  crds->char_size  = char_size;
  crds->char_table = char_table;
  crds->seq = seq;
  crds->p_que = p_que;

  initCRDS(crds);

  return crds;
}

static
void destructCRDS(CRDS *crds)
{
  uint i;

  free(crds->seq);
  free(crds->char_table);
  for (i = 0; i < crds->char_size; i++) {
    free(crds->p_que[i]->h_entry);
    free(crds->p_que[i]->p_head);
    free(crds->p_que[i]);
  }
  free(crds->p_que);
  free(crds);
}

static
PAIR *getMaxPair(CRDS *crds, PCODE pchar)
{
  PQUE *p_que = crds->p_que[pchar];
  uint i = p_que->mp_pos;
  PAIR *p;
  PAIR *max_pair;
  uint max;

  if (p_que->p_head[0] != NULL) {
    p = p_que->p_head[0];
    max = 0; max_pair = NULL;
    while (p != NULL) {
      if (max < p->freq) {
	max = p->freq;
	max_pair = p;
      }
      p = p->p_next;
    }
  }
  else {
    max_pair = NULL;
    if (i == 0) i = p_que->p_max-1;
    while (i >= THRESHOLD) {
      if (p_que->p_head[i] != NULL) {
	max_pair = p_que->p_head[i];
	break;
      }
      i--;
    }
  }
  p_que->mp_pos = i;
  return max_pair;
}

static INLINE
uint leftPos(CRDS *crds, uint pos)
{
  SEQ *seq = crds->seq;

  assert(pos != DUMMY_POS);
  if (pos == 0) {
    return DUMMY_POS;
  }

  if (seq[pos-1].code == DUMMY_CODE) {
    return seq[pos-1].next;
  }
  else {
    return pos-1;
  }
}

static INLINE
uint rightPos(CRDS *crds, uint pos)
{
  SEQ *seq = crds->seq;

  assert(pos != DUMMY_POS);
  if (pos == crds->txt_len-1) {
    return DUMMY_POS;
  }

  if (seq[pos+1].code == DUMMY_CODE) {
    return seq[pos+1].prev;
  }
  else {
    return pos+1;
  }
}

static INLINE
void removeLink(CRDS *crds, uint target_pos)
{
  SEQ *seq = crds->seq;
  uint prev_pos, next_pos;

  assert(seq[target_pos].code != DUMMY_CODE);

  prev_pos = seq[target_pos].prev;
  next_pos = seq[target_pos].next;

  if (prev_pos != DUMMY_POS && next_pos != DUMMY_POS) {
    seq[prev_pos].next = next_pos;
    seq[next_pos].prev = prev_pos;
  }
  else if (prev_pos == DUMMY_POS && next_pos != DUMMY_POS) {
    seq[next_pos].prev = DUMMY_POS;
  }
  else if (prev_pos != DUMMY_POS && next_pos == DUMMY_POS) {
    seq[prev_pos].next = DUMMY_POS;
  }
}

static
void updateBlock(CRDS *crds, CODE new_code, uint target_pos)
{
  SEQ *seq = crds->seq;
  uint l_pos, r_pos, rr_pos, nx_pos;
  CODE c_code, r_code, l_code, rr_code;
  PCODE c_pchar, r_pchar, l_pchar;
  PAIR *l_pair, *c_pair, *r_pair;

  l_pos   = leftPos(crds, target_pos);
  r_pos   = rightPos(crds, target_pos);
  rr_pos  = rightPos(crds, r_pos);
  c_code  = seq[target_pos].code;
  c_pchar = seq[target_pos].pchar;
  r_code  = seq[r_pos].code;
  r_pchar = seq[r_pos].pchar;

  nx_pos = seq[target_pos].next;
  if (nx_pos == r_pos) {
    nx_pos = seq[nx_pos].next;
  }

  assert(c_code != DUMMY_CODE);
  assert(r_code != DUMMY_CODE);

  if (l_pos != DUMMY_POS) {
    l_code = seq[l_pos].code;
    l_pchar = seq[l_pos].pchar;
    assert(seq[l_pos].code != DUMMY_CODE);
    removeLink(crds, l_pos);
    if ((l_pair = locatePair(crds, l_pchar, l_code, c_code)) != NULL) {
      if (l_pair->f_pos == l_pos) {
	l_pair->f_pos = seq[l_pos].next;
      }
      decrementPair(crds, l_pair);
    }
    if ((l_pair = locatePair(crds, l_pchar, l_code, new_code)) == NULL) {
      seq[l_pos].prev = DUMMY_POS;
      seq[l_pos].next = DUMMY_POS;
      createPair(crds, l_pchar, l_code, new_code, l_pos);
    }
    else {
      seq[l_pos].prev = l_pair->b_pos;
      seq[l_pos].next = DUMMY_POS;
      seq[l_pair->b_pos].next = l_pos;
      l_pair->b_pos = l_pos;
      incrementPair(crds, l_pair);
    }
  }

  removeLink(crds, target_pos);
  removeLink(crds, r_pos);
  seq[target_pos].code = new_code;
  seq[r_pos].code = DUMMY_CODE;
  
  if (rr_pos != DUMMY_POS) {
    rr_code = seq[rr_pos].code;
    assert(rr_code != DUMMY_CODE);
    if ((r_pair = locatePair(crds, r_pchar, r_code, rr_code)) != NULL) {
      if (r_pair->f_pos == r_pos) {
	r_pair->f_pos = seq[r_pos].next;
      }
      decrementPair(crds, r_pair);
    }

    if (target_pos + 1 == rr_pos - 1) {
      seq[target_pos+1].prev = rr_pos;
      seq[target_pos+1].next = target_pos;
    }
    else {
      seq[target_pos+1].prev = rr_pos;
      seq[target_pos+1].next = DUMMY_POS;
      seq[rr_pos-1].prev = DUMMY_POS;
      seq[rr_pos-1].next = target_pos;
    }
    /*
    if (seq[target_pos+2].code == DUMMY_CODE) {
      if( target_pos+2 < rr_pos-1) {
	seq[target_pos+2].prev = seq[target_pos+2].next = DUMMY_POS;
      }
    }
    if (seq[rr_pos-2].code == DUMMY_CODE) { 
      if (rr_pos-2 > target_pos+1) {
	seq[rr_pos-2].prev = seq[rr_pos-2].next = DUMMY_POS;
      }
    }
    */
    if (nx_pos > rr_pos) {
      if ((c_pair = locatePair(crds, c_pchar, new_code, rr_code)) == NULL) {
	seq[target_pos].prev = seq[target_pos].next = DUMMY_POS;
	createPair(crds, c_pchar, new_code, rr_code, target_pos);
      }
      else {
	seq[target_pos].prev = c_pair->b_pos;
	seq[target_pos].next = DUMMY_POS;
	seq[c_pair->b_pos].next = target_pos;
	c_pair->b_pos = target_pos;
	incrementPair(crds, c_pair);
      }
    }
    else {
      seq[target_pos].next = seq[target_pos].prev = DUMMY_POS;
    }
  }
  else if (target_pos < crds->txt_len - 1) {
    assert(seq[target_pos+1].code == DUMMY_CODE);
    seq[target_pos+1].prev = DUMMY_POS;
    seq[target_pos+1].next = target_pos;
    seq[r_pos].prev = seq[r_pos].next = DUMMY_POS;
  }
}

static
uint replacePairs(CRDS *crds, PAIR *target, CODE new_code)
{
  uint i, j;
  uint num_replaced = 0;
  SEQ *seq = crds->seq;

  i = target->f_pos;
  while (i != DUMMY_POS) {
    j = seq[i].next;
    if (j == rightPos(crds, i)) {
      j = seq[j].next;
    }
    updateBlock(crds, new_code, i);
    i = j;
    num_replaced++;
  }

  return num_replaced;
}

static
DICT *createDict(CRDS *crds, uint code_len)
{
  uint i, j;
  CODE c;
  CODE *left, *right;
  DICT *dict = (DICT *)malloc(sizeof(DICT));

  dict->txt_len   = crds->txt_len;
  dict->code_len  = code_len;
  dict->char_size = crds->char_size;

  dict->echar_table = 
    (CODE *)malloc(sizeof(CODE)*dict->char_size);
  for (i = 0; i < dict->char_size; i++) 
    dict->echar_table[i] = DUMMY_CODE;
  for (i = 0; i < MAX_CHAR_SIZE; i++) {
    if ((c = crds->char_table[i]) != DUMMY_CODE) {
      dict->echar_table[c] = i;
    }
  }

  dict->rule = 
    (RULE **)malloc(sizeof(RULE *) * dict->char_size);
  for (i = 0; i < dict->char_size; i++) {
    left  = 
      (CODE *)malloc(sizeof(CODE) * INIT_DICTIONARY_SIZE);
    right = 
      (CODE *)malloc(sizeof(CODE) * INIT_DICTIONARY_SIZE);
    for (j = 0; j < dict->char_size; j++) {
      left[j]  = (CODE)j;
      right[j] = DUMMY_CODE;
    }
    for (j = dict->char_size; j < INIT_DICTIONARY_SIZE; j++) {
      left[j] = right[j] = DUMMY_CODE;
    }

    dict->rule[i] = (RULE *)malloc(sizeof(RULE));
    dict->rule[i]->num_rules = dict->char_size;
    dict->rule[i]->left  = left;
    dict->rule[i]->right = right;
    dict->rule[i]->buff_size = INIT_DICTIONARY_SIZE;
  }

  dict->seq_len = 0;
  dict->comp_seq = NULL;

  return dict;
}

static
CODE addNewPair(DICT *dict, CODE new_code, PAIR *max_pair)
{
  CODE pchar = max_pair->pchar;
  RULE *rule = dict->rule[pchar];
  
  rule->left[new_code]  = max_pair->left;
  rule->right[new_code] = max_pair->right;

  rule->num_rules++;
  if (rule->num_rules >= rule->buff_size) {
    rule->buff_size *= DICTIONARY_SCALING_FACTOR;
    rule->left  = 
      (CODE *)realloc(rule->left, sizeof(CODE) * rule->buff_size);
    rule->right = 
      (CODE *)realloc(rule->right, sizeof(CODE) * rule->buff_size);
    if (rule->left == NULL || rule->right == NULL) {
      puts("Memory reallocate error (rule) at addNewPair.");
      exit(1);
    }
  }

  return new_code;
}

static
void copyCompSeq(CRDS *crds, DICT *dict)
{
  uint i, j;
  SEQ *seq = crds->seq;
  uint seq_len;
  CODE *comp_seq;

  i = 0; seq_len = 0;
  while (i < crds->txt_len) {
    if (seq[i].code == DUMMY_CODE) {
      i = seq[i].prev;
      continue;
    }
    seq_len++;
    i++;
  }

  comp_seq = (CODE *)malloc(sizeof(CODE) * seq_len);
  i = j = 0;
  while (i < crds->txt_len) {
    if (seq[i].code == DUMMY_CODE) {
      i = seq[i].prev;
      continue;
    }
    comp_seq[j++] = seq[i].code;
    i++;
  }
  dict->comp_seq = comp_seq;
  dict->seq_len  = seq_len;
}

static
int comparePair(const void* a, const void* b)
{
  PAIR *pair1 = *(PAIR **)a;
  PAIR *pair2 = *(PAIR **)b;
  
  if (pair1 != NULL && pair2 != NULL) {
    return pair2->freq - pair1->freq;
  }
  else if (pair1 == NULL && pair2 != NULL) {
    return 1;
  }
  else if (pair1 != NULL && pair2 == NULL) {
    return -1;
  }
  else {
    return 0;
  }
}

static
void outputCompTxt8(DICT *dict, FILE *output) 
{
  uint i, j;
  RULE *rule;

  for (i = 0; i < dict->char_size; i++) {
    rule = dict->rule[i];
    for (j = dict->char_size; j < rule->num_rules; j++) {
      fwrite(&(rule->left[j]), 1, 1, output);
      fwrite(&(rule->right[j]), 1, 1, output);
    }
  }
  for (i = 0; i < dict->seq_len; i++) {
    fwrite(&(dict->comp_seq[i]), 1, 1, output);
  }
}

static
void outputCompTxt16(DICT *dict, FILE *output) 
{
  uint i, j;
  RULE *rule;

  for (i = 0; i < dict->char_size; i++) {
    rule = dict->rule[i];
    for (j = dict->char_size; j < rule->num_rules; j++) {
      fwrite(&(rule->left[j]),  2, 1, output);
      fwrite(&(rule->right[j]), 2, 1, output);
    }
  }
  for (i = 0; i < dict->seq_len; i++) {
    fwrite(&(dict->comp_seq[i]), 2, 1, output);
  }
}

static
void outputCompTxtVar(DICT *dict, FILE *output) 
{
  uint i, j;
  RULE *rule;
  BITOUT *bitout;

  bitout = createBitout(output);
  for (i = 0; i < dict->char_size; i++) {
    rule = dict->rule[i];
    for (j = dict->char_size; j < rule->num_rules; j++) {
      writeBits(bitout, rule->left[j],  dict->code_len);
      writeBits(bitout, rule->right[j], dict->code_len);
    }
  }
  for (i = 0; i < dict->seq_len; i++) {
    writeBits(bitout, dict->comp_seq[i], dict->code_len);
  }
  flushBitout(bitout);
}

void OutputCompTxt(DICT *dict, FILE *output)
{
  uint i;

  fwrite(&(dict->code_len),  sizeof(uint), 1, output);
  fwrite(&(dict->txt_len),   sizeof(uint), 1, output);
  fwrite(&(dict->char_size), sizeof(uint), 1, output);
  fwrite(&(dict->seq_len),   sizeof(uint), 1, output);
  for (i = 0; i < dict->char_size; i++) {
    fwrite(&(dict->echar_table[i]), sizeof(uchar), 1, output);
  }
  for (i = 0; i < dict->char_size; i++) {
    fwrite(&(dict->rule[i]->num_rules), sizeof(uint), 1, output);
  }

  switch (dict->code_len) {
  case 8:
    outputCompTxt8(dict, output); 
    break;
  case 16:
    outputCompTxt16(dict, output);
    break;
  default:
    outputCompTxtVar(dict, output);
    break;
  }
}

void DestructDict(DICT *dict)
{
  uint i;
  for (i = 0; i < dict->char_size; i++) {
    free(dict->rule[i]->left);
    free(dict->rule[i]->right);
    free(dict->rule[i]);
  }
  free(dict->echar_table);
  free(dict->rule);
  free(dict->comp_seq);
  free(dict);
}

static
double calCompRatio(uint txt_len, uint char_size, uint seq_len, 
		    uint t_num_rules, uint code_len, bool print)
{
  uint total_size;
  uint head_size;
  uint dict_bits, seq_bits;
  uint dict_size, seq_size;
  double comp_ratio;

  total_size = 0;
  head_size  = sizeof(uint)*4+(sizeof(uint)+sizeof(uchar))*char_size;
  dict_size  = 0;
  seq_size   = 0;

  if (code_len == 8) {
    dict_size  = t_num_rules * 2 * sizeof(uchar);
    seq_size   = seq_len * sizeof(uchar);
    total_size = dict_size + seq_size + head_size;
  }
  else if (code_len == 16) {
    dict_size  = t_num_rules * 2 * sizeof(ushort);
    seq_size   = seq_len * sizeof(ushort);
    total_size = dict_size + seq_size + head_size;
  }
  else {
    dict_bits  = t_num_rules * 2 * code_len;
    dict_size  = (dict_bits+LONG_BITS-1) / LONG_BITS * sizeof(ulong);
    seq_bits   = seq_len * code_len;
    seq_size   = (seq_bits + LONG_BITS - 1) / LONG_BITS * sizeof(ulong);
    total_size = (dict_bits + seq_bits + LONG_BITS - 1)
                 / LONG_BITS * sizeof(ulong) + head_size;
  }
  
  comp_ratio = ((double)total_size / (double)txt_len) * 100;

  if (print == true) {
    printf("\n");
    printf("///////////////////////////////////\n");
    printf(" |D| = %d, |S| = %d.\n", t_num_rules, seq_len);
    printf(" Header     = %d (bytes).\n", head_size);
    printf(" Dictionary = %d (bytes).\n", dict_size);
    printf(" Sequence   = %d (bytes).\n", seq_size);
    printf(" Total      = %d (bytes).\n", total_size);
    printf(" Comp.ratio = %0.3f [%%].\n", comp_ratio);
    printf("///////////////////////////////////\n");
    printf("\n");
  }

  return comp_ratio;
}

DICT *RunCodeRepair(FILE *input, uint code_len)
{
  uint i, j;
  CRDS *crds;
  DICT *dict;
  CODE new_code;
  PAIR **mp_ary;
  uint limit = (uint)pow(2, code_len);
  uint num_loop, num_replaced;
  uint t_num_rules, c_seq_len;
  double comp_ratio;

  //initialization
#ifdef DISPLAY
  printf("\n");
  printf("Initializing ...\n");
#endif
  crds = createCRDS(input);
  dict = createDict(crds, code_len);

#ifdef DISPLAY
  printf("///////////////////////////////////////\n");
  printf(" Input text size = %d (bytes).\n", crds->txt_len);
  printf(" Alphabet size   = %d.\n", crds->char_size);
  printf(" Code length     = %d (bits).\n", code_len);
  printf(" # of new_code   = %d.\n", limit - crds->char_size);
  printf("///////////////////////////////////////\n");
  printf("\n");
  printf("Compressing text ...\n");
#endif

  mp_ary = (PAIR**)malloc(sizeof(PAIR*)*(crds->char_size+1));
  num_loop = 0; num_replaced = 0;
  new_code = crds->char_size;
  t_num_rules = 0;
  c_seq_len = crds->txt_len;

  //select replaced pairs
  while (new_code < limit) {
    for (i = 0; i <= crds->char_size; i++) {
      mp_ary[i] = NULL;
    }
    for (i = 0; i < crds->char_size; i++) {
      mp_ary[i] = getMaxPair(crds, i);
    }
    //sort mp_ary by frequencies.
    qsort(mp_ary, crds->char_size+1, sizeof(PAIR*), 
	  (int(*)(const void*, const void*))comparePair);

    //if mp_ary is empty, then break.
    if (mp_ary[0] == NULL) break;

    //replace pairs
    for (i = 0; mp_ary[i] != NULL; i++) {
      addNewPair(dict, new_code, mp_ary[i]);
      c_seq_len -= replacePairs(crds, mp_ary[i], new_code);
      t_num_rules++;
    }

#ifdef DISPLAY
    comp_ratio = calCompRatio(crds->txt_len, crds->char_size, 
			      c_seq_len, t_num_rules, code_len, false);
    printf("\rnew_code = [%5d], Comp.ratio = %0.3f %%.",
	   new_code, comp_ratio);
    fflush(stdout);
#endif

    //free replaced pairs
    for (i = 0; mp_ary[i] != NULL; i++) {
      destructPair(crds, mp_ary[i]);
    }
    //free unused pairs
    for (i = 0; i < crds->char_size; i++) {
      for (j = 1; j < THRESHOLD; j++) {
	deletePQ(crds, j, i);
      }
    }

    new_code++;
  }

#ifdef DISPLAY
  printf("\n");
  calCompRatio(crds->txt_len, crds->char_size, 
	       c_seq_len, t_num_rules, code_len, true);
#endif

  //post processing
  copyCompSeq(crds, dict);
  free(mp_ary);
  destructCRDS(crds);

  return dict;
}
