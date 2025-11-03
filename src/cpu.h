#ifndef CPU_H
#define CPU_H

#include "gbmemory.h"
#include "input.h"
#include "ppu.h"

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

struct cpu {
    union {
        struct {
            uint8_t F;
            uint8_t A;
        };
        uint16_t AF;
    };
    union {
        struct {
            uint8_t C;
            uint8_t B;
        };
        uint16_t BC;
    };
    union {
        struct {
            uint8_t E;
            uint8_t D;
        };
        uint16_t DE;
    };
    union {
        struct {
            uint8_t L;
            uint8_t H;
        };
        uint16_t HL;
    };
    uint16_t SP;
    uint16_t PC;
};

struct cpu cpu;

int IME_flag = 0;
int IME_flag_next = 0;

void init_cpu_registers() {
    cpu.AF = 0x01B0;
    cpu.BC = 0x0013;
    cpu.DE = 0x00D8;
    cpu.HL = 0x014D;
    cpu.SP = 0xFFFE;
    cpu.PC = 0x0100;
}

int is_set(uint8_t flag){
    return (cpu.F & flag) != 0;
}

void set_flag(uint8_t flag){    
    cpu.F |= flag;
}

void clear_flag(uint8_t flag){
    cpu.F &= ~flag;
}

void dma_transfer(uint8_t transfer_source) {
    uint16_t start_addr = (transfer_source << 8);
    for (int i = 0; i < 160; i++) {
        memory[0xFE00+i] = memory[start_addr + i];
    }
}

uint8_t read_from_memory(uint16_t addr) {
    if (addr >= 0x8000 && addr <= 0x9FFF && get_ppu_mode() == 3)
        return 0xFF;
    if (addr >= 0xFE00 && addr <= 0xFE9F && (get_ppu_mode() == 2 || get_ppu_mode() == 3))
        return 0xFF;
    return memory[addr];
}

void write_to_memory(uint16_t addr, uint8_t value){
    if (addr == DMA) {
        dma_transfer(value);
        return;
    }
    if (addr < 0x8000) 
        return;
    if (addr >= 0x8000 && addr <= 0x9FFF && get_ppu_mode() == 3)
        return;
    if (addr >= 0xFE00 && addr <= 0xFE9F && (get_ppu_mode() == 2 || get_ppu_mode() == 3))
        return;
    if (addr == LCDC) {
        if (value & 0x80) {
            set_ppu_mode(2);
            memory[LY] = 0;
            scanline_dot_counter = 0;
        }
        else {
            set_ppu_mode(0);
        }
    }
    memory[addr] = value;
    return;
}

