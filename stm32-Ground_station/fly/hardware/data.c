#include "data.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 函数声明
void redraw_path(void);

// 坐标系统定义
#define GRID_COLS 9     // A1-A9
#define GRID_ROWS 7     // B1-B7

// 实际屏幕坐标范围
#define START_X 325     // A1的X坐标
#define END_X 760       // A9的X坐标  
#define START_Y 135     // B7的Y坐标
#define END_Y 410       // B1的Y坐标

// 坐标转换宏（修正版）
#define PIXEL_X(grid_x) (START_X + ((grid_x)-1) * (END_X - START_X) / (GRID_COLS-1))
#define PIXEL_Y(grid_y) (END_Y - ((grid_y)-1) * (END_Y - START_Y) / (GRID_ROWS-1))





uint8_t rx1; 
uint8_t rx1_buffer[100]; //接收缓冲区（改为合适大小）
uint16_t rx1_index = 0; //接收缓冲区索引，改为uint16_t


// 串口2接收变量
uint8_t rx2;
uint8_t rx2_buffer[1024];  // 进一步增大缓冲区以处理超长路径数据
uint16_t rx2_index = 0;   // 改为uint16_t以匹配缓冲区大小


// 路径缓冲区
#define MAX_PATH_POINTS 100  // 增加到100个点以支持更长的路径
GridPoint path_buffer[MAX_PATH_POINTS];
uint16_t path_count = 0;  // 改为uint16_t以支持更多点

// 动物统计系统 - 五个字符串数组分别存储
#define MAX_ANIMAL_RECORDS 20  // 每种动物最多记录数
#define MAX_ANIMAL_STRING 200  // 每种动物累积字符串的最大长度

// 五个字符串数组分别存储五种动物信息 (格式: "数量+坐标", 如"3A1B2")
char elephant_records[MAX_ANIMAL_RECORDS][8];   // 象 (E)
char tiger_records[MAX_ANIMAL_RECORDS][8];      // 虎 (I)  
char wolf_records[MAX_ANIMAL_RECORDS][8];       // 狼 (W)
char peacock_records[MAX_ANIMAL_RECORDS][8];    // 孔雀 (P)
char monkey_records[MAX_ANIMAL_RECORDS][8];     // 猴子 (M)

// 每种动物的记录数量
uint8_t elephant_count = 0;
uint8_t tiger_count = 0;
uint8_t wolf_count = 0;
uint8_t peacock_count = 0;
uint8_t monkey_count = 0;

// 五个累积字符串变量存储所有记录 (格式: "数量+坐标,数量+坐标,...")
char elephant_data[MAX_ANIMAL_STRING] = "";  // 象累积数据 - 如"2A1B1,1A2B2"
char tiger_data[MAX_ANIMAL_STRING] = "";     // 虎累积数据  
char wolf_data[MAX_ANIMAL_STRING] = "";      // 狼累积数据
char peacock_data[MAX_ANIMAL_STRING] = "";   // 孔雀累积数据
char monkey_data[MAX_ANIMAL_STRING] = "";    // 猴子累积数据

uint16_t elephant_total = 0;
uint16_t peacock_total = 0;
uint16_t monkey_total = 0;
uint16_t tiger_total = 0;
uint16_t wolf_total = 0;



