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


#ifndef common
#define common 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <wmmintrin.h>

typedef __m128i block;

#define xorBlocks(x,y) _mm_xor_si128(x,y)
#define zero_block() _mm_setzero_si128()
#define unequal_blocks(x,y) (_mm_movemask_epi8(_mm_cmpeq_epi8(x,y)) != 0xffff)

#define dbg(X) printf("DEBUG %s:%d says %d \n",__FILE__, __LINE__, X)
#define dbgx(X) printf("DEBUG %s:%d says %X \n",__FILE__, __LINE__, X)
#define dbgp(X) printf("DEBUG %s:%d says %p \n",__FILE__, __LINE__, X)
#define dbgs(X) printf("DEBUG %s:%d says %s \n",__FILE__, __LINE__, X)
#define dbgb(X) printf("DEBUG %s:%d says %lx %lx\n",__FILE__, __LINE__, ((long *)X)[0], ((long *)X)[1])

#define getLSB(x) (*((unsigned short *)&x)&1)
#define makeBlock(X,Y) _mm_set_epi64((__m64)(X), (__m64)(Y))

#define SUCCESS 0
#define FAILURE -1

#define TIMES 10

#endif
