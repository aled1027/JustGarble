#ifndef GC_H
#define GC_H

#include "justGarble.h"

#include <stdbool.h>

size_t
garbledCircuitSize(const GarbledCircuit *gc);

size_t
copyGarbledCircuitToBuffer(const GarbledCircuit *gc, char *buffer);

size_t
copyGarbledCircuitFromBuffer(GarbledCircuit *gc, const char *buffer, bool wires);

#endif
