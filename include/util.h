/*
 This file is part of JustGarble.

    JustGarble is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JustGarble is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JustGarble.  If not, see <http://www.gnu.org/licenses/>.

*/

/////////////////////////////////////////////////////////////////////////////
// The Software is provided "AS IS" and possibly with faults. 
// Intel disclaims any and all warranties and guarantees, express, implied or
// otherwise, arising, with respect to the software delivered hereunder,
// including but not limited to the warranty of merchantability, the warranty
// of fitness for a particular purpose, and any warranty of non-infringement
// of the intellectual property rights of any third party.
// Intel neither assumes nor authorizes any person to assume for it any other
// liability. Customer will use the software at its own risk. Intel will not
// be liable to customer for any direct or indirect damages incurred in using
// the software. In no event will Intel be liable for loss of profits, loss of
// use, loss of data, business interruption, nor for punitive, incidental,
// consequential, or special damages of any kind, even if advised of
// the possibility of such damages.
//
// Copyright (c) 2003 Intel Corporation
//
// Third-party brands and names are the property of their respective owners
//
///////////////////////////////////////////////////////////////////////////
// Random Number Generation for SSE / SSE2
// Source File
// Version 0.1
// Author Kipp Owens, Rajiv Parikh
////////////////////////////////////////////////////////////////////////

#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"
#include "aes.h"
#include <emmintrin.h>
#include <wmmintrin.h>
#include <xmmintrin.h>

int countToN(int *a, int N);
int dbgBlock(block a);
#define RDTSC                                                           \
    ({                                                                  \
        unsigned long long res;                                         \
        unsigned hi, lo;                                                \
        __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));            \
        res = ((unsigned long long) lo) | (((unsigned long long) hi) << 32); \
        res;                                                            \
    })
int getWords(char *line, char *words[], int maxwords);
block randomBlock();
void randAESBlock(block* out);
int median(int A[], int n);
double doubleMean(double A[], int n);

// Compute AES in place. out is a block and sched is a pointer to an 
// expanded AES key.
#define inPlaceAES(out, sched)                                          \
    {                                                                   \
        int jx;                                                         \
        out = _mm_xor_si128(out, sched[0]);                             \
        for (jx = 1; jx < 10; jx++)                                     \
            out = _mm_aesenc_si128(out, sched[jx]);                     \
        out = _mm_aesenclast_si128(out, sched[jx]);                     \
    }

block __current_rand_index;
AES_KEY __rand_aes_key;

#define getRandContext() ((__m128i *) (__rand_aes_key.rd_key));
#define randAESBlock(out,sched)                                         \
    {                                                                   \
        __current_rand_index++;                                         \
        *out = __current_rand_index;                                    \
        inPlaceAES(*out, sched);                                        \
    }

block *
allocate_blocks(size_t nblocks);

#endif /* UTIL_H_ */
