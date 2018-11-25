/*
 * Copyright (c) 2009 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */

#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>

#define UINT32_T uint32_t
#define UINT64_T uint64_t

#define HRT_CALIBRATE(freq) do {  \
  static volatile HRT_TIMESTAMP_T t1, t2; \
  static volatile UINT64_T elapsed_ticks, min = (UINT64_T)(~0x1); \
  int notsmaller=0; \
  while(notsmaller<3) { \
    HRT_GET_TIMESTAMP(t1); \
    sleep(1); \
    HRT_GET_TIMESTAMP(t2); \
    HRT_GET_ELAPSED_TICKS(t1, t2, &elapsed_ticks); \
    \
    notsmaller++; \
    if(elapsed_ticks<min) { \
      min = elapsed_ticks; \
      notsmaller = 0; \
    } \
  } \
  freq = min; \
} while(0);


#define HRT_INIT(print, freq) do {\
  if(print) printf("# initializing x86-64 timer (takes some seconds)\n"); \
  HRT_CALIBRATE(freq); \
} while(0) 


typedef struct {
	UINT32_T l;
	UINT32_T h;
} x86_64_timeval_t;

#define HRT_TIMESTAMP_T x86_64_timeval_t

/* TODO: Do we need a while loop here? aka Is rdtsc atomic? - check in the documentation */	
#define HRT_GET_TIMESTAMP(t1)   \
  __asm__ __volatile__ (        \
  "xorl %%eax, %%eax \n\t"      \
  "cpuid             \n\t"      \
  "rdtsc             \n\t"      \
  : "=a" (t1.l), "=d" (t1.h)    \
  :                             \
  : "%ebx", "%ecx"              \
  );

#define HRT_GET_ELAPSED_TICKS(t1, t2, numptr)   \
  *numptr = (((( UINT64_T ) t2.h) << 32) | t2.l) - (((( UINT64_T ) t1.h) << 32) | t1.l);

#define HRT_GET_TIME(t1, time) time = (((( UINT64_T ) t1.h) << 32) | t1.l)

/* global timer frequency in Hz */
unsigned long long g_timerfreq;

//static double HRT_GET_USEC(unsigned long long ticks) {
//  return 1e6*(double)ticks/(double)g_timerfreq;
//}

static double HRT_GET_MSEC(unsigned long long ticks) {
  return 1e3*(double)ticks/(double)g_timerfreq;
}
