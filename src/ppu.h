#ifndef PPU_H
#define PPU_H

#include "gbmemory.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define SCR_WIDTH 160
#define SCR_HEIGHT 144
#define PIXEL_SIZE 4

// #define BLACK 0x0F380F
// #define DARK_GREY 0x306230
// #define LIGHT_GREY 0x8BAC0F
// #define WHTIE 0x9BBC0F

struct color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct object {
    uint8_t y_pos;
    uint8_t x_pos;
    uint8_t tile_index;
    uint8_t attributes;
};

uint8_t obj_line_buffer[176];
uint8_t win_line_buffer[176];
uint8_t bg_line_buffer[176];
uint8_t frame_buffer[SCR_HEIGHT][SCR_WIDTH];
uint8_t mode = 2;
uint8_t scanlines = 0;
uint16_t scanline_dot_counter = 0;
struct object objects[10];
uint8_t obj_counter = 0;

uint8_t get_ppu_mode() {
    return memory[STAT] & 0b11;
}

void set_ppu_mode(uint8_t mode) {
    memory[STAT] &= ~0b11;
    memory[STAT] |= mode;
}

uint8_t get_obj_height() {
    if (memory[LCDC] & 0b100) 
        return 16;
    else 
        return 8;
}

uint8_t lcd_enable() {
    return memory[LCDC] & 0x80;
}

