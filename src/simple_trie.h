#ifndef SIMPLE_TRIE_H
#define SIMPLE_TRIE_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "basics.h"

#define ROOT_NODE 0
#define DUMMY_CHAR UINT_MAX
#define DUMMY_NODE UINT_MAX
#define DUMMY_ID   UINT_MAX
#define UNDEFINED 0

typedef unsigned char KEY;

typedef struct simple_trie_node {
  KEY  ch;
  uint sibling;
  uint child;
  uint id;
} TNODE;

typedef struct simple_trie_structure {
  uint num_node;
  uint max_size;
  TNODE *nodes;
} STRIE;

STRIE *createSTrie(uint max_size);
void destructSTrie(STRIE *st);
uint lookupSTrie(STRIE *st, KEY *key, uint key_len);
uint insertSTrie(STRIE *st, KEY *key, uint key_len, uint id);
void writeSTrie(STRIE *st, FILE *output);
STRIE *readSTrie(FILE *input);

#endif /* SIMPLE_TRIE_H */
