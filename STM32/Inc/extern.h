/*
 * extern.h
 *
 *  Created on: Jun 10, 2025
 *      Author: microsoft
 */

#ifndef INC_EXTERN_H_
#define INC_EXTERN_H_

#define STRING_NUMBER 	30
#define STRING_LENGTH 	1024

extern volatile uint8_t uart1_rx_buff[STRING_NUMBER][STRING_LENGTH];		// URAT0로부터 들어온 문자를 저장하는 버퍼(변수)
extern volatile uint8_t uart2_rx_buff[STRING_NUMBER][STRING_LENGTH];		// URAT0로부터 들어온 문자를 저장하는 버퍼(변수)
extern volatile uint8_t uart6_rx_buff[STRING_NUMBER][STRING_LENGTH];		// URAT0로부터 들어온 문자를 저장하는 버퍼(변수)

extern volatile int uart1_rear;		// input index: UART0_RX_vect에서 넣어주는 index
extern volatile int uart1_front;
extern volatile int uart2_rear;		// input index: UART0_RX_vect에서 넣어주는 index
extern volatile int uart2_front;
extern volatile int uart6_rear;		// input index: UART0_RX_vect에서 넣어주는 index
extern volatile int uart6_front;

extern void uart1_processing(void);
extern void uart2_processing(void);
extern void uart6_processing(void);

#define PLATE_INIT	'0'

#define	ADV			'1'
#define MSG			'2'
#define PEER		'3'
#define RESPONSE	'4'

#endif /* INC_EXTERN_H_ */