int fputc(int ch,FILE *f)
{
//采用轮询方式发送1字节数据，超时时间设置为无限等待
HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,100);
return ch;
}
int fgetc(FILE *f)
{
uint8_t ch;
// 采用轮询方式接收 1字节数据，超时时间设置为无限等待
HAL_UART_Receive( &huart1,(uint8_t*)&ch,1, 100 );
return ch;
}
 

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

    if (huart->Instance == USART1) // TFT屏返回的禁飞区数据   AA1B1A2B2T
    {
        // 防止缓冲区溢出
        if (rx1_index >= sizeof(rx1_buffer)) {
            memset(rx1_buffer, 0, sizeof(rx1_buffer));
            rx1_index = 0;
        }

        rx1_buffer[rx1_index++] = rx1; // 存储接收到的数据

        // 如果第一个字节是'L'，触发你的函数
        if (rx1_index == 1 && rx1_buffer[0] == 'L') {
            refference(); // 替换为你的实际函数名
            memset(rx1_buffer, 0, sizeof(rx1_buffer));
            rx1_index = 0;   // 重置索引
        }
        // 如果第一个字节是'R'，重新绘制之前的路径
        else if (rx1_index == 1 && rx1_buffer[0] == 'R') {
            redraw_path(); // 重新绘制路径函数
            memset(rx1_buffer, 0, sizeof(rx1_buffer));
            rx1_index = 0;   // 重置索引
        }
        // 检查是否接收到完整的动物数据包 (AA开头，T结尾，长度为14字节)
        else if(rx1_index == 14 && rx1_buffer[0] == 'A' && rx1_buffer[1] == 'A' && rx1_buffer[13] == 'T') {
            // 创建临时缓冲区，避免发送过程中数据被清空
            uint8_t temp_buffer[14];
            for(int i = 0; i < 14; i++) {
                temp_buffer[i] = rx1_buffer[i];
            }
            HAL_UART_Transmit(&huart2, temp_buffer, 14, 100);

            rx1_index = 0; // 重置索引
        }
        // 如果第一个字节不是A或L或R，重置
        else if(rx1_index == 1 && rx1_buffer[0] != 'A' && rx1_buffer[0] != 'L' && rx1_buffer[0] != 'R') {
            memset(rx1_buffer, 0, sizeof(rx1_buffer));
            rx1_index = 0;
        }
        // 如果超过14字节还没匹配，重置（增加到14以匹配动物数据长度）
        else if(rx1_index > 14) {
            memset(rx1_buffer, 0, sizeof(rx1_buffer));
            rx1_index = 0;
        }

        HAL_UART_Receive_IT(&huart1, &rx1, 1); // 重新启动接收中断
    }




    
    if(huart->Instance == USART2) // 确保是 USART2 的回调
    {
        
        // 防止缓冲区溢出
        if (rx2_index >= sizeof(rx2_buffer) - 1) {
            memset(rx2_buffer, 0, sizeof(rx2_buffer));
            rx2_index = 0; // 缓冲区满时重置，避免丢失数据
        }
        
        rx2_buffer[rx2_index++] = rx2;
        
        // 如果第一个字节不是D或A，直接清空重新接收
        if(rx2_index == 1 && rx2_buffer[0] != 'D' && rx2_buffer[0] != 'A') {
            memset(rx2_buffer, 0, sizeof(rx2_buffer));
            rx2_index = 0;
        }
        // 只有在第一个字节是D或A的情况下才判断帧尾
        else if(rx2_buffer[0] == 'D' || rx2_buffer[0] == 'A') {
            // 检查是否接收到帧尾
            if(rx2 == 'T') {
                
                if(rx2_index > 1) {
                    // 添加字符串结束符，避免后续处理时的边界问题
                    if(rx2_index < sizeof(rx2_buffer)) {
                        rx2_buffer[rx2_index] = '\0';
                    }
                    
                    // 快速判断数据类型并处理
                    uint8_t data_type = rx2_buffer[0];
                    if(data_type == 'D') {
                        parse_complete_path(rx2_buffer, rx2_index - 1);
                    }
                    else if(data_type == 'A') {
                        parse_animal_data(rx2_buffer, rx2_index - 1);
                    }
                }
                
                // 清空缓冲区准备下次接收
                memset(rx2_buffer, 0, sizeof(rx2_buffer));
                rx2_index = 0;
            }
        }
        
        HAL_UART_Receive_IT(&huart2, &rx2, 1); // 重新启动接收中断
    }


}







void refference(void){
    printf("page2.t5.txt=\"%s\"\xff\xff\xff", elephant_data);
    printf("page2.t6.txt=\"%s\"\xff\xff\xff", peacock_data);
    printf("page2.t7.txt=\"%s\"\xff\xff\xff", monkey_data);
    printf("page2.t8.txt=\"%s\"\xff\xff\xff", tiger_data);
    printf("page2.t9.txt=\"%s\"\xff\xff\xff", wolf_data);

    //总数
    printf("page2.t10.txt=\"%d\"\xff\xff\xff", elephant_total);
    printf("page2.t11.txt=\"%d\"\xff\xff\xff", peacock_total);
    printf("page2.t12.txt=\"%d\"\xff\xff\xff", monkey_total);
    printf("page2.t13.txt=\"%d\"\xff\xff\xff", tiger_total);
    printf("page2.t14.txt=\"%d\"\xff\xff\xff", wolf_total);


}

//绘图：单个坐标解析
bool parse_single_coordinate(char* coord_str, GridPoint* point) {
    if(!coord_str || !point) return false;
    
    // 查找A和B位置
    char* a_pos = strchr(coord_str, 'A');
    char* b_pos = strchr(coord_str, 'B');
    
    if(a_pos && b_pos && a_pos < b_pos) {
        int col = atoi(a_pos + 1);
        int row = atoi(b_pos + 1);
        
        // 验证坐标范围
        if(col >= 1 && col <= GRID_COLS && row >= 1 && row <= GRID_ROWS) {
            point->grid_x = col;
            point->grid_y = row;
            point->pixel_x = PIXEL_X(col);
            point->pixel_y = PIXEL_Y(row);
            return true;
        }
    }
    return false;
}

