#include "timer.h"

static
double gettimeofday_sec()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec + (double)tv.tv_usec*1e-6;
}

static
void gettimeofday_op(int mode, int arg) {
  static double t1, t2;

  switch (mode) {
  case 0: /* INIT */
    t1 = 0;
    t2 = 0;
    break;
  case 1: /* START */
    t1 = gettimeofday_sec();
    break;
  case 2: /* STOP */
    t2 = gettimeofday_sec();
    break;
  case 3: /* SHOW */
    printf("Search Time = %.3f [msec]\n", (t2 - t1) * 1000);
    break;
  case 4: /* SHOW AVERAGE */
    printf("Search Time (Ave.) = %.3f [msec] (#iteration = %d)\n", 
	   (t2 - t1)/(double)arg*1000, arg);
    break;
  default:
    break;
  }
}

void init_timer() { gettimeofday_op(0, 0);}
void start_timer(){ gettimeofday_op(1, 0);}
void stop_timer() { gettimeofday_op(2, 0);}
void show_timer() { gettimeofday_op(3, 0);}
void show_average(int i) { gettimeofday_op(4, i);}
