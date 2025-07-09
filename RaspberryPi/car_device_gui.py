import sys
import os
import time
import threading

# --- GUI 및 시리얼 통신 라이브러리 ---
from PyQt5.QtCore import QObject, QThread, pyqtSignal, Qt, QTimer
from PyQt5.QtGui import QFont
from PyQt5.QtWidgets import (
    QApplication, QWidget, QHBoxLayout, QVBoxLayout,
    QListWidget, QLabel, QMessageBox, QSizePolicy,
    QListWidgetItem, QDialog, QLineEdit, QGridLayout, QPushButton
)
try:
    import serial
except ImportError:
    print("pyserial 라이브러리가 없습니다. 'pip install pyserial' 명령어로 설치해주세요.")
    sys.exit(1)

# --- STT/TTS 및 Whisper 라이브러리 ---
try:
    import sounddevice as sd
    from scipy.io.wavfile import write
    import whisper
    import pyttsx3
except ImportError:
    print("라이브러리가 누락되었습니다. 'pip install sounddevice scipy whisper pyttsx3' 명령어로 설치해주세요.")
    sys.exit(1)

CONFIG_FILE = 'vehicle_info.txt'
WHISPER_MODEL_SIZE = 'tiny'
RECORD_DURATION = 5   # 녹음 시간 (초)
SAMPLERATE = 44100    # Whisper 권장 샘플링 레이트
TEMP_WAV = 'temp_record.wav'

