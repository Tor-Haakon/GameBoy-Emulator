#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "cpu.h"
#include "ppu.h"
#include "input.h"
#include "interrupt.h"

#define PROGRAM "tetris.gb"

const struct color BLACK = {0x0F, 0x38, 0x0F};
const struct color DARK_GREY = {0x30, 0x62, 0x30};
const struct color LIGHT_GREY = {0x8B, 0xAC, 0x0F};
const struct color WHITE = {0x9B, 0xBC, 0x0F};

struct color colors[4];

const double CPU_CLOCK_HZ = 4194304;
double current_time;
double last_time;
double delta_time;
double time_accumulator;

const int total_dots_per_frame = 70224;
int frame_dot_counter = 0;

uint16_t opcode = 0;
uint16_t last_opcode = 0;
int instruction_counter = 0;

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_FPoint points[500];



/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{

    // SDL_SetAppMetadata("Example Renderer Primitives", "1.0", "com.example.renderer-primitives");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("chip-8", SCR_WIDTH*PIXEL_SIZE, SCR_HEIGHT*PIXEL_SIZE, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    init_memory("ROMS/"PROGRAM);
    init_cpu_registers();

    current_time = 0;
    last_time = 0;
    time_accumulator = 0;

    colors[0] = WHITE;
    colors[1] = LIGHT_GREY;
    colors[2] = DARK_GREY;
    colors[3] = BLACK;

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{    
    current_time = SDL_GetTicks();
    delta_time = current_time - last_time;
    last_time = current_time;

    printf("%f ", delta_time);
    
    while (frame_dot_counter < total_dots_per_frame) {
        handle_input();
        
        // Fetch opcode
        last_opcode = opcode;
        opcode = memory[cpu.PC];
        if (opcode == 0xCB) {
            opcode = (opcode << 8) | memory[cpu.PC+1];
        }

        int M_cycles;
        M_cycles = cpu_execute(opcode);
        M_cycles += handle_interrupts();
        if (lcd_enable())
            ppu_execute(4*M_cycles);
        frame_dot_counter += 4*M_cycles;

        instruction_counter += 1;
    }
    frame_dot_counter = 0;
    
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

      /* blue, full alpha */
    SDL_FRect rect;
    for (int i = 0; i < SCR_HEIGHT; i++) {
        for (int j = 0; j < SCR_WIDTH; j++) {
            struct color current_color = colors[frame_buffer[i][j]];
            rect.x = j*PIXEL_SIZE;
            rect.y = i*PIXEL_SIZE;
            rect.w = PIXEL_SIZE;
            rect.h = PIXEL_SIZE;
            SDL_SetRenderDrawColor(renderer, current_color.red, current_color.green, current_color.blue, SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    
    
    // SDL_RenderFillRect(renderer, &rect);
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}