/*
  Pulse Sensor BPM Reader (Arduino)
  Heart rate (BPM) calculation algorithm using Pulse Sensor analog output.
*/

const int PulsePin = A0;        // Analog input pin connected to the Pulse Sensor signal wire
const int LED = LED_BUILTIN;    // Built-in LED indicator for heartbeat pulse

int signal;                     // Raw analog voltage value from the sensor (0 - 1023)
int threshold = 130;            // Threshold value for pulse peak detection

unsigned long lastBeatTime = 0; // Timestamp (ms) of the previous valid beat
unsigned long beatInterval = 0; // Inter-Beat Interval (IBI) between consecutive beats (ms)

// Human physiological heart rate limits for noise filtering:
const int MIN_IBI = 300;   // 300 ms -> corresponds to 200 BPM maximum
const int MAX_IBI = 1500;  // 1500 ms -> corresponds to 40 BPM minimum

// 4-sample moving average buffer for smoothing BPM readings
int bpmBuffer[4];
int bpmIndex = 0;
bool bufferFilled = false;

// Flag for rising edge detection
bool aboveThreshold = false;

void setup() {
  Serial.begin(115200);         // Baud rate for serial communication with host PC / Raspberry Pi
  pinMode(LED, OUTPUT);
}

void loop() {
  signal = analogRead(PulsePin);
  unsigned long now = millis();

  // 1. Rising edge detection: signal crosses threshold and previous pulse has ended
  if (signal > threshold && !aboveThreshold) {
    aboveThreshold = true;      // Lock flag to prevent duplicate triggers on the same pulse wave

    if (lastBeatTime > 0) {
      // 2. Measure Inter-Beat Interval (IBI) between consecutive pulses
      beatInterval = now - lastBeatTime;

      // 3. Physiological noise filtering: accept only realistic human heart rates (40 - 200 BPM)
      if (beatInterval >= MIN_IBI && beatInterval <= MAX_IBI) {
        // Convert interval (ms) into Beats Per Minute: BPM = 60000 / IBI
        int bpm = 60000 / beatInterval;

        // Store into circular buffer for smoothing
        bpmBuffer[bpmIndex++] = bpm;
        if (bpmIndex >= 4) {
          bpmIndex = 0;
          bufferFilled = true;
        }

        // 4. Calculate moving average across the 4 most recent valid beats
        if (bufferFilled) {
          int sum = 0;
          for (int i = 0; i < 4; i++) {
            sum += bpmBuffer[i];
          }
          int avgBPM = sum / 4;

          // Send smoothed BPM over Serial
          Serial.println(avgBPM);
          digitalWrite(LED, HIGH);  // Pulse LED indicator on beat
        }
      }
    }

    lastBeatTime = now;
  }

  // 5. Hysteresis reset (Schmitt Trigger-like behavior):
  // Reset the flag only when signal falls below (threshold - 5) to prevent ripple noise triggers
  if (signal < threshold - 5) {
    aboveThreshold = false;
    digitalWrite(LED, LOW);         // Turn off pulse LED indicator
  }

  delay(5);  // ~200 Hz sampling rate
}
