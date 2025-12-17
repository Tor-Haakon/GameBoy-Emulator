#ifndef TIMER_H
#define TIMER_H

#include "gbmemory.h"
int DIV_counter = 0;
int TIMA_counter = 0;

void increment_timers(int M_cycles) {
    for (int i = 0; i < M_cycles; i++) {
        DIV_counter += 1;
        if (DIV_counter >= 64) {
            memory[DIV] += 1;
            DIV_counter = 0;
        }
    }
    int increment_rate;
    if (memory[TAC] & 3 == 0)
        increment_rate = 256;
    else if (memory[TAC] & 3 == 1)
        increment_rate = 4;
    else if (memory[TAC] & 3 == 2)
        increment_rate = 16;
    else if (memory[TAC] & 3 == 3)
        increment_rate == 64;

    if (memory[TAC] & 4) {
        for (int i = 0; i < M_cycles; i++) {
            TIMA_counter += 1;
            if (TIMA_counter >= increment_rate) {
                if (memory[TAC] == 0xFF) {
                    memory[TAC] = memory[TMA];
                    memory[IF] |= 4;
                }
                memory[TAC] += 1;
                TIMA_counter = 0;
            }
        }
    }
}

#endif