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
#include <stdint.h>
#include <time.h>
#include <wmmintrin.h>

#include "justGarble.h"

int
createNewWire(Wire *in, GarblingContext *ctxt, int id)
{
	in->id = id;
	in->label0 = randomBlock();
	in->label1 = xorBlocks(ctxt->R, in->label0);
	return 0;
}

static unsigned long currentId = 0;

int
getNextId(void)
{
	currentId++;
	return currentId;
}

int
getFreshId(void)
{
	currentId = 0;
	return currentId;
}

int
getNextWire(GarblingContext *garblingContext)
{
	int i = garblingContext->wireIndex;
	garblingContext->wireIndex++;
	return i;
}

int
createEmptyGarbledCircuit(GarbledCircuit *gc, int n, int m,
                          int q, int r, block *inputLabels)
{
	gc->id = getNextId();
    (void) posix_memalign((void **) &gc->garbledGates, 128, sizeof(GarbledGate) * q);
    (void) posix_memalign((void **) &gc->garbledTable, 128, sizeof(GarbledTable) * q);
    (void) posix_memalign((void **) &gc->wires, 128, sizeof(Wire) * r);
    (void) posix_memalign((void **) &gc->outputs, 128, sizeof(int) * m);

	if (gc->garbledGates == NULL || gc->garbledTable == NULL
        || gc->wires == NULL || gc->outputs == NULL) {
		dbgs("Linux is a cheap miser that refuses to give us memory");
		exit(1);
	}
	gc->m = m;
	gc->n = n;
    /* q is set in finishBuilding() */
	gc->q = -1;
	gc->r = r;
	for (int i = 0; i < r; i++) {
		gc->wires[i].id = 0;
	}
	for (int i = 0; i < q; i++) {
		gc->garbledGates[i].id = 0;
	}

	for (int i = 0; i < 2 * n; i += 2) {
		gc->wires[i / 2].id = i + 1;
		gc->wires[i / 2].label0 = inputLabels[i];
		gc->wires[i / 2].label1 = inputLabels[i + 1];
	}
	return 0;
}

void
removeGarbledCircuit(GarbledCircuit *gc)
{
	/* gc->id = getNextId(); */
	free(gc->garbledGates);
    free(gc->garbledTable);
	free(gc->wires);
    free(gc->outputs);
}

void
startBuilding(GarbledCircuit *gc, GarblingContext *ctxt)
{
    ctxt->wireIndex = gc->n; /* start at first non-input wire */
	ctxt->gateIndex = 0;
	ctxt->R = xorBlocks(gc->wires[0].label0, gc->wires[0].label1);
	ctxt->fixedWires = malloc(sizeof(int) * gc->r);
    for (int i = 0; i < gc->r; ++i) {
        ctxt->fixedWires[i] = NO_GATE;
    }
	gc->globalKey = randomBlock();
}

void
finishBuilding(GarbledCircuit *gc, GarblingContext *ctxt,
               block *outputMap, const int *outputs)
{
	for (int i = 0; i < gc->m; i++) {
		outputMap[2 * i] = gc->wires[outputs[i]].label0;
		outputMap[2 * i + 1] = gc->wires[outputs[i]].label1;
		gc->outputs[i] = outputs[i];
	}
	for (int i = 0; i < gc->r; i++) {
        switch (ctxt->fixedWires[i]) {
        case FIXED_ZERO_GATE:
            gc->wires[i].label = gc->wires[i].label0;
            break;
        case FIXED_ONE_GATE:
            gc->wires[i].label = gc->wires[i].label1;
            break;
        default:
            break;
        }
	}
	gc->q = ctxt->gateIndex;
    free(ctxt->fixedWires);
}

void
extractLabels(block *extractedLabels, const block *inputLabels,
              const int *inputBits, int n)
{
	for (int i = 0; i < n; i++) {
		if (inputBits[i]) {
			extractedLabels[i] = inputLabels[2 * i + 1];
		} else {
			extractedLabels[i] = inputLabels[2 * i];
		}
	}
}

static void
hash2(block *A, block *B, const block tweak, AES_KEY *key)
{
    block keys[2];
    block masks[2];

    keys[0] = xorBlocks(DOUBLE(*A), tweak);
    keys[1] = xorBlocks(DOUBLE(*B), tweak);
    masks[0] = keys[0];
    masks[1] = keys[1];
    AES_ecb_encrypt_blks(keys, 2, key);
    *A = xorBlocks(keys[0], masks[0]);
    *B = xorBlocks(keys[1], masks[1]);
}

