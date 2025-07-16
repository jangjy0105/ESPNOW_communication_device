# 🚗 ESP-NOW 기반 차량 간 통신 디바이스

> **프로젝트 기간**: 2025.06.09 ~ 2025.07.05  
> **담당 역할**: 전체 시스템 설계, ESP01 펌웨어 개발, STM32 펌웨어 개발, 시스템 통합

---

## 📌 프로젝트 개요

ESP-NOW를 기반으로 한 차량 간 메시지 송수신 시스템입니다.  
PyQt 기반의 GUI를 통해 터치스크린에서 직관적인 조작이 가능하며, **STT(Speech-to-Text)**, **TTS(Text-to-Speech)** 기능을 탑재하여 음성으로도 간편하게 텍스트 기반 통신을 수행할 수 있습니다.  
또한, **오프라인 환경**에서도 사용이 가능하도록 **인터넷 연결 없이 작동**하도록 설계되었습니다.

---

## 🧩 시스템 구성

![System Architecture](https://github.com/user-attachments/assets/fd39ac15-fd1c-458d-a9a2-eecfc9bd3408)

- **ESP8266 (ESP-01)**  
  - ESP-NOW 기반의 디바이스 간 무선 통신 담당
  - UART를 통해 STM32와 통신

- **STM32**  
  - FreeRTOS 기반으로 명령 제어
  - ESP-01과 Raspberry Pi 간 UART 중계

- **Raspberry Pi**  
  - PyQt 기반 GUI 제공
  - USB 마이크/스피커를 통한 음성 녹음 및 재생
  - STT / TTS 처리 및 터치스크린 인터페이스

---

## ⚙️ 주요 기능

- 📡 **ESP-NOW 무선 통신**을 통한 차량 간 메시지 송수신
- 🧠 **STT (음성 → 텍스트)** 기능으로 운전 중 음성 입력 가능
- 🔊 **TTS (텍스트 → 음성)** 기능으로 수신 메시지를 음성으로 안내
- 📱 **PyQt GUI 기반 터치스크린 UI** 제공
- 🌐 **인터넷 없이도 사용 가능한 완전 오프라인 시스템**
- 🔌 **UART 통신** 기반의 보드 간 신호 전달 및 제어

---

## 📷 시연 이미지 및 영상

![통신디바이스](https://github.com/user-attachments/assets/9075feef-23d8-43a8-9340-cbb46f8ee509)
https://www.youtube.com/watch?v=OlyLxImdYlo

---

## 🛠️ 개발 환경 및 도구

| 구성 요소 | 사용 도구/보드 |
|-----------|----------------|
| MCU             | STM32F411RE  |
| Communication   | ESP8266 (ESP-01) |
| SBC             | Raspberry Pi 4 |
| GUI             | PyQt5 (Python) |
| 음성 처리        | Openai-Whisper (STT), pyttsx3 (TTS) |
| 인터페이스       | 터치스크린, USB 마이크/스피커 |

---

## 🙋 기타

본 프로젝트는 실시간 차량 간 통신의 직관성과 편의성을 높이기 위해 기획되었으며,  
외부 통신망 없이도 차량 간 의사소통이 가능하도록 설계되었습니다.

---
