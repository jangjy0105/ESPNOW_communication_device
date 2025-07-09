#include <ESP8266WiFi.h>
#include <espnow.h>
extern "C" {
  #include <user_interface.h>
}

#define UART_BAUD               115200
#define CHANNEL                 1
#define PEER_TIMEOUT            30000  // 30초 동안 미수신 시 삭제
#define UART_SIZE               20
#define UART_LENGTH             256

#define PLATE_INIT '0'
#define ADV   '1'
#define MSG   '2'
#define PEER  '3'
#define RESPONSE  '4'
#define RESPONSE_ERR      '0'
#define RESPONSE_SUCCESS  '1'

char uart_buff[UART_SIZE][UART_LENGTH];
int rear = 0;
int front = 0;

struct peer_info {    // peer 정보 구조체
  uint8_t mac[6];     // MAC주소
  char plate[16];     // 차량 번호 문자열
  unsigned long lastSeen;   // 마지막으로 광고 수신한 시점
};

#define MAX_PEERS 20
peer_info peers[MAX_PEERS];   // peer_info 구조체 배열
int peerCount = 0;

char PLATE_NO[16] = {0};
unsigned long lastBroadcast = 0;
unsigned long lastListTX   = 0;

// 1) 브로드캐스트 주소 전역 선언
static uint8_t broadcastAddress[6] = {
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

void setup() {
  // 부팅 직후 안정화 대기
  delay(500);

  Serial.begin(UART_BAUD);
  Serial.setTimeout(100);

  Serial.println("aa");

  // STA 모드 설정 및 기존 AP 연결 해제
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // UART로 stm32에 "start" 전송
  Serial.println("start");
  delay(100);

  while(PLATE_NO[0] == 0)   // stm32로부터 차량 번호 수신 대기
  {
    if (Serial.available())
    {
      char temp[16] = {0};
      size_t len = Serial.readBytesUntil('\n', temp, sizeof(temp)-1);
      temp[len] = '\0'; 
      if( len>0 && temp[0]==PLATE_INIT )    // 차량 번호 수신 시
      {
        strncpy(PLATE_NO, (temp+1), sizeof(PLATE_NO)-1);  // 차량 번호 초기화
        PLATE_NO[sizeof(PLATE_NO)-1] = '\0';
      }
    }
  }
  
  // ESP-NOW 초기화
  int result = esp_now_init();
  if (result != 0) 
  {
    Serial.print("[ERR]ESP-NOW init failed, code: ");
    Serial.println(result);
    return;
  }

  // 역할 설정
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);

  // 채널 설정
  wifi_set_channel(CHANNEL);
  delay(100);

  // 브로드캐스트 peer 등록
  result = esp_now_add_peer(
    broadcastAddress,
    ESP_NOW_ROLE_COMBO,
    CHANNEL,
    NULL, 0
  );
  if (result != 0)
  {
    Serial.print("[ERR]Broadcast peer add failed, code: ");
    Serial.println(result);
  }
  
  Serial.println("setup done");

  // 수신 콜백 등록
  esp_now_register_recv_cb(onDataReceive);

}

void loop()
{
  check_rx();         // uart 수신 체크
  check_uart_buff();  // uart 버퍼 체크 및 동작
  yield();
}

void check_rx()
{
  static volatile int i = 0;
  if (Serial.available())   // uart 데이터 수신 시 해당 문자열 uart_buff에 추가
  {
    char c = Serial.read();
    if (c != '\r')
    {
      if(c == '\n')
      {
        uart_buff[rear][i] = '\0';
        rear = (rear + 1) % UART_SIZE;
        i = 0;
      }
      else if (i < UART_LENGTH - 1)
      {
        uart_buff[rear][i++] = c;
      }
    }
  }
}

void check_uart_buff()
{
  if (front != rear)    // uart_buff에 데이터 추가 됐을 시
  {
    char* packet = uart_buff[front];  
    if( packet[0] == MSG )        // 메시지 데이터일때 
      send_esp((packet+1), MSG);    // esp-now로 송신
    else if ( packet[0] == ADV )  // 광고 명령어일 때
      send_adv();                   // 차량 번호 광고
    else if ( packet[0] == PEER ) // Peer 명령어일 때
    {
      check_peer_timeout();         // 마지막 광고 송신이 오래된 Peer 삭제
      send_peer_list();             // Peer 리스트 uart 송신
    }
    else if ( packet[0] == RESPONSE ) // 응답 데이터일 때
      send_esp((packet+1), RESPONSE);   // esp-now로 송신
    
    front = (front + 1) % UART_SIZE;
  }
}

