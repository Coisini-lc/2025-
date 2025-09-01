#ifndef __DATA_H
#define __DATA_H

#include <stdio.h>
#include "stm32f1xx_hal.h"  // Adjust this include based on your HAL library path
#include "usart.h"         // Adjust this include based on your USART configuration
#include "stdbool.h"
#include <stdlib.h> 
int fputc(int ch,FILE *f);


int fgetc(FILE *f);

// 坐标点结构体
typedef struct {
    uint8_t grid_x;
    uint8_t grid_y;
    uint16_t pixel_x;
    uint16_t pixel_y;
} GridPoint;

// 函数声明
bool parse_single_coordinate(char* coord_str, GridPoint* point);
void parse_complete_path(uint8_t* buffer, int length);
void parse_animal_data(uint8_t* buffer, int length);
void clear_screen(void);
void draw_point(GridPoint point);
void draw_line(GridPoint start, GridPoint end);
void draw_complete_path(void);



void refference(void);





extern uint8_t rx1;
extern uint8_t rx2;

#endif