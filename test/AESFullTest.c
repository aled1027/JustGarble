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
#include "circuits.h"
#include "util.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <openssl/sha.h>

#define AES_CIRCUIT_FILE_NAME "./aesCircuit"

static const int roundLimit = 10;
static const int n = 128 * (10 + 1);
static const int m = 128;
static const int times = 100;

static void
buildAESCircuit(GarbledCircuit *gc, block *inputLabels)
{
	GarblingContext ctxt;

	int q = 50000; //Just an upper bound
	int r = 50000;
    int *addKeyInputs = calloc(2 * m, sizeof(int));
    int *addKeyOutputs = calloc(m, sizeof(int));
    int *subBytesOutputs = calloc(m, sizeof(int));
    int *shiftRowsOutputs = calloc(m, sizeof(int));
    int *mixColumnOutputs = calloc(m, sizeof(int));
    block *outputMap = allocate_blocks(2 * m);

	createEmptyGarbledCircuit(gc, n, m, q, r, inputLabels);
	startBuilding(gc, &ctxt);

	countToN(addKeyInputs, 256);

	for (int round = 0; round < roundLimit; round++) {

		AddRoundKey(gc, &ctxt, addKeyInputs, addKeyOutputs);

		for (int i = 0; i < 16; i++) {
			SubBytes(gc, &ctxt, addKeyOutputs + 8 * i, subBytesOutputs + 8 * i);
		}

		ShiftRows(gc, &ctxt, subBytesOutputs, shiftRowsOutputs);

		for (int i = 0; i < 4; i++) {
			if (round != roundLimit - 1)
				MixColumns(gc, &ctxt, shiftRowsOutputs + i * 32,
                           mixColumnOutputs + 32 * i);
		}
		for (int i = 0; i < 128; i++) {
			addKeyInputs[i] = mixColumnOutputs[i];
			addKeyInputs[i + 128] = (round + 2) * 128 + i;
		}
	}

	finishBuilding(gc, &ctxt, outputMap, mixColumnOutputs);
	/* writeCircuitToFile(gc, AES_CIRCUIT_FILE_NAME); */

    free(addKeyInputs);
    free(addKeyOutputs);
    free(subBytesOutputs);
    free(shiftRowsOutputs);
    free(mixColumnOutputs);
    free(outputMap);
}

int
main(int argc, char *argv[])
{
	GarbledCircuit gc;

    block *outputMap = allocate_blocks(2 * m);
    block *inputLabels = allocate_blocks(2 * n);
    block seed;

	int *timeGarble = calloc(times, sizeof(int));
	int *timeEval = calloc(times, sizeof(int));
	double *timeGarbleMedians = calloc(times, sizeof(double));
	double *timeEvalMedians = calloc(times, sizeof(double));

    unsigned char hash[SHA_DIGEST_LENGTH];

    GarbleType type = GARBLE_TYPE_STANDARD;

    seed = seedRandom(NULL);

    createInputLabels(inputLabels, n);
    buildAESCircuit(&gc, inputLabels);
	/* readCircuitFromFile(&gc, AES_CIRCUIT_FILE_NAME); */
    garbleCircuit(&gc, outputMap, type);
    hashGarbledCircuit(&gc, hash, type);

    {
        block *extractedLabels = allocate_blocks(n);
        block *computedOutputMap = allocate_blocks(m);
        int *inputs = calloc(n, sizeof(int));
        int *outputVals = calloc(m, sizeof(int));
        for (int i = 0; i < n; ++i) {
            inputs[i] = rand() % 2;
        }
        extractLabels(extractedLabels, inputLabels, inputs, gc.n);
        evaluate(&gc, extractedLabels, computedOutputMap, type);
        assert(mapOutputs(outputMap, computedOutputMap, outputVals, m) == SUCCESS);
        {
            GarbledCircuit gc2;

            (void) seedRandom(&seed);
            createInputLabels(inputLabels, n);
            buildAESCircuit(&gc2, inputLabels);
            assert(checkGarbledCircuit(&gc2, hash, type) == SUCCESS);
        }
        free(extractedLabels);
        free(computedOutputMap);
        free(inputs);
        free(outputVals);
    }

	for (int j = 0; j < times; j++) {
		for (int i = 0; i < times; i++) {
			timeGarble[i] = timedGarble(&gc, outputMap, type);
			timeEval[i] = timedEval(&gc, inputLabels, type);
		}
		timeGarbleMedians[j] = ((double) median(timeGarble, times)) / gc.q;
		timeEvalMedians[j] = ((double) median(timeEval, times)) / gc.q;
	}
	double garblingTime = doubleMean(timeGarbleMedians, times);
	double evalTime = doubleMean(timeEvalMedians, times);
	printf("%lf %lf\n", garblingTime, evalTime);

    free(outputMap);
    free(inputLabels);
    free(timeGarble);
    free(timeEval);
    free(timeGarbleMedians);
    free(timeEvalMedians);
	return 0;
}

