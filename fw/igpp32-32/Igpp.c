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

#include <Igpp.h>

//Double buffering mechanism
volatile uint8_t frameBuffer[2][PANEL_DATA_SIZE] = {{0x00}, {0x00}};

uint8_t anodesEmpty[ANODE_BYTES] = {0xFF, 0xFF, 0xFF, 0xFF
#if PANEL_HEIGHT > 1
                                    ,0xFF, 0xFF, 0xFF, 0xFF
#endif
#if PANEL_HEIGHT > 2
                                    ,0xFF, 0xFF, 0xFF, 0xFF
#endif
};

//Current Frame is currently displayed.
volatile uint8_t currentFrame = 0;//or 1

volatile uint8_t cathodePos = 0;

volatile uint16_t rotation = 0;

/*
anode_latch P1.0
anode_mr P1.1
anode OE P1.2
anode srclk P4.0
anode DI P4.4

cathode_latch P1.3
cathode_mr P1.4
cathode OE P1.5
cathode srclk P4.3
cathode DI P4.1
*/

volatile uint8_t igppFlags = igppFlagNone;

uint8_t* igppLoadBufferPtr()
{
    uint8_t loadedFrame = (currentFrame + 1) & 0x01;
    return (uint8_t*)(frameBuffer[loadedFrame]);
}

void igppChangeBuffer()
{
    currentFrame = (currentFrame + 1) & 0x01;
}


void igppInit()
{
    P1DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
    P1OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5);

    P4DIR |= BIT1 | BIT3;
    P4OUT &= ~(BIT1 | BIT3);

    SpiInit(dmaAnodesHandler);

    //Scan TA0: 40kHz

    TA0R = 0x00;
    TA0CCTL0 |= CCIE; //CCR0 interrupt
    TA0CCR0 = 406;// 6.5MHz/ 16kHz
    TA0CTL |= TASSEL_2 | MC_1 | ID_2; //SMCLK 26MHz div 4 = 6.5MHz

    P1OUT |= BIT1 | BIT4; //MR is up

    for (uint16_t x = 0; x < DISPLAY_WIDTH; ++x)
    {
        for (uint16_t y = 0; y < ANODE_BYTES; ++y)
        {
            frameBuffer[0][y + (ANODE_BYTES * x)] = x % 2 ? 0xAA : 0x55;
            frameBuffer[1][y + (ANODE_BYTES * x)] = x % 2 ? 0x55 : 0xAA;
        }
    }
}

inline void igppNextCathode()
{
    if (cathodePos == 0)
    {
        igppCathodeClear();
        igppCathodeDataHigh();
        igppCathodeTick();
        igppCathodeDataLow();
    }
    else
    {
        igppCathodeTick();
    }
    if (++cathodePos >= DISPLAY_WIDTH)
    {
        cathodePos = 0;
    }
}

void dmaAnodesHandler()
{
    while (UCA0STAT & UCBUSY)
    {
        ;
    }
    if (igppFlags == igppAnodesErase)
    {
        igppFlags = igppFlagNone;
        igppNextCathode();
        igppLatchAll();
        //Select next cathode and anode data:
        uint8_t* AnodesDataPtr = (uint8_t*)(frameBuffer[currentFrame] + (ANODE_BYTES * cathodePos));
        //Send data to SPI channel
        SpiASend(AnodesDataPtr, ANODE_BYTES);
    }
    else
    {
        igppLatchAll();
    }
}

void igppNextFrame()
{
    //Clear Anodes and cathodes:
    igppAnodeClear();
    SpiASend(anodesEmpty, ANODE_BYTES);
    igppFlags = igppAnodesErase;
}

//This timer does 16kHz f_scan
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMERA0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMERA0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    igppNextFrame();
}
