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


#define PANEL_WIDTH (32)
#define PANEL_HEIGHT (32)

#define PANEL_ROWS (3)
#define PANEL_COLS (3)

#define DISPLAY_WIDTH (PANEL_WIDTH * PANEL_COLS)
#define DISPLAY_HEIGTH (PANEL_HEIGHT * PANEL_ROWS)

#endif /* DEFINE_H_ */