static void
hash4(block *A0, block *A1, block *B0, block *B1,
      const block tweak1, const block tweak2, AES_KEY *key)
{
    block keys[4];
    block masks[4];

    keys[0] = xorBlocks(DOUBLE(*A0), tweak1);
    keys[1] = xorBlocks(DOUBLE(*A1), tweak1);
    keys[2] = xorBlocks(DOUBLE(*B0), tweak2);
    keys[3] = xorBlocks(DOUBLE(*B1), tweak2);
    masks[0] = keys[0];
    masks[1] = keys[1];
    masks[2] = keys[2];
    masks[3] = keys[3];
    AES_ecb_encrypt_blks(keys, 4, key);
    *A0 = xorBlocks(keys[0], masks[0]);
    *A1 = xorBlocks(keys[1], masks[1]);
    *B0 = xorBlocks(keys[2], masks[2]);
    *B1 = xorBlocks(keys[3], masks[3]);
}

static void
garbleCircuitHalfGates(GarbledCircuit *gc, block *inputLabels, block *outputMap)
{
	block R;
    AES_KEY K;

	gc->id = getFreshId();

    R = xorBlocks(gc->wires[0].label0, gc->wires[0].label1);
    /* Set input wire labels */
	createInputLabelsWithR(inputLabels, gc->n, R);

	for (int i = 0; i < 2 * gc->n; i += 2) {
		gc->wires[i/2].id = i+1;
		gc->wires[i/2].label0 = inputLabels[i];
		gc->wires[i/2].label1 = inputLabels[i+1];
	}

    gc->globalKey = randomBlock();
    AES_set_encrypt_key((unsigned char *) &gc->globalKey, 128, &K);

    /* Process garbled gates */
	for (long i = 0; i < gc->q; i++) {
        GarbledGate *gg;
        block A0, A1, B0, B1;
        int output;

        gg = &(gc->garbledGates[i]);
		output = gg->output;

        A0 = gc->wires[gg->input0].label0;
		A1 = gc->wires[gg->input0].label1;
		B0 = gc->wires[gg->input1].label0;
		B1 = gc->wires[gg->input1].label1;

        if (gg->type == XORGATE) {
			gc->wires[output].label0 = xorBlocks(A0, B0);
        } else if (gg->type == NOTGATE) {
            block tweak;
            long pa = getLSB(A0);

            tweak = makeBlock(2 * i, (long) 0);
            hash2(&A0, &A1, tweak, &K);
            gc->garbledTable[i].table[pa] =
                xorBlocks(A0, gc->wires[gg->input0].label1);
            gc->garbledTable[i].table[!pa] =
                xorBlocks(A1, gc->wires[gg->input0].label0);
            gc->wires[output].label0 = gc->wires[gg->input0].label1;
        } else {
            long pa = getLSB(A0);
            long pb = getLSB(B0);
            block table[2];
            block tweak1, tweak2;
            block tmp, W0;

            tweak1 = makeBlock(2 * i, (long) 0);
            tweak2 = makeBlock(2 * i + 1, (long) 0);

            hash4(&A0, &A1, &B0, &B1, tweak1, tweak2, &K);
            switch (gg->type) {
            case ANDGATE:
                table[0] = xorBlocks(A0, A1);
                if (pb)
                    table[0] = xorBlocks(table[0], R);
                W0 = A0;
                if (pa)
                    W0 = xorBlocks(W0, table[0]);
                tmp = xorBlocks(B0, B1);
                table[1] = xorBlocks(tmp, gc->wires[gg->input0].label0);
                W0 = xorBlocks(W0, B0);
                if (pb)
                    W0 = xorBlocks(W0, tmp);
                goto finish;
            case ORGATE:
                table[0] = xorBlocks(A0, A1);
                if (!pb)
                    table[0] = xorBlocks(table[0], R);
                W0 = pa ? A1 : A0;
                if ((!pa * !pb) ^ 1)
                    W0 = xorBlocks(W0, R);
                table[1] = xorBlocks(B0, B1);
                table[1] = xorBlocks(table[1], gc->wires[gg->input0].label1);
                W0 = xorBlocks(W0, pb ? B1 : B0);
                goto finish;
            default:
                assert(0);
                exit(1);
            finish:
                gc->garbledTable[i].table[0] = table[0];
                gc->garbledTable[i].table[1] = table[1];
                gc->wires[output].label0 = W0;
            }
        }

        gc->wires[output].label1 = xorBlocks(gc->wires[output].label0, R);
	}
    /* Set output wire labels */
	for (int i = 0; i < gc->m; ++i) {
		outputMap[2*i] = gc->wires[gc->outputs[i]].label0;
		outputMap[2*i+1] = gc->wires[gc->outputs[i]].label1;
	}
}

