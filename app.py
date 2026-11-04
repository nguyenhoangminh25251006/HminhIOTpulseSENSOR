from flask import Flask, jsonify
import serial
import serial.tools.list_ports
import threading
import time
import os
import sys

app = Flask(__name__)

bpm_value = 0
ser = None

def get_serial_port():
    # 1. Check command-line argument or environment variable
    if len(sys.argv) > 1:
        return sys.argv[1]
    env_port = os.environ.get("SERIAL_PORT")
    if env_port:
        return env_port

    # 2. Default device path on Raspberry Pi / Linux
    default_linux_port = "/dev/ttyACM0"
    if os.path.exists(default_linux_port):
        return default_linux_port

    # 3. Auto-detect available COM / TTY port if running on Windows or other platforms
    ports = list(serial.tools.list_ports.comports())
    for p in ports:
        if "Arduino" in p.description or "ACM" in p.device or "USB" in p.description:
            return p.device
    if ports:
        return ports[0].device
    return default_linux_port


port_name = get_serial_port()

# Establish Serial connection to Arduino
try:
    ser = serial.Serial(port_name, 115200, timeout=0.1)
    time.sleep(2)
    ser.reset_input_buffer()
    print(f"Serial connected successfully on {port_name}")
except Exception as e:
    print(f"Serial error on {port_name}: {e}")
    ser = None


# Background thread for continuously reading serial data from Arduino
def read_serial():
    global bpm_value
    while True:
        if ser and ser.in_waiting:
            try:
                line = ser.readline().decode(errors="ignore").strip()
                if line:
                    bpm = int(line)
                    bpm_value = bpm
                    print(f"BPM: {bpm}")
            except Exception:
                pass
        time.sleep(0.02)


threading.Thread(target=read_serial, daemon=True).start()


@app.route("/bpm")
def bpm():
    return jsonify({"bpm": bpm_value})


@app.route("/")
def home():
    return """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Heart Rate Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>

<style>
* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}
body {
    background: #0f172a;
    color: #f8fafc;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 100vh;
    padding: 20px;
}
.card {
    background: #1e293b;
    border-radius: 16px;
    padding: 32px;
    box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.4);
    max-width: 800px;
    width: 100%;
    text-align: center;
}
h1 {
    color: #ff4757;
    margin-bottom: 8px;
    font-size: 2rem;
    letter-spacing: 0.5px;
}
.subtitle {
    color: #94a3b8;
    font-size: 0.95rem;
    margin-bottom: 24px;
}
.bpm-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    margin-bottom: 28px;
}
#bpm {
    font-size: 96px;
    font-weight: 800;
    color: #ff4757;
    line-height: 1;
    transition: transform 0.15s ease-in-out;
}
.unit {
    color: #64748b;
    font-size: 1.1rem;
    text-transform: uppercase;
    letter-spacing: 1.5px;
    margin-top: 6px;
}
.chart-wrapper {
    position: relative;
    width: 100%;
    height: 320px;
}
</style>
</head>

<body>

<div class="card">
    <h1>PI Heart Monitor</h1>
    <p class="subtitle">Real-time Heart Rate Monitoring Dashboard</p>

    <div class="bpm-container">
        <div id="bpm">--</div>
        <div class="unit">BPM (Beats Per Minute)</div>
    </div>

    <div class="chart-wrapper">
        <canvas id="chart"></canvas>
    </div>
</div>

<script>
const ctx = document.getElementById('chart').getContext('2d');

const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: Array(20).fill(''),
        datasets: [{
            label: 'Heart Rate (BPM)',
            data: Array(20).fill(0),
            borderColor: '#ff4757',
            backgroundColor: 'rgba(255, 71, 87, 0.1)',
            borderWidth: 3,
            fill: true,
            tension: 0.35,
            pointRadius: 4,
            pointBackgroundColor: '#ff4757'
        }]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: { display: false }
        },
        scales: {
            y: {
                min: 40,
                max: 160,
                grid: { color: 'rgba(255, 255, 255, 0.08)' },
                ticks: { color: '#94a3b8' }
            },
            x: {
                grid: { display: false },
                ticks: { display: false }
            }
        }
    }
});

async function update() {
    try {
        let res = await fetch('/bpm');
        let data = await res.json();
        const bpmEl = document.getElementById("bpm");

        bpmEl.innerText = data.bpm > 0 ? data.bpm : '--';

        // Pulse scale animation on heartbeat update
        if (data.bpm > 0) {
            bpmEl.style.transform = 'scale(1.05)';
            setTimeout(() => { bpmEl.style.transform = 'scale(1)'; }, 150);
        }

        chart.data.datasets[0].data.push(data.bpm);
        if (chart.data.datasets[0].data.length > 20) {
            chart.data.datasets[0].data.shift();
        }
        chart.update();
    } catch (err) {
        console.error("Error fetching BPM data:", err);
    }
}

setInterval(update, 1000);
</script>

</body>
</html>
"""

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, threaded=True)