void ppu_execute(uint8_t dots) {
    while (dots > 0) {
        // OAM scan
        if (get_ppu_mode() == 2) {
            while (dots > 0 && get_ppu_mode() == 2) {
                if (scanline_dot_counter >= 80) {
                    set_ppu_mode(3);
                    break;
                }
                
                if (((memory[0xFE00 + scanline_dot_counter] - 16) <= memory[LY]) 
                    && (memory[0xFE00 + scanline_dot_counter] - 16 + get_obj_height()) >= memory[LY]) { // Checks if obj is on line
                    objects[obj_counter].y_pos = memory[0xFE00 + scanline_dot_counter] - 16;
                    objects[obj_counter].x_pos = memory[0xFE00 + scanline_dot_counter + 1] - 8;
                    objects[obj_counter].tile_index = memory[0xFE00 + scanline_dot_counter + 2];
                    objects[obj_counter].attributes = memory[0xFE00 + scanline_dot_counter + 3];

                    obj_counter++;
                }
                scanline_dot_counter += 4;        
                dots -= 4;
            }
        }

        // Drawing pixels (to frame_buffer)
        if (get_ppu_mode() == 3)  {
            while (dots > 0 && get_ppu_mode() == 3) {
                if (scanline_dot_counter >= 240) {
                    for (int i = 0; i < obj_counter; i++) {
                        objects[i].x_pos = 0;
                        objects[i].y_pos = 0;
                        objects[i].attributes = 0;
                        objects[i].tile_index = 0;
                    }
                    obj_counter = 0;
                    set_ppu_mode(0);
                    break;
                } 
                int current_x = scanline_dot_counter - 80;
                int current_y = memory[LY];
                uint8_t first_byte, second_byte;
                uint8_t pixel = 0;

                if (memory[LCDC] & 0x2 && obj_counter) {
                    for (int i = 0; i < obj_counter; i++) {
                        struct object obj = objects[i];
                        if (obj.x_pos <= current_x && (obj.x_pos + 8) >= current_x) {
                            if (obj.attributes & 0x40) {                        
                                first_byte = memory[0x8000 + (obj.tile_index+1)*16 - (current_y - obj.y_pos)*2 - 2];
                                second_byte = memory[0x8000 + (obj.tile_index+1)*16 - (current_y - obj.y_pos)*2 - 1];
                            }
                            else {
                                first_byte = memory[0x8000 + obj.tile_index*16 + (current_y - obj.y_pos)*2];
                                second_byte = memory[0x8000 + obj.tile_index*16 + (current_y - obj.y_pos)*2 + 1];
                            }
                            if (obj.attributes & 0x20) {
                                pixel |= (first_byte >> (((current_x - obj.x_pos) % 8))) & 0b1;
                                pixel |= (second_byte >> (((current_x - obj.x_pos) % 8) << 1)) & 0b10;
                            }
                            else {
                                pixel |= (first_byte >> (7-((current_x - obj.x_pos) % 8))) & 0b1;
                                pixel |= (second_byte >> (7-((current_x - obj.x_pos) % 8)) << 1) & 0b10;
                            }
                            uint8_t color_pallette;
                            if (obj.attributes & 0x10)
                                color_pallette = memory[OBP1];
                            else 
                                color_pallette = memory[OBP0];

                            if (pixel == 0b11)
                                pixel = color_pallette >> 6;
                            else if (pixel == 0b10)
                                pixel = (color_pallette >> 4) & 0b11;
                            else if (pixel == 0b01)
                                pixel = (color_pallette >> 2) & 0b11;
                            break;
                        }
                    }

                }

                if ((!pixel) && (memory[LCDC] & 0x1) && (memory[LCDC] & 0x20) 
                        && (current_x > (memory[WX] + 7)) && (current_y > memory[WY])) {
                    uint16_t tile_map_start;
                    if (memory[LCDC] & 0x4) // Check window tile map area
                        tile_map_start = 0x9C00;
                    else
                        tile_map_start = 0x9800;
                    
                    uint8_t tile_x = (current_x - memory[WX]) / 8;
                    uint8_t tile_y = (current_y - memory[WY]) / 8;

                    uint16_t tile_index = tile_map_start + tile_x + 32*tile_y;
                    uint8_t tile = memory[tile_index];
                    uint16_t base_pointer;
                    uint16_t pixel_byte_pointer;
                    if (memory[LCDC] & 0x10) {
                        base_pointer = 0x8000;
                        pixel_byte_pointer = base_pointer + 16*tile + 2*((current_y - memory[WY]) % 8);
                    }
                    else {
                        base_pointer = 0x9000;
                        pixel_byte_pointer = base_pointer + 16*(int8_t)tile + 2*((current_y - memory[WY]) % 8);
                    }
                    first_byte = memory[pixel_byte_pointer];
                    second_byte = memory[pixel_byte_pointer+1];
                    pixel |= (first_byte >> (7-((current_x - memory[WX]) % 8))) & 0b01;
                    pixel |= ((second_byte >> (7-((current_x - memory[WX]) % 8))) << 1) & 0b10;

                    uint8_t color_pallette = memory[BGP];

                    if (pixel == 0b11)
                        pixel = color_pallette >> 6;
                    else if (pixel == 0b10)
                        pixel = (color_pallette >> 4) & 0b11;
                    else if (pixel == 0b01)
                        pixel = (color_pallette >> 2) & 0b11;
                    else if (pixel == 0)
                        pixel = color_pallette & 0b11;
                }

                else if (!pixel && memory[LCDC] & 0x1) {
                    uint16_t tile_map_start;
                    if (memory[LCDC] & 0x8) // Check BG tile map area
                        tile_map_start = 0x9C00;
                    else
                        tile_map_start = 0x9800;
                    
                    uint8_t tile_x = ((memory[SCX] + current_x) % 256) / 8;
                    uint8_t tile_y = ((memory[SCY] + current_y) % 256 ) / 8;
                    
                    uint16_t tile_index = tile_map_start + tile_x + 32*tile_y;
                    uint8_t tile = memory[tile_index];
                    uint16_t base_pointer;
                    uint16_t pixel_byte_pointer;
                    if (memory[LCDC] & 0x10) {
                        base_pointer = 0x8000;
                        pixel_byte_pointer = base_pointer + 16*tile + 2*((current_y + memory[SCY]) % 8);
                    }
                    else {
                        base_pointer = 0x9000;
                        pixel_byte_pointer = base_pointer + 16*(int8_t)tile + 2*((current_y + memory[SCY]) % 8);
                    }
                    first_byte = memory[pixel_byte_pointer];
                    second_byte = memory[pixel_byte_pointer+1];
                    pixel |= (first_byte >> (7-((current_x + memory[SCX]) % 8))) & 0b01;
                    pixel |= ((second_byte >> (7-((current_x + memory[SCX]) % 8))) << 1) & 0b10;

                    uint8_t color_pallette = memory[BGP];

                    if (pixel == 0b11)
                        pixel = color_pallette >> 6;
                    else if (pixel == 0b10)
                        pixel = (color_pallette >> 4) & 0b11;
                    else if (pixel == 0b01)
                        pixel = (color_pallette >> 2) & 0b11;
                    else if (pixel == 0)
                        pixel = color_pallette & 0b11;
                }

                // if (obj_line_buffer[current_x+8])
                //     frame_buffer[current_y][current_x] = obj_line_buffer[current_x+8];
                // else
                frame_buffer[current_y][current_x] = pixel;
                scanline_dot_counter += 1;
        
                dots--;
            }
        }

        // Horizontal blank
        if (get_ppu_mode() == 0) {
            while (dots > 0 && get_ppu_mode() == 0) {
                if (scanline_dot_counter >= 456) {
                    scanline_dot_counter = 0;
                    memory[LY]++;
                    for (int i = 0; i < 176; i++) {
                        obj_line_buffer[i] = 0;
                        win_line_buffer[i] = 0;
                        bg_line_buffer[i] = 0;
                    }
                    if (memory[LY] > 143) {
                        set_ppu_mode(1);
                        memory[IF] |= 1;
                        break;
                    }
                    else {
                        set_ppu_mode(2);
                        break;
                    }
                }
                scanline_dot_counter++;
                dots--;
            }
        }

        // Vertical blank
        if (get_ppu_mode() == 1) {
            while (dots > 0 && get_ppu_mode() == 1) {
                if (scanline_dot_counter >= 456) {
                    scanline_dot_counter = 0;
                    memory[LY]++;
                    if (memory[LY] > 153) {
                        memory[LY] = 0;
                        set_ppu_mode(2);
                        dots--;
                        break;
                    }
                }
                scanline_dot_counter++;
                dots--;
            }
        }
    }
}


#endif