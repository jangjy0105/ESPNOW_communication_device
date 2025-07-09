/*
 * uart.h
 *
 *  Created on: 2025. 6. 10.
 *      Author: microsoft
 */

#ifndef INC_UART_H_
#define INC_UART_H_

#include "main.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart6;

extern uint8_t uart1_rx_data;
extern uint8_t uart2_rx_data;
extern uint8_t uart6_rx_data;

#define STRING_NUMBER 	30
#define STRING_LENGTH 	1024

volatile uint8_t uart1_rx_buff[STRING_NUMBER][STRING_LENGTH];		// URAT0로부터 들어온 문자를 저장하는 버퍼(변수)
volatile uint8_t uart2_rx_buff[STRING_NUMBER][STRING_LENGTH];		// URAT0로부터 들어온 문자를 저장하는 버퍼(변수)
volatile uint8_t uart6_rx_buff[STRING_NUMBER][STRING_LENGTH];		// URAT0로부터 들어온 문자를 저장하는 버퍼(변수)

volatile int uart1_rear = 0;		// input index: UART0_RX_vect에서 넣어주는 index
volatile int uart1_front = 0;		// output index

volatile int uart2_rear = 0;		// input index: UART0_RX_vect에서 넣어주는 index
volatile int uart2_front = 0;		// output index

volatile int uart6_rear = 0;		// input index: UART0_RX_vect에서 넣어주는 index
volatile int uart6_front = 0;		// output index

void uart1_processing(void);
void uart2_processing(void);
void uart6_processing(void);

#define PLATE_INIT	'0'

#define	ADV			'1'
#define MSG			'2'
#define PEER		'3'
#define RESPONSE	'4'

#endif /* INC_UART_H_ */
