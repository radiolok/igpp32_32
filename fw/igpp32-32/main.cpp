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

#include "define.h"
#include "driverlib/MSP430F5xx_6xx/wdt_a.h"
#include "driverlib/MSP430F5xx_6xx/ucs.h"
#include "driverlib/MSP430F5xx_6xx/pmm.h"

#include "driverlib.h"

#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"                 // USB-specific functions
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"

#include "Igpp.h"

// Global flags set by events
volatile uint8_t bCDCDataReceived_event = FALSE;  // Flag set by event handler to
                                               // indicate data has been
                                               // received into USB buffer

#define BUFFER_SIZE 256
char dataBuffer[BUFFER_SIZE] = "";
char nl[2] = "\n";
uint16_t count;

int main(void)
{
	/*
    XT1 = DCO, 24MHz

    MCLK = 24MHz

    SMCLK = 6MHz

    F_scan = 40kHz

    XT2 = 16MHz - for USB Clock

	 */
  // Hold the watchdog timer.
  WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);

  // MSP430 USB requires a Vcore setting of at least 2.
  PMM_setVCore(PMM_CORE_LEVEL_3);

      P2DIR |= BIT2;                            // SMCLK set out to pins
      P2SEL |= BIT2;

    UCSCTL3 = SELREF_2;                       // Set DCO FLL reference = REFO
    UCSCTL4 |= SELA_2;                        // Set ACLK = REFO

    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_7;                      // Select DCO range 50MHz operation
    UCSCTL2 = FLLD_0 + 731;                   // Set DCO Multiplier for 24MHz
                                            // (N + 1) * FLLRef = Fdco
                                            // (731 + 1) * 32768 = 24MHz
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    __delay_cycles(782000);

    UCSCTL4 |= SELM_5;                       //MCLK = XT2 = 26MHz
    UCSCTL5 |= DIVM_0;                       //f_dco/1

    UCSCTL4 |= SELS_5;                      // SMCLK = XT2 = 26MHz
    UCSCTL5 |= DIVS_0;                      // f_dco / 1

    USB_setup(TRUE, TRUE); // Init USB & events; if a host is present, connect

    igppInit();

    __enable_interrupt();  // Enable interrupts globally

        while (1)
        {
            uint8_t ReceiveError = 0;
            uint8_t SendError = 0;
            uint16_t count;

            // Check the USB state and directly main loop accordingly
            switch (USB_getConnectionState())
            {
                // This case is executed while your device is enumerated on the
                // USB host
                case ST_ENUM_ACTIVE:

                    // Sleep if there are no bytes to process.
                    __disable_interrupt();
                    if (!USBCDC_getBytesInUSBBuffer(CDC0_INTFNUM)) {

                        // Enter LPM0 until awakened by an event handler
                        __bis_SR_register(LPM0_bits + GIE);
                    }

                    __enable_interrupt();

                    // Exit LPM because of a data-receive event, and
                    // fetch the received data
                    if (bCDCDataReceived_event){

                        // Clear flag early -- just in case execution breaks
                        // below because of an error
                        bCDCDataReceived_event = FALSE;

                        count = USBCDC_receiveDataInBuffer((uint8_t*)dataBuffer,
                            BUFFER_SIZE,
                            CDC0_INTFNUM);

                        // Count has the number of bytes received into dataBuffer
                        // Echo back to the host.
                        if (USBCDC_sendDataInBackground((uint8_t*)dataBuffer,
                                count, CDC0_INTFNUM, 1)){
                            // Exit if something went wrong.
                            SendError = 0x01;
                            break;
                        }
                    }
                    break;

                // These cases are executed while your device is disconnected from
                // the host (meaning, not enumerated); enumerated but suspended
                // by the host, or connected to a powered hub without a USB host
                // present.
                case ST_PHYS_DISCONNECTED:
                case ST_ENUM_SUSPENDED:
                case ST_PHYS_CONNECTED_NOENUM_SUSP:
                    __bis_SR_register(LPM3_bits + GIE);
                    _NOP();
                    break;

                // The default is executed for the momentary state
                // ST_ENUM_IN_PROGRESS.  Usually, this state only last a few
                // seconds.  Be sure not to enter LPM3 in this state; USB
                // communication is taking place here, and therefore the mode must
                // be LPM0 or active-CPU.
                case ST_ENUM_IN_PROGRESS:
                default:;
            }

            if (ReceiveError || SendError){
                // TO DO: User can place code here to handle error
            }
        }  //while(1)
}

/*
 * ======== UNMI_ISR ========
 */
#if defined(__TI_COMPILER_VERSION__) || (__IAR_SYSTEMS_ICC__)
#pragma vector = UNMI_VECTOR
__interrupt void UNMI_ISR (void)
#elif defined(__GNUC__) && (__MSP430__)
void __attribute__ ((interrupt(UNMI_VECTOR))) UNMI_ISR (void)
#else
#error Compiler not found!
#endif
{
    switch (__even_in_range(SYSUNIV, SYSUNIV_BUSIFG ))
    {
        case SYSUNIV_NONE:
            __no_operation();
            break;
        case SYSUNIV_NMIIFG:
            __no_operation();
            break;
        case SYSUNIV_OFIFG:
            UCS_clearFaultFlag(UCS_XT2OFFG);
            UCS_clearFaultFlag(UCS_DCOFFG);
            SFR_clearInterrupt(SFR_OSCILLATOR_FAULT_INTERRUPT);
            break;
        case SYSUNIV_ACCVIFG:
            __no_operation();
            break;
        case SYSUNIV_BUSIFG:
            // If the CPU accesses USB memory while the USB module is
            // suspended, a "bus error" can occur.  This generates an NMI.  If
            // USB is automatically disconnecting in your software, set a
            // breakpoint here and see if execution hits it.  See the
            // Programmer's Guide for more information.
            SYSBERRIV = 0; // clear bus error flag
            USB_disable(); // Disable
    }
}