void send_esp(char* str, char flag)   // esp-now 송신 함수
{
  uint8_t packet[256] = {0};          // 송신할 패킷(페이로드)
  char plate[16] = {0};               // 송신 대상 차량 번호
  char* comma = strchr(str, ',');     // 넘겨받은 문자열에서 ','의 주소
  
  strncpy(plate, str, comma-str);     // plate에 ',' 직전까지 복사 (차량번호임)
  packet[0] = flag;                   // 패킷(페이로드)의 첫 글자를 넘겨받은 플래그로 함

  strcpy((char *)(packet+1), comma+1);  // 패킷(페이로드)에 넘겨받은 문자열의 콤마 이후의 문자열을 복사함
  
  uint8_t packet_len; // 패킷(페이로드)의 길이
  if (flag == MSG)
    packet_len = generate_packet(packet);   // 패킷(페이로드)의 끝에 crc16 값을 추가하고 길이 반환
  else
    packet_len = strlen((char *)packet);    // 응답일 경우 crc16 없음

  for (int i = 0; i < peerCount; i++)   // 차량 번호를 기준으로 MAC주소 찾는 반복문
  {
    if ( strcmp(peers[i].plate, plate) == 0 )
    {
      esp_now_send(peers[i].mac, (uint8_t*)packet, packet_len);   // 찾은 MAC주소에 패킷 송신
      break;
    }
  }
}

void send_adv() {         // 차량 번호 광고 함수
  uint8_t adv_packet[32] = {0};
  adv_packet[0] = ADV;    // 패킷(페이로드)의 첫 글자는 ADV 플래그
  strcpy((char *)(adv_packet+1), PLATE_NO);   // 두번째 글자부터 자신의 차량번호 복사
  uint8_t packet_len = generate_packet(adv_packet);   // 패킷(페이로드)의 끝에 crc16 값을 추가하고 길이 반환
  
  int result = esp_now_send( broadcastAddress, (uint8_t*)adv_packet, packet_len );  // 브로드캐스트로 차량 번호 전송

  // if (result != 0) 
  // {
  //   Serial.print("[ERR]Send failed, code: ");
  //   Serial.println(result);
  // } 
  // else Serial.println("Send done");
}

void send_peer_list() {     // peer 리스트 uart 전송 함수
  Serial.print(PEER);       // PEER 플래그 전송
  for (int i = 0; i < peerCount; i++)   // peer_info 구조체 배열 순회
  {
    Serial.print(peers[i].plate);
    if (i < peerCount - 1) Serial.print(",");   // ','로 구분하여 등록된 모든 peer 전송
  }
  Serial.println();       // 개행문자로 마무리
}

void check_peer_timeout() {   // 오래된 peer 삭제 함수
  unsigned long now = millis();     // 현재 시점
  for (int i = 0; i < peerCount;) {   // peer_info 구조체 배열 순회
    if (now - peers[i].lastSeen > PEER_TIMEOUT)   // 마지막으로 광고 수신한 시점이 PEER_TIMEOUT보다 오래 됐으면
    {
      esp_now_del_peer(peers[i].mac);       // 해당 MAC주소 peer 삭제
      for (int j = i; j < peerCount - 1; j++) // 배열 한 칸 앞으로 당기기
        peers[j] = peers[j + 1];
      peerCount--;
    } 
    else i++;
  }
}

