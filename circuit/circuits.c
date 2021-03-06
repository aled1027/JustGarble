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
#include "circuits.h"

#include <assert.h>

int
MUX21Circuit(GarbledCircuit *gc, GarblingContext *gcContext, 
             int theSwitch, int input0, int input1, int *output)
{
    int notSwitch = getNextWire(gcContext);
    NOTGate(gc, gcContext, theSwitch, notSwitch);

    int and0 = getNextWire(gcContext);
    ANDGate(gc, gcContext, notSwitch, input0, and0);

    int and1 = getNextWire(gcContext);
    ANDGate(gc, gcContext, theSwitch, input1, and1);

    *output = getNextWire(gcContext);
    ORGate(gc, gcContext, and0, and1, *output);
    return 0;
}

int
ANDCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n, const int *inputs,
           int *outputs)
{
	int i;
	int oldInternalWire = getNextWire(ctxt);
	int newInternalWire;

	ANDGate(gc, ctxt, inputs[0], inputs[1],
			oldInternalWire);

	for (i = 2; i < n - 1; i++) {
		newInternalWire = getNextWire(ctxt);
		ANDGate(gc, ctxt, inputs[i], oldInternalWire,
				newInternalWire);
		oldInternalWire = newInternalWire;
	}
	outputs[0] = getNextWire(ctxt);
	return ANDGate(gc, ctxt, inputs[n - 1],
			oldInternalWire, outputs[0]);
}

int MIXEDCircuit(GarbledCircuit *gc,
		GarblingContext *ctxt, int n, int *inputs, int *outputs) {
	int i;
	int oldInternalWire = inputs[0];
	int newInternalWire;

	for (i = 0; i < n - 1; i++) {
		newInternalWire = getNextWire(ctxt);
		if (i % 3 == 2)
			ORGate(gc, ctxt, inputs[i + 1],
					oldInternalWire, newInternalWire);
		if (i % 3 == 1)
			ANDGate(gc, ctxt, inputs[i + 1],
					oldInternalWire, newInternalWire);
		if (i % 3 == 0)
			XORGate(gc, ctxt, inputs[i + 1],
					oldInternalWire, newInternalWire);
		oldInternalWire = newInternalWire;
	}
	outputs[0] = oldInternalWire;
	return 0;
}

void
GF16SQCLCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
                int *outputs)
{
	int a[2], b[2];
	a[0] = inputs[2];
	a[1] = inputs[3];
	b[0] = inputs[0];
	b[1] = inputs[1];

	int ab[4];
	ab[0] = a[0];
	ab[1] = a[1];
	ab[2] = b[0];
	ab[3] = b[1];

	int tempx[2];
	XORCircuit(gc, ctxt, 4, ab, tempx);
	int p[2], q[2];
	GF4SQCircuit(gc, ctxt, tempx, p);
	int tempx2[4];
	GF4SQCircuit(gc, ctxt, b, tempx2);
	GF4SCLN2Circuit(gc, ctxt, tempx2, q);

	outputs[0] = q[0];
	outputs[1] = q[1];
	outputs[2] = p[0];
	outputs[3] = p[1];
}

void
GF16MULCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
               int *outputs)
{
	int ab[4], cd[4], e[2];
	ab[0] = inputs[2];
	ab[1] = inputs[3];
	ab[2] = inputs[0];
	ab[3] = inputs[1];

	cd[0] = inputs[2 + 4];
	cd[1] = inputs[3 + 4];
	cd[2] = inputs[0 + 4];
	cd[3] = inputs[1 + 4];

	int abcdx[4];
	XORCircuit(gc, ctxt, 4, ab, abcdx);
	XORCircuit(gc, ctxt, 4, cd, abcdx + 2);
	GF4MULCircuit(gc, ctxt, abcdx, e);

    int em[2];
	GF4SCLNCircuit(gc, ctxt, e, em);
	int p[2], q[2];

	int ac[4];
	ac[0] = ab[0];
	ac[1] = ab[1];
	ac[2] = cd[0];
	ac[3] = cd[1];

	int bd[4];
	bd[0] = ab[2 + 0];
	bd[1] = ab[2 + 1];
	bd[2] = cd[2 + 0];
	bd[3] = cd[2 + 1];

	int tmpx1[4], tmpx2[4];
	GF4MULCircuit(gc, ctxt, ac, tmpx1);
	GF4MULCircuit(gc, ctxt, bd, tmpx2);

	tmpx1[2] = em[0];
	tmpx1[3] = em[1];
	tmpx2[2] = em[0];
	tmpx2[3] = em[1];

	XORCircuit(gc, ctxt, 4, tmpx1, p);
	XORCircuit(gc, ctxt, 4, tmpx2, q);

	outputs[0] = q[0];
	outputs[1] = q[1];
	outputs[2] = p[0];
	outputs[3] = p[1];
}

