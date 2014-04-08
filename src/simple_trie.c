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

#include "simple_trie.h"

STRIE *createSTrie(uint max_size) {
  uint i;
  STRIE *st = (STRIE*)malloc(sizeof(STRIE));
  st->num_node = 1;
  st->max_size = max_size;
  st->nodes = (TNODE*)malloc(sizeof(TNODE)*max_size);
  for (i = 0; i < max_size; i++) {
    st->nodes[i].ch      = UNDEFINED;
    st->nodes[i].sibling = DUMMY_NODE;
    st->nodes[i].child   = DUMMY_NODE;
    st->nodes[i].id      = DUMMY_ID;
  }
  return st;
}

void destructSTrie(STRIE *st) {
  free(st->nodes);
  free(st);
}

uint lookupSTrie(STRIE *st, KEY *key, uint key_len) {
  uint i = 0;
  uint target = st->nodes[ROOT_NODE].child;
  while (i < key_len) {
    while (target != DUMMY_NODE) {
      if (key[i] == st->nodes[target].ch) {
	if (++i == key_len) return st->nodes[target].id;
	target = st->nodes[target].child;
	break;
      }
      target = st->nodes[target].sibling;
    }
    if (target == DUMMY_NODE) break;
  }
  return DUMMY_ID;
}

uint insertSTrie(STRIE *st, KEY *key, uint key_len, uint id) {
  uint i = 0;
  uint parent = ROOT_NODE;
  uint prev   = DUMMY_NODE;
  uint target = st->nodes[ROOT_NODE].child;
  while (target != DUMMY_NODE) {
    if (key[i] == st->nodes[target].ch) {
      if (++i == key_len) return st->nodes[target].id;
      parent = target;
      target = st->nodes[target].child;
      prev = DUMMY_NODE;
    }
    else {
      prev = target;
      target = st->nodes[target].sibling;
    }
  }
  while (i < key_len) {
    if (st->num_node >= st->max_size) {
      puts("Number of node in simple_trie is overflow.");
      exit(1);
    }
    target = st->num_node++;
    st->nodes[target].ch = key[i++];
    if (prev != DUMMY_NODE) {
      st->nodes[prev].sibling = target;
      prev = DUMMY_NODE;
    }
    else {
      st->nodes[parent].child = target;
    }
    parent = target;
  }
  st->nodes[target].id = id;
  return id;
}

void writeSTrie(STRIE *st, FILE *output) {
  fwrite(&st->num_node, sizeof(uint), 1, output);
  fwrite(&st->max_size, sizeof(uint), 1, output);
  fwrite(st->nodes, sizeof(TNODE), st->num_node, output);
}

STRIE *readSTrie(FILE *input) {
  uint i;
  STRIE *st = (STRIE*)malloc(sizeof(STRIE));
  fread(&st->num_node, sizeof(uint), 1, input);
  fread(&st->max_size, sizeof(uint), 1, input);
  st->nodes = (TNODE*)malloc(sizeof(TNODE)*st->max_size);
  fread(st->nodes, sizeof(TNODE), st->num_node, input);
  for (i = st->num_node; i < st->max_size; i++) {
    st->nodes[i].ch      = UNDEFINED;
    st->nodes[i].sibling = DUMMY_NODE;
    st->nodes[i].child   = DUMMY_NODE;
    st->nodes[i].id      = DUMMY_ID;
  }
  return st;
}

#ifdef _TEST_SIMPLE_TRIE
int main(int argc, char *argv[])
{
  FILE *input, *output;
  STRIE *st;
  uint id;
  
  st = createSTrie(100);
  id = insertSTrie(st, "abaaba", 6, 0);
  if (id == 0) printf("success!\n");
  else         printf("fail!\n");
  id = insertSTrie(st, "abaaba", 6, 5);
  if (id == 5) printf("success!\n");
  else         printf("fail! id = %d\n", id);
  id = insertSTrie(st, "abaabc", 6, 7);
  if (id == 7) printf("success!\n");
  else         printf("fail id = %d!\n", id);
  id = insertSTrie(st, "abcba", 5, 1);
  if (id == 1) printf("success!\n");
  else         printf("fail!\n");
  id = insertSTrie(st, "bacdef", 6, 2);
  if (id == 2) printf("success!\n");
  else         printf("fail!\n");
  id = insertSTrie(st, "abcdef", 6, 3);
  if (id == 3) printf("success!\n");
  else         printf("fail!\n");

  printf("id for %s = %d\n", "abaabc", lookupSTrie(st, "abaabc", 6));
  printf("id for %s = %d\n", "abaaba", lookupSTrie(st, "abaaba", 6));
  printf("id for %s = %d\n", "abcba",  lookupSTrie(st, "abcba",  5));
  printf("id for %s = %d\n", "bacdef", lookupSTrie(st, "bacdef", 6));
  printf("id for %s = %d\n", "abcdef", lookupSTrie(st, "abcdef", 6));
  printf("id for %s = %d\n", "abcbac", lookupSTrie(st, "abcbac", 6));

  output = fopen("simple_trie.data", "wb");
  if (output == NULL) {
    puts("File open error at the beginning.");
    exit(1);
  }

  writeSTrie(st, output);
  destructSTrie(st);

  input = fopen("simple_trie.data", "rb");
  if (input == NULL) {
    puts("File open error at the beginning.");
    exit(1);
  }
  st = readSTrie(input);

  printf("id for %s = %d\n", "abaaba", lookupSTrie(st, "abaaba", 6));
  printf("id for %s = %d\n", "abcba",  lookupSTrie(st, "abcba",  5));
  printf("id for %s = %d\n", "bacdef", lookupSTrie(st, "bacdef", 6));
  printf("id for %s = %d\n", "abcdef", lookupSTrie(st, "abcdef", 6));
  printf("id for %s = %d\n", "abcbac", lookupSTrie(st, "abcbac", 6));
  
  fclose(input); fclose(output);
  remove("simple_trie.data");
  exit(0);
}
#endif /* _TEST_SIMPLE_TRIE */
