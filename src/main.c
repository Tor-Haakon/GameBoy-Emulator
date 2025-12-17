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
#include "timer.h"

#define PROGRAM "Tetris.gb"

const struct color BLACK = {0x0F, 0x38, 0x0F};
const struct color DARK_GREY = {0x30, 0x62, 0x30};
const struct color LIGHT_GREY = {0x8B, 0xAC, 0x0F};
const struct color WHITE = {0x9B, 0xBC, 0x0F};

struct color colors[4];

struct log {
    uint8_t A, F, B , C, D, E, H, L;
    uint16_t SP, PC;
    uint8_t PCMEM1, PCMEM2, PCMEM3, PCMEM4;
};
int log_counter = 0;


// struct log log_array[1000];

const double CPU_CLOCK_HZ = 4194304;
int frame_start;
int frame_time;

const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000/60;

const int total_dots_per_frame = 70224;
int frame_dot_counter = 0;

uint16_t opcode = 0;
uint16_t last_opcode = 0;
int instruction_counter = 1;

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_FPoint points[500];

void log_current_state(char *file, struct log *log_array) {
    FILE *log_file = fopen(file, "a");
    if (log_file == NULL){
        printf("Could not open log_file");
        return;
    }
    for (int i = 0; i < log_counter; i++) {
        fprintf(log_file, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
                log_array[i].A, log_array[i].F, log_array[i].B, log_array[i].C, log_array[i].D, log_array[i].E, log_array[i].H, 
                log_array[i].L, log_array[i].SP, log_array[i].PC, log_array[i].PCMEM1, log_array[i].PCMEM2, log_array[i].PCMEM3, log_array[i].PCMEM4);
        
        log_array[i].A = 0;
        log_array[i].F = 0;
        log_array[i].B = 0;
        log_array[i].C = 0;
        log_array[i].D = 0;
        log_array[i].E = 0;
        log_array[i].H = 0;
        log_array[i].L = 0;
        log_array[i].SP = 0;
        log_array[i].PC = 0;
        log_array[i].PCMEM1 = 0;
        log_array[i].PCMEM2 = 0;
        log_array[i].PCMEM3 = 0;
        log_array[i].PCMEM4 = 0;
    }

    fclose(log_file);
}

void update_log_array(struct log* plog) {
    plog->A = cpu.A;
    plog->F = cpu.F;
    plog->B = cpu.B;
    plog->C = cpu.C;
    plog->D = cpu.D;
    plog->E = cpu.E;
    plog->H = cpu.H;
    plog->L = cpu.L;
    plog->SP = cpu.SP;
    plog->PC = cpu.PC;
    plog->PCMEM1 = memory[cpu.PC];
    plog->PCMEM2 = memory[cpu.PC+1];
    plog->PCMEM3 = memory[cpu.PC+2];
    plog->PCMEM4 = memory[cpu.PC+3];
}

void init_log_file(char *file){
    FILE* log_file = fopen(file, "w");
    if (log_file == NULL){
        printf("Could not open log_file");
        return;
    }
    fclose(log_file);
}


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

    init_log_file("logfile.txt");
    struct log *log_array = malloc(1000*sizeof(struct log));
    update_log_array(&log_array[log_counter]);
    free(log_array);

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
    frame_start = SDL_GetTicks();

    // struct log *log_array = malloc(20000*sizeof(struct log));
    // log_counter = 0;
    
    while (frame_dot_counter < total_dots_per_frame) {
        handle_input();
        
        // Fetch opcode
        last_opcode = opcode;
        opcode = memory[cpu.PC];
        if (opcode == 0xCB) {
            opcode = (opcode << 8) | memory[cpu.PC+1];
        }

        // update_log_array(&log_array[log_counter]);
        // log_counter += 1;
        // printf("%d\n", memory[0xff00]);

        int M_cycles;
        M_cycles = cpu_execute(opcode);
        M_cycles += handle_interrupts();
        if (lcd_enable())
            ppu_execute(4*M_cycles);
        frame_dot_counter += 4*M_cycles;

        increment_timers(M_cycles);

        instruction_counter += 1;
    }
    frame_dot_counter = 0;
    
    // log_current_state("logfile.txt", log_array);
    // free(log_array);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);  /* dark gray, full alpha */
    SDL_RenderClear(renderer);  /* start with a blank canvas. */

      /* blue, full alpha */
    SDL_FRect rect;
    for (int i = 0; i < SCR_HEIGHT; i++) {
        for (int j = 0; j < SCR_WIDTH; j++) {
            struct color current_color = colors[frame_buffer[i][j]];
            uint8_t red = current_color.red;
            uint8_t green = current_color.green;
            uint8_t blue = current_color.blue;
            rect.x = j*PIXEL_SIZE;
            rect.y = i*PIXEL_SIZE;
            rect.w = PIXEL_SIZE;
            rect.h = PIXEL_SIZE;
            if (!SDL_SetRenderDrawColor(renderer, red, green, blue, SDL_ALPHA_OPAQUE)){
                printf("%s \n", SDL_GetError());
            }
            if (!SDL_RenderFillRect(renderer, &rect)){
                printf("%s \n", SDL_GetError());
            };
        }
    }
    
    
    // SDL_RenderFillRect(renderer, &rect);
    if (!SDL_RenderPresent(renderer))
        printf("%s \n", SDL_GetError());

    frame_time = SDL_GetTicks() - frame_start;
    if (FRAME_DELAY > frame_time) {
        SDL_Delay(FRAME_DELAY - frame_time);
    }
    // printf("%d ", frame_time);

    return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}