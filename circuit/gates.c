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
#include "gates.h"

int
genericGate(GarbledCircuit *gc, GarblingContext *ctxt,
            int input0, int input1, int output, int *vals, int type)
{
	createNewWire(&gc->wires[output], ctxt, output);

	GarbledGate *garbledGate = &gc->garbledGates[ctxt->gateIndex];

	garbledGate->id = ctxt->gateIndex;
	garbledGate->type = type;
	garbledGate->input0 = input0;
	garbledGate->input1 = input1;
	garbledGate->output = output;

	ctxt->gateIndex++;

	return garbledGate->id;
}

int
ANDGate(GarbledCircuit *gc, GarblingContext *ctxt,
        int input0, int input1, int output)
{
	int vals[] = { 0, 0, 0, 1 };
	return genericGate(gc, ctxt, input0, input1, output, vals, ANDGATE);
}

int
XORGate(GarbledCircuit *gc, GarblingContext *ctxt,
        int input0, int input1, int output)
{
	if (gc->wires[input0].id == 0) {
		printf("ERROR: Uninitialized input at wire 0 %d, gate %ld\n",
               input0, ctxt->gateIndex);
        return -1;
	}
	if (gc->wires[input1].id == 0) {
		printf("ERROR: Uninitialized input at wire 1 %d, gate %ld\n",
               input1, ctxt->gateIndex);
        return -1;
	}
	if (gc->wires[output].id != 0) {
		printf("ERROR: Reusing output at wire %d\n", output);
        return -1;
	}
	createNewWire(&(gc->wires[output]), ctxt, output);

	gc->wires[output].label0 = xorBlocks(gc->wires[input0].label0,
                                         gc->wires[input1].label0);
	gc->wires[output].label1 = xorBlocks(gc->wires[input0].label1,
                                         gc->wires[input1].label0);
	GarbledGate *garbledGate = &(gc->garbledGates[ctxt->gateIndex]);
	if (garbledGate->id != 0)
        dbgs("Reusing a gate");
	garbledGate->id = XOR_ID;
	garbledGate->type = XORGATE;
	ctxt->gateIndex++;
	garbledGate->input0 = input0;
	garbledGate->input1 = input1;
	garbledGate->output = output;

	return 0;
}

int
ORGate(GarbledCircuit *gc, GarblingContext *ctxt,
       int input0, int input1, int output)
{
	int vals[] = { 0, 1, 1, 1 };
	return genericGate(gc, ctxt, input0, input1, output, vals, ORGATE);
}

int
fixedZeroWire(GarbledCircuit *gc, GarblingContext *ctxt)
{
	int ind = getNextWire(ctxt);
	ctxt->fixedWires[ind] = FIXED_ZERO_GATE;
	Wire *wire = &gc->wires[ind];
	if (wire->id != 0) {
		printf("ERROR: Reusing output at wire %d\n", ind);
        return -1;
    }
	wire->id = ind;
	wire->label0 = randomBlock();
	wire->label1 = xorBlocks(ctxt->R, wire->label0);
	return ind;

}
int
fixedOneWire(GarbledCircuit *gc, GarblingContext *ctxt)
{
	int ind = getNextWire(ctxt);
	ctxt->fixedWires[ind] = FIXED_ONE_GATE;
	Wire *wire = &gc->wires[ind];
	wire->id = ind;
	wire->label0 = randomBlock();
	wire->label1 = xorBlocks(ctxt->R, wire->label0);
	return ind;
}

int
NOTGate(GarbledCircuit *gc, GarblingContext *ctxt,
		int input0, int output)
{
	int vals[] = { 1, 0, 1, 0 };
	return genericGate(gc, ctxt, input0, input0, output, vals, NOTGATE);
}