uint8_t cpu_execute(uint16_t opcode) {
    // printf("%d\n", opcode);

    int M_cycles;
    switch (opcode) {
        // Load instructions

        // LD r8,r8
        case(0x40): case(0x41): case(0x42): case(0x43): case(0x44): case(0x45): case(0x47):
        case(0x48): case(0x49): case(0x4A): case(0x4B): case(0x4C): case(0x4D): case(0x4F):
        case(0x50): case(0x51): case(0x52): case(0x53): case(0x54): case(0x55): case(0x57):
        case(0x58): case(0x59): case(0x5A): case(0x5B): case(0x5C): case(0x5D): case(0x5F):
        case(0x60): case(0x61): case(0x62): case(0x63): case(0x64): case(0x65): case(0x67):
        case(0x68): case(0x69): case(0x6A): case(0x6B): case(0x6C): case(0x6D): case(0x6F):
        case(0x78): case(0x79): case(0x7A): case(0x7B): case(0x7C): case(0x7D): case(0x7F): {

            uint8_t right_r8;
            uint8_t *left_r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            right_r8 = *regs[(opcode - 0x40) % 8];
            left_r8p = regs[(opcode - 0x40) / 8];

            *left_r8p = right_r8;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // LD r8,n8
        case(0x06): case(0x16): case(0x26): case(0x0E):
        case(0x1E): case(0x2E): case(0x3E): {
            uint8_t n8 = memory[cpu.PC+1];
            uint8_t* r8;
            if (opcode == 0x06) r8 = &cpu.B;
            else if (opcode == 0x16) r8 = &cpu.D;
            else if (opcode == 0x26) r8 = &cpu.H;
            else if (opcode == 0x0E) r8 = &cpu.C;
            else if (opcode == 0x1E) r8 = &cpu.E;
            else if (opcode == 0x2E) r8 = &cpu.L;
            else r8 = &cpu.A;

            *r8 = n8;
            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // LD [HL],n8
        case(0x36): {
            uint8_t n8 = memory[cpu.PC+1];
            write_to_memory(cpu.HL, n8);

            cpu.PC += 2;
            M_cycles = 3;
            break;
        }

        // LD r16,n16
        case(0x01): case(0x11): case(0x21): case(0x31): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            uint16_t* r16p;
            if (opcode == 0x01) r16p = &cpu.BC;
            else if (opcode == 0x11) r16p = &cpu.DE;
            else if (opcode == 0x21) r16p = &cpu.HL;
            else r16p = &cpu.SP;

            *r16p = n16;
            cpu.PC += 3;
            M_cycles = 3;
            break;
        }

        // LD [HL],r8
        case(0x70): case(0x71): case(0x72): case(0x73):
        case(0x74): case(0x75): case(0x77): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0x70];

            write_to_memory(cpu.HL, r8);
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD r8,[HL]
        case(0x46): case(0x56): case(0x66): case(0x4E):
        case(0x5E): case(0x6E): case(0x7E): {
            uint8_t* r8p;
            if (opcode == 0x46) r8p = &cpu.B;
            else if (opcode == 0x4E) r8p = &cpu.C;
            else if (opcode == 0x56) r8p = &cpu.D;
            else if (opcode == 0x5E) r8p = &cpu.E;
            else if (opcode == 0x66) r8p = &cpu.H;
            else if (opcode == 0x6E) r8p = &cpu.L;
            else r8p = &cpu.A;

            *r8p = read_from_memory(cpu.HL);
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD [r16],A
        case(0x02): case(0x12): {
            uint16_t r16;
            if (opcode == 0x02) r16 = cpu.BC;
            else r16 = cpu.DE;

            write_to_memory(r16, cpu.A);
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD [n16],A
        case(0xEA): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            write_to_memory(n16, cpu.A);
            cpu.PC += 3;
            M_cycles = 4;
            break;
        }

        // LDH [n16],A
        case(0xE0): {
            uint16_t n16 = 0xFF00 + memory[cpu.PC+1];
            write_to_memory(n16, cpu.A);
            cpu.PC += 2;
            M_cycles = 3;
            break;
        }

        // LDH [C],A
        case(0xE2): {
            write_to_memory(0xFF00 + cpu.C, cpu.A);
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD A,[r16]
        case(0x0A): case(0x1A): {
            uint16_t r16;
            if (opcode == 0x0A) r16 = cpu.BC;
            else r16 = cpu.DE;

            cpu.A = read_from_memory(r16);
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD A,[n16]
        case(0xFA): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            cpu.A = read_from_memory(n16);
            cpu.PC += 3;
            M_cycles = 4;
            break;
        }

        // LDH A,[n16]
        case(0xF0): {
            uint16_t n16 = 0xFF00 + memory[cpu.PC+1];
            cpu.A = read_from_memory(n16);
            cpu.PC += 2;
            M_cycles = 3;
            break;
        }

        // LDH A,[C]
        case(0xF2): {
            cpu.A = read_from_memory(0xFF00 + cpu.C);
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD [HLI]/[HLD],A
        case(0x22): case(0x32): {
            write_to_memory(cpu.HL, cpu.A);
            if (opcode == 0x22) cpu.HL += 1;
            else cpu.HL -= 1;
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD A,[HLI]/[HLD]
        case(0x2A): case(0x3A): {
            cpu.A = read_from_memory(cpu.HL);
            if (opcode == 0x2A) cpu.HL += 1;
            else cpu.HL -= 1;
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // LD [n16],SP
        case(0x08): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            write_to_memory(n16, cpu.SP & 0xFF);
            write_to_memory(n16+1, cpu.SP >> 8);
            cpu.PC += 3;
            M_cycles = 5;
            break;
        }

        // LD HL,SP+e8
        case(0xF8): {
            int8_t e8;
            uint8_t n8 = memory[cpu.PC+1];
            if (n8 & 0x80) e8 = -(n8 & 0x7F);
            else e8 = n8 & 0x7F;
            uint16_t result = cpu.SP + e8;

            cpu.F = 0;
            if (((cpu.SP & 0xF) + (e8 & 0xF)) > 0xF) set_flag(FLAG_H);
            if (result > 0xFF) set_flag(FLAG_C);

            cpu.HL = result;
            cpu.PC += 2;
            M_cycles = 3;
            break;
        }

        // LD SP,HL
        case(0xF9): {
            cpu.SP = cpu.HL;
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // 8-bit arithmetic instructions

        // ADC A, r8
        case(0x88): case(0x89): case(0x8A): case(0x8B):
        case(0x8C): case(0x8D): case(0x8F): {

            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0x88];

            uint8_t carry = is_set(FLAG_C);
            uint16_t result = r8 + carry + cpu.A;

            cpu.F = 0;
            if ((result & 0xFF) == 0) set_flag(FLAG_Z);
            // N flag is alwayz zero for ADC
            if (((cpu.A & 0xF) + (r8 & 0xF) + carry) & 0x10) set_flag(FLAG_H);
            if (result > 0xFF) set_flag(FLAG_C);

            cpu.A = (uint8_t)result;
            cpu.PC += 1;       
            M_cycles = 1;
            break;
        }

        // ADC A, [HL]/n8
        case(0x8E): case(0xCE): {
            uint8_t n8;
            if (opcode == 0x8E) n8 = memory[cpu.PC+1];
            else n8 = read_from_memory(cpu.HL);
            uint8_t carry = is_set(FLAG_C);
            uint16_t result = n8 + carry + cpu.A;

            cpu.F = 0;
            if ((result & 0xFF) == 0) set_flag(FLAG_Z);
            // N flag is alwayz zero for ADC
            if (((cpu.A & 0xF) + (n8 & 0xF) + carry) & 0x10) set_flag(FLAG_H);
            if (result > 0xFF) set_flag(FLAG_C);  

            cpu.A = (uint8_t)result;
            if (opcode == 0x8E) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }
        
        // ADD A, r8
        case(0x80): case(0x81): case(0x82): case(0x83):
        case(0x84): case(0x85): case(0x87): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0x80];
            uint16_t result = r8 + cpu.A;

            cpu.F = 0;
            if ((result & 0xFF) == 0) set_flag(FLAG_Z);
            // N flag is alwayz zero for ADD
            if (((cpu.A & 0xF) + (r8 & 0xF)) & 0x10) set_flag(FLAG_H);
            if (result > 0xFF) set_flag(FLAG_C);

            cpu.A = (uint8_t)result;
            cpu.PC += 1;
            M_cycles = 1;
        }

        // ADD A, [HL]/n8
        case(0x86): case(0xC6): {
            uint8_t n8;
            if (opcode == 0x86) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC+1];
            uint16_t result = n8 + cpu.A;

            cpu.F = 0;
            if ((result & 0xFF) == 0) set_flag(FLAG_Z);
            // N flag is alwayz zero for ADD
            if (((cpu.A & 0xF) + (n8 & 0xF)) & 0x10) set_flag(FLAG_H);
            if (result > 0xFF) set_flag(FLAG_C);

            cpu.A = (uint8_t)result;
            if (opcode == 0x86) cpu.PC += 2;
            else cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // CP A, r8
        case(0xB8): case(0xB9): case(0xBA): case(0xBB): 
        case(0xBC): case(0xBD): case(0xBF): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0xB8];

            cpu.F = 0;
            if (cpu.A == r8) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((cpu.A & 0xF) < (r8 & 0xF)) set_flag(FLAG_H);
            if (cpu.A < r8) set_flag(FLAG_C);

            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // CP A, [HL]/n8
        case(0xBE): case(0xFE): {
            uint8_t n8;
            if (opcode == 0xBE) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC+1];

            cpu.F = 0;
            if (cpu.A == n8) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((cpu.A & 0xF) < (n8 & 0xF)) set_flag(FLAG_H);
            if (cpu.A < n8) set_flag(FLAG_C);

            if (opcode == 0xBE) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }
        
        // DEC r8
        case(0x05): case(0x15): case(0x25): case(0x0D):
        case(0x1D): case(0x2D): case(0x3D): {
            uint8_t* reg;
            if (opcode == 0x05) reg = &cpu.B;
            else if (opcode == 0x0D) reg = &cpu.C;
            else if (opcode == 0x15) reg = &cpu.D;
            else if (opcode == 0x1D) reg = &cpu.E;
            else if (opcode == 0x25) reg = &cpu.H;
            else if (opcode == 0x2D) reg = &cpu.L;
            else reg = &cpu.A;

            uint8_t r8 = *reg;
            uint8_t result = r8-1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_H);
            if (result == 0) 
                set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((r8 & 0xF) == 0) 
                set_flag(FLAG_H);

            *reg = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // DEC [HL]
        case(0x35): {;
            uint8_t n8 = read_from_memory(cpu.HL);
            uint8_t result = n8-1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_H);
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((n8 & 0xF) == 0) set_flag(FLAG_H);

            write_to_memory(cpu.HL, result);
            cpu.PC += 1;
            M_cycles = 3;
            break;
        }

        // INC r8
        case(0x04): case(0x14): case(0x24): case(0x0C):
        case(0x1C): case(0x2C): case(0x3C): {
            uint8_t* reg;
            if (opcode == 0x04) reg = &cpu.B;
            else if (opcode == 0x0C) reg = &cpu.C;
            else if (opcode == 0x14) reg = &cpu.D;
            else if (opcode == 0x1C) reg = &cpu.E;
            else if (opcode == 0x24) reg = &cpu.H;
            else if (opcode == 0x2C) reg = &cpu.L;
            else reg = &cpu.A;

            uint8_t r8 = *reg;
            uint8_t result = r8+1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (result == 0) set_flag(FLAG_Z);
            if (((r8 & 0xF) + 1) > 0x10) set_flag(FLAG_H);

            *reg = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // INC [HL]
        case(0x34): {
            uint8_t n8 = read_from_memory(cpu.HL);
            uint8_t result = n8+1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (result == 0) set_flag(FLAG_Z);
            if (((n8 & 0xF) + 1) > 0xF) set_flag(FLAG_H);

            write_to_memory(cpu.HL, result);
            cpu.PC += 1;
            M_cycles = 3;
            break;
        }

        // SBC A r8
        case(0x98): case(0x99): case(0x9A): case(0x9B): 
        case(0x9C): case(0x9D): case(0x9F): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0x98];
            uint8_t carry = is_set(FLAG_C);
            uint8_t result = cpu.A - (r8 + carry);

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((cpu.A & 0xF) < ((r8 & 0xF) + carry)) set_flag(FLAG_H);
            if (cpu.A < (r8 + carry)) set_flag(FLAG_C);

            cpu.A = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // SBC [HL]/n8
        case(0x9E): case(0xDE): {
            uint8_t n8;
            if (opcode == 0x9E) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC + 1];
            uint8_t carry = is_set(FLAG_C);
            uint8_t result = cpu.A - (n8 + carry);

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((cpu.A & 0xF) < ((n8 & 0xF) + carry)) set_flag(FLAG_H);
            if (cpu.A < (n8 + carry)) set_flag(FLAG_C);

            cpu.A = result;
            if (opcode == 0x9E) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // SUB r8
        case(0x90): case(0x91): case(0x92): case(0x93):
        case(0x94): case(0x95): case(0x97): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0x90];
            uint8_t result = cpu.A - r8;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((cpu.A & 0xF) < (r8 & 0xF)) set_flag(FLAG_H);
            if (cpu.A < r8) set_flag(FLAG_C);

            cpu.A = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // SUB [HL]/n8
        case(0x96): case(0xD6): {
            uint8_t n8;
            if (opcode == 0x96) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC + 1];
            uint8_t result = cpu.A - n8;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_N);
            if ((cpu.A & 0xF) < (n8 & 0xF)) set_flag(FLAG_H);
            if (cpu.A < n8) set_flag(FLAG_C);

            cpu.A = result;
            if (opcode == 0x96) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // 16-bit arithmetic instructions

        // ADD HL, r16
        case(0x09): case(0x19): case(0x29): case(0x39): {
            uint16_t r16;
            if (opcode == 0x09) r16 = cpu.BC;
            else if (opcode == 0x19) r16 = cpu.DE;
            else if (opcode == 0x29) r16 = cpu.HL;
            else r16 = cpu.SP;
            uint32_t result = cpu.HL + r16;

            clear_flag(FLAG_H);
            clear_flag(FLAG_C);
            set_flag(FLAG_N);
            if ((r16 & 0xFFF) + (cpu.HL & 0xFFF) > 0xFFF) set_flag(FLAG_H);
            if (result > 0xFFFF) set_flag(FLAG_C);

            cpu.HL = (uint16_t)result;
            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // DEC HL, r16
        case(0x0B): case(0x1B): case(0x2B): case(0x3B): {
            uint16_t* reg;
            if (opcode == 0x0B) reg = &cpu.BC;
            else if (opcode == 0x1B) reg = &cpu.DE;
            else if (opcode == 0x2B) reg = &cpu.HL;
            else reg = &cpu.SP;
            *reg -= 1;

            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // INC HL, r16
        case(0x03): case(0x13): case(0x23): case(0x33): {
            uint16_t* reg;
            if (opcode == 0x03) reg = &cpu.BC;
            else if (opcode == 0x13) reg = &cpu.DE;
            else if (opcode == 0x23) reg = &cpu.HL;
            else reg = &cpu.SP;
            *reg += 1;

            cpu.PC += 1;
            M_cycles = 2;
            break;
        }

        // Bitwise logic instructions
        
        // AND A,r8
        case(0xA0): case(0xA1): case(0xA2): case(0xA3):
        case(0xA4): case(0xA5): case(0xA7): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0xA0];
            uint8_t result = r8 & cpu.A;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_H);

            cpu.A = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // AND A,[HL]/n8
        case(0xA6): case(0xE6): {
            uint8_t n8;
            if (opcode == 0xA6) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC+1];
            uint8_t result = n8 & cpu.A;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);
            set_flag(FLAG_H);

            cpu.A = result;
            if (opcode == 0xA6) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // CPL
        case(0x2F): {
            cpu.A = ~cpu.A;
            set_flag(FLAG_N);
            set_flag(FLAG_H);
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // OR A,r8
        case(0xB0): case(0xB1): case(0xB2): case(0xB3):
        case(0xB4): case(0xB5): case(0xB7): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0xB0];
            uint8_t result = r8 | cpu.A;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);

            cpu.A = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // OR A,[HL]/n8
        case(0xB6): case(0xF6): {
            uint8_t n8;
            if (opcode == 0xB6) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC+1];
            uint8_t result = n8 | cpu.A;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);

            cpu.A = result;
            if (opcode == 0xB6) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // XOR A,r8
        case(0xA8): case(0xA9): case(0xAA): case(0xAB):
        case(0xAC): case(0xAD): case(0xAF): {
            uint8_t r8;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8 = *regs[opcode - 0xA8];
            uint8_t result = r8 ^ cpu.A;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);

            cpu.A = result;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // XOR A,[HL]/n8
        case(0xAE): case(0xEE): {
            uint8_t n8;
            if (opcode == 0xAE) n8 = read_from_memory(cpu.HL);
            else n8 = memory[cpu.PC+1];
            uint8_t result = n8 ^ cpu.A;

            cpu.F = 0;
            if (result == 0) set_flag(FLAG_Z);

            cpu.A = result;
            if (opcode == 0xAE) cpu.PC += 1;
            else cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // Bit flag instructions

        // BIT u3,r8/[HL]
        case(0xCB40): case(0xCB41): case(0xCB42): case(0xCB43): case(0xCB44): case(0xCB45): case(0xCB46): case(0xCB47):
        case(0xCB48): case(0xCB49): case(0xCB4A): case(0xCB4B): case(0xCB4C): case(0xCB4D): case(0xCB4E): case(0xCB4F):
        case(0xCB50): case(0xCB51): case(0xCB52): case(0xCB53): case(0xCB54): case(0xCB55): case(0xCB56): case(0xCB57):
        case(0xCB58): case(0xCB59): case(0xCB5A): case(0xCB5B): case(0xCB5C): case(0xCB5D): case(0xCB5E): case(0xCB5F):
        case(0xCB60): case(0xCB61): case(0xCB62): case(0xCB63): case(0xCB64): case(0xCB65): case(0xCB66): case(0xCB67):
        case(0xCB68): case(0xCB69): case(0xCB6A): case(0xCB6B): case(0xCB6C): case(0xCB6D): case(0xCB6E): case(0xCB6F):
        case(0xCB70): case(0xCB71): case(0xCB72): case(0xCB73): case(0xCB74): case(0xCB75): case(0xCB76): case(0xCB77):
        case(0xCB78): case(0xCB79): case(0xCB7A): case(0xCB7B): case(0xCB7C): case(0xCB7D): case(0xCB7E): case(0xCB7F): {

            uint8_t r8;
            uint8_t u3;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, &memory[cpu.HL], &cpu.A};
            if ((opcode - 0xCB40) % 8 == 6)
                r8 = read_from_memory(cpu.HL);
            else
                r8 = *regs[(opcode - 0xCB40) % 8];
            u3 = (opcode - 0xCB40) / 8;

            if (!(r8 & (1 << u3))) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            set_flag(FLAG_H);

            cpu.PC += 2;
            if ((opcode - 0xCB40) % 8 == 6) M_cycles = 3;
            else M_cycles = 2;
            break;
        }

        // RES u3,r8/[HL]
        case(0xCB80): case(0xCB81): case(0xCB82): case(0xCB83): case(0xCB84): case(0xCB85): case(0xCB86): case(0xCB87):
        case(0xCB88): case(0xCB89): case(0xCB8A): case(0xCB8B): case(0xCB8C): case(0xCB8D): case(0xCB8E): case(0xCB8F):
        case(0xCB90): case(0xCB91): case(0xCB92): case(0xCB93): case(0xCB94): case(0xCB95): case(0xCB96): case(0xCB97):
        case(0xCB98): case(0xCB99): case(0xCB9A): case(0xCB9B): case(0xCB9C): case(0xCB9D): case(0xCB9E): case(0xCB9F):
        case(0xCBA0): case(0xCBA1): case(0xCBA2): case(0xCBA3): case(0xCBA4): case(0xCBA5): case(0xCBA6): case(0xCBA7):
        case(0xCBA8): case(0xCBA9): case(0xCBAA): case(0xCBAB): case(0xCBAC): case(0xCBAD): case(0xCBAE): case(0xCBAF):
        case(0xCBB0): case(0xCBB1): case(0xCBB2): case(0xCBB3): case(0xCBB4): case(0xCBB5): case(0xCBB6): case(0xCBB7):
        case(0xCBB8): case(0xCBB9): case(0xCBBA): case(0xCBBB): case(0xCBBC): case(0xCBBD): case(0xCBBE): case(0xCBBF): {

            uint8_t* r8p;
            uint8_t value;
            uint8_t u3;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, &memory[cpu.HL], &cpu.A};
            u3 = (opcode - 0xCB80) / 8;
            if ((opcode - 0xCB80) & 8 == 6){
                value = read_from_memory(cpu.HL);
                write_to_memory(cpu.HL, value & ~(1 << u3));
            }
            else{
                r8p = regs[(opcode - 0xCB80) % 8];
                *r8p &= ~(1 << u3);
            }

            cpu.PC += 2;
            if ((opcode - 0xCB80) % 8 == 6) M_cycles = 4;
            else M_cycles = 2;
            break;
        }

        // SET u3,r8/[HL]
        case(0xCBC0): case(0xCBC1): case(0xCBC2): case(0xCBC3): case(0xCBC4): case(0xCBC5): case(0xCBC6): case(0xCBC7):
        case(0xCBC8): case(0xCBC9): case(0xCBCA): case(0xCBCB): case(0xCBCC): case(0xCBCD): case(0xCBCE): case(0xCBCF):
        case(0xCBD0): case(0xCBD1): case(0xCBD2): case(0xCBD3): case(0xCBD4): case(0xCBD5): case(0xCBD6): case(0xCBD7):
        case(0xCBD8): case(0xCBD9): case(0xCBDA): case(0xCBDB): case(0xCBDC): case(0xCBDD): case(0xCBDE): case(0xCBDF):
        case(0xCBE0): case(0xCBE1): case(0xCBE2): case(0xCBE3): case(0xCBE4): case(0xCBE5): case(0xCBE6): case(0xCBE7):
        case(0xCBE8): case(0xCBE9): case(0xCBEA): case(0xCBEB): case(0xCBEC): case(0xCBED): case(0xCBEE): case(0xCBEF):
        case(0xCBF0): case(0xCBF1): case(0xCBF2): case(0xCBF3): case(0xCBF4): case(0xCBF5): case(0xCBF6): case(0xCBF7):
        case(0xCBF8): case(0xCBF9): case(0xCBFA): case(0xCBFB): case(0xCBFC): case(0xCBFD): case(0xCBFE): case(0xCBFF): {

            uint8_t* r8p;
            uint8_t value;
            uint8_t u3;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, &memory[cpu.HL], &cpu.A};
            u3 = (opcode - 0xCBC0) / 8;
            if ((opcode - 0xCBC0) % 8 == 6){
                value = read_from_memory(cpu.HL);
                write_to_memory(cpu.HL, value | (1 << u3));
            }
            else {
                r8p = regs[(opcode - 0xCBC0) % 8];
                *r8p |= (1 << u3);
            }

            cpu.PC += 2;
            if ((opcode - 0xCBC0) % 8 == 6) M_cycles = 4;
            else M_cycles = 2;
            break;
        }

        // Bit shift instructions

        // RL r8
        case(0xCB10): case(0xCB11): case(0xCB12): case(0xCB13): 
        case(0xCB14): case(0xCB15): case(0xCB17): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB10];

            uint8_t r8_value = *r8p;

            if (is_set(FLAG_C))
                *r8p = (r8_value << 1) & 1;
            else
                *r8p = (r8_value << 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // RL [HL]
        case(0xCB16): {
            uint8_t value = read_from_memory(cpu.HL);

            if (is_set(FLAG_C))
                write_to_memory(cpu.HL, (value << 1) & 1);
            else
                write_to_memory(cpu.HL, value << 1);

            uint8_t result = read_from_memory(cpu.HL);
            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // RLA
        case(0x17): {
            uint8_t A_value = cpu.A;
            if (is_set(FLAG_C))
                cpu.A = (A_value << 1) & 1;
            else
                cpu.A = A_value << 1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (A_value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // RLC r8
        case(0xCB00): case(0xCB01): case(0xCB02): case(0xCB03): 
        case(0xCB04): case(0xCB05): case(0xCB07): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB00];

            uint8_t r8_value = *r8p;

            if (r8_value & 0x80)
                *r8p = (r8_value << 1) & 1;
            else
                *r8p = (r8_value << 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // RLC [HL]
        case(0xCB06): {
            uint8_t value = read_from_memory(cpu.HL);

            if (value & 0x80)
                write_to_memory(cpu.HL, (value << 1) & 1);
            else
                write_to_memory(cpu.HL, value << 1);

            uint8_t result = read_from_memory(cpu.HL);
            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // RLCA
        case(0x07): {
            uint8_t A_value = cpu.A;
            if (A_value & 0x80)
                cpu.A = (A_value << 1) & 1;
            else
                cpu.A = A_value << 1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (A_value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // RR r8
        case(0xCB18): case(0xCB19): case(0xCB1A): case(0xCB1B): 
        case(0xCB1C): case(0xCB1D): case(0xCB1F): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB18];

            uint8_t r8_value = *r8p;

            if (is_set(FLAG_C))
                *r8p = (r8_value >> 1) & 0x80;
            else
                *r8p = (r8_value >> 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // RR [HL]
        case(0xCB1E): {
            uint8_t value = read_from_memory(cpu.HL);
            uint8_t result;

            if (is_set(FLAG_C))
                result = (value >> 1) & 0x80;
            else
                result = (value >> 1);
            
            write_to_memory(cpu.HL, result);

            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // RRA
        case(0x1F): {
            uint8_t A_value = cpu.A;
            if (is_set(FLAG_C))
                cpu.A = (A_value >> 1) & 0x80;
            else
                cpu.A = A_value >> 1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (A_value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // RRC r8
        case(0xCB08): case(0xCB09): case(0xCB0A): case(0xCB0B): 
        case(0xCB0C): case(0xCB0D): case(0xCB0F): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB08];

            uint8_t r8_value = *r8p;

            if (r8_value & 1)
                *r8p = (r8_value >> 1) & 0x80;
            else
                *r8p = (r8_value >> 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // RRC [HL]
        case(0xCB0E): {
            uint8_t value = read_from_memory(cpu.HL);
            uint8_t result;

            if (value & 1)
                result = (value >> 1) & 0x80;
            else
                result = (value >> 1);
            
            write_to_memory(cpu.HL, result);

            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // RRCA
        case(0x0F): {
            uint8_t A_value = cpu.A;
            if (A_value & 1)
                cpu.A = (A_value >> 1) & 0x80;
            else
                cpu.A = A_value >> 1;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (A_value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // SLA r8
        case(0xCB20): case(0xCB21): case(0xCB22): case(0xCB23): 
        case(0xCB24): case(0xCB25): case(0xCB27): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB20];

            uint8_t r8_value = *r8p;
            *r8p = (r8_value << 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // SLA [HL]
        case(0xCB26): {
            uint8_t value = read_from_memory(cpu.HL);
            uint8_t result = value << 1;
            write_to_memory(cpu.HL, result);

            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 0x80) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // SRA r8
        case(0xCB28): case(0xCB29): case(0xCB2A): case(0xCB2B): 
        case(0xCB2C): case(0xCB2D): case(0xCB2F): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB28];

            uint8_t r8_value = *r8p;
            if (r8_value & 0x80)
                *r8p = (r8_value >> 1) & 0x80;
            else
                *r8p = (r8_value >> 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // SRA [HL]
        case(0xCB2E): {
            uint8_t value = read_from_memory(cpu.HL);
            uint8_t result;
            if (value & 0x80)
                result = (value >> 1) & 0x80;
            else
                result = (value >> 1);
            write_to_memory(cpu.HL, result);

            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // SRL r8
        case(0xCB38): case(0xCB39): case(0xCB3A): case(0xCB3B): 
        case(0xCB3C): case(0xCB3D): case(0xCB3F): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, NULL, &cpu.A};
            r8p = regs[opcode - 0xCB38];

            uint8_t r8_value = *r8p;
            *r8p = (r8_value >> 1);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (r8_value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // SRL [HL]
        case(0xCB3E): {
            uint8_t value = read_from_memory(cpu.HL);
            uint8_t result = value >> 1;

            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (value & 1) set_flag(FLAG_C);
            else clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // SWAP r8
        case(0xCB30): case(0xCB31): case(0xCB32): case(0xCB33): 
        case(0xCB34): case(0xCB35): case(0xCB37): {
            uint8_t* r8p;
            uint8_t *regs[8] = {&cpu.B, &cpu.C, &cpu.D, &cpu.E, &cpu.H, &cpu.L, &memory[cpu.HL], &cpu.A};
            r8p = regs[opcode - 0xCB30];

            uint8_t upper_4 = *r8p >> 4;
            uint8_t lower_4 = *r8p & 0xF;
            *r8p = (lower_4 << 4) | (upper_4);

            if (*r8p == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 2;
            break;
        }

        // SWAP [HL]
        case(0xCB36): {
            uint8_t value = read_from_memory(cpu.HL);

            uint8_t upper_4 = value >> 4;
            uint8_t lower_4 = value & 0xF;
            uint8_t result = (lower_4 << 4) | (upper_4);

            write_to_memory(cpu.HL, result);

            if (result == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            clear_flag(FLAG_C);

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // Jumps and subroutine instructions

        // Call n16
        case(0xCD): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            cpu.PC += 3;
            memory[cpu.SP-1] = cpu.PC >> 8;
            memory[cpu.SP-2] = cpu.PC & 0xFF;
            cpu.SP -= 2;
            cpu.PC = n16;
            M_cycles = 6;
            break;            
        }

        // Call cc,n16
        case(0xC4): case(0xD4): case(0xCC): case(0xDC): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            cpu.PC += 3;
            if (((opcode == 0xC4) && !is_set(FLAG_Z)) || 
                ((opcode == 0xD4) && !is_set(FLAG_C)) ||
                ((opcode == 0xCC) &&  is_set(FLAG_Z)) || 
                ((opcode == 0xDC) &&  is_set(FLAG_C))) {
                memory[cpu.SP-1] = cpu.PC >> 8;
                memory[cpu.SP-2] = cpu.PC & 0xFF;
                cpu.SP -= 2;
                cpu.PC = n16;
                M_cycles = 6;       
            }
            else 
                M_cycles = 3;
            break;            
        }

        // JP HL
        case(0xE9): {
            cpu.PC = cpu.HL;
            M_cycles = 1;
            break;
        }

        // JP n16
        case(0xC3): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            cpu.PC = n16;
            M_cycles = 4;
            break;
        }

        // JP cc,n16
        case(0xC2): case(0xD2): case(0xCA): case(0xDA): {
            uint16_t n16 = (memory[cpu.PC+2] << 8) | memory[cpu.PC+1];
            if (((opcode == 0xC2) && !is_set(FLAG_Z)) || 
                ((opcode == 0xD2) && !is_set(FLAG_C)) ||
                ((opcode == 0xCA) &&  is_set(FLAG_Z)) || 
                ((opcode == 0xDA) &&  is_set(FLAG_C))) {
                cpu.PC = n16;
                M_cycles = 4;
            }
            else {
                cpu.PC += 3;
                M_cycles = 3;
            }
            break;
        }

        // JR n16
        case(0x18): {
            uint8_t n8 = memory[cpu.PC+1];
            int8_t e8;
            if (n8 & 0x80) e8 = -(~n8 +1);
            else e8 = n8 & ~0x80;

            cpu.PC += e8 + 2;
            M_cycles = 3;
            break;
        }

        // JR cc,n16
        case(0x20): case(0x30): case(0x28): case(0x38): {
            if (((opcode == 0x28) && is_set(FLAG_Z)) || ((opcode == 0x38) && is_set(FLAG_C)) ||
                ((opcode == 0x20) && !is_set(FLAG_Z)) || ((opcode == 0x30) && !is_set(FLAG_C))) {
                uint8_t n8 = memory[cpu.PC+1];
                int8_t e8;
                if (n8 & 0x80) e8 = -(~n8 + 1);
                else e8 = n8 & ~0x80;

                cpu.PC += e8 + 2;
                M_cycles = 3;
            }
            else {
                cpu.PC += 2;
                M_cycles = 2;
            }
            break;
        }

        // RET
        case(0xC9): {
            cpu.PC = memory[cpu.SP];
            cpu.PC |= memory[cpu.SP+1] << 8;
            cpu.SP += 2;

            M_cycles = 4;
            break;
        }

        // RET CC
        case(0xC0): case(0xD0): case(0xC8): case(0xD8): {
            if (((opcode == 0xC8) &&  is_set(FLAG_Z)) || 
                ((opcode == 0xD8) &&  is_set(FLAG_C)) ||
                ((opcode == 0xC0) && !is_set(FLAG_Z)) || 
                ((opcode == 0xD8) && !is_set(FLAG_C))) {
                cpu.PC = memory[cpu.SP];
                cpu.PC |= memory[cpu.SP+1] << 8;
                cpu.SP += 2;

                M_cycles = 5;
            }
            else {
                cpu.PC += 1;
                M_cycles = 2;
            }
            break;
        }

        // RETI
        case(0xD9): {
            cpu.PC = memory[cpu.SP];
            cpu.PC |= memory[cpu.SP+1] << 8;
            cpu.SP += 2;

            M_cycles = 4;
            IME_flag = 0;
            break;
        }

        // RST, vec
        case(0xC7): case(0xD7): case(0xE7): case(0xF7):
        case(0xCF): case(0xDF): case(0xEF): case(0xFF): {
            uint8_t vec;
            if (opcode == 0xC7) vec = 0x00;
            else if (opcode == 0xCF) vec = 0x08;
            else if (opcode == 0xD7) vec = 0x10;
            else if (opcode == 0xDF) vec = 0x18;
            else if (opcode == 0xE7) vec = 0x20;
            else if (opcode == 0xEF) vec = 0x28;
            else if (opcode == 0xF7) vec = 0x30;
            else vec = 0x38;

            memory[cpu.SP-1] = cpu.PC >> 8;
            memory[cpu.SP-2] = cpu.PC & 0xFF;
            cpu.SP -= 2;
            cpu.PC = vec;
            M_cycles = 4;
            break;
        }

        // Carry flag instructions

        // CCF
        case(0x3F): {
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            if (is_set(FLAG_C)) clear_flag(FLAG_C);
            else set_flag(FLAG_C);
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // SCF
        case(0x37): {
            clear_flag(FLAG_N);
            clear_flag(FLAG_H);
            set_flag(FLAG_C);
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // Stack manipulation instructions

        // ADD SP, e8
        case(0xE8): {
            uint8_t n8 = memory[cpu.PC+1];
            int8_t e8;
            if (n8 & 0x80) e8 = -(n8 & 0x7F);
            else e8 = n8 & 0x7F;

            cpu.SP += e8;

            clear_flag(FLAG_Z);
            clear_flag(FLAG_N);
            if (e8 >= 0) {
                if ((cpu.SP & 0xF) + (e8 & 0xF) > 0xF) set_flag(FLAG_H);
                if ((cpu.SP & 0xFF) + e8 > 0xFF) set_flag(FLAG_C);
            }
            else {
                if ((cpu.SP & 0xF) < (e8 & 0xF)) set_flag(FLAG_H);
                if ((cpu.SP & 0xFF) < e8) set_flag(FLAG_C);
            }

            cpu.PC += 2;
            M_cycles = 4;
            break;
        }

        // POP r16
        case(0xC1): case(0xD1): case(0xE1): case(0xF1): {
            uint16_t* r16p;
            if (opcode == 0xC1) r16p = &cpu.BC;
            else if (opcode == 0xD1) r16p = &cpu.DE;
            else if (opcode == 0xE1) r16p = &cpu.HL;
            else r16p = &cpu.AF;

            *r16p = (memory[cpu.SP+1] << 8) | memory[cpu.SP];
            cpu.SP += 2;

            cpu.PC += 1;
            M_cycles = 3;
            break;
        }

        // PUSH r16
        case(0xC5): case(0xD5): case(0xE5): case(0xF5): {
            uint16_t r16;
            if (opcode == 0xC5) r16 = cpu.BC;
            else if (opcode == 0xD5) r16 = cpu.DE;
            else if (opcode == 0xE5) r16 = cpu.HL;
            else r16 = cpu.AF;

            memory[cpu.SP-1] = r16 >> 8;
            memory[cpu.SP-2] = r16 & 0xFF;
            cpu.SP -= 2;

            cpu.PC += 1;
            M_cycles = 4;
            break;
        }

        // Interupt related instructions

        // DI
        case(0xF3): {
            IME_flag = 0;
            IME_flag_next = 0;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // EI
        case(0xFB): {
            IME_flag_next = 0;
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // HALT
        case(0x76): {
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        // miscellaneous instructions

        // DAA
        case(0x27): {
            int adjustment = 0;
            if (is_set(FLAG_N)) {
                if (is_set(FLAG_H)) adjustment += 0x6;
                if (is_set(FLAG_C)) adjustment += 0x60;
                cpu.A -= adjustment;

            }
            else {
                if (is_set(FLAG_H) || (cpu.A & 0xF) > 0x9) adjustment += 0x6;
                if (is_set(FLAG_C) || cpu.A > 0x99) adjustment += 0x60;
                cpu.A += adjustment; 
            }
            if (adjustment == 0) set_flag(FLAG_Z);
            clear_flag(FLAG_H);

            cpu.PC += 1;
            M_cycles = 4;
            break;
        }

        // NOP
        case(0x00): {
            cpu.PC += 1;
            M_cycles = 1;
            break;
        }

        //STOP
        case(0x10): {
            cpu.PC += 2;
            M_cycles = 1;
            break;
        }

        default:
            cpu.PC += 1;
            M_cycles = 1;
    };

    if (IME_flag_next == 1)
        IME_flag_next++;
    else if (IME_flag_next == 2) {
        IME_flag = 1;
        IME_flag_next = 0;
    }

    return M_cycles;
}

#endif