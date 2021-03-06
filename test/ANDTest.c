
#include <stdio.h>
#include <time.h>

#include "justGarble.h"
#include "garble.h"
#include "gates.h"

void
buildCircuit(GarbledCircuit *gc, int n, int nlayers)
{
    block inputLabels[2 * n];
    block outputLabels[n];
    GarblingContext ctxt;
    int wire;
    int wires[n];
    int r = n + n / 2 * nlayers;
    int q = n / 2 * nlayers;
    /* int r = n + n * nlayers; */
    /* int q = n * nlayers; */

    printf("# gates = %d\n", q);

    countToN(wires, n);

    createInputLabels(inputLabels, n);
    createEmptyGarbledCircuit(gc, n, n, q, r, inputLabels);
    startBuilding(gc, &ctxt);

    for (int i = 0; i < nlayers; ++i) {
        for (int j = 0; j < n; j += 2) {
            wire = getNextWire(&ctxt);
            ANDGate(gc, &ctxt, wires[j], wires[j+1], wire);
            wires[j] = wires[j+1] = wire;
        }
    }

    finishBuilding(gc, &ctxt, outputLabels, wires);
}

void
test(int n, int nlayers, int times, GarbleType type)
{
    GarbledCircuit gc;

    block inputLabels[2 * n];
    block outputLabels[2 * n];
    block extractedLabels[n];

    int timeGarble[times];
    int timeEval[times];
    double timeGarbleMedians[times];
    double timeEvalMedians[times];

    (void) seedRandom(NULL);

    buildCircuit(&gc, n, nlayers);
    (void) garbleCircuit(&gc, outputLabels, type);
    {
        block computedOutputMap[gc.n];
        int inputs[gc.n];
        int outputs[gc.n];

        printf("Input:  ");
        for (int i = 0; i < gc.n; ++i) {
            inputs[i] = rand() % 2;
            printf("%d", inputs[i]);
        }
        printf("\n");
        extractLabels(extractedLabels, inputLabels, inputs, gc.n);
        evaluate(&gc, extractedLabels, computedOutputMap, type);
        mapOutputs(outputLabels, computedOutputMap, outputs, gc.n);
        printf("Output: ");
        for (int i = 0; i < gc.n; ++i) {
            printf("%d", outputs[i]);
        }
        printf("\n");
    }

    for (int i = 0; i < times; ++i) {
        for (int j = 0; j < times; ++j) {
            timeGarble[j] = timedGarble(&gc, outputLabels, type);
            timeEval[j] = timedEval(&gc, inputLabels, type);
        }
        timeGarbleMedians[i] = ((double) median(timeGarble, times)) / gc.q;
        timeEvalMedians[i] = ((double) median(timeEval, times)) / gc.q;
    }
    if (times > 0)
        printf("%lf %lf\n",
               doubleMean(timeGarbleMedians, times),
               doubleMean(timeEvalMedians, times));

    removeGarbledCircuit(&gc);
}

int
main(int argc, char *argv[])
{
    int ninputs, nlayers, ntimes;
    GarbleType type;

    if (argc != 5) {
        fprintf(stderr, "Usage: %s <ninputs> <nlayers> <ntimes> <type>\n", argv[0]);
        exit(1);
    }

    ninputs = atoi(argv[1]);
    nlayers = atoi(argv[2]);
    ntimes = atoi(argv[3]);
    type = atoi(argv[4]);

    if (ninputs % 2 != 0) {
        fprintf(stderr, "Error: ninputs must be even\n");
        exit(1);
    }

    printf("Type: ");
    switch (type) {
    default:
        type = GARBLE_TYPE_STANDARD;
    case GARBLE_TYPE_STANDARD:
        printf("Standard\n");
        break;
    case GARBLE_TYPE_HALFGATES:
        printf("Half-gates\n");
        break;
    }

    test(ninputs, nlayers, ntimes, type);

    return 0;
}
