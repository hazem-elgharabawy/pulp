/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * Mantainer: Luca Valente, luca.valente2@unibo.it
 */
#include <stdio.h>
#include "pulp.h"

unsigned int g_errors = 0;

RT_LOCAL_DATA static unsigned int _Bit_Mask[] =
{
        0x1<<0, 0x1<<1, 0x1<<2, 0x1<<3, 0x1<<4, 0x1<<5, 0x1<<6, 0x1<<7,
        0x1<<8, 0x1<<9, 0x1<<10, 0x1<<11, 0x1<<12, 0x1<<13, 0x1<<14, 0x1<<15,
        0x1<<16, 0x1<<17, 0x1<<18, 0x1<<19, 0x1<<20, 0x1<<21, 0x1<<22, 0x1<<23,
        0x1<<24, 0x1<<25, 0x1<<26, 0x1<<27, 0x1<<28, 0x1<<29, 0x1<<30, 0x1<<31,
};

RT_LOCAL_DATA static unsigned int _N_Bit_Mask[] =
{
        ~(0x1<<0), ~(0x1<<1), ~(0x1<<2), ~(0x1<<3), ~(0x1<<4), ~(0x1<<5), ~(0x1<<6), ~(0x1<<7),
        ~(0x1<<8), ~(0x1<<9), ~(0x1<<10), ~(0x1<<11), ~(0x1<<12), ~(0x1<<13), ~(0x1<<14), ~(0x1<<15),
        ~(0x1<<16), ~(0x1<<17), ~(0x1<<18), ~(0x1<<19), ~(0x1<<20), ~(0x1<<21), ~(0x1<<22), ~(0x1<<23),
        ~(0x1<<24), ~(0x1<<25), ~(0x1<<26), ~(0x1<<27), ~(0x1<<28), ~(0x1<<29), ~(0x1<<30), ~(0x1<<31),
};

RT_LOCAL_DATA static unsigned int _Mask[32];

#define BIT_S(v, pos) (v = v | _Bit_Mask[pos])
#define BIT_C(v, pos) (v = v & _N_Bit_Mask[pos])
#define BIT_V(v, pos) ((v>>pos)&0x1)

void Process_Descriptor(unsigned int *Descr, int Size, void action(int index))

{
  int i, j;
  for (i=0; i<Size/(sizeof(unsigned int)*8); i++) {
    unsigned int D = Descr[i];
    for (j= 0; j<32; j++) {
      if (BIT_V(D, j))
        action(32*i+j);
    }
  }
}

#ifndef __riscv__
#ifndef __cv32e40p__
void Process_Descriptor_Bis(unsigned int *Descr, int Size, void action(int index))

{
  int i, j;

  for (i=0; i<Size/(sizeof(unsigned int)*8); i++) {
    unsigned int D = Descr[i];
    int S = 0;
    while (D) {
      int s;
      asm ("l.ff1 %[s], %[D]"
           : [s] "+r" (s)
           : [D] "r"  (D));
      S += s;
      action(32*i+S-1);
      D = D >> s;
    }
  }
}
#endif
#endif


RT_LOCAL_DATA unsigned int Descr8[8] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x0,0x0};
RT_LOCAL_DATA unsigned int Descr7[8] = {0xFFFFFFFF,0xFFFFFFFF,0x0,0x0,0x0,0x0,0x0,0xF0};
RT_LOCAL_DATA unsigned int Descr6[8] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x0,0x0,0x0,0x0};
RT_LOCAL_DATA unsigned int Descr5[8] = {0x0,0x0,0x0,0x0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
RT_LOCAL_DATA unsigned int Descr4[8] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
RT_LOCAL_DATA unsigned int Descr3[8] = {0x1, 0xF0A0, 0x23, 0x345, 0xFFFF, 0, 0x7, 0xA};
RT_LOCAL_DATA unsigned int Descr2[8] = {0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555, 0x55555555};
RT_LOCAL_DATA unsigned int Descr1[8] = {0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA, 0xAAAAAAAA};
RT_LOCAL_DATA unsigned int Descr0[8] = {0,0,0,0,0,0,0,0};
RT_LOCAL_DATA int Count;

void signal(int index)

{
  Count = Count + 1;
}

void Process(testresult_t *result, char *Mess, unsigned int *Descr, int Descr_Size, unsigned int *cTime, unsigned int *asmTime)

{
  int Count_Regular;

  Count = 0;
  printf("\n==========Process %s Starting==============\n", Mess);
  reset_timer();
  start_timer();
#ifdef PROFILE
  perf_start();
#endif
  Process_Descriptor(Descr, Descr_Size, signal);
#ifdef PROFILE
  perf_stop();
#endif
  stop_timer();
  *cTime += get_time();
  printf("Nr. cycles: %d\n", get_time());
  printf("Regular:  %s -> %d actions triggered\n", Mess, Count);
  Count_Regular = Count;

#ifndef __riscv__
#ifndef __cv32e40p__
  Count = 0;
  reset_timer();
  start_timer();
  Process_Descriptor_Bis(Descr, Descr_Size, signal);
  stop_timer();
  *asmTime += get_time();
  printf("Nr. cycles: %d\n", get_time());
  printf("Optimized:%s -> %d actions triggered\n", Mess, Count);

  if(Count != Count_Regular) {
    result->errors += 1;
  }
#endif
#endif
}

void check_bit(testresult_t *result, void (*start)(), void (*stop)());

testcase_t testcases[] = {
  { .name = "bit",          .test = check_bit          },
  {0, 0}
};

int main()
{
  if (get_core_id() == 0) {
    return run_suite(testcases);
  }

  return 0;
}

void check_bit(testresult_t *result, void (*start)(), void (*stop)()) {
  Count = 0;
  if(get_core_id() == 0) {
    int Descr_Size = 256;
    unsigned int cTime = 0, asmTime = 0;
    start();
    perf_stop();
    Process(result, "All 0", Descr0, Descr_Size, &cTime, &asmTime);
    Process(result, "All 1", Descr4, Descr_Size, &cTime, &asmTime);
    Process(result, "Alternate 1/0", Descr1, Descr_Size, &cTime, &asmTime);
    Process(result, "Alternate 0/1", Descr2, Descr_Size, &cTime, &asmTime);
    Process(result, "Half 1 then half 0", Descr6, Descr_Size, &cTime, &asmTime);
    Process(result, "Half 0 then half 1", Descr5, Descr_Size, &cTime, &asmTime);
    Process(result, "25% 1, 75% 0", Descr7, Descr_Size, &cTime, &asmTime);
    Process(result, "75% 1, 25% 0", Descr8, Descr_Size, &cTime, &asmTime);
    Process(result, "Random", Descr3, Descr_Size, &cTime, &asmTime);
    stop();
    printf("C version cycles: %d\n", cTime);
    printf("ASM version cycles: %d\n", asmTime);

    printf("#Errors: %d\n", g_errors);
  }
}