static void
garbleCircuitStandard(GarbledCircuit *gc, block *inputLabels,
                      block *outputMap)
{
    block R;
    AES_KEY K;

	GarbledTable *garbledTable;

    R = xorBlocks(gc->wires[0].label0,
                  gc->wires[0].label1);

	createInputLabelsWithR(inputLabels, gc->n, R);

	gc->id = getFreshId();

	for (int i = 0; i < 2 * gc->n; i += 2) {
		gc->wires[i/2].id = i+1;
		gc->wires[i/2].label0 = inputLabels[i];
		gc->wires[i/2].label1 = inputLabels[i+1];
	}
	garbledTable = gc->garbledTable;
	gc->globalKey = randomBlock();
	AES_set_encrypt_key((unsigned char *) &gc->globalKey, 128, &K);

	for (long i = 0; i < gc->q; i++) {
        GarbledGate *garbledGate;
        int input0, input1, output;
        block tweak, blocks[4], keys[4];
        long lsb0, lsb1;

        garbledGate = &(gc->garbledGates[i]);
		input0 = garbledGate->input0;
        input1 = garbledGate->input1;
		output = garbledGate->output;

		if (garbledGate->type == XORGATE) {
			gc->wires[output].label0
                = xorBlocks(gc->wires[input0].label0,
                            gc->wires[input1].label0);
			gc->wires[output].label1
                = xorBlocks(gc->wires[input0].label1,
                            gc->wires[input1].label0);
			continue;
		}
		tweak = makeBlock(i, (long)0);
		lsb0 = getLSB(gc->wires[input0].label0);
		lsb1 = getLSB(gc->wires[input1].label0);

		block A0, A1, B0, B1;
		A0 = DOUBLE(gc->wires[input0].label0);
		A1 = DOUBLE(gc->wires[input0].label1);
		B0 = DOUBLE(DOUBLE(gc->wires[input1].label0));
		B1 = DOUBLE(DOUBLE(gc->wires[input1].label1));

		keys[0] = xorBlocks(A0, B0);
		keys[0] = xorBlocks(keys[0], tweak);
		keys[1] = xorBlocks(A0,B1);
		keys[1] = xorBlocks(keys[1], tweak);
		keys[2] = xorBlocks(A1, B0);
		keys[2] = xorBlocks(keys[2], tweak);
		keys[3] = xorBlocks(A1, B1);
		keys[3] = xorBlocks(keys[3], tweak);

        block mask[4];
        block newToken;

        mask[0] = keys[0];
        mask[1] = keys[1];
        mask[2] = keys[2];
        mask[3] = keys[3];
        AES_ecb_encrypt_blks(keys, 4, &K);
        mask[0] = xorBlocks(mask[0], keys[0]);
        mask[1] = xorBlocks(mask[1], keys[1]);
        mask[2] = xorBlocks(mask[2], keys[2]);
        mask[3] = xorBlocks(mask[3], keys[3]);

        if (2*lsb0 + lsb1 ==0)
            newToken = mask[0];
        else if (2*lsb0 + 1-lsb1 ==0)
            newToken = mask[1];
        else if (2*(1-lsb0) + lsb1 ==0)
            newToken = mask[2];
        else /* if (2*(1-lsb0) + 1-lsb1 ==0) */
            newToken = mask[3];

		block newToken2 = xorBlocks(R, newToken);
		block *label0 = &gc->wires[garbledGate->output].label0;
		block *label1 = &gc->wires[garbledGate->output].label1;

        switch (garbledGate->type) {
        case ANDGATE:
			if (lsb1 & lsb0) {
                *label1 = newToken;
                *label0 = newToken2;
			} else {
                *label0 = newToken;
                *label1 = newToken2;
			}
			blocks[0] = *label0;
			blocks[1] = *label0;
			blocks[2] = *label0;
			blocks[3] = *label1;
            break;
        case ORGATE:
			if (!(!lsb1 & !lsb0)) {
				*label1 = newToken;
				*label0 = newToken2;
			} else {
				*label0 = newToken;
				*label1 = newToken2;
			}
			blocks[0] = *label0;
			blocks[1] = *label1;
			blocks[2] = *label1;
			blocks[3] = *label1;
            break;
        case NOTGATE:
			if (!lsb0) {
				*label1 = newToken;
				*label0 = newToken2;
			} else {
				*label0 = newToken;
				*label1 = newToken2;
			}
			blocks[0] = *label1;
			blocks[1] = *label0;
			blocks[2] = *label1;
			blocks[3] = *label0;
            break;
        default:
            assert(0);
            exit(1);
        }

		if (2*lsb0 + lsb1 !=0)
            garbledTable[i].table[2*lsb0 + lsb1 -1] = xorBlocks(blocks[0], mask[0]);
		if (2*lsb0 + 1-lsb1 !=0)
            garbledTable[i].table[2*lsb0 + 1-lsb1-1] = xorBlocks(blocks[1], mask[1]);
		if (2*(1-lsb0) + lsb1 !=0)
            garbledTable[i].table[2*(1-lsb0) + lsb1-1] = xorBlocks(blocks[2], mask[2]);
		if (2*(1-lsb0) + (1-lsb1) !=0)
            garbledTable[i].table[2*(1-lsb0) + (1-lsb1)-1] = xorBlocks(blocks[3], mask[3]);
	}
    /* Set output wire labels */
	for (int i = 0; i < gc->m; ++i) {
		outputMap[2*i] = gc->wires[gc->outputs[i]].label0;
		outputMap[2*i+1] = gc->wires[gc->outputs[i]].label1;
	}
}