void
GF4MULCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
              int *outputs)
{
	int a, b, c, d, e, p, q, temp1, temp2;

	a = inputs[1];
	b = inputs[0];
	c = inputs[3];
	d = inputs[2];

    temp1 = getNextWire(ctxt);
	XORGate(gc, ctxt, a, b, temp1);
	temp2 = getNextWire(ctxt);
	XORGate(gc, ctxt, c, d, temp2);
	e = getNextWire(ctxt);
	ANDGate(gc, ctxt, temp1, temp2, e);
	temp1 = getNextWire(ctxt);
	ANDGate(gc, ctxt, a, c, temp1);
	p = getNextWire(ctxt);
	XORGate(gc, ctxt, temp1, e, p);
	temp2 = getNextWire(ctxt);
	ANDGate(gc, ctxt, b, d, temp2);
	q = getNextWire(ctxt);
	XORGate(gc, ctxt, temp2, e, q);

	outputs[1] = p;
	outputs[0] = q;
}

void
GF4SCLNCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
               int *outputs)
{
	outputs[0] = getNextWire(ctxt);
	XORGate(gc, ctxt, inputs[0], inputs[1], outputs[0]);
	outputs[1] = inputs[0];
}

void
GF4SQCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
             int *outputs)
{
	outputs[0] = inputs[1];
	outputs[1] = inputs[0];
}

void
GF256InvCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
                int *outputs)
{
	int E[4], P[4], Q[4], tempX[4], tempX2[4];
	int CD[8], EA[8], EB[8];

	XORCircuit(gc, ctxt, 8, inputs, tempX);
	GF16SQCLCircuit(gc, ctxt, tempX, CD);
	GF16MULCircuit(gc, ctxt, inputs, CD + 4);

	XORCircuit(gc, ctxt, 8, CD, tempX2);
	GF16INVCircuit(gc, ctxt, tempX2, E);

	for (int i = 0; i < 4; i++) {
		EB[i] = E[i];
		EB[i + 4] = inputs[i + 4];
	}

	for (int i = 0; i < 4; i++) {
		EA[i] = E[i];
		EA[i + 4] = inputs[i];
	}

	GF16MULCircuit(gc, ctxt, EB, P);
	GF16MULCircuit(gc, ctxt, EA, Q);

	outputs[4] = P[0];
	outputs[5] = P[1];
	outputs[6] = P[2];
	outputs[7] = P[3];

	outputs[0] = Q[0];
	outputs[1] = Q[1];
	outputs[2] = Q[2];
	outputs[3] = Q[3];
}

void
GF16INVCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
               int *outputs)
{
	int a[2], b[2];
	a[0] = inputs[2];
	a[1] = inputs[3];
	b[0] = inputs[0];
	b[1] = inputs[1];

	int ab[4], cd[4];
	ab[0] = a[0];
	ab[1] = a[1];
	ab[2] = b[0];
	ab[3] = b[1];

	int tempx[2], tempxs[2];
	XORCircuit(gc, ctxt, 4, ab, tempx);
	GF4SQCircuit(gc, ctxt, tempx, tempxs);

	int c[2], d[2], e[2], p[2], q[2];
	GF4SCLNCircuit(gc, ctxt, tempxs, c);

	GF4MULCircuit(gc, ctxt, ab, d);

	cd[0] = c[0];
	cd[1] = c[1];
	cd[2] = d[0];
	cd[3] = d[1];

	int tempx2[2];
	XORCircuit(gc, ctxt, 4, cd, tempx2);
	GF4SQCircuit(gc, ctxt, tempx2, e);
	int eb[4], ea[4];
	ea[0] = e[0];
	ea[1] = e[1];
	ea[2] = a[0];
	ea[3] = a[1];

	eb[0] = e[0];
	eb[1] = e[1];
	eb[2] = b[0];
	eb[3] = b[1];

	GF4MULCircuit(gc, ctxt, eb, p);
	GF4MULCircuit(gc, ctxt, ea, q);

	outputs[0] = q[0];
	outputs[1] = q[1];
	outputs[2] = p[0];
	outputs[3] = p[1];
}

