/*BSD 2-Clause License

Copyright (c) 2020, Artem Kashkanov
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#include <string.h>

#include "define.h"
#include "driverlib.h"

#include "Igpp.h"

#include "USB_config/descriptors.h"
#include "USB_API/USB_Common/device.h"
#include "USB_API/USB_Common/usb.h"                 // USB-specific functions
#include "USB_API/USB_CDC_API/UsbCdc.h"
#include "USB_app/usbConstructs.h"

#include "hal.h"

// Global flags set by events
volatile uint8_t bCDCDataReceived_event = FALSE;  // Flag set by event handler to
                                               // indicate data has been
                                               // received into USB buffer

#define BUFFER_SIZE 1152
char nl[2] = "\n";
uint16_t count;

int main(void)
{
	P6DIR |= BIT0;
	P6OUT &= ~BIT0;

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

  USBHAL_initPorts();           // Config GPIOS for low-power (output low)
  USBHAL_initClocks(26000000);   // Config clocks. MCLK=SMCLK=FLL=8MHz; ACLK=REFO=32kHz

    //USB_setup(TRUE, TRUE); // Init USB & events; if a host is present, connect

    __enable_interrupt();  // Enable interrupts globally

    igppInit();
    uint16_t rotation = 0;
        while (1)
        {
            __bis_SR_register(LPM0_bits + GIE);
           /*uint8_t ReceiveError = 0;
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
                        uint8_t* currentBuffer = igppLoadBufferPtr();
                        count = USBCDC_receiveDataInBuffer((uint8_t*)currentBuffer,
                            BUFFER_SIZE,
                            CDC0_INTFNUM);

                        // Count has the number of bytes received into dataBuffer
                        // Echo back to the host.
                        if (USBCDC_sendDataInBackground((uint8_t*)currentBuffer,
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
                    __bis_SR_register(LPM0_bits + GIE);
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
            }*/
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
