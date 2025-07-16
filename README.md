# ESP-NOW 기반 차량 간 통신 시스템

---

###  담당 역할
- 전체 시스템 설계
- ESP01 펌웨어 개발
- STM32 펌웨어 개발
- 시스템 통합

### 사용 기술
- **C**
- **ESP-NOW**
- **UART**
- **FreeRTOS**
- **CRC16**

---

## 1. 개요
- 차량 간 메시지를 송수신하는 시스템
- **PyQt** 기반의 GUI를 통해 터치스크린으로 직관적인 조작 가능
- **STT (Speech-to-Text)** 및 **TTS (Text-to-Speech)**를 활용하여 음성만으로 통신 (오해 및 다툼 발생 가능성 최소화)
- 인터넷 연결이 없는 오프라인 환경에서 독립적으로 동작

---

## 2. 시스템 구성
- **하드웨어**: ESP8266(ESP01), STM32, Raspberry Pi, USB 마이크/스피커, 터치스크린
- **통신**:
    - 보드 간: **UART**
    - 디바이스 간: **ESP-NOW**

<img width="773" height="576" alt="image" src="https://github.com/user-attachments/assets/7618a7a0-737c-4305-b599-93293f2e2fb1" />



---

## 3. 개발 및 구현 내용

### 1) 패킷 포맷 및 통신
- 애플리케이션 레벨에서 페이로드의 첫 1바이트를 데이터 식별자로 사용
- 데이터 종류 (5가지): `차량번호 설정`, `광고`, `메시지`, `Peer 목록`, `응답`

### 2) ESP8266 (ESP01)
- 외부 제어 신호(UART)에 따라 동작
- **STM32로부터 수신한 신호 처리**:
    - ① 차량번호 광고
    - ② Peer 목록 관리 및 UART 전송
    - ③ ESP-NOW 메시지 송신
- **다른 디바이스로부터 수신한 ESP-NOW 신호 처리**:
    - ① Peer 목록 업데이트 (광고 수신 시)
    - ② ESP-NOW 응답 및 UART 전송 (메시지 수신 시)
    - ③ UART로 데이터 전송

### 3) STM32
- **FreeRTOS** 기반으로 동작하며 주기적으로 ESP01에 명령어 전송
- Raspberry Pi와 ESP01 간의 통신을 중계
- 수신된 UART 신호는 Queue로 관리하여 **FIFO** 방식으로 순차 처리

### 4) Raspberry Pi
- **PyQt** 기반의 GUI 애플리케이션 실행
- STM32로부터 수신한 Peer 목록을 화면에 출력
- 수신된 메시지를 **TTS**로 변환하여 음성 출력
- 사용자 음성을 녹음하여 **STT**로 텍스트 변환 후 STM32로 전송

---

## 4. 결과 및 시연 영상
![통신디바이스](https://github.com/user-attachments/assets/cbd59949-d0b5-499e-8b9f-2b332891d670)
- [시연 영상](https://www.youtube.com/watch?v=OlyLxImdYlo)

---

## 5. 결론 및 시사점
- **개선 필요 사항**:
    - STT 처리 속도 및 정확도 향상
    - TTS 음질 개선
- **향후 발전 방향**:
    - ESP01 펌웨어를 Arduino IDE 대신 **ESP-IDF** 환경에서 개발 시, **FreeRTOS**를 직접 활용하여 **STM32를 제거**하고 시스템을 단순화하여 양산 단가 절감 가능
    - **ESP32** 칩 사용 시, ESP-NOW 콜백 함수 내에서 **RSSI (수신 신호 강도)** 값을 직접 획득할 수 있어, 신호 강도 기준으로 주변 차량(Peer) 목록을 정렬하는 기능 추가 가능
