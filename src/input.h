#ifndef INPUT_H
#define INPUT_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include "gbmemory.h"

void handle_input() {
    const bool* key_state = SDL_GetKeyboardState(NULL);

    if (!(memory[P1] & 0x10)) {
        if (key_state[SDL_SCANCODE_S]) {
            if (memory[P1] & 8) memory[IF] |= 0x10;
            memory[P1] &= ~8;
        }
        else
            memory[P1] |= 8;

        if (key_state[SDL_SCANCODE_W]) {
            if (memory[P1] & 4) memory[IF] |= 0x10;
            memory[P1] &= ~4;
        }
        else
            memory[P1] |= 4;

        if (key_state[SDL_SCANCODE_A]) {
            if (memory[P1] & 2) memory[IF] |= 0x10;
            memory[P1] &= ~2;
        }
        else
            memory[P1] |= 2;

        if (key_state[SDL_SCANCODE_D]) {
            if (memory[P1] & 1) memory[IF] |= 0x10;
            memory[P1] &= ~1;
        }
        else
            memory[P1] |= 1;
    }
    if (!(memory[P1] & 0x20)) {
        if (key_state[SDL_SCANCODE_X]) {
            if (memory[P1] & 8) memory[IF] |= 0x10;
            memory[P1] &= ~8;
        }
        else
            memory[P1] |= 8;

        if (key_state[SDL_SCANCODE_C]) {
            if (memory[P1] & 4) memory[IF] |= 0x10;
            memory[P1] &= ~4;
        }
        else
            memory[P1] |= 4;

        if (key_state[SDL_SCANCODE_J]) {
            if (memory[P1] & 2) memory[IF] |= 0x10;
            memory[P1] &= ~2;
        }
        else
            memory[P1] |= 2;

        if (key_state[SDL_SCANCODE_K]) {
            if (memory[P1] & 1) memory[IF] |= 0x10;
            memory[P1] &= ~1;
        }
        else
            memory[P1] |= 1;
    }

    if (memory[P1] & 0x30) {
        memory[P1] &= 0xF;
    }
}

#endif