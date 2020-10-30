/*MIT License

Copyright (c) 2020 Artem Kashkanov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef IGPP_H_
#define IGPP_H_

#include <string.h>
#include "define.h"

enum igppFlagTypes
{
  igppFlagNone = 0,
  igppAnodesErase = 1
};

void spiInit();

void igppInit();

void igppAnodeClear();

inline void igppCathodeTick()
{
    P4OUT  |= BIT3;
    P4OUT  &= ~BIT3;
}

inline void igppCathodeDataHigh()
{
    P4OUT  |= BIT1;
}

inline void igppCathodeDataLow()
{
    P4OUT  &= ~BIT1;
}

inline void igppLatchAll()
{
    while (UCA0STAT & UCBUSY)
    {
        ;
    }
    P1OUT  |= BIT0 | BIT3;
    P1OUT  &= ~( BIT0 | BIT3);
}

inline void igppCathodeClear()
{
    P1OUT  &= ~BIT4;
    P1OUT  |= BIT4;
}

inline void igppCathodeLatch()
{
    P1OUT  |= BIT3;
    P1OUT  &= ~BIT3;
}

inline void igppAnodeLatch()
{
    P1OUT  |= BIT0;
    P1OUT  &= ~( BIT0);
}

inline void igppAnodeClear()
{
    P1OUT  &= ~(BIT1);
    P1OUT  |= (BIT1);
}

inline void igppClearAll()
{
    P1OUT  &= ~(BIT1 | BIT4);
    P1OUT  |= (BIT1 | BIT4);
}

void igppNextFrame();

uint8_t* igppLoadBufferPtr();

void igppChangeBuffer();

#endif /* IGPP_H_ */
