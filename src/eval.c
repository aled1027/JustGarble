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

#include "../include/common.h"
#include "../include/garble.h"
#include "../include/circuits.h"
#include "../include/gates.h"
#include "../include/util.h"
#include "../include/dkcipher.h"
#include "../include/aes.h"
#include "../include/justGarble.h"

#include <assert.h>
#include <malloc.h>
/* #include <wmmintrin.h> */

void
hash2(block *A, block *B, const block tweak1, const block tweak2, AES_KEY *key)
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

static int
evaluateHalfGates(GarbledCircuit *gc, block *extractedLabels, block *outputMap)
{
	DKCipherContext dkCipherContext;
	DKCipherInit(&gc->globalKey, &dkCipherContext);

    /* Set input wire labels */
	for (long i = 0; i < gc->n; i++) {
		gc->wires[i].label = extractedLabels[i];
	}

    /* Process garbled gates */
	for (long i = 0; i < gc->q; i++) {
		GarbledGate *gg = &gc->garbledGates[i];
		if (gg->type == XORGATE) {
			gc->wires[gg->output].label
                = xorBlocks(gc->wires[gg->input0].label,
                            gc->wires[gg->input1].label);
		} else {
            block A, B, W;
            int sa, sb;
            block tweak1, tweak2;

            A = gc->wires[gg->input0].label;
            B = gc->wires[gg->input1].label;

            sa = getLSB(A);
            sb = getLSB(B);

            tweak1 = makeBlock(2 * i, (long) 0);
            tweak2 = makeBlock(2 * i + 1, (long) 0);

            hash2(&A, &B, tweak1, tweak2, &dkCipherContext.K);

            W = xorBlocks(A, B);
            if (sa)
                W = xorBlocks(W, gc->garbledTable[i].table[0]);
            if (sb) {
                W = xorBlocks(W, gc->garbledTable[i].table[1]);
                W = xorBlocks(W, gc->wires[gg->input0].label);
            }
            gc->wires[gg->output].label = W;
        }
	}
    /* Set output wire labels */
	for (long i = 0; i < gc->m; i++) {
		outputMap[i] = gc->wires[gc->outputs[i]].label;
	}
	return 0;
}

static int
evaluateStandard(GarbledCircuit *gc, block *extractedLabels,
                 block *outputMap)
{
	DKCipherContext dkCipherContext;
	DKCipherInit(&(gc->globalKey), &dkCipherContext);
	block zer = zero_block();

    /* Set input wire labels */
	for (long i = 0; i < gc->n; i++) {
		gc->wires[i].label = extractedLabels[i];
	}
    /* Process garbled gates */
	for (long i = 0; i < gc->q; i++) {
		GarbledGate *gg = &gc->garbledGates[i];
		if (gg->type == XORGATE) {
			gc->wires[gg->output].label =
                xorBlocks(gc->wires[gg->input0].label,
                          gc->wires[gg->input1].label);
		} else {
            block A, B, tmp, tweak, val;
            int a, b;

            A = DOUBLE(gc->wires[gg->input0].label);
            B = DOUBLE(DOUBLE(gc->wires[gg->input1].label));

            a = getLSB(gc->wires[gg->input0].label);
            b = getLSB(gc->wires[gg->input1].label);

            val = xorBlocks(A, B);
            tweak = makeBlock(i, (long) 0);
            val = xorBlocks(val, tweak);
            if (a+b==0)
                tmp = zer;
            else
                tmp = gc->garbledTable[i].table[2*a+b-1];

            tmp = xorBlocks(tmp, val);
            AES_ecb_encrypt_blks(&val, 1, &dkCipherContext.K);

            /* val = _mm_xor_si128(val, sched[0]); */
            /* for (int j = 1; j < 10; j++) */
            /*     val = _mm_aesenc_si128(val, sched[j]); */
            /* val = _mm_aesenclast_si128(val, sched[j]); */

            gc->wires[gg->output].label = xorBlocks(val, tmp);
        }
	}
    /* Set output wire labels */
	for (long i = 0; i < gc->m; i++) {
		outputMap[i] = gc->wires[gc->outputs[i]].label;
	}
	return 0;
}

void
evaluate(GarbledCircuit *gc, block *extractedLabels, block *outputMap,
         GarbleType type)
{
    switch (type) {
    case GARBLE_TYPE_STANDARD:
        evaluateStandard(gc, extractedLabels, outputMap);
        break;
    case GARBLE_TYPE_HALFGATES:
        evaluateHalfGates(gc, extractedLabels, outputMap);
        break;
    default:
        assert(0);
        exit(1);
    }
}

unsigned long
timedEval(GarbledCircuit *garbledCircuit, block *inputLabels,
          GarbleType type)
{
	int n = garbledCircuit->n;
	int m = garbledCircuit->m;
	block extractedLabels[n];
	block outputs[m];
	int inputs[n];
	unsigned long startTime, endTime;

	for (int i = 0; i < n; i++) {
		inputs[i] = rand() % 2;
	}
	extractLabels(extractedLabels, inputLabels, inputs, n);
	startTime = RDTSC;
	evaluate(garbledCircuit, extractedLabels, outputs, type);
	endTime = RDTSC;
    return endTime - startTime;
}