int GF4SCLN2Circuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int *inputs, int *outputs) {

	outputs[1] = getNextWire(ctxt);
	XORGate(gc, ctxt, inputs[0], inputs[1], outputs[1]);
	outputs[0] = inputs[1];

	return 0;
}

int RANDCircuit(GarbledCircuit *gc,
		GarblingContext *ctxt, int n, int *inputs, int *outputs,
		int q, int qf) {
	int i;
	int oldInternalWire = getNextWire(ctxt);
	int newInternalWire;

	ANDGate(gc, ctxt, 0, 1, oldInternalWire);

	for (i = 2; i < q + qf - 1; i++) {
		newInternalWire = getNextWire(ctxt);
		if (i < q)
			ANDGate(gc, ctxt, i % n, oldInternalWire,
					newInternalWire);
		else
			XORGate(gc, ctxt, i % n, oldInternalWire,
					newInternalWire);
		oldInternalWire = newInternalWire;
	}
	outputs[0] = oldInternalWire;
	return 0;
}

int INCCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int n, int *inputs, int *outputs) {
	int i;
	for (i = 0; i < n; i++)
		outputs[i] = getNextWire(ctxt);

	NOTGate(gc, ctxt, inputs[0], outputs[0]);
	int carry = inputs[0];
	int newCarry;
	for (i = 1; i < n; i++) {
		XORGate(gc, ctxt, inputs[i], carry, outputs[i]);
		newCarry = getNextWire(ctxt);
		ANDGate(gc, ctxt, inputs[i], carry, newCarry);
		carry = newCarry;
	}
	return 0;
}

int SUBCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int n, int *inputs, int *outputs) {
	int tempWires[n / 2];
	int tempWires2[n];
	int split = n / 2;
	NOTCircuit(gc, ctxt, n / 2, inputs + split,
			tempWires);
	INCCircuit(gc, ctxt, n / 2, tempWires,
			tempWires2 + split);
	memcpy(tempWires2, inputs, sizeof(int) * split);
	return ADDCircuit(gc, ctxt, n, tempWires2, outputs);
}

int SHLCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int n, int *inputs, int *outputs) {
	outputs[0] = fixedZeroWire(gc, ctxt);
	memcpy(outputs + 1, inputs, sizeof(int) * (n - 1));
	return 0;
}

int SHRCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int n, int *inputs, int *outputs) {
	outputs[n - 1] = fixedZeroWire(gc, ctxt);
	memcpy(outputs, inputs + 1, sizeof(int) * (n - 1));
	return 0;
}

int MULCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int nt, int *inputs, int *outputs) {
	int i, j;
	int n = nt / 2;
	int *A = inputs;
	int *B = inputs + n;

	int tempAnd[n][2 * n];
	int tempAddIn[4 * n];
	int tempAddOut[4 * n];

	for (i = 0; i < n; i++) {
		for (j = 0; j < i; j++) {
			tempAnd[i][j] = fixedZeroWire(gc, ctxt);
		}
		for (j = i; j < i + n; j++) {
			tempAnd[i][j] = getNextWire(ctxt);
			ANDGate(gc, ctxt, A[j - i], B[i],
					tempAnd[i][j]);
		}
		for (j = i + n; j < 2 * n; j++)
			tempAnd[i][j] = fixedZeroWire(gc, ctxt);
	}

	for (j = 0; j < 2 * n; j++) {
		tempAddOut[j] = tempAnd[0][j];
	}
	for (i = 1; i < n; i++) {
		for (j = 0; j < 2 * n; j++) {
			tempAddIn[j] = tempAddOut[j];
		}
		for (j = 2 * n; j < 4 * n; j++) {
			tempAddIn[j] = tempAnd[i][j - 2 * n];
		}
		ADDCircuit(gc, ctxt, 4 * n, tempAddIn,
				tempAddOut);
	}
	for (j = 0; j < 2 * n; j++) {
		outputs[j] = tempAddOut[j];
	}
	return 0;

}

int GRECircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n,
		int *inputs, int *outputs) {
	int tempWires[n];
	int i;
	for (i = 0; i < n / 2; i++) {
		tempWires[i] = inputs[i + n / 2];
		tempWires[i + n / 2] = inputs[i];
	}
	return LESCircuit(gc, ctxt, n, tempWires, outputs);

}

