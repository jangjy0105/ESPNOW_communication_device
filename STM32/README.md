 FreeRTOS 기반으로 일정 주기로 ESP01에 명령을 내리며, RapberryPi와 ESP01 간의 통신을 중계한다.
 이 때 수신받은 UART 신호는 queue로 관리하여 FIFO 방식으로 처리하였다.
