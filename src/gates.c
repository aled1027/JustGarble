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
#include "garble.h"
#include "gates.h"

static int
genericGate(GarbledCircuit *gc, GarblingContext *ctxt,
            int input0, int input1, int output, int type)
{
    GarbledGate *gg;

	createNewWire(&gc->wires[output], ctxt, output);

	gg = &gc->garbledGates[ctxt->gateIndex];

	ctxt->gateIndex++;
	gg->type = type;
	gg->input0 = input0;
	gg->input1 = input1;
	gg->output = output;

    return 0;
}

int
ANDGate(GarbledCircuit *gc, GarblingContext *ctxt,
        int input0, int input1, int output)
{
	return genericGate(gc, ctxt, input0, input1, output, ANDGATE);
}

int
XORGate(GarbledCircuit *gc, GarblingContext *ctxt,
        int input0, int input1, int output)
{
    GarbledGate *gg;

	createNewWire(&gc->wires[output], ctxt, output);

	gc->wires[output].label0 = xorBlocks(gc->wires[input0].label0,
                                         gc->wires[input1].label0);
	gc->wires[output].label1 = xorBlocks(gc->wires[input0].label1,
                                         gc->wires[input1].label0);

	gg = &gc->garbledGates[ctxt->gateIndex];

    ctxt->gateIndex++;
	gg->type = XORGATE;
	gg->input0 = input0;
	gg->input1 = input1;
	gg->output = output;

	return 0;
}

int
ORGate(GarbledCircuit *gc, GarblingContext *ctxt,
       int input0, int input1, int output)
{
	return genericGate(gc, ctxt, input0, input1, output, ORGATE);
}

int
fixedZeroWire(GarbledCircuit *gc, GarblingContext *ctxt)
{
    int ind;
    Wire *wire;
    
	ind = getNextWire(ctxt);
	ctxt->fixedWires[ind] = FIXED_ZERO_GATE;
	wire = &gc->wires[ind];
	wire->label0 = randomBlock();
	wire->label1 = xorBlocks(ctxt->R, wire->label0);
	return ind;

}
int
fixedOneWire(GarbledCircuit *gc, GarblingContext *ctxt)
{
    int ind;
    Wire *wire;
    
	ind = getNextWire(ctxt);
	ctxt->fixedWires[ind] = FIXED_ONE_GATE;
	wire = &gc->wires[ind];
	wire->label0 = randomBlock();
	wire->label1 = xorBlocks(ctxt->R, wire->label0);
	return ind;
}

int
NOTGate(GarbledCircuit *gc, GarblingContext *ctxt,
		int input0, int output)
{
	return genericGate(gc, ctxt, input0, input0, output, NOTGATE);
}

