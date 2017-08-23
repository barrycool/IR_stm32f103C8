#ifndef COMM_H
#define COMM_H

#include <stdio.h>
#include "stm32f1xx_hal.h"

/*
#include "stm32f10x_conf.h"
int fputc(int ch, FILE *f);
*/

void delay_n_0_1_ms(uint8_t time);
void delay_ms(uint32_t time);

#endif