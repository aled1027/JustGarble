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

#include "justGarble.h"

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

int
countToN(int *a, int n)
{
	for (int i = 0; i < n; i++)
		a[i] = i;
	return 0;
}

int
dbgBlock(block a)
{
	int *A = (int *) &a;
	int out = 0;
	for (int i = 0; i < 4; i++)
		out = out + (A[i] + 13432) * 23517;
	return out;
}

int
compare(const void * a, const void * b)
{
	return (*(int*) a - *(int*) b);
}

int
median(int *values, int n)
{
	qsort(values, n, sizeof(int), compare);
	if (n % 2 == 1)
		return values[(n + 1) / 2];
	else
		return (values[n / 2] + values[n / 2 + 1]) / 2;
}

double
doubleMean(double *values, int n)
{
	double total = 0;
	for (int i = 0; i < n; i++)
		total += values[i];
	return total / n;
}

// This is only for testing and benchmark purposes. Use a more 
// secure seeding mechanism for actual use.
block
seedRandom(block *seed)
{
    block cur_seed;

    srand(time(NULL));
    __current_rand_index = zero_block();
    cur_seed = seed ? *seed : _mm_set_epi32(rand(), rand(), rand(), rand());
    AES_set_encrypt_key((unsigned char *) &cur_seed, 128, &__rand_aes_key);
    return cur_seed;
}


block
randomBlock(void)
{
    block out;
    const __m128i *sched = ((__m128i *) (__rand_aes_key.rd_key));
    randAESBlock(&out, sched);
    return out;
}

block *
allocate_blocks(size_t nblocks)
{
    block *blks = NULL;;
    (void) posix_memalign((void **) &blks, 128, sizeof(block) * nblocks);
    if (blks == NULL) {
        perror("allocate_blocks");
        return NULL;
    } else {
        return blks;
    }
}

void
print_block(block blk)
{
    uint64_t *val = (uint64_t *) &blk;
    printf("%016lx%016lx", val[1], val[0]);
}
