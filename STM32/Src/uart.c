/*
 * uart.c
 *
 *  Created on: 2025. 6. 10.
 *      Author: microsoft
 */

#include "uart.h"

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
  *                the configuration information for the specified UART module.
  * @retval None
  * move from Drivers/STM32F4xx_HAL_driver/Src/stm324xx_hal_uart.c to here
  * ex) comportmaster로부터 1char를 수신하면 HAL_UART_RxCpltCallback으로 진입
  * 9600bps의 경우 HAL_UART_RxCpltCallback를 1ms 이내에는 빠져나가야 함.
  * 115200bps의 경우						   86us
  */

//volatile int temp = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	static volatile int uart1_i = 0;
	static volatile int uart2_i = 0;
	static volatile int uart6_i = 0;

//	HAL_UART_Transmit(&huart2, (uint8_t*)"start", strlen("start"), 100);
	if (huart == &huart1)
	{

//		HAL_UART_Transmit(&huart2, (uint8_t*)"a", strlen("1"), HAL_MAX_DELAY);
		if (uart1_rx_data == '\n')
		{
			uart1_rx_buff[uart1_rear][uart1_i++] = '\n';			// 문장의 끝을 NULL로 함
			uart1_rx_buff[uart1_rear++][uart1_i] = '\0';			// 문장의 끝을 NULL로 함
			uart1_rear %= STRING_NUMBER;		// rear -> 0 ~ 9
//			temp = uart1_i;
			uart1_i = 0;
			// !!!! rx_buff queue full check 하는 logic 추가
		}
		else
		{
			uart1_rx_buff[uart1_rear][uart1_i++] = uart1_rx_data;
			// STRING_LENGTH를 check하는 logic 추가
		}
		HAL_UART_Receive_IT(&huart1, &uart1_rx_data, 1);	// 반드시 집어넣어야 다음 INT가 발생
	}

	if (huart == &huart2)
	{
		if (uart2_rx_data == '\n')
		{
			uart2_rx_buff[uart2_rear][uart2_i++] = '\n';			// 문장의 끝을 NULL로 함
			uart2_rx_buff[uart2_rear++][uart2_i] = '\0';			// 문장의 끝을 NULL로 함
			uart2_rear %= STRING_NUMBER;		// rear -> 0 ~ 9
			uart2_i = 0;
			// !!!! rx_buff queue full check 하는 logic 추가
		}
		else
		{
			uart2_rx_buff[uart2_rear][uart2_i++] = uart2_rx_data;
			// STRING_LENGTH를 check하는 logic 추가
		}
		HAL_UART_Receive_IT(&huart2, &uart2_rx_data, 1);	// 반드시 집어넣어야 다음 INT가 발생
	}

	if (huart == &huart6)
	{
		if (uart6_rx_data == '\n')
		{
			uart6_rx_buff[uart6_rear][uart6_i++] = '\n';			// 문장의 끝을 NULL로 함
			uart6_rx_buff[uart6_rear++][uart6_i] = '\0';			// 문장의 끝을 NULL로 함
			uart6_rear %= STRING_NUMBER;		// rear -> 0 ~ 9
			uart6_i = 0;
			// !!!! rx_buff queue full check 하는 logic 추가
		}
		else
		{
			uart6_rx_buff[uart6_rear][uart6_i++] = uart6_rx_data;
			// STRING_LENGTH를 check하는 logic 추가
		}
		HAL_UART_Receive_IT(&huart6, &uart6_rx_data, 1);	// 반드시 집어넣어야 다음 INT가 발생
	}
//	HAL_UART_Transmit(&huart2, (uint8_t*)"end", strlen("end"), 100);
}

void uart1_processing(void)
{
//	HAL_UART_Transmit(&huart2, (uint8_t*)"1", strlen("1"), HAL_MAX_DELAY);
	if(uart1_front != uart1_rear)	// rx_buff에 data가 존재
	{
		char msg[STRING_LENGTH];
		strcpy(msg, (const char*)uart1_rx_buff[uart1_front]);
		if( msg[0] == MSG )
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}
		else if( msg[0] == PEER )
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}
		else if( msg[0] == RESPONSE )
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}
		else
		{
			HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
			HAL_UART_Transmit(&huart6, (uint8_t*)msg, strlen(msg), 100);
		}

//		HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
//		char a[16] = {0};
//		sprintf(a, "uart1_i = %d//\n", temp);
//		HAL_UART_Transmit(&huart2, (uint8_t*)a, strlen(a), 100);


		uart1_front++;
		uart1_front %= STRING_NUMBER;
	}
}

void uart2_processing(void)
{
//	HAL_UART_Transmit(&huart2, (uint8_t*)"2", strlen("2"), HAL_MAX_DELAY);
	if(uart2_front != uart2_rear)	// rx_buff에 data가 존재
	{
		char msg[STRING_LENGTH];
		strcpy(msg, (const char*)uart2_rx_buff[uart2_front]);
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
//		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

//		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

		uart2_front++;
		uart2_front %= STRING_NUMBER;
	}
}

void uart6_processing(void)
{
//	HAL_UART_Transmit(&huart2, (uint8_t*)"2", strlen("2"), HAL_MAX_DELAY);
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
//		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

//		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

		uart6_front++;
		uart6_front %= STRING_NUMBER;
	}
}

