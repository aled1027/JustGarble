#include "justGarble.h"

size_t
garbledCircuitSize(const GarbledCircuit *gc)
{
    size_t size = 0;

    size += sizeof(int) * 5;
    size += sizeof(GarbledGate) * gc->q;
    size += sizeof(GarbledTable) * gc->q;
    if (gc->wires) {
        size += sizeof(Wire) * gc->r;
    }
    size += sizeof(FixedWire) * gc->nFixedWires;
    size += sizeof(int) * gc->m;
    size += sizeof(block);

    return size;
}

void
saveGarbledCircuit(const GarbledCircuit *gc, FILE *f)
{
    fwrite(&gc->n, sizeof gc->n, 1, f);
    fwrite(&gc->m, sizeof gc->m, 1, f);
    fwrite(&gc->q, sizeof gc->q, 1, f);
    fwrite(&gc->r, sizeof gc->r, 1, f);
    fwrite(&gc->nFixedWires, sizeof gc->nFixedWires, 1, f);
    fwrite(gc->garbledGates, sizeof(GarbledGate), gc->q, f);
    fwrite(gc->garbledTable, sizeof(GarbledTable), gc->q, f);
    if (gc->wires) {
        fwrite(gc->wires, sizeof(Wire), gc->r, f);
    }
    fwrite(gc->fixedWires, sizeof(FixedWire), gc->nFixedWires, f);
    fwrite(gc->outputs, sizeof(int), gc->m, f);
    fwrite(&gc->globalKey, sizeof(block), 1, f);
}

void
loadGarbledCircuit(GarbledCircuit *gc, FILE *f, bool isGarbler)
{
    fread(&gc->n, sizeof gc->n, 1, f);
    fread(&gc->m, sizeof gc->m, 1, f);
    fread(&gc->q, sizeof gc->q, 1, f);
    fread(&gc->r, sizeof gc->r, 1, f);
    fread(&gc->nFixedWires, sizeof gc->nFixedWires, 1, f);
    gc->garbledGates = calloc(gc->q, sizeof(GarbledGate));
    fread(gc->garbledGates, sizeof(GarbledGate), gc->q, f);
    gc->garbledTable = calloc(gc->q, sizeof(GarbledTable));
    fread(gc->garbledTable, sizeof(GarbledTable), gc->q, f);
    if (isGarbler) {
        gc->wires = calloc(gc->r, sizeof(Wire));
        fread(gc->wires, sizeof(Wire), gc->r, f);
    } else {
        gc->wires = NULL;
    }
    gc->fixedWires = calloc(gc->nFixedWires, sizeof(FixedWire));
    fread(gc->fixedWires, sizeof(FixedWire), gc->nFixedWires, f);
    gc->outputs = calloc(gc->m, sizeof(int));
    fread(gc->outputs, sizeof(int), gc->m, f);
    fread(&gc->globalKey, sizeof(block), 1, f);
}


size_t
copyGarbledCircuitToBuffer(const GarbledCircuit *gc, char *buffer)
{
    size_t p = 0;

    memcpy(buffer, &gc->n, sizeof gc->n);
    p += sizeof gc->n;
    memcpy(buffer + p, &gc->m, sizeof gc->m);
    p += sizeof gc->m;
    memcpy(buffer + p, &gc->q, sizeof gc->q);
    p += sizeof gc->q;
    memcpy(buffer + p, &gc->r, sizeof gc->r);
    p += sizeof gc->r;
    memcpy(buffer + p, &gc->nFixedWires, sizeof gc->nFixedWires);
    p += sizeof gc->nFixedWires;
    memcpy(buffer + p, gc->garbledGates, sizeof(GarbledGate) * gc->q);
    p += sizeof(GarbledGate) * gc->q;
    memcpy(buffer + p, gc->garbledTable, sizeof(GarbledTable) * gc->q);
    p += sizeof(GarbledTable) * gc->q;
    if (gc->wires) {
        memcpy(buffer + p, gc->wires, sizeof(Wire) * gc->r);
        p += sizeof(Wire) * gc->r;
    }
    memcpy(buffer + p, gc->fixedWires, sizeof(FixedWire) * gc->nFixedWires);
    p += sizeof(Wire) * gc->nFixedWires;
    memcpy(buffer + p, gc->outputs, sizeof(int) * gc->m);
    p += sizeof(int) * gc->m;
    memcpy(buffer + p, &gc->globalKey, sizeof(block));
    p += sizeof(block);

    return p;
}

size_t
copyGarbledCircuitFromBuffer(GarbledCircuit *gc, const char *buffer, bool wires)
{
    size_t p = 0;

    memcpy(&gc->n, buffer + p, sizeof gc->n);
    p += sizeof gc->n;
    memcpy(&gc->m, buffer + p, sizeof gc->m);
    p += sizeof gc->m;
    memcpy(&gc->q, buffer + p, sizeof gc->q);
    p += sizeof gc->q;
    memcpy(&gc->r, buffer + p, sizeof gc->r);
    p += sizeof gc->r;
    memcpy(&gc->nFixedWires, buffer + p, sizeof gc->nFixedWires);
    p += sizeof gc->nFixedWires;
    gc->garbledGates = malloc(sizeof(GarbledGate) * gc->q);
    memcpy(gc->garbledGates, buffer + p, sizeof(GarbledGate) * gc->q);
    p += sizeof(GarbledGate) * gc->q;
    gc->garbledTable = malloc(sizeof(GarbledTable) * gc->q);
    memcpy(gc->garbledTable, buffer + p, sizeof(GarbledTable) * gc->q);
    p += sizeof(GarbledTable) * gc->q;
    if (wires) {
        gc->wires = malloc(sizeof(Wire) * gc->r);
        memcpy(gc->wires, buffer + p, sizeof(Wire) * gc->r);
        p += sizeof(Wire) * gc->r;
    } else {
        gc->wires = NULL;
    }
    gc->fixedWires = malloc(sizeof(Wire) * gc->nFixedWires);
    memcpy(gc->fixedWires, buffer + p, sizeof(FixedWire) * gc->nFixedWires);
    p += sizeof(FixedWire) * gc->nFixedWires;
    gc->outputs = malloc(sizeof(int) * gc->m);
    memcpy(gc->outputs, buffer + p, sizeof(int) * gc->m);
    p += sizeof(int) * gc->m;
    memcpy(&gc->globalKey, buffer + p, sizeof(block));
    p += sizeof(block);

    return p;
}
