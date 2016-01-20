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

#include <assert.h>
#include <malloc.h>

#include "justGarble.h"
#include "aes.h"

static void
hash1(block *A, block tweak, const AES_KEY *K)
{
    block key;
    block mask;

    key = xorBlocks(DOUBLE(*A), tweak);
    mask = key;
    AES_ecb_encrypt_blks(&key, 1, K);
    *A = xorBlocks(key, mask);
}

static void
hash2(block *A, block *B, block tweak1, block tweak2,
      const AES_KEY *key)
{
    block keys[2];
    block masks[2];

    keys[0] = xorBlocks(DOUBLE(*A), tweak1);
    keys[1] = xorBlocks(DOUBLE(*B), tweak2);
    masks[0] = keys[0];
    masks[1] = keys[1];
    AES_ecb_encrypt_blks(keys, 2, key);
    *A = xorBlocks(keys[0], masks[0]);
    *B = xorBlocks(keys[1], masks[1]);
}

static void
evaluateHalfGates(const GarbledCircuit *gc, block *wireLabels, const AES_KEY *K)
{
	for (long i = 0; i < gc->q; i++) {
		GarbledGate *gg = &gc->garbledGates[i];
		if (gg->type == XORGATE) {
			wireLabels[gg->output] =
                xorBlocks(wireLabels[gg->input0], wireLabels[gg->input1]);
		} else if (gg->type == NOTGATE) {
            block A, tweak;
            unsigned short pa;

            A = wireLabels[gg->input0];
            tweak = makeBlock(2 * i, (long) 0);
            pa = getLSB(A);
            hash1(&A, tweak, K);
            wireLabels[gg->output] = xorBlocks(A, gc->garbledTable[i].table[pa]);
        } else {
            block A, B, W;
            int sa, sb;
            block tweak1, tweak2;

            A = wireLabels[gg->input0];
            B = wireLabels[gg->input1];

            sa = getLSB(A);
            sb = getLSB(B);

            tweak1 = makeBlock(2 * i, (long) 0);
            tweak2 = makeBlock(2 * i + 1, (long) 0);

            hash2(&A, &B, tweak1, tweak2, K);

            W = xorBlocks(A, B);
            if (sa)
                W = xorBlocks(W, gc->garbledTable[i].table[0]);
            if (sb) {
                W = xorBlocks(W, gc->garbledTable[i].table[1]);
                W = xorBlocks(W, wireLabels[gg->input0]);
            }
            wireLabels[gg->output] = W;
        }
	}
}

static void
evaluateStandard(const GarbledCircuit *gc, block *wireLabels, AES_KEY *K)
{
	for (long i = 0; i < gc->q; i++) {
		GarbledGate *gg = &gc->garbledGates[i];
		if (gg->type == XORGATE) {
			wireLabels[gg->output] =
                xorBlocks(wireLabels[gg->input0], wireLabels[gg->input1]);
		} else {
            block A, B, tmp, tweak, val;
            int a, b;

            A = DOUBLE(wireLabels[gg->input0]);
            B = DOUBLE(DOUBLE(wireLabels[gg->input1]));

            a = getLSB(wireLabels[gg->input0]);
            b = getLSB(wireLabels[gg->input1]);

            val = xorBlocks(A, B);
            tweak = makeBlock(i, (long) 0);
            val = xorBlocks(val, tweak);
            if (a + b == 0) {
                tmp = val;
            } else {
                tmp = xorBlocks(gc->garbledTable[i].table[2*a+b-1], val);
            }
            AES_ecb_encrypt_blks(&val, 1, K);

            wireLabels[gg->output] = xorBlocks(val, tmp);
        }
	}
}

void
evaluate(const GarbledCircuit *gc, const block *extractedLabels,
         block *outputLabels, GarbleType type)
{
    AES_KEY K;
    block *wireLabels;

	AES_set_encrypt_key((unsigned char *) &gc->globalKey, 128, &K);
    wireLabels = allocate_blocks(gc->r);

    /* Set input wire labels */
	for (long i = 0; i < gc->n; ++i) {
        wireLabels[i] = extractedLabels[i];
	}

    /* Set fixed wire labels */
    for (long i = 0; i < gc->nFixedWires; ++i) {
        wireLabels[gc->fixedWires[i].idx] = gc->fixedWires[i].label;
    }

    /* Process gates */
    switch (type) {
    case GARBLE_TYPE_STANDARD:
        evaluateStandard(gc, wireLabels, &K);
        break;
    case GARBLE_TYPE_HALFGATES:
        evaluateHalfGates(gc, wireLabels, &K);
        break;
    default:
        assert(0);
        abort();
    }

    /* Set output wire labels */
	for (long i = 0; i < gc->m; i++) {
		outputLabels[i] = wireLabels[gc->outputs[i]];
	}

    free(wireLabels);
}

void
extractLabels(block *extractedLabels, const block *labels, const int *bits,
              long n)
{
    for (long i = 0; i < n; ++i) {
        extractedLabels[i] = labels[2 * i + (bits[i] ? 1 : 0)];
    }
}

static int
blockEqual(block a, block b)
{
	long *ap = (long*) &a;
	long *bp = (long*) &b;
	return ((ap[0] == bp[0]) && (ap[1] == bp[1]));
}

int
mapOutputs(const block *outputLabels, const block *outputMap, int *vals, int m)
{
	for (int i = 0; i < m; i++) {
		if (blockEqual(outputMap[i], outputLabels[2 * i])) {
			vals[i] = 0;
        } else if (blockEqual(outputMap[i], outputLabels[2 * i + 1])) {
			vals[i] = 1;
		} else {
            printf("MAP FAILED %d\n", i);
            return FAILURE;
        }
	}
	return SUCCESS;
}

unsigned long
timedEval(const GarbledCircuit *gc, const block *inputLabels, GarbleType type)
{
    block extractedLabels[gc->n];
	block outputMap[gc->m];
	int inputs[gc->n];
	unsigned long start;

	for (int i = 0; i < gc->n; ++i) {
		inputs[i] = rand() % 2;
	}

	start = RDTSC;
    extractLabels(extractedLabels, inputLabels, inputs, gc->n);
	evaluate(gc, extractedLabels, outputMap, type);
    return RDTSC - start;
}
