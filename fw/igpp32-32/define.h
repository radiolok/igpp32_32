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

#include <msp430.h>

#include "stdint.h"

#ifndef NULL
    #define NULL 0
#endif

#ifndef DEFINE_H_
#define DEFINE_H_

#define SPI_BUFFER_SIZE (32)
#define SPI_BUFFER_MASK (SPI_BUFFER_SIZE - 1)

#define PANEL_WIDTH (32)
#define PANEL_HEIGHT (32)

#define PANEL_ROWS (3)
#define PANEL_COLS (3)

#define DISPLAY_WIDTH (PANEL_WIDTH * PANEL_COLS)
#define DISPLAY_HEIGTH (PANEL_HEIGHT * PANEL_ROWS)

#define ANODE_BYTES ((PANEL_HEIGHT >> 3) * PANEL_ROWS)

#define CATHODE_BYTES ((PANEL_WIDTH >> 3) * PANEL_COLS)

#define PANEL_DATA_SIZE (ANODE_BYTES * DISPLAY_WIDTH)

enum activityType
{
    ActivityNone = 0,
    ActivityBufferDone = 1
};

//64 bytes buffer
typedef struct _dataBuffer_t
{
    uint8_t payload[60];
    uint8_t pos;
    uint8_t activity;
    uint8_t reserved1;
    uint8_t reserved2;
}dataBuffer_t;


#endif /* DEFINE_H_ */
