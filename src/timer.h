#ifndef TIMERINCLUDED
#define TIMERINCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

void init_timer();
void start_timer();
void stop_timer();
void show_timer();
void show_average(int i);

#endif
