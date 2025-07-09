/*
 * uart.c
 *
 *  Created on: 2025. 6. 10.
 *      Author: microsoft
 */

#include "uart.h"

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	static volatile int uart1_i = 0;
	static volatile int uart2_i = 0;
	static volatile int uart6_i = 0;

	if (huart == &huart1)
	{

		if (uart1_rx_data == '\n')	// \n이면 문자열 종료
		{
			uart1_rx_buff[uart1_rear][uart1_i++] = '\n';
			uart1_rx_buff[uart1_rear++][uart1_i] = '\0';			// 문장의 끝을 NULL로 함
			uart1_rear %= STRING_NUMBER;
			uart1_i = 0;
		}
		else
		{
			uart1_rx_buff[uart1_rear][uart1_i++] = uart1_rx_data;
		}
		HAL_UART_Receive_IT(&huart1, &uart1_rx_data, 1);
	}

	// if (huart == &huart2)
	// {
	// 	if (uart2_rx_data == '\n')
	// 	{
	// 		uart2_rx_buff[uart2_rear][uart2_i++] = '\n';
	// 		uart2_rx_buff[uart2_rear++][uart2_i] = '\0';
	// 		uart2_rear %= STRING_NUMBER;
	// 		uart2_i = 0;
	// 	}
	// 	else
	// 	{
	// 		uart2_rx_buff[uart2_rear][uart2_i++] = uart2_rx_data;
	// 	}
	// 	HAL_UART_Receive_IT(&huart2, &uart2_rx_data, 1);
	// }

	if (huart == &huart6)
	{
		if (uart6_rx_data == '\n')	// \n이면 문자열 종료
		{
			uart6_rx_buff[uart6_rear][uart6_i++] = '\n';
			uart6_rx_buff[uart6_rear++][uart6_i] = '\0';			// 문장의 끝을 NULL로 함
			uart6_rear %= STRING_NUMBER;
			uart6_i = 0;
		}
		else
		{
			uart6_rx_buff[uart6_rear][uart6_i++] = uart6_rx_data;
		}
		HAL_UART_Receive_IT(&huart6, &uart6_rx_data, 1);
	}
}

void uart1_processing(void)
{
	if(uart1_front != uart1_rear)	// rx_buff에 data가 존재
	{
		char msg[STRING_LENGTH];
		strcpy(msg, (const char*)uart1_rx_buff[uart1_front]);
		if( msg[0] == MSG )
		{
			// HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}
		else if( msg[0] == PEER )
		{
			// HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}
		else if( msg[0] == RESPONSE )
		{
			// HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}
		// else
		// {
		// 	HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
		// 	HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		// }
		uart1_front++;
		uart1_front %= STRING_NUMBER;
	}
}

// void uart2_processing(void)
// {
// //	HAL_UART_Transmit(&huart2, (uint8_t*)"2", strlen("2"), HAL_MAX_DELAY);
// 	if(uart2_front != uart2_rear)	// rx_buff에 data가 존재
// 	{
// 		char msg[STRING_LENGTH];
// 		strcpy(msg, (const char*)uart2_rx_buff[uart2_front]);
// 		if( msg[0] == MSG )
// 		{
// 			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
// 		}
// 		else if ( msg[0] == PLATE_INIT )
// 		{
// 			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
// 		}
// 		else if ( msg[0] == RESPONSE )
// 		{
// 			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
// 		}
// //		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

// //		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

// 		uart2_front++;
// 		uart2_front %= STRING_NUMBER;
// 	}
// }

void uart6_processing(void)
{
	if(uart6_front != uart6_rear)	// rx_buff에 data가 존재
	{
		char msg[STRING_LENGTH];
		strcpy(msg, (const char*)uart6_rx_buff[uart6_front]);
		if( msg[0] == MSG )
		{
			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
		}
		else if ( msg[0] == PLATE_INIT )
		{
			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
		}
		else if ( msg[0] == RESPONSE )
		{
			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
		}
		uart6_front++;
		uart6_front %= STRING_NUMBER;
	}
}

