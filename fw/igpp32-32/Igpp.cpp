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

volatile uint8_t cathodeSelector[CATHODE_BYTES] = {0x00};

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

void igppInit()
{
    P1DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5;
    P1OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT4 | BIT5);

    P4DIR |= BIT1 | BIT3;
    P4OUT &= ~(BIT1 | BIT3);

    SpiInit();

    //Scan TA0: 40kHz

    TA0R = 0x00;
    TA0CCTL0 |= CCIE; //CCR0 interrupt
    TA0CCR0 = 150;// 7.5MHz/ 40kHz
    TA0CTL |= TASSEL_2 | MC_1 | ID_2; //SMCLK 26MHz div 4 = 7.5MHz

    //timeout TB):

    TB0R = 0x00;
    TB0CCTL0 |= CCIE; //CCR0 interrupt
    TB0CTL |= TBSSEL_2; //SMCLK, 26MHz. Inerrupt Enabled , flag enabled

    P1OUT |= BIT1 | BIT4; //MR is up

    igppTick();

    for (uint16_t x = 0; x < DISPLAY_WIDTH; ++x)
    {
        for (uint16_t y = 0; y < ANODE_BYTES; ++y)
        {
            frameBuffer[0][y + (ANODE_BYTES * x)] = x % 2 ? 0xAA : 0x55;
            frameBuffer[1][y + (ANODE_BYTES * x)] = x % 2 ? 0x55 : 0xAA;
        }
    }
}

uint8_t* igppLoadBufferPtr()
{
    uint8_t loadedFrame = (currentFrame + 1) & 0x01;
    return (uint8_t*)(frameBuffer[loadedFrame]);
}

void igppChangeBuffer()
{
    currentFrame = (currentFrame + 1) & 0x01;
}

void igppAnodeClear()
{
    P1OUT  &= ~BIT1;
    P1OUT  |= BIT1;
    igppTick();
}

void cathodeTick()
{
    while (UCB0STAT & UCBUSY)
    {
        ;
    }
    igppCathodeLatch();
    if (++cathodePos >= DISPLAY_WIDTH)
    {
        cathodePos = 0;
    }
    uint8_t byteNum = (cathodePos >> 3);
    uint8_t bitPos = cathodePos & 0x07;
    memset((uint8_t*)(&cathodeSelector), 0, CATHODE_BYTES);
    cathodeSelector[byteNum] = (1 << bitPos);
}

void igppLatch()
{
    while (UCA0STAT & UCBUSY)
    {
        ;
    }
    //anodesLatch
    P1OUT  |= BIT0;
    P1OUT  &= ~( BIT0);
    igppAnodeWait(40, igppAnodeClear);
}

void igppTick()
{
    uint8_t* AnodesDataPtr = (uint8_t*)(frameBuffer[currentFrame] + (ANODE_BYTES * cathodePos));
    igppCathodeClear();
    SpiBSend((uint8_t*)cathodeSelector, CATHODE_BYTES, cathodeTick);
    SpiASend(AnodesDataPtr, ANODE_BYTES, igppLatch);
}


void (*m_callback)()  = NULL;

inline void igppAnodeWait(uint16_t us, void (*callback)())
{
//Time setup   TimerTB0:
    TB0CCR0 = us * 23 ; // 6MHz SMCLK
    TB0R = 0;
    TB0CTL |= MC_1; // Count up to the TB0CCR0 value
    m_callback = callback;
}

//This timer does 50kHz f_scan
// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMERA0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMERA0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    //igppTick();
}

// Timer B0 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMERB0_ISR (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_B0_VECTOR))) TIMERB0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    TB0CTL &= ~MC_1;
    if (m_callback)
        (m_callback)();

}
