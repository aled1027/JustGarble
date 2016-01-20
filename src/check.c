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

int
checkCircuit(GarbledCircuit *gc, block *inputLabels, block *outputMap,
             GarbleType type, int check(int *a, int *out, int s))
{
	int j;
	int n = gc->n;
	int m = gc->m;
	block computedOutputMap[m];
    block extractedLabels[n];
	int outputVals[m];
	int outputReal[m];
	int inputs[n];

    for (j = 0; j < n; j++) {
        inputs[j] = rand() % 2;
    }
    extractLabels(extractedLabels, inputLabels, inputs, n);
    evaluate(gc, extractedLabels, computedOutputMap, type);
    mapOutputs(outputMap, computedOutputMap, outputVals, m);
    check(inputs, outputReal, n);
    for (j = 0; j < m; j++) {
        if (outputVals[j] != outputReal[j]) {
            fprintf(stderr, "Check failed %u\n", j);
        }
    }

	return 0;
}
