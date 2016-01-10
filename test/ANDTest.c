
#include <stdio.h>
#include <time.h>

#include "../include/justGarble.h"
#include "../include/gates.h"

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
test(int n, int nlayers)
{
    GarbledCircuit gc;

    block inputLabels[2 * n];
    block outputLabels[2 * n];

    int timeGarble[TIMES];
    int timeEval[TIMES];
    double timeGarbleMedians[TIMES];
    double timeEvalMedians[TIMES];

    srand(time(NULL));
    srand_sse(time(NULL));

    buildCircuit(&gc, n, nlayers);

    for (int i = 0; i < TIMES; ++i) {
        for (int j = 0; j < TIMES; ++j) {
            timeGarble[j] = garbleCircuit(&gc, inputLabels, outputLabels);
            timeEval[j] = timedEval(&gc, inputLabels);
        }
        timeGarbleMedians[i] = ((double) median(timeGarble, TIMES)) / gc.q;
        timeEvalMedians[i] = ((double) median(timeEval, TIMES)) / gc.q;
    }
    printf("%lf %lf\n",
           doubleMean(timeGarbleMedians, TIMES),
           doubleMean(timeEvalMedians, TIMES));
}

int
main(int argc, char *argv[])
{
    int ninputs, nlayers;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ninputs> <nlayers>\n", argv[0]);
        exit(1);
    }

    ninputs = atoi(argv[1]);
    nlayers = atoi(argv[2]);

    if (ninputs % 2 != 0) {
        fprintf(stderr, "Error: ninputs must be even\n");
        exit(1);
    }

    test(ninputs, nlayers);

    return 0;
}
