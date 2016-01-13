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

typedef enum garbleType { GARBLE_TYPE_STANDARD, GARBLE_TYPE_HALFGATES } GarbleType;

typedef struct {
	long value, id;
	block label, label0, label1;
} Wire;

typedef struct {
	block label; long id;
} GarbledWire;


typedef struct {
	long type, id; 
	Wire *input0, *input1, *output;
} Gate;

typedef struct {
	long input0, input1, output; int id, type;
} GarbledGate;

typedef struct {
	block table[4];
} GarbledTable;

typedef struct {
	int n,m,q,r;
	Gate* gates;
	Wire* wires;
	long id;
} Circuit;

typedef struct {
	int n, m, q, r;
	block* inputLabels, outputLabels;
	GarbledGate* garbledGates;
	GarbledTable* garbledTable;
	Wire* wires;
	int *outputs;
	long id;
	block globalKey;
} GarbledCircuit;

typedef struct {
	int m;
	block *outputLabels;
	long id;
} GarbledOutput;

/* Used for constructing a circuit, and not for garbling as the name suggests */
typedef struct {
	long wireIndex, gateIndex;
	int *fixedWires;
	block R;
} GarblingContext;


typedef block* InputLabels;
typedef block* ExtractedLabels;
typedef block* OutputMap;

void
seedRandom(void);

/*
 * The following are the functions involved in creating, garbling, and 
 * evaluating circuits. Most of the data-structures involved are defined
 * above, and the rest are in other header files.
 */

//1. Basic circuit building
// Create memory for an empty circuit of the specified size.
int createEmptyCircuit(Circuit *circuit, int n, int m, int q, int r);

// Start and finish building a circuit. In between these two steps, gates
// and sub-circuits can be added in. See AESFullTest and LargeCircuitTest
// for examples. Note that the data-structures involved are GarbledCircuit
// and GarblingContext, although these steps do not involve any garbling.
// The reason for this is efficiency. Typically, garbleCircuit is called
// right after finishBuilding. So, using a GarbledCircuit data-structure
// here means that there is no need to create and initialize a new 
// data-structure just before calling garbleCircuit.
int startBuilding(GarbledCircuit *gc, GarblingContext *ctx);
int finishBuilding(GarbledCircuit *garbledCircuit,
		GarblingContext *garbledContext, OutputMap outputMap, int *outputs);



// Create memory for an empty circuit of the specified size.
int createEmptyGarbledCircuit(GarbledCircuit *garbledCircuit, int n, int m,
		int q, int r, InputLabels inputLabels);

//Create memory for 2*n input labels.
int createInputLabels(InputLabels inputLabels, int n);
int createInputLabelsWithR(InputLabels inputLabels, int n, block* R);

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
garbleCircuit(GarbledCircuit *gc, block *inputLabels,
              block *outputMap, GarbleType type);
unsigned long
timedGarble(GarbledCircuit *gc, block *inputLabels, block *outputMap,
            GarbleType type);

//Evaluate a garbled circuit, using n input labels in the Extracted Labels
//to return m output labels. The garbled circuit might be generated either in 
//one piece, as the result of running garbleCircuit, or may be pieced together,
// by building the circuit (startBuilding ... finishBuilding), and adding 
// garbledTable from another source, say, a network transmission.
void
evaluate(GarbledCircuit *gc, block *extractedLabels, block *outputMap,
         GarbleType type);
unsigned long
timedEval(GarbledCircuit *gc, InputLabels inputLabels, GarbleType type);

// A simple function that selects n input labels from 2n labels, using the 
// inputBits array where each element is a bit.
int extractLabels(ExtractedLabels extractedLabels, InputLabels inputLabels,
		int* inputBits, int n);

// A simple function that takes 2m output labels, m labels from evaluate, 
// and returns a m bit output by matching the labels. If one or more of the
// m evaluated labels donot match either of the two corresponding output labels,
// then the function flags an error.
int mapOutputs(OutputMap outputMap, OutputMap extractedMap, int *outputVals,
		int m);


int writeCircuitToFile(GarbledCircuit *garbledCircuit, char *fileName);
int readCircuitFromFile(GarbledCircuit *garbledCircuit, char *fileName);


#include "garble.h"
#include "circuits.h"
#include "check.h"
#include "util.h"

#endif
