/*
 * Pseudo-random generator for Game of Life.
 *
 * fw/msp430-rng/rand.c also contains a hardware rand() written for the
 * MSP430x2xx registers (BCSCTL3, TACTL) that does not build for F5xx with
 * msp430-elf-gcc. Only prand() is used here, so it is provided locally with the
 * same LCG constants from rand.h.
 */
#include "rand.h"

unsigned int prand(unsigned int state)
{
    return M * state + I;
}