void onDataReceive(uint8_t *mac, uint8_t *data, uint8_t len) {    // esp-now 수신 콜백함수
  char plate[16] = {0};       // 전송한 차량 번호
  uint8_t packet[256] = {0};  // 수신 패킷(페이로드)

  if( data[0] == RESPONSE )   // 응답 데이터이면
  {
    memcpy(packet, data, len);
    if ( find_plate(mac, plate) ) // 전송 차량 MAC주소 기반으로 차량 번호 탐색
    {
      Serial.print((char)packet[0]);  // 응답 플래그
      Serial.print(plate);            // 차량번호
      Serial.print(",");              // ','(구분자)
      Serial.println((char *)(packet+1));   // 패킷 내용 (응답 데이터의 경우 0 or 1)
    }
    return; // 함수 종료
  }

  memcpy(packet, data, len-2);    // packet에 crc16값을 제외하고 복사
  uint16_t crc = (data[len-2] << 8) | (data[len-1] & 0xff);   // 수신받은 crc16 값
  uint16_t cal_crc = crc16((packet+1), len-3);                // 직접 계산한 crc16 값
  
  if ( crc != cal_crc )   // 불일치 시(오류 발생 시)
  {
    if( packet[0] == MSG )  // 메시지 데이터이면
    {
      uint8_t response_packet[32] = {0};
      response_packet[0] = RESPONSE;
      response_packet[1] = RESPONSE_ERR;
      esp_now_send(mac, (uint8_t*)response_packet, 2);   // 실패 응답
    }
    return;   // 함수 종료 (광고에서 오류 발생 시 별다른 처리 없이 종료)
  }
    // 오류 미발생
  if ( packet[0] == ADV )   // 광고 데이터이면
  {
    strcpy(plate, (char *)(packet+1));
    update_peer(mac, plate);          // 해당 차량 번호 기준으로 peer 업데이트
  } 
  else if ( packet[0] == MSG ) // 메시지 데이터이면
  {
    uint8_t response_packet[4] = {0};
    response_packet[0] = RESPONSE;
    if ( find_plate(mac, plate) )   // MAC주소 기반으로 차량 번호 탐색 
    {
      Serial.print((char)packet[0]);
      Serial.print(plate);
      Serial.print(",");
      Serial.println((char *)(packet+1));   // uart로 stm32에 수신한 메세지 전송
      response_packet[1] = RESPONSE_SUCCESS;  // 전송한 차량에게 성공 응답
    }
    else // 차량 번호 탐색 실패 시 (사실 이럴 일은 거의 없음)
      response_packet[1] = RESPONSE_ERR;    // 실패 응답
    esp_now_send(mac, (uint8_t*)response_packet, 2);
  }
  else if ( packet[0] == RESPONSE ) // 응답 데이터 이면
  {
    if ( find_plate(mac, plate) )   // 차량번호 탐색
    {
      Serial.print((char)packet[0]);
      Serial.print(plate);
      Serial.print(",");
      Serial.println((char *)(packet+1));   // uart로 stm32에 수신한 응답 전송
    }
  }
}

int find_plate(uint8_t *mac, char* plate) {   // MAC주소 기반 차량 번호 탐색 함수
  for (int i = 0; i < peerCount; i++) 
  {
    if (memcmp(peers[i].mac, mac, 6) == 0)
    {
      strcpy(plate, peers[i].plate);
      return 1;
    }
  }
  return 0;
}

void update_peer(uint8_t *mac, char* plate) {   // peer 업데이트 함수
  // 이미 있는 peer면 lastSeen만 갱신
  for (int i = 0; i < peerCount; i++) {
    if (memcmp(peers[i].mac, mac, 6) == 0) {
      peers[i].lastSeen = millis();
      return;
    }
  }

  if (peerCount >= MAX_PEERS) return;

  // 새로운 peer 등록
  memcpy(peers[peerCount].mac, mac, 6);
  strcpy(peers[peerCount].plate, plate);
  peers[peerCount].lastSeen = millis();

  // peer 리스트에도 등록 (CHANNEL, ROLE 동일)
  esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, CHANNEL, NULL, 0);
  peerCount++;
}

uint8_t generate_packet(uint8_t *packet)    // 패킷(페이로드) 끝에 crc16 값 추가하고, 길이 반환하는 함수
{
  uint8_t idx = strlen((char *)packet);
  uint16_t crc = crc16((uint8_t *)(packet+1), idx-1);
  packet[idx++] = (crc >> 8);
  packet[idx++] = crc & 0xff;
  return idx;
}

uint16_t crc16(const uint8_t *data, size_t length) {    // crc16 계산  함수
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1;
    }
  }
  return crc;
}