int
MINCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n,
           int *inputs, int *outputs)
{
	int i;
	int lesOutput;
	int notOutput = getNextWire(ctxt);
	LESCircuit(gc, ctxt, n, inputs, &lesOutput);
	NOTGate(gc, ctxt, lesOutput, notOutput);
    int split = n / 2;
	for (i = 0; i < split; i++)
        MUX21Circuit(gc, ctxt, lesOutput, inputs[i], inputs[split + i], outputs+i);
	return 0;
}

int LEQCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n,
		int *inputs, int *outputs) {
	int tempWires;
	GRECircuit(gc, ctxt, n, inputs, &tempWires);
	int outWire = getNextWire(ctxt);
	NOTGate(gc, ctxt, tempWires, outWire);
	outputs[0] = outWire;
	return 0;
}

int GEQCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n,
		int *inputs, int *outputs) {
	int tempWires;
	LESCircuit(gc, ctxt, n, inputs, &tempWires);
	int outWire = getNextWire(ctxt);
	NOTGate(gc, ctxt, tempWires, outWire);
	outputs[0] = outWire;
	return 0;
}

int
LESCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n,
           int *inputs, int *outputs)
{
    /* Returns 0 if the first number is less.
     * Returns 1 if the second number is less.
     * Returns 0 if the numbers are equal.
     */
    assert(n < 22); /* Tests fail for n >= 22 */
    assert(n % 2 == 0);

    int split = n/2;
    int **andInputs = malloc(sizeof(int*) * (split - 1));
    for (int i = 0; i < split - 1; i++) {
        andInputs[i] = malloc(sizeof(int) * (split - i));
        assert(andInputs[i]);
        //printf("allocating addInputs[%d] with %d spots\n", i, split-i);
    }
    int *finalORInputs = malloc(sizeof(int) * split);
    /* Go bit by bit and those operations */
    for (int i = 0; i < split; i++) {
        int A = inputs[split + i];
        int B = inputs[i];

	    int notA = getNextWire(ctxt);
	    NOTGate(gc, ctxt, A, notA);

	    int notB = getNextWire(ctxt);
	    NOTGate(gc, ctxt, B, notB);

	    int case1 = getNextWire(ctxt);
	    ANDGate(gc, ctxt, notA, B, case1);

	    int case2 = getNextWire(ctxt);
	    ANDGate(gc, ctxt, A, notB, case2);

        if (i != split - 1)
            andInputs[i][0] = case1;

        int orOutput = getNextWire(ctxt);
        ORGate(gc, ctxt, case1, case2, orOutput);

        int norOutput = getNextWire(ctxt);
        NOTGate(gc, ctxt, orOutput, norOutput);

        for (int j = 0; j < i; j++) {
            //printf("filling add[%d,%d]\n", j, i-j);
            andInputs[j][i-j] = norOutput;
        }
        if (i == split - 1)
            finalORInputs[split - 1] = case1;
	}

    /*  Do the aggregate operations with orInputs */
    for (int i = 0; i < split - 1; i++) {
        int nAndInputs = split - i;
        ANDCircuit(gc, ctxt, nAndInputs, andInputs[i], &finalORInputs[i]);
    }

    /* Final OR Circuit  */
    if (split == 1) {
        outputs[0] = finalORInputs[0];
    } else {
        ORCircuit(gc, ctxt, split, finalORInputs, outputs);
    }
    for (int i = 0; i < split - 1; i++)
        free(andInputs[i]);
    free(andInputs);
    free(finalORInputs);
	return 0;
}

int EQUCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n,
		int *inputs, int *outputs) {
	int tempWires[n / 2];

	XORCircuit(gc, ctxt, n, inputs, tempWires);
	int i;
	int tempWire1 = tempWires[0];
	int tempWire2;
	for (i = 1; i < n / 2; i++) {
		tempWire2 = getNextWire(ctxt);
		ORGate(gc, ctxt, tempWire1, tempWires[i], tempWire2);
		tempWire1 = tempWire2;
	}
	int outWire = getNextWire(ctxt);

	NOTGate(gc, ctxt, tempWire1, outWire);
	outputs[0] = outWire;
	return 0;
}

int
NOTCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n, const int *inputs,
           int *outputs)
{
	for (int i = 0; i < n; i++) {
		outputs[i] = getNextWire(ctxt);
		NOTGate(gc, ctxt, inputs[i], outputs[i]);
	}
	return 0;
}

int ADDCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
		int n, int *inputs, int *outputs) {
	int i;
	int tempOut[3];
	int split = n / 2;
	int tempIn[3];

	tempIn[0] = inputs[0];
	tempIn[1] = inputs[split];
	ADD22Circuit(gc, ctxt, tempIn, tempOut);
	outputs[0] = tempOut[0];

	for (i = 1; i < split; i++) {
		tempIn[2] = tempOut[1];
		tempIn[1] = inputs[split + i];
		tempIn[0] = inputs[i];
		ADD32Circuit(gc, ctxt, tempIn, tempOut);
		outputs[i] = tempOut[0];
	}
	return 0;
}

int ADD32Circuit(GarbledCircuit *gc,
                 GarblingContext *ctxt, int *inputs, int *outputs) {
	int wire1 = getNextWire(ctxt);
	int wire2 = getNextWire(ctxt);
	int wire3 = getNextWire(ctxt);
	int wire4 = getNextWire(ctxt);
	int wire5 = getNextWire(ctxt);

	XORGate(gc, ctxt, inputs[2], inputs[0], wire1);
	XORGate(gc, ctxt, inputs[1], inputs[0], wire2);
	XORGate(gc, ctxt, inputs[2], wire2, wire3);
	ANDGate(gc, ctxt, wire1, wire2, wire4);
	XORGate(gc, ctxt, inputs[0], wire4, wire5);
	outputs[0] = wire3;
	outputs[1] = wire5;
	return 0;
}

int
ADD22Circuit(GarbledCircuit *gc, GarblingContext *ctxt,
             int *inputs, int *outputs)
{
	int wire1 = getNextWire(ctxt);
	int wire2 = getNextWire(ctxt);

	XORGate(gc, ctxt, inputs[0], inputs[1], wire1);
	ANDGate(gc, ctxt, inputs[0], inputs[1], wire2);
	outputs[0] = wire1;
	outputs[1] = wire2;
	return 0;
}

int
ORCircuit(GarbledCircuit *gc, GarblingContext *ctxt,
          int n, int *inputs, int *outputs)
{
	int oldInternalWire = getNextWire(ctxt);
    assert(n >= 2);
	ORGate(gc, ctxt, inputs[0], inputs[1],
           oldInternalWire);
	for (int i = 2; i < n - 1; i++) {
		int newInternalWire = getNextWire(ctxt);
		ORGate(gc, ctxt, inputs[i], oldInternalWire,
               newInternalWire);
		oldInternalWire = newInternalWire;
	}
	outputs[0] = getNextWire(ctxt);
	int res = ORGate(gc, ctxt, inputs[n - 1],
                     oldInternalWire, outputs[0]);
	return res;
}

int
MultiXORCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int d,
                int n, int *inputs, int *outputs)
{
	int div = n / d;

	int tempInWires[n];
	int tempOutWires[n];
	int res = 0;
	for (int i = 0; i < div; i++) {
		tempOutWires[i] = inputs[i];
	}

	for (int i = 1; i < d; i++) {
		for (int j = 0; j < div; j++) {
			tempInWires[j] = tempOutWires[j];
			tempInWires[div + j] = inputs[div * i + j];
		}
		res = XORCircuit(gc, ctxt, 2 * div, tempInWires,
                         tempOutWires);
	}
	for (int i = 0; i < div; i++) {
		outputs[i] = tempOutWires[i];
	}

	return res;
}

int
XORCircuit(GarbledCircuit *gc, GarblingContext *ctxt, int n, const int *inputs,
           int *outputs)
{
	for (int i = 0; i < n / 2; i++) {
		int internalWire = getNextWire(ctxt);
		XORGate(gc, ctxt, inputs[i], inputs[n / 2 + i], internalWire);
		outputs[i] = internalWire;
	}
	return 0;
}

//http://edipermadi.files.wordpress.com/2008/02/aes_galois_field.jpg

int
GF8MULCircuit(GarbledCircuit *gc, GarblingContext *ctxt, const int *inputs,
              int *outputs)
{
	outputs[0] = inputs[7];
	outputs[2] = inputs[1];
	outputs[3] = getNextWire(ctxt);
	XORGate(gc, ctxt, inputs[7], inputs[2], outputs[3]);

	outputs[4] = getNextWire(ctxt);
	XORGate(gc, ctxt, inputs[7], inputs[3], outputs[4]);

	outputs[5] = inputs[4];
	outputs[6] = inputs[5];
	outputs[7] = inputs[6];
	outputs[1] = getNextWire(ctxt);
	XORGate(gc, ctxt, inputs[7], inputs[0], outputs[1]);

	return 0;
}
