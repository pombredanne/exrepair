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


#ifdef CREPAIR
#include "exrepair.h"

int main(int argc, char *argv[])
{
  char *target_filename;
  char output_filename[256];
  FILE *input, *output;
  uint code_len, cont_len, cont_size;
  DICT *dict;

  if (argc != 4 && argc != 5) {
    printf("usage: %s target_text_file [code_length (bits)]\n", argv[0]);
    printf("default code_length: 8 bits\n");
    exit(1);
  }

  target_filename = argv[1];
  strcpy(output_filename, target_filename);

  if (argc == 2) {
    code_len = 8;
    cont_len = 1;
    cont_size = 256;
    strcat(output_filename, ".cr8");
  }
  else {
    code_len = atoi(argv[2]);
    if (code_len < 8 || code_len > 24) {
      printf("range of code length: 8-24 (bits)\n"); 
      exit(1);
    }
    cont_len = atoi(argv[3]);
    if (cont_len < 1 || cont_len > 3) {
      printf("range of context length: 1-3\n");
      exit(1);
    }
    cont_size = atoi(argv[4]);
    if (cont_size < 1 || cont_size > 256) {
      printf("range of context size: 1-256");
      exit(1);
    }
    strcat(output_filename, ".cr");
    strcat(output_filename, argv[2]);
  }
  printf("output_filename = %s\n", output_filename);

  input  = fopen(target_filename, "r");
  output = fopen(output_filename, "wb");
  if (input == NULL || output == NULL) {
    puts("File open error at the beginning.");
    exit(1);
  }

  dict = RunCodeRepair(input, code_len, cont_len, cont_size);
  OutputCompTxt(dict, output);
  DestructDict(dict);

  fclose(input);
  fclose(output);
  exit(0);
}
#endif


#ifdef CDESPAIR
#include "exdespair.h"

int main(int argc, char *argv[])
{
  FILE *input, *output;

  if (argc != 3) {
    printf("usage: %s target_compressed_file output_text_file\n", argv[0]);
    exit(1);
  }
  
  input  = fopen(argv[1], "rb");
  output = fopen(argv[2], "w");
  if (input == NULL || output == NULL) {
    puts("File open error at the beginning.");
    exit(1);
  }

  if (RunCodeDespair(input, output) == false) {
    printf("Error: wrong input file.\n");
  }
  fclose(input);
  fclose(output);
  exit(0);
}
#endif

#ifdef CPM
#include <stdio.h>
#include <string.h>
#include "cpm.h"
#include "timer.h"

#define ITERATION_NUM 1

int main(int argc, char *argv[])
{
  uchar pat[256];
  FILE *input;
  uint i;
  uint occ;
  if(argc != 3) {
    printf("Usage:\n");
    printf("%s <CompressedFile> pattern\n", argv[0]);
    exit(1);
  }

  printf("Pattern length = %d\n", (int)strlen(argv[2]));
  init_timer();
  start_timer();
  for (i = 0; i < ITERATION_NUM; i++) {
    strcpy((char*)pat, (char*)argv[2]);
    if ((input = fopen(argv[1], "rb")) == NULL) {
      puts("File open error!!");
      exit(1);
    }
    occ = Search(pat, input);
    fclose(input);
  }
  stop_timer();
  printf("# of occurrence(s) = %d\n", occ);
  show_average(ITERATION_NUM);
  //show_timer();
  exit(0);
}
#endif


#ifdef CPM8
#include <stdio.h>
#include <string.h>
#include "cpm8.h"
#include "timer.h"

#define ITERATION_NUM 1

int main(int argc, char *argv[])
{
  uchar pat[256];
  FILE *input;
  uint i;
  uint occ;
  if(argc != 3) {
    printf("Usage:\n");
    printf("%s <CompressedFile> pattern\n", argv[0]);
    exit(1);
  }

  printf("Pattern length = %d\n", (int)strlen(argv[2]));
  init_timer();
  start_timer();
  for (i = 0; i < ITERATION_NUM; i++) {
    strcpy((char*)pat, (char*)argv[2]);
    if ((input = fopen(argv[1], "rb")) == NULL) {
      puts("File open error!!");
      exit(1);
    }
    occ = Search8(pat, input);
    fclose(input);
  }
  stop_timer();
  printf("# of occurrence(s) = %d\n", occ);
  show_average(ITERATION_NUM);
  //show_timer();
  exit(0);
}
#endif
