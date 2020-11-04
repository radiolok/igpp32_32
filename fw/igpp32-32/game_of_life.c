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

#include <game_of_life.h>



uint8_t* currentEpoch = NULL;
uint8_t* newEpoch = NULL;

static uint16_t Turns = 0;      // counter for turns
static uint16_t NoChanges = 0; // counter for turns without changes


void checkEpoch() {

    countEpoch();

  Turns++;

  // reset the grid if no changes have occured recently
  // for when the game enters a static stable state
  if (NoChanges > NO_CHANGES_RESET) {
    reset_grid();
  }
  // reset the grid if the loop has been running a long time
  // for when the game cycles between a few stable states
  if (Turns > TURNS_MAX) {
    reset_grid();
  }

  display_grid();
}

inline uint8_t getArrayBit(uint8_t* ptr, uint16_t x, uint16_t y)
{
    uint8_t byte = y >> 3;
    uint8_t pos = y & 0x07;
    uint8_t* cell = ptr + ANODE_BYTES * x + byte;
    return ((*cell) & (1 << pos)) ? 1 : 0;
}

inline void setArrayBit(uint8_t* ptr, uint16_t x, uint16_t y, uint8_t set)
{
    uint8_t byte = y >> 3;
    uint8_t pos = y & 0x07;
    uint8_t* cell = ptr + ANODE_BYTES * x + byte;
    if (set != 0)
    {
        *cell |= (1 << pos);
    }
    else
    {
        *cell &= ~(1<< pos);
    }
}

// play game of life
void countEpoch() {
  /*
    1. Any live cell with fewer than two neighbours dies, as if by loneliness.
    2. Any live cell with more than three neighbours dies, as if by overcrowding.
    3. Any live cell with two or three neighbours lives, unchanged, to the next generation.
    4. Any dead cell with exactly three neighbours comes to life.
    */
    for (uint16_t x = 0; x < DISPLAY_WIDTH ; ++x)
    {
        for (uint16_t y = 0; y < DISPLAY_HEIGTH; ++y)
        {
            uint16_t neighboughs = count_neighboughs(x, y);
            if ((neighboughs == 3) ||
               ((neighboughs == 2) && (getArrayBit(currentEpoch, x, y))))
            {
                setArrayBit(newEpoch, x, y, 1);
            }
            else
            {
                setArrayBit(newEpoch, x, y, 0);
            }
        }
    }
    uint16_t i = 0;
    for (; i < PANEL_DATA_SIZE; ++i)
    {
        if (*(newEpoch + i) != *(currentEpoch))
        {
            break;
        }
    }
    if (i == PANEL_DATA_SIZE)
    {
        NoChanges++;
    }
}



// count the number of neighbough live cells for a given cell
uint8_t count_neighboughs(uint8_t x, uint8_t y) {
    uint8_t count = 0;
  // -- Row above us ---
  if (y > 0) {
    // above left
    if (x > 0) {
        count += getArrayBit(currentEpoch, x-1, y-1);
    }
    // above
    count += getArrayBit(currentEpoch, x, y-1);
    // above right
    if ((x + 1) < DISPLAY_WIDTH) {
      count += getArrayBit(currentEpoch, x+1, y-1);
    }
  }

  // -- Same row -------
  // left
  if (x > 0) {
    count += getArrayBit(currentEpoch, x-1, y);
  }
  // right
  if ((x + 1) < 8) {
      count += getArrayBit(currentEpoch, x+1, y);
  }

  // -- Row below us ---
  if ((y + 1) < DISPLAY_HEIGTH) {
    // below left
    if (x > 0) {
        count += getArrayBit(currentEpoch, x-1, y+1);
    }
    // below
    count += getArrayBit(currentEpoch, x, y+1);
    // below right
    if ((x + 1) < DISPLAY_WIDTH) {
      count += getArrayBit(currentEpoch, x+1, y+1);
    }
  }

  return count;
}

void init_grid()
{
    currentEpoch = igppCurrentBufferPtr();
    reset_grid();
}

// reset the grid
// we could set it all to zero then flip some bits on
// but that leads to some predictable games I see quite frequently
// instead, keep previous game state and flip some bits on
void reset_grid() {
  NoChanges = 0;
  Turns = 0;
  uint16_t* ptr = (uint16_t*)currentEpoch;
  const uint16_t panelSize = PANEL_DATA_SIZE/2;
  static uint16_t value = 61169;
  for (uint16_t i = 0; i < panelSize; ++i)
  {
      value =  prand(value);
      ptr[i] = value;
  }
  //TODO: random should be here
  memset(currentEpoch, 0, PANEL_DATA_SIZE);
  display_grid();
}

// display the current grid to the LED matrix
void display_grid() {
    igppChangeBuffer();
    currentEpoch = igppCurrentBufferPtr();
    newEpoch = igppLoadBufferPtr();
}
