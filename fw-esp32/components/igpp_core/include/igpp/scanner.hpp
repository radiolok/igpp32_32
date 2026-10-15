// Column scan sequencer for the IGPP-32/32 display.
//
// Reproduces the sequence of the MSP430 firmware (master, Igpp.c) one to one:
//
//   1. anode MR pulse, shift 12 x 0xFF into the anode chain (blanking);
//   2. move the cathode one step (at column 0: cathode MR pulse, shift in a 1);
//   3. latch both chains            -> anodes dark, new cathode selected;
//   4. shift the column data into the anode chain;
//   5. latch both chains            -> the column is lit until the next step.
//
// Every SPI transfer must be complete (all bits shifted out) before the
// following latch, as `UCBUSY` was polled on the MSP430.
//
// The hardware access goes through `Port`, so the same code runs in the
// firmware (real GPIO/SPI) and in host tests (event recorder). Port contract:
//
//   void anode_clear();                   // MR pulse on the anode chain
//   void anode_send(const uint8_t* col);  // 12 bytes, returns when shifted out
//   void cathode_start();                 // MR pulse + shift a single 1 in
//   void cathode_clock();                 // shift the cathode chain by one
//   void latch_all();                     // LATCH pulse on both chains
//   void cathode_off();                   // MR + latch: no cathode selected
#pragma once

#include <atomic>
#include <cstdint>

#include "igpp/frame.hpp"

namespace igpp {

template <class Port>
class Scanner {
public:
    explicit Scanner(Port& port) : port_(port) {
        fill(buffers_[0], false);
        fill(buffers_[1], false);
    }

    // One column of the scan. Called with a fixed period from the scan loop.
    void step() {
        port_.anode_clear();
        port_.anode_send(kBlankColumn.data());

        if (++chain_pos_ >= kWidth) {
            chain_pos_ = 0;
            if (pending_.load(std::memory_order_acquire)) {
                front_ ^= 1;
                pending_.store(false, std::memory_order_release);
            }
            frames_.fetch_add(1, std::memory_order_relaxed);
            port_.cathode_start();
        } else {
            port_.cathode_clock();
        }
        port_.latch_all();

        const Frame& f = buffers_[front_];
        port_.anode_send(column_ptr(f, frame_column_for_chain(chain_pos_)));
        port_.latch_all();
    }

    // Writer side. back() may be written only while swap_pending() is false.
    Frame& back() { return buffers_[front_ ^ 1]; }
    void publish() { pending_.store(true, std::memory_order_release); }
    bool swap_pending() const { return pending_.load(std::memory_order_acquire); }

    const Frame& front() const { return buffers_[front_]; }
    uint32_t frames() const { return frames_.load(std::memory_order_relaxed); }
    int chain_pos() const { return chain_pos_; }

    // Emergency stop: deselect every cathode. Safe to call from another core.
    void cathode_off() { port_.cathode_off(); }

private:
    Port& port_;
    Frame buffers_[2];
    int front_ = 0;
    int chain_pos_ = kWidth;  // the first step() starts a new frame
    std::atomic<bool> pending_{false};
    std::atomic<uint32_t> frames_{0};
};

}  // namespace igpp