int
garbleCircuit(GarbledCircuit *gc, block *inputLabels, block *outputMap,
              GarbleType type)
{
    switch (type) {
    case GARBLE_TYPE_STANDARD:
        garbleCircuitStandard(gc, inputLabels, outputMap);
        break;
    case GARBLE_TYPE_HALFGATES:
        garbleCircuitHalfGates(gc, inputLabels, outputMap);
        break;
    default:
        assert(0);
        return FAILURE;
    }
    return SUCCESS;
}

unsigned long
timedGarble(GarbledCircuit *gc, block *inputLabels, block *outputMap,
            GarbleType type)
{
    unsigned long start, end;

    start = RDTSC;
    garbleCircuit(gc, inputLabels, outputMap, type);
    end = RDTSC;
    return end - start;
}

int
blockEqual(block a, block b)
{
	long *ap = (long*) &a;
	long *bp = (long*) &b;
	return ((ap[0] == bp[0]) && (ap[1] == bp[1]));
}

int
mapOutputs(const block *outputMap, const block *outputMap2, int *vals, int m)
{
	for (int i = 0; i < m; i++) {
		if (blockEqual(outputMap2[i], outputMap[2 * i])) {
			vals[i] = 0;
        } else if (blockEqual(outputMap2[i], outputMap[2 * i + 1])) {
			vals[i] = 1;
		} else {
            printf("MAP FAILED %d\n", i);
            return FAILURE;
        }
	}
	return SUCCESS;
}

void
createInputLabelsWithR(block *inputLabels, int n, block R)
{
    block *rctxt = getRandContext();
	for (int i = 0; i < 2 * n; i += 2) {
		randAESBlock(&inputLabels[i], rctxt);
		inputLabels[i + 1] = xorBlocks(R, inputLabels[i]);
	}
}


void
createInputLabels(block *inputLabels, int n)
{
    block R = randomBlock();
    *((char *) &R) |= 1;
    createInputLabelsWithR(inputLabels, n, R);
}

int
findGatesWithMatchingInputs(GarbledCircuit *gc,
                            block *inputLabels, block *outputMap,
                            int *outputs)
{
	GarbledGate *garbledGate1, *garbledGate2;
	int matching = 0;

	for (int i = 0; i < gc->n; i++) {
		gc->wires[i].label = inputLabels[i];
	}

	for (int i = 0; i < gc->q; i++) {
		garbledGate1 = &gc->garbledGates[i];
		for (int j = i + 1; j < gc->q; j++) {
			garbledGate2 = &gc->garbledGates[j];
			if (garbledGate1->input0 == garbledGate2->input0
					&& garbledGate1->input1 == garbledGate2->input1) {
				matching++;
			}
		}
	}
	return 0;
}
