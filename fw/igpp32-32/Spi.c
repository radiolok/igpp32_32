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

#include <Spi.h>



void SpiInit()
{
    P4SEL |= BIT0 | BIT1 | BIT3 | BIT4;

    PMAPKEYID = 0x2D52;

    P4MAP0 = PM_UCA0CLK;
    P4MAP1 = PM_UCB0SIMO;

    PMAPKEYID = 0x2D52;

    P4MAP3 = PM_UCB0CLK;
    P4MAP4 = PM_UCA0SIMO;

    UCA0CTL0 = UCMST | UCSYNC;//MSB, syncronous
    UCA0CTL1 = UCSSEL_2; //SMCLK

    UCA0BR0 = 4;// 26MHz/4 = 7.5MHz

    UCB0CTL0 = UCMST | UCSYNC;//MSB, syncronous
    UCB0CTL1 = UCSSEL_2; //SMCLK

    UCB0BR0 = 4;// 26MHz/4 = 7.5MHz

    //UCA0IE = UCTXIE; //enable interrupt
    DMACTL0 |= DMA0TSEL_17;  // UCA0TXIFG as trigger
    DMA0CTL = DMASRCINCR_3 + DMADSTBYTE + DMASRCBYTE + DMAIE + DMAIFG;
    __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCA0TXBUF);

    DMACTL0 |= DMA0TSEL_19;  // UCB0TXIFG as trigger
    DMA1CTL = DMASRCINCR_3 + DMADSTBYTE + DMASRCBYTE + DMAIE + DMAIFG;
    __data16_write_addr((unsigned short) &DMA1DA,(unsigned long) &UCB0TXBUF);
}

/**Anodes Spi*/
void (*m_SpiACallback)()  = NULL;
void SpiASend(uint8_t* data, uint16_t size, void (*callback)())
{
    m_SpiACallback = callback;

    __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) data);
    DMA0SZ = size;
    if (!(DMA0CTL & DMAEN)) {
        DMA0SZ = size;
        DMA0CTL |= DMAEN;
    }

    UCA0IFG &= ~UCTXIFG;
    UCA0IFG |=  UCTXIFG;
}


/**Cathodes Spi*/
void (*m_SpiBCallback)()  = NULL;
void SpiBSend(uint8_t* data, uint16_t size, void (*callback)())
{
    m_SpiBCallback = callback;

    __data16_write_addr((unsigned short) &DMA1SA,(unsigned long) data);
    DMA1SZ = size;
    if (!(DMA1CTL & DMAEN)) {
        DMA1SZ = size;
        DMA1CTL |= DMAEN;
    }

    UCB0IFG &= ~UCTXIFG;
    UCB0IFG |=  UCTXIFG;
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=DMA_VECTOR
__interrupt void DmaIsr (void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(DMA_VECTOR))) DmaIsr (void)
#else
#error Compiler not supported!
#endif
{
    if (DMAIV & DMAIV_DMA0IFG) {
        if (m_SpiACallback)
        {
            (m_SpiACallback)();
        }
        return;
    }
    if (DMAIV & DMAIV_DMA1IFG) {
        if (m_SpiBCallback)
        {
            (m_SpiBCallback)();
        }
        return;
    }
    if (DMAIV & DMAIV_DMA2IFG) {
        return;
    }
}