//绘图：全部坐标解析
void parse_complete_path(uint8_t* buffer, int length) {
    if(!buffer || length < 2) return; // 至少需要'D'和一个坐标
    
    // 重置路径计数
    path_count = 0;
    
    // 直接在原缓冲区上操作，避免大块内存复制
    char* coord_data = (char*)(buffer + 1); // 跳过开头的'D'
    int coord_length = length - 1;
    
    // 添加调试信息
    
    
    
    
    // 使用简化的坐标解析：直接按4字节或5字节解析
    int i = 0;
    while(i < coord_length && path_count < MAX_PATH_POINTS) {
        // 查找A字符
        if(coord_data[i] == 'A') {
            // 快速解析：A后面跟1-2位数字，然后B，再跟1-2位数字
            int col = 0, row = 0;
            int j = i + 1;
            
            // 解析列号（A后面的数字）
            while(j < coord_length && coord_data[j] >= '0' && coord_data[j] <= '9') {
                col = col * 10 + (coord_data[j] - '0');
                j++;
            }
            
            // 检查B
            if(j < coord_length && coord_data[j] == 'B') {
                j++;
                // 解析行号（B后面的数字）
                while(j < coord_length && coord_data[j] >= '0' && coord_data[j] <= '9') {
                    row = row * 10 + (coord_data[j] - '0');
                    j++;
                }
                
                // 验证坐标范围并添加到缓冲区
                if(col >= 1 && col <= GRID_COLS && row >= 1 && row <= GRID_ROWS) {
                    path_buffer[path_count].grid_x = col;
                    path_buffer[path_count].grid_y = row;
                    path_buffer[path_count].pixel_x = PIXEL_X(col);
                    path_buffer[path_count].pixel_y = PIXEL_Y(row);
                    path_count++;
                } else {
                    // 坐标超出范围的调试信息
                   
                }
            }
            i = j; // 移动到下一个位置
        } else {
            i++;
        }
    }
    
    // 添加解析结果的调试信息
   
    
    // 检查是否达到最大点数限制
    if(path_count >= MAX_PATH_POINTS) {
        
    }
    
    // 解析完成后绘制路径
    if(path_count > 0) {
        draw_complete_path();
    } else {
        
    }
}

