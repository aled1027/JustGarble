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

#ifndef justGarble
#define justGarble 1

#include "common.h"

typedef enum {
    GARBLE_TYPE_STANDARD,
    GARBLE_TYPE_HALFGATES,
} GarbleType;

typedef struct {
	block label0, label1;
} Wire;

typedef struct {
    long idx;
    block label;
} FixedWire;

typedef struct {
    int type;
	long input0, input1, output;
} GarbledGate;

typedef struct {
	block table[4];
} GarbledTable;

typedef struct {
	int n, m, q, r, nFixedWires;
	GarbledGate *garbledGates;  /* q */
	GarbledTable *garbledTable; /* q */
	Wire *wires;                /* r */
    FixedWire *fixedWires;      /* r */
	int *outputs;               /* m */
	block globalKey;
} GarbledCircuit;

/* Used for constructing a circuit, and not for garbling as the name suggests */
typedef struct {
	long wireIndex, gateIndex;
	int *fixedWires;
    int nFixedWires;
	block R;
} GarblingContext;

#define DOUBLE(B) _mm_slli_epi64(B,1)

#define FIXED_ZERO_GATE 0
#define FIXED_ONE_GATE 15
#define ANDGATE 8
#define ORGATE 14
#define XORGATE 6
#define NOTGATE 5
#define NO_GATE -1

block
seedRandom(block *seed);

/*
 * The following are the functions involved in creating, garbling, and 
 * evaluating circuits. Most of the data-structures involved are defined
 * above, and the rest are in other header files.
 */

// Start and finish building a circuit. In between these two steps, gates
// and sub-circuits can be added in. See AESFullTest and LargeCircuitTest
// for examples. Note that the data-structures involved are GarbledCircuit
// and GarblingContext, although these steps do not involve any garbling.
// The reason for this is efficiency. Typically, garbleCircuit is called
// right after finishBuilding. So, using a GarbledCircuit data-structure
// here means that there is no need to create and initialize a new 
// data-structure just before calling garbleCircuit.
void
startBuilding(GarbledCircuit *gc, GarblingContext *ctxt);
void
finishBuilding(GarbledCircuit *gc, GarblingContext *ctxt,
               block *outputMap, const int *outputs);

// Create memory for an empty circuit of the specified size.
int
createEmptyGarbledCircuit(GarbledCircuit *gc, int n, int m, int q, int r,
                          const block *inputLabels);
void
removeGarbledCircuit(GarbledCircuit *gc);

//Create memory for 2*n input labels.
void
createInputLabels(block *inputLabels, int n);
void
createInputLabelsWithR(block *inputLabels, int n, block R);

//Garble the circuit described in garbledCircuit. For efficiency reasons,
//we use the garbledCircuit data-structure for representing the input 
//circuit and the garbled output. The garbling process is non-destructive and 
//only affects the garbledTable member of the GarbledCircuit data-structure.
//In other words, the same GarbledCircuit object can be reused multiple times,
//to create multiple garbled circuit copies, 
//by just switching the garbledTable field to a new one. Also, the garbledTable
//field is the only one that should be sent over the network in the case of an 
//MPC-type application, as the topology is expected to be avaiable to the 
//receiver, and the gate-types are to be hidden away.
//The inputLabels field is expected to contain 2n fresh input labels, obtained
//by calling createInputLabels. The outputMap is expected to be a 2m-block sized
//empty array.
void
garbleCircuit(GarbledCircuit *gc, block *outputMap, GarbleType type);
unsigned long
timedGarble(GarbledCircuit *gc, block *outputMap, GarbleType type);
void
hashGarbledCircuit(GarbledCircuit *gc, unsigned char *hash, GarbleType type);
int
checkGarbledCircuit(GarbledCircuit *gc, const unsigned char *hash,
                    GarbleType type);

void
extractLabels(block *extractedLabels, const block *labels, const int *bits,
              long n);
void
evaluate(const GarbledCircuit *gc, const block *extractedLabels,
         block *outputLabels, GarbleType type);
unsigned long
timedEval(const GarbledCircuit *gc, const block *inputLabels, GarbleType type);

// A simple function that takes 2m output labels, m labels from evaluate, 
// and returns a m bit output by matching the labels. If one or more of the
// m evaluated labels donot match either of the two corresponding output labels,
// then the function flags an error.
int
mapOutputs(const block *outputLabels, const block *outputMap, int *vals, int m);

int
writeCircuitToFile(GarbledCircuit *gc, char *fileName);
int
readCircuitFromFile(GarbledCircuit *gc, char *fileName);

#include "gc.h"
#include "util.h"

#endif
