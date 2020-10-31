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