// 解析动物数据 (格式：A01A1B1，T) - 优化版本，减少字符串操作
void parse_animal_data(uint8_t* buffer, int length) {
    if (!buffer || length < 6) return;
    
    // 直接在原缓冲区上操作，跳过帧头A
    char* data = (char*)(buffer + 1);
    int data_len = length - 1; // 只去掉帧头A（帧尾T已经在调用前被去掉了）
    
    // 简化解析：直接扫描而不使用strtok
    int i = 0;
    while(i < data_len) {
        if(data[i] >= '0' && data[i] <= '4') { // 动物类型
            int type = data[i] - '0';
            i++;
            
            if(i < data_len && data[i] >= '0' && data[i] <= '9') { // 数量
                int count = data[i] - '0';
                i++;
                
                // 查找坐标起始位置（A字符）
                if(i < data_len && data[i] == 'A') {
                    int coord_start = i;
                    // 修正：应该找到完整的AxBx格式，而不是简单找逗号
                    int temp_i = i;
                    // 跳过A
                    temp_i++;
                    // 跳过A后面的数字
                    while(temp_i < data_len && data[temp_i] >= '0' && data[temp_i] <= '9') {
                        temp_i++;
                    }
                    // 检查是否有B
                    if(temp_i < data_len && data[temp_i] == 'B') {
                        temp_i++; // 跳过B
                        // 跳过B后面的数字
                        while(temp_i < data_len && data[temp_i] >= '0' && data[temp_i] <= '9') {
                            temp_i++;
                        }
                    }
                    i = temp_i; // 更新i到完整坐标的结束位置
                    
                    // 快速构建记录字符串，避免sprintf
                    char record[16];
                    record[0] = count + '0';
                    int record_len = 1;
                    int coord_len = i - coord_start;
                    if(coord_len < 15) { // 防止溢出
                        for(int j = 0; j < coord_len; j++) {
                            record[record_len++] = data[coord_start + j];
                        }
                        record[record_len] = '\0';
                        
                        // 存储到对应类别（简化版本，减少字符串操作）
                        switch (type) {
                            case 0: // elephant
                            
                                if (elephant_count < MAX_ANIMAL_RECORDS) {
                                    for(int k = 0; k < record_len; k++) {
                                        elephant_records[elephant_count][k] = record[k];
                                    }
                                    elephant_records[elephant_count][record_len] = '\0';
                                    elephant_count++;
                                    elephant_total += count;
                                    
                                    // 更新累积字符串
                                    if(strlen(elephant_data) > 0) {
                                        strcat(elephant_data, ",");
                                    }
                                    strcat(elephant_data, record);
                                    
                                    printf("page1.t5.txt=\"%s\"\xff\xff\xff", record);
                                }
                                break;
                            case 1: // peacock
                                if (peacock_count < MAX_ANIMAL_RECORDS) {
                                    for(int k = 0; k < record_len; k++) {
                                        peacock_records[peacock_count][k] = record[k];
                                    }
                                    peacock_records[peacock_count][record_len] = '\0';
                                    peacock_count++;
                                    peacock_total += count;
                                    
                                    // 更新累积字符串
                                    if(strlen(peacock_data) > 0) {
                                        strcat(peacock_data, ",");
                                    }
                                    strcat(peacock_data, record);
                                    
                                    printf("page1.t6.txt=\"%s\"\xff\xff\xff", record);
                                }
                                break;
                            case 2: // monkey
                                if (monkey_count < MAX_ANIMAL_RECORDS) {
                                    for(int k = 0; k < record_len; k++) {
                                        monkey_records[monkey_count][k] = record[k];
                                    }
                                    monkey_records[monkey_count][record_len] = '\0';
                                    monkey_count++;
                                    monkey_total += count;
                                    
                                    // 更新累积字符串
                                    if(strlen(monkey_data) > 0) {
                                        strcat(monkey_data, ",");
                                    }
                                    strcat(monkey_data, record);
                                    
                                    printf("page1.t7.txt=\"%s\"\xff\xff\xff", record);
                                }
                                break;
                            case 3: // tiger
                                if (tiger_count < MAX_ANIMAL_RECORDS) {
                                    for(int k = 0; k < record_len; k++) {
                                        tiger_records[tiger_count][k] = record[k];
                                    }
                                    tiger_records[tiger_count][record_len] = '\0';
                                    tiger_count++;
                                    tiger_total += count;
                                    
                                    // 更新累积字符串
                                    if(strlen(tiger_data) > 0) {
                                        strcat(tiger_data, ",");
                                    }
                                    strcat(tiger_data, record);
                                    
                                    printf("page1.t8.txt=\"%s\"\xff\xff\xff", record);
                                }
                                break;
                            case 4: // wolf
                                if (wolf_count < MAX_ANIMAL_RECORDS) {
                                    for(int k = 0; k < record_len; k++) {
                                        wolf_records[wolf_count][k] = record[k];
                                    }
                                    wolf_records[wolf_count][record_len] = '\0';
                                    wolf_count++;
                                    wolf_total += count;
                                    
                                    // 更新累积字符串
                                    if(strlen(wolf_data) > 0) {
                                        strcat(wolf_data, ",");
                                    }
                                    strcat(wolf_data, record);
                                    
                                    printf("page1.t9.txt=\"%s\"\xff\xff\xff", record);
                                }
                                break;
                        }
                    }
                }
            }
        }
        
        // 跳过逗号
        if(i < data_len && data[i] == ',') {
            i++;
        } else if(i < data_len) {
            i++; // 跳过其他字符
        }
    }
}


// 清屏函数
void clear_screen(void) {
    // 使用printf发送清屏命令
    printf("cls\xff\xff\xff");
  
}

// 绘制单个坐标点
void draw_point(GridPoint point) {
    // 绘制实心圆，使用红色(RGB565: 31744)标记坐标点
    printf("cirs %d,%d,%d,%d\xff\xff\xff", 
           point.pixel_x, point.pixel_y, 3, 31744);
   
}

// 绘制两点间的连线
void draw_line(GridPoint start, GridPoint end) {
    // 使用绿色(RGB565: 1024)绘制巡查航线
    printf("line %d,%d,%d,%d,%d\xff\xff\xff", 
           start.pixel_x, start.pixel_y,
           end.pixel_x, end.pixel_y, 1024);

}

// 绘制完整的巡查路径 - 优化版本
void draw_complete_path(void) {
    // 清屏
    clear_screen();
    
   
    
    // 批量绘制，减少单独的printf调用
    if(path_count > 0) {
        // 先绘制所有点
        for(int i = 0; i < path_count; i++) {
            printf("cirs %d,%d,%d,%d\xff\xff\xff", 
                   path_buffer[i].pixel_x, path_buffer[i].pixel_y, 3, 31744);
        }
        
        // 再绘制所有线
        for(int i = 1; i < path_count; i++) {
            printf("line %d,%d,%d,%d,%d\xff\xff\xff", 
                   path_buffer[i-1].pixel_x, path_buffer[i-1].pixel_y,
                   path_buffer[i].pixel_x, path_buffer[i].pixel_y, 1024);
        }
        
        
    } else {
        
    }
}

// 重新绘制之前的路径
void redraw_path(void) {
    // 如果有保存的路径数据，重新绘制
    if(path_count > 0) {
        draw_complete_path();
    } else {
        // 如果没有路径数据，只清屏
        clear_screen();
    }
}









