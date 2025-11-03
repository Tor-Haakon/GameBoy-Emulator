#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

#define P1 0xFF00
#define SB 0xFF01
#define SC 0xFF02
#define DIV 0xFF04
#define TIMA 0xFF05
#define TMA 0xFF06
#define TAC 0xFF07
#define IF 0xFF0F
#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14
#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19
#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E
#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23
#define NR50 0xFF24
#define NR51 0xFF25
#define NR52 0xFF26
#define LCDC 0xFF40
#define STAT 0xFF41
#define SCY 0xFF42
#define SCX 0xFF43
#define LY 0xFF44
#define LYC 0xFF45
#define DMA 0xFF46
#define BGP 0xFF47
#define OBP0 0xFF48 
#define OBP1 0xFF49
#define WY 0xFF4A
#define WX 0xFF4B

#define IE 0xFFFF

uint8_t memory[0x10000];

void init_memory(char* rom_path){
    FILE *rom_file = fopen(rom_path, "rb");
    if (rom_file == NULL) {
        printf("Could not open file!\n");
    }
    fseek(rom_file, 0, SEEK_END);
    int rom_length = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);
    fread(memory, 1, rom_length, rom_file);
    fclose(rom_file);

    if (memory[0x147] == 0x01) {
        
    }

    memory[P1] = 0xCF;
    memory[SB] = 0x00;
    memory[SC] = 0x7E;
    memory[DIV] = 0xAB;
    memory[TIMA] = 0x00;
    memory[TMA] = 0x00;
    memory[TAC] = 0xF8;
    memory[IF] = 0xCF;
    memory[NR10] = 0x80;
    memory[NR11] = 0xBF;
    memory[NR12] = 0xF3;
    memory[NR13] = 0xFF;
    memory[NR14] = 0xBF;
    memory[NR21] = 0x3F;
    memory[NR22] = 0x00;
    memory[NR23] = 0XFF;
    memory[NR24] = 0xBF;
    memory[NR30] = 0x7F;
    memory[NR31] = 0xFF;
    memory[NR32] = 0x9F;
    memory[NR33] = 0xFF;
    memory[NR34] = 0xBF;
    memory[NR41] = 0xFF;
    memory[NR42] = 0x00;
    memory[NR43] = 0x00;
    memory[NR44] = 0xBF;
    memory[NR50] = 0x77;
    memory[NR51] = 0xF3;
    memory[NR52] = 0xF1;
    memory[LCDC] = 0x11;
    memory[STAT] = 0x82;
    memory[SCY] = 0x00;
    memory[SCX] = 0x00;
    memory[LY] = 0x91;
    memory[LYC] = 0x00;
    memory[DMA] = 0xFF;
    memory[BGP] = 0xFC;
    memory[WY] = 0x00;
    memory[WX] = 0x00;

    uint8_t sprite[16] = {0x3C, 0x7E, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7E, 0x5E, 0x7E, 0x0A, 0x7C, 0x56, 0x38, 0x7C};
    for (int i = 0; i < 16; i++) {
        memory[0x8000+i] = sprite[i];
    }
    memory[0xFE00] = 16;
    memory[0xFE01] = 8;
    memory[0xFE02] = 0;
    memory[0xFE03] = 0;
    
}

#endif