# ------------------------------------------------------------------------------------
# 1번 코드의 한글 조합 및 가상 키보드 클래스
# ------------------------------------------------------------------------------------
class HangulAutomata:
    CHOSUNG_START = 0x1100
    JUNGSUNG_START = 0x1161
    JONGSUNG_START = 0x11A8
    HANGUL_START = 0xAC00

    CHOSUNG = ['ㄱ', 'ㄲ', 'ㄴ', 'ㄷ', 'ㄸ', 'ㄹ', 'ㅁ', 'ㅂ', 'ㅃ', 'ㅅ', 'ㅆ', 'ㅇ', 'ㅈ', 'ㅉ', 'ㅊ', 'ㅋ', 'ㅌ', 'ㅍ', 'ㅎ']
    JUNGSUNG = ['ㅏ', 'ㅐ', 'ㅑ', 'ㅒ', 'ㅓ', 'ㅔ', 'ㅕ', 'ㅖ', 'ㅗ', 'ㅘ', 'ㅙ', 'ㅚ', 'ㅛ', 'ㅜ', 'ㅝ', 'ㅞ', 'ㅟ', 'ㅠ', 'ㅡ', 'ㅢ', 'ㅣ']
    JONGSUNG = ['', 'ㄱ', 'ㄲ', 'ㄳ', 'ㄴ', 'ㄵ', 'ㄶ', 'ㄷ', 'ㄹ', 'ㄺ', 'ㄻ', 'ㄼ', 'ㄽ', 'ㄾ', 'ㄿ', 'ㅀ', 'ㅁ', 'ㅂ', 'ㅄ', 'ㅅ', 'ㅆ', 'ㅇ', 'ㅈ', 'ㅊ', 'ㅋ', 'ㅌ', 'ㅍ', 'ㅎ']

    COMPLEX_VOWELS = {('ㅗ', 'ㅏ'): 'ㅘ', ('ㅗ', 'ㅐ'): 'ㅙ', ('ㅗ', 'ㅣ'): 'ㅚ', ('ㅜ', 'ㅓ'): 'ㅝ', ('ㅜ', 'ㅔ'): 'ㅞ', ('ㅜ', 'ㅣ'): 'ㅟ', ('ㅡ', 'ㅣ'): 'ㅢ'}
    COMPLEX_CONSONANTS = {('ㄱ', 'ㅅ'): 'ㄳ', ('ㄴ', 'ㅈ'): 'ㄵ', ('ㄴ', 'ㅎ'): 'ㄶ', ('ㄹ', 'ㄱ'): 'ㄺ', ('ㄹ', 'ㅁ'): 'ㄻ', ('ㄹ', 'ㅂ'): 'ㄼ', ('ㄹ', 'ㅅ'): 'ㄽ', ('ㄹ', 'ㅌ'): 'ㄾ', ('ㄹ', 'ㅍ'): 'ㄿ', ('ㄹ', 'ㅎ'): 'ㅀ', ('ㅂ', 'ㅅ'): 'ㅄ'}

    @staticmethod
    def is_hangul(char):
        return 0xAC00 <= ord(char) <= 0xD7A3

    @staticmethod
    def decompose(char):
        if not HangulAutomata.is_hangul(char):
            return None, None, None

        code = ord(char) - HangulAutomata.HANGUL_START
        jongsung_idx = code % 28
        jungsung_idx = (code // 28) % 21
        chosung_idx = (code // 28) // 21

        return HangulAutomata.CHOSUNG[chosung_idx], HangulAutomata.JUNGSUNG[jungsung_idx], HangulAutomata.JONGSUNG[jongsung_idx]

    @staticmethod
    def combine(cho, jung, jong):
        chosung_idx = HangulAutomata.CHOSUNG.index(cho)
        jungsung_idx = HangulAutomata.JUNGSUNG.index(jung)
        jongsung_idx = HangulAutomata.JONGSUNG.index(jong)

        return chr(HangulAutomata.HANGUL_START + (chosung_idx * 21 * 28) + (jungsung_idx * 28) + jongsung_idx)

class VirtualKeyboard(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        # ─── 프레임 제거 및 전체화면 ───
        self.setWindowFlags(Qt.FramelessWindowHint)
        self.showFullScreen()

        # 화면 크기 계산
        screen = QApplication.primaryScreen().size()
        screen_width = screen.width()
        screen_height = screen.height()
        max_cols = 10
        btn_width = screen_width // max_cols
        btn_height = screen_height // 12   # 버튼 세로 크기

        # 한글 자동조합 및 Shift 상태 초기화
        self.automata = HangulAutomata()
        self.is_shift_pressed = False
        self.shift_map = {'ㅂ': 'ㅃ', 'ㅈ': 'ㅉ', 'ㄷ': 'ㄸ', 'ㄱ': 'ㄲ', 'ㅅ': 'ㅆ'}
        self.shift_map_buttons = {}

        # 레이아웃 마진/간격 제거
        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(0,0,0,0)
        main_layout.setSpacing(0)

        # 위쪽 여백 → 입력 칸을 중앙으로
        main_layout.addStretch()

        # 입력 안내 문구
        title_lbl = QLabel("차 번호를 입력하세요")
        title_lbl.setFont(QFont("Arial", 18))
        title_lbl.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(title_lbl)

        # 입력 칸 (가로 폭 제한)
        self.line_edit = QLineEdit()
        self.line_edit.setFont(QFont('Arial', 16))
        self.line_edit.setReadOnly(True)
        self.line_edit.setMaximumWidth(int(screen_width * 0.6))
        self.line_edit.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(self.line_edit, alignment=Qt.AlignHCenter)

        # 아래 여백 → 키보드는 맨 아래에
        main_layout.addStretch()

        # 키보드 그리드
        grid_layout = QGridLayout()
        grid_layout.setContentsMargins(0,0,0,0)
        grid_layout.setHorizontalSpacing(0)
        grid_layout.setVerticalSpacing(0)

        # 1) 숫자 키 (0행)
        keys_numbers = "1234567890"
        for i, key in enumerate(keys_numbers):
            btn = QPushButton(key)
            btn.setFixedSize(btn_width, btn_height)
            btn.clicked.connect(self.on_simple_key_clicked)
            grid_layout.addWidget(btn, 0, i)

        # 2) 자음/모음 키 (1행,2행,3행)
        keys_row1 = "ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔ"
        keys_row2 = "ㅁㄴㅇㄹㅎㅗㅓㅏㅣ"
        keys_row3 = "ㅋㅌㅊㅍㅠㅜㅡ"
        for i, key in enumerate(keys_row1):
            btn = QPushButton(key)
            btn.setFixedSize(btn_width, btn_height)
            btn.clicked.connect(self.on_key_clicked)
            grid_layout.addWidget(btn, 1, i)
            if key in self.shift_map: self.shift_map_buttons[key] = btn
        for i, key in enumerate(keys_row2):
            btn = QPushButton(key)
            btn.setFixedSize(btn_width, btn_height)
            btn.clicked.connect(self.on_key_clicked)
            grid_layout.addWidget(btn, 2, i)
            if key in self.shift_map: self.shift_map_buttons[key] = btn
        for i, key in enumerate(keys_row3):
            btn = QPushButton(key)
            btn.setFixedSize(btn_width, btn_height)
            btn.clicked.connect(self.on_key_clicked)
            grid_layout.addWidget(btn, 3, i+1)
            if key in self.shift_map: self.shift_map_buttons[key] = btn

        # 3) Shift 키 (3행,0열)
        shift_btn = QPushButton("⇧ Shift")
        shift_btn.setFixedSize(btn_width, btn_height)
        shift_btn.clicked.connect(self.on_shift_clicked)
        grid_layout.addWidget(shift_btn, 3, 0)

        # 4) Backspace 키를 2행 9열에 배치 (’ㅔ’ 아래, ’ㅣ’ 오른쪽)
        back_btn = QPushButton("←")
        back_btn.setFixedSize(btn_width, btn_height)
        back_btn.clicked.connect(self.on_backspace_clicked)
        grid_layout.addWidget(back_btn, 2, len(keys_row2))  # len(keys_row2)=9

        # 5) 입력 완료 버튼 (3행,8~9열)
        enter_btn = QPushButton("입력 완료")
        enter_btn.setFixedSize(btn_width*2, btn_height)
        enter_btn.clicked.connect(self.accept)
        grid_layout.addWidget(enter_btn, 3, len(keys_row3)+1, 1, 2)

        main_layout.addLayout(grid_layout)
        self.setLayout(main_layout)


    def on_simple_key_clicked(self):
        key = self.sender().text()
        self.line_edit.setText(self.line_edit.text() + key)

    def on_shift_clicked(self):
        self.is_shift_pressed = not self.is_shift_pressed
        for base, btn in self.shift_map_buttons.items():
            btn.setText(self.shift_map[base] if self.is_shift_pressed else base)

    def on_key_clicked(self):
        key = self.sender().text()
        text = self.line_edit.text()
        is_vowel = key in self.automata.JUNGSUNG
        if not text:
            self.line_edit.setText(key)
        else:
            last = text[-1]
            cho, jung, jong = self.automata.decompose(last)
            if cho is None:
                if last in self.automata.CHOSUNG and is_vowel:
                    new_char = self.automata.combine(last, key, '')
                    self.line_edit.setText(text[:-1] + new_char)
                else:
                    self.line_edit.setText(text + key)
            else:
                # 자음/모음 결합 로직 (간소화)
                self.line_edit.setText(text + key)
        if self.is_shift_pressed:
            self.on_shift_clicked()

    def on_backspace_clicked(self):
        txt = self.line_edit.text()
        if txt:
            self.line_edit.setText(txt[:-1])

    def get_text(self):
        return self.line_edit.text()
# ------------------------------------------------------------------------------------
# SerialWorker (2번 코드)
# ------------------------------------------------------------------------------------
class SerialWorker(QObject):
    port_opened = pyqtSignal()
    peer_list_updated = pyqtSignal(list)
    message_received = pyqtSignal(str, str)
    response_received = pyqtSignal(str, str)
    log_message = pyqtSignal(str)

    def __init__(self, port='/dev/serial0', baudrate=115200):
        super().__init__()
        self.port, self.baudrate = port, baudrate
        self.running = True
        self.ser = None

    def run(self):
        self.log_message.emit(f"시리얼 포트 {self.port} 연결 시도 중... 속도 {self.baudrate}")
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            self.log_message.emit("시리얼 포트 연결 성공.")
            self.port_opened.emit()
        except Exception as e:
            self.log_message.emit(f"시리얼 포트 연결 실패: {e}")
            return
        while self.running:
            try:
                if self.ser.in_waiting > 0:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if not line: continue
                    flag, content = line[0], line[1:]
                    self.log_message.emit(f"DEBUG: 수신 -> '{line}'")
                    if flag=='2' and ',' in content:
                        car,msg = content.split(',',1)
                        self.message_received.emit(car,msg)
                    elif flag=='3':
                        peers = content.split(',') if content else []
                        self.peer_list_updated.emit(peers)
                    elif flag=='4' and ',' in content:
                        car,status = content.split(',',1)
                        self.response_received.emit(car,status)
            except Exception:
                break
            time.sleep(0.1)
        if self.ser and self.ser.is_open:
            self.ser.close()
        self.log_message.emit("시리얼 스레드 종료.")

    def send_data(self, data):
        if self.ser and self.ser.is_open:
            self.ser.write(data.encode('utf-8'))
            self.log_message.emit(f"DEBUG: 송신 -> '{data.strip()}'")
        else:
            self.log_message.emit("WARN: 포트 미연결, 전송 실패.")

    def stop(self):
        self.running = False

# ------------------------------------------------------------------------------------
# MainApp (2번 코드)
# ------------------------------------------------------------------------------------
class MainApp(QWidget):
    status_update = pyqtSignal(str)
    close_rec_dialog = pyqtSignal()

    def __init__(self, my_car_number):
        super().__init__()
        self.my_car_number = my_car_number
        self.peer_buttons = []
        # ─── 전체화면 + 프레임 제거 ───
        self.setWindowFlags(Qt.FramelessWindowHint)
        self.showFullScreen()

        # Whisper 및 TTS 초기화
        self.whisper_model = whisper.load_model(WHISPER_MODEL_SIZE)
        self.tts = pyttsx3.init(driverName='espeak')
        self.tts.setProperty('rate',150)

        self.initUI()
        self.init_serial_thread()

    def initUI(self):
        layout = QVBoxLayout(self)
        layout.setContentsMargins(20, 20, 20, 20)
        layout.setSpacing(30)
        layout.setAlignment(Qt.AlignTop | Qt.AlignHCenter)

        # --- 최상단 차량 목록 안내 문구 ---
        title_label = QLabel("통신 가능 차량 목록")
        title_label.setAlignment(Qt.AlignTop | Qt.AlignHCenter)
        title_label.setFont(QFont("Arial", 20, QFont.Bold))
        layout.addWidget(title_label)

        # --- 숨겨둘 QListWidget (시그널 재활용용) ---
        self.peer_list_widget = QListWidget()
        self.peer_list_widget.itemClicked.connect(self.on_peer_selected)
        self.peer_list_widget.hide()

        # --- 중앙 큰 박스 (차량 버튼 배치 영역) ---
        center_container = QWidget()
        center_container.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        container_layout = QVBoxLayout(center_container)
        container_layout.setAlignment(Qt.AlignCenter)
        container_layout.setContentsMargins(0, 0, 0, 0)

        center_widget = QWidget()
        center_layout = QGridLayout(center_widget)
        center_layout.setContentsMargins(0, 0, 0, 0)
        center_layout.setSpacing(30)
        # GridLayout에 stretch 설정 (5열 × 4행)
        for col in range(5):
            center_layout.setColumnStretch(col, 1)
        for row in range(4):
            center_layout.setRowStretch(row, 1)
        self.center_layout = center_layout

        center_widget.setLayout(center_layout)
        container_layout.addWidget(center_widget, alignment=Qt.AlignCenter)

        # center_container를 stretch=1로 추가
        layout.addWidget(center_container, stretch=1)
        # 숨겨뒀던 peer_list_widget을 마지막에 추가
        layout.addWidget(self.peer_list_widget)

    def init_serial_thread(self):
        self.thread = QThread()
        self.worker = SerialWorker()
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        self.worker.port_opened.connect(self.send_initial)
        self.worker.peer_list_updated.connect(self.update_peers)
        self.worker.message_received.connect(self.handle_message)
        self.worker.response_received.connect(self.handle_response)
        self.worker.log_message.connect(self.log)
        self.thread.start()

    def send_initial(self):
        msg = f"0{self.my_car_number}\n"
        self.worker.send_data(msg)
        self.log(f"INFO: 초기 차량번호 전송: {self.my_car_number}")

    def update_peers(self, peers):
        # 기존 버튼 모두 제거
        for btn in self.peer_buttons:
            btn.deleteLater()
        self.peer_buttons.clear()

        # 나 자신(my_car_number) 제외, 최대 20개
        filtered_peers = [p for p in peers if p and p != self.my_car_number][:20]

        # 5열 × 4행
        for row in range(4):
            for col in range(5):
                idx = row * 5 + col
                btn = QPushButton()
                # Expanding size policy로 버튼이 균등 확대되도록 설정
                btn.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
                btn.setMinimumSize(125, 70)   # ← 여기를 원하는 크기로 조정하세요
                btn.setFont(QFont("Arial", 18))  # 글씨도 조금 키우면 밸런스가 좋아집니다
                btn.setStyleSheet("QPushButton { text-align: center; padding: 10px; }")

                if idx < len(filtered_peers):
                    peer = filtered_peers[idx]
                    btn.setText(peer)
                    btn.clicked.connect(lambda _, p=peer: self.on_peer_selected_by_name(p))
                else:
                    btn.setEnabled(False)

                self.center_layout.addWidget(btn, row, col, alignment=Qt.AlignCenter)
                self.peer_buttons.append(btn)

        self.log(f"INFO: 차량 목록 업데이트 완료. {len(filtered_peers)}개")

    def show_tts_dialog(self, car, msg):
        dlg = QDialog(self)
        dlg.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog)
        dlg.showFullScreen()

        layout = QVBoxLayout()
        dlg.setLayout(layout)

        lbl_car = QLabel(f"송신 차량: {car}", dlg)
        lbl_car.setAlignment(Qt.AlignCenter)
        font = lbl_car.font()
        font.setPointSize(12)
        font.setBold(True)
        lbl_car.setFont(font)
        layout.addWidget(lbl_car)

        layout.addStretch()
        lbl_msg = QLabel(msg)
        lbl_msg.setWordWrap(True)
        lbl_msg.setAlignment(Qt.AlignCenter)
        font_msg = lbl_msg.font()
        font_msg.setPointSize(16)
        lbl_msg.setFont(font_msg)
        layout.addWidget(lbl_msg)
        layout.addStretch()

        threading.Thread(target=self._play_tts, args=(msg,), daemon=True).start()
        QTimer.singleShot(5000, dlg.accept)
        dlg.exec_()

    def _play_tts(self, text):
        try:
            for i in range(0,len(text),100):
                self.tts.say(text[i:i+100])
                self.tts.runAndWait()
        except Exception as e:
            self.log(f"ERROR: TTS 오류 - {e}")

    def handle_message(self, car, msg):
        dlg = QDialog(self)
        dlg.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog)
        dlg.setModal(True)
        dlg.showFullScreen()

        layout = QVBoxLayout()
        dlg.setLayout(layout)

        lbl_title = QLabel(f"[{car}]로부터 메시지를 수신하시겠습니까?", dlg)
        lbl_title.setAlignment(Qt.AlignCenter)
        lbl_title.setFont(QFont("Arial", 18, QFont.Bold))
        layout.addStretch()
        layout.addWidget(lbl_title)
        layout.addStretch()

        btn_layout = QHBoxLayout()
        btn_yes = QPushButton("예")
        btn_no = QPushButton("아니오")

        btn_yes.setFixedHeight(60)
        btn_no.setFixedHeight(60)
        btn_yes.setFont(QFont("Arial", 14))
        btn_no.setFont(QFont("Arial", 14))

        btn_layout.addWidget(btn_yes)
        btn_layout.addWidget(btn_no)
        layout.addLayout(btn_layout)

        accepted = []

        btn_yes.clicked.connect(lambda: (accepted.append(True), dlg.accept()))
        btn_no.clicked.connect(dlg.reject)

        if dlg.exec_() == QDialog.Accepted and accepted:
            self.log(f"MSG: {car} 메시지 수신 수락.")
            self.show_tts_dialog(car, msg)
        else:
            self.log(f"MSG: {car} 메시지 수신 거부.")

    def on_peer_selected_by_name(self, car):
        item = QListWidgetItem(car)
        self.peer_list_widget.clear()
        self.peer_list_widget.addItem(item)
        self.peer_list_widget.setCurrentItem(item)
        self.on_peer_selected(item)

    def on_peer_selected(self, item):
        car = item.text()
        rec_dlg = QDialog(self)
        rec_dlg.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog)

        rec_layout = QVBoxLayout()
        rec_layout.setContentsMargins(0, 0, 0, 0)
        rec_layout.setSpacing(0)
        rec_dlg.setLayout(rec_layout)

        label = QLabel("녹음하세요…", rec_dlg)
        label.setAlignment(Qt.AlignCenter)
        font = label.font(); font.setPointSize(14)
        label.setFont(font)
        rec_layout.addWidget(label)

        self.status_update.connect(label.setText)
        self.close_rec_dialog.connect(rec_dlg.accept)

        def record_and_send():
            text = ""
            try:
                sd.default.device = 1#편집(성은)

                rec = sd.rec(int(RECORD_DURATION*SAMPLERATE),
                             samplerate=SAMPLERATE, channels=1, dtype='int16')
                sd.wait()
                write(TEMP_WAV, SAMPLERATE, rec)

                self.status_update.emit("변환중…")
                if os.path.exists(TEMP_WAV):
                    try:
                        res = self.whisper_model.transcribe(
                            TEMP_WAV, language='ko', task='transcribe', fp16=False
                        )
                        text = res.get('text', '').strip()
                    except Exception as e:
                        self.log(f"ERROR: Whisper 변환 실패 – {e}")
                else:
                    self.log("WARN: WAV 파일을 찾을 수 없습니다")

                if text:
                    msg = f"2{car},{text}\n"
                    self.worker.send_data(msg)
                else:
                    self.log("WARN: 변환된 텍스트가 없습니다")
            except Exception as e:
                self.log(f"ERROR in record_and_send – {e}")
            finally:
                if os.path.exists(TEMP_WAV):
                    os.remove(TEMP_WAV)
                self.close_rec_dialog.emit()

        threading.Thread(target=record_and_send, daemon=True).start()

        rec_dlg.showFullScreen()
        rec_dlg.exec_()

        self.status_update.disconnect(label.setText)
        self.close_rec_dialog.disconnect(rec_dlg.accept)


    def handle_response(self, car, status):
        result = f"[{car}] 전송 {'성공' if status=='1' else '실패'}"
        self.log(f"RESPONSE: {result}")

        dlg = QDialog(self)
        dlg.setWindowFlags(Qt.FramelessWindowHint | Qt.Dialog)
        dlg.setModal(True)
        dlg.showFullScreen()

        layout = QVBoxLayout()
        dlg.setLayout(layout)

        label = QLabel(result)
        label.setAlignment(Qt.AlignCenter)
        label.setFont(QFont("Arial", 18))
        layout.addStretch()
        layout.addWidget(label)
        layout.addStretch()

        QTimer.singleShot(2000, dlg.accept)
        dlg.exec_()



    def log(self, msg):
        print(f"[{time.strftime('%H:%M:%S')}] {msg}")

    def closeEvent(self, event):
        self.worker.stop()
        self.thread.quit(); self.thread.wait(2000)
        event.accept()

# ------------------------------------------------------------------------------------
# 프로그램 시작 및 가상 키보드 적용
# ------------------------------------------------------------------------------------
if __name__ == '__main__':
    app = QApplication(sys.argv)
    num = None
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE) as f:
            num = f.read().strip()
    if not num:
        kb = VirtualKeyboard()
        if kb.exec_() == QDialog.Accepted:
            text = kb.get_text().strip()
            if text:
                num = text
                with open(CONFIG_FILE, 'w') as f:
                    f.write(num)
    if not num:
        QMessageBox.critical(None, None, "차량 번호가 설정되지 않아 프로그램을 종료합니다.")
        sys.exit(0)

    window = MainApp(num)
    sys.exit(app.exec_())
