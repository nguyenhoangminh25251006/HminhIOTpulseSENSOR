/*
  Pulse Sensor BPM Reader (Arduino)
  Thuật toán đo nhịp tim (BPM) từ tín hiệu Analog của Pulse Sensor.
*/

const int PulsePin = A0;        // Chân đọc tín hiệu Analog từ Pulse Sensor
const int LED = LED_BUILTIN;    // LED báo nhịp tim tích hợp trên Arduino

int signal;                     // Giá trị điện áp Analog đọc từ cảm biến (0 - 1023)
int threshold = 130;            // Ngưỡng phát hiện đỉnh xung nhịp tim

unsigned long lastBeatTime = 0; // Mốc thời gian (ms) của nhịp đập trước
unsigned long beatInterval = 0; // Khoảng thời gian IBI (Inter-Beat Interval) giữa 2 nhịp (ms)

// Giới hạn sinh lý nhịp tim người để lọc nhiễu:
const int MIN_IBI = 300;   // 300 ms -> tương đương tối đa 200 BPM
const int MAX_IBI = 1500;  // 1500 ms -> tương đương tối thiểu 40 BPM

// Bộ nhớ đệm tính trung bình trượt 4 nhịp (Moving Average Buffer)
int bpmBuffer[4];
int bpmIndex = 0;
bool bufferFilled = false;

// Cờ phát hiện sườn lên (Rising Edge Detection)
bool aboveThreshold = false;

void setup() {
  Serial.begin(115200);         // Baudrate kết nối với Raspberry Pi / PC
  pinMode(LED, OUTPUT);
}

void loop() {
  signal = analogRead(PulsePin);
  unsigned long now = millis();

  // 1. Phát hiện sườn lên (Rising Edge): tín hiệu vượt ngưỡng và nhịp trước đã kết thúc
  if (signal > threshold && !aboveThreshold) {
    aboveThreshold = true;      // Đánh dấu đang ở đỉnh xung, chống đếm lặp 1 nhịp

    if (lastBeatTime > 0) {
      // 2. Tính khoảng thời gian giữa 2 nhịp tim liên tiếp (IBI)
      beatInterval = now - lastBeatTime;

      // 3. Lọc nhiễu sinh lý: chỉ nhận nhịp trong khoảng 40 - 200 BPM
      if (beatInterval >= MIN_IBI && beatInterval <= MAX_IBI) {
        // Quy đổi từ chu kỳ (ms) sang nhịp/phút: BPM = 60000 / IBI
        int bpm = 60000 / beatInterval;

        // Lưu vào bộ đệm để làm mượt dữ liệu
        bpmBuffer[bpmIndex++] = bpm;
        if (bpmIndex >= 4) {
          bpmIndex = 0;
          bufferFilled = true;
        }

        // 4. Khi bộ đệm đã đủ dữ liệu, tính trung bình cộng 4 nhịp gần nhất
        if (bufferFilled) {
          int sum = 0;
          for (int i = 0; i < 4; i++) {
            sum += bpmBuffer[i];
          }
          int avgBPM = sum / 4;

          // Gửi kết quả BPM qua cổng Serial
          Serial.println(avgBPM);
          digitalWrite(LED, HIGH);  // Bật LED báo nhịp
        }
      }
    }

    lastBeatTime = now;
  }

  // 5. Hạ cờ với vùng trễ (Hysteresis - Schmitt Trigger):
  // Chỉ reset cờ khi tín hiệu rơi xuống dưới (threshold - 5) nhằm tránh nhiễu dao động quanh ngưỡng
  if (signal < threshold - 5) {
    aboveThreshold = false;
    digitalWrite(LED, LOW);         // Tắt LED
  }

  delay(5);  // Chu kỳ lấy mẫu xấp xỉ 200 Hz
}
