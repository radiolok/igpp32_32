/*
 * define.h
 *
 *  Created on: 9 сент. 2020 г.
 *      Author: radiolok
 */

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
