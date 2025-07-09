# 프로젝트명: ESP-NOW 기반 차량 간 통신 디바이스
# 수행 기간: 2025.06.09 ~ 2025.07.05
# 담당 역할: 전체 시스템 설계, ESP01 펌웨어 개발, STM32 펌웨어 개발, 시스템 통합
![image](https://github.com/user-attachments/assets/fd39ac15-fd1c-458d-a9a2-eecfc9bd3408)
시스템은 크게 ESP8266(ESP01), STM32, RaspberryPi로 구성된다. 
각 보드들은 UART를 통해 통신하며, 수신한 UART 신호에 따라 동작을 수행한다.
각 디바이스끼리는 ESP-NOW를 통해 통신한다.
음성 녹음, 재생은 라즈베리파이에 USB 마이크, USB 스피커를 활용하며, 화면은 터치스크린을 활용한다.
