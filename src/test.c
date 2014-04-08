#include <stdio.h>
#include <stdlib.h>
#include "cont.h"
#include "basics.h"

int main()
{
  uint i, j;
  uchar str1[3] = {4, 3, 8};
  uchar str2[2] = {3, 8};
  uchar str3[1] = {8};
  uchar strtmp[3];

  i = getContextID(256, 3, str1);
  printf("ID = %d\n", i);
  getContext(256, 3, i, strtmp);
  for (j = 0; j < 3; j++) {
    printf("%d, ", strtmp[j]);
  }
  printf("\n");
  
  /*
  printf("begID = %d\n", getContextBegID(10, 3, str2, 2));
  printf("endID = %d\n", getContextEndID(10, 3, str2, 2));
  printf("begID = %d\n", getContextBegID(10, 3, str3, 1));
  printf("endID = %d\n", getContextEndID(10, 3, str3, 1));
  printf("begID = %d\n", getContextBegID(10, 3, str1, 3));
  printf("endID = %d\n", getContextEndID(10, 3, str1, 3));
  printf("RangeSize = %d\n", getContextRangeSize(10, 3, 3));
  printf("RangeSize = %d\n", getContextRangeSize(10, 3, 2));
  printf("RangeSize = %d\n", getContextRangeSize(10, 3, 1));
  */
}
