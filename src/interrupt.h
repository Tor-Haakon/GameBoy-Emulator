#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "cpu.h"

uint8_t handle_interrupts() {
    if (IME_flag == 0)
        return 0;
    else if (memory[IE] & memory[IF] == 0)
        return 0;

    memory[cpu.SP-1] = cpu.PC >> 8;
    memory[cpu.SP-2] = cpu.PC & 0xFF;
    cpu.SP -= 2;

    if (memory[IE] & 0x1 && memory[IF] & 0x1) {
        cpu.PC = 0x40;
        memory[IF] &= ~0x1;
    }
    else if (memory[IE] & 0x2 && memory[IF] & 0x2) {
        cpu.PC = 0x48;
        memory[IF] &= ~0x2;
    }
    else if (memory[IE] & 0x4 && memory[IF] & 0x4) {
        cpu.PC = 0x50;
        memory[IF] &= ~0x4;
    }
    else if (memory[IE] & 0x8 && memory[IF] & 0x8) {
        cpu.PC = 0x58;
        memory[IF] &= ~0x8;
    }
    else if (memory[IE] & 0x10 && memory[IF] & 0x10) {
        cpu.PC = 0x58;
        memory[IF] &= ~0x10;
    }
    return 5;
}

#endif