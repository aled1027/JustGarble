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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "../include/justGarble.h"
#include "../include/circuits.h"

#define AES_CIRCUIT_FILE_NAME "./aesCircuit"

void
buildAESCircuit(GarbledCircuit *gc)
{
	GarblingContext garblingContext;

	int roundLimit = 10;
	int n = 128 * (roundLimit + 1);
	int m = 128;
	int q = 36000; //Just an upper bound
	int r = 50000;
	int inp[n];
	countToN(inp, n);
	int addKeyInputs[n * (roundLimit + 1)];
	int addKeyOutputs[n];
	int subBytesOutputs[n];
	int shiftRowsOutputs[n];
	int mixColumnOutputs[n];
	block inputLabels[2 * n];
	block outputMap[m];

	createInputLabels(inputLabels, n);
	createEmptyGarbledCircuit(gc, n, m, q, r, inputLabels);
	startBuilding(gc, &garblingContext);

	countToN(addKeyInputs, 256);

	for (int round = 0; round < roundLimit; round++) {

		AddRoundKey(gc, &garblingContext, addKeyInputs, addKeyOutputs);

		for (int i = 0; i < 16; i++) {
			SubBytes(gc, &garblingContext, addKeyOutputs + 8 * i,
                     subBytesOutputs + 8 * i);
		}

		ShiftRows(gc, &garblingContext, subBytesOutputs, shiftRowsOutputs);

		for (int i = 0; i < 4; i++) {
			if (round != roundLimit - 1)
				MixColumns(gc, &garblingContext,
                           shiftRowsOutputs + i * 32, mixColumnOutputs + 32 * i);
		}
		for (int i = 0; i < 128; i++) {
			addKeyInputs[i] = mixColumnOutputs[i];
			addKeyInputs[i + 128] = (round + 2) * 128 + i;
		}
	}

	finishBuilding(gc, &garblingContext, outputMap, mixColumnOutputs);
	/* writeCircuitToFile(gc, AES_CIRCUIT_FILE_NAME); */
}

int
main(int argc, char *argv[])
{
	int rounds = 10;
	int n = 128 + (128 * rounds);
	int m = 128;
    int times = 100;

	GarbledCircuit gc;
    block extractedLabels[n];
	block inputLabels[2 * n];
	block outputMap[2 * m];
	int i, j;

	int timeGarble[times];
	int timeEval[times];
	double timeGarbleMedians[times];
	double timeEvalMedians[times];

    GarbleType type = GARBLE_TYPE_STANDARD;

    seedRandom();

    buildAESCircuit(&gc);
	/* readCircuitFromFile(&gc, AES_CIRCUIT_FILE_NAME); */

    (void) garbleCircuit(&gc, inputLabels, outputMap, type);
    {
        block computedOutputMap[m];
        int inputs[n];
        int outputVals[n];
        for (int i = 0; i < n; ++i) {
            inputs[i] = rand() % 2;
        }
        extractLabels(extractedLabels, inputLabels, inputs, gc.n);
        evaluate(&gc, extractedLabels, computedOutputMap, type);
        mapOutputs(outputMap, computedOutputMap, outputVals, m);
    }

	for (j = 0; j < times; j++) {
		for (i = 0; i < times; i++) {
			timeGarble[i] = timedGarble(&gc, inputLabels, outputMap, type);
			timeEval[i] = timedEval(&gc, inputLabels, type);
		}
		timeGarbleMedians[j] = ((double) median(timeGarble, times)) / gc.q;
		timeEvalMedians[j] = ((double) median(timeEval, times)) / gc.q;
	}
	double garblingTime = doubleMean(timeGarbleMedians, times);
	double evalTime = doubleMean(timeEvalMedians, times);
	printf("%lf %lf\n", garblingTime, evalTime);
	return 0;
}

