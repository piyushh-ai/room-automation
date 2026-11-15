# 🏠 ESP32 Smart Room Dashboard

A WiFi dashboard to control a fan and lights from your phone. The dashboard is hosted on the ESP32 itself, so no app or cloud service is needed. Control lights, the fan and one-tap "scenes" without getting out of bed.

> Built for an Instagram reel: *"Not lazy, just smart, Mom 😎"*

## ✨ Features

- Control 3 devices: **Fan**, **Main Light**, **Mandir Light**
- **Live status:** the dashboard shows ON only after the ESP32 confirms the relay changed
- **One-tap scenes (modes):**

  | Scene | Fan | Main Light | Mandir Light |
  |---|---|---|---|
  | 🌙 Good Night | ON | OFF | OFF |
  | ☀️ Good Morning | OFF | ON | ON |
  | 🙏 Pooja | OFF | OFF | ON |
  | 🎬 Movie Time | ON | OFF | ON |
  | 🎉 Party | unchanged | flashing | flashing (12 sec, then the previous state is restored) |
  | ⏻ All Off | OFF | OFF | OFF |

- Phone-first dark glass UI: the fan spins, the bulb and diya glow
- Online/Offline indicator, "devices ON" counter, and a time-based greeting
- Toast message and light vibration on every tap
- Static IP, so the dashboard address never changes
- Auto-refresh every 1.5 seconds to keep the state in sync

## 🔧 Hardware Used

| Component | Purpose |
|---|---|
| ESP32 Dev Board (CH340/CH34x USB chip) | Main controller and web server |
| 8-channel relay module (3 channels used) | Switching the fan and lights |
| Fan, Main Light, Mandir Light | Devices being controlled |
| Jumper wires | Connections |
| USB cable and 5V power supply | Powering the ESP32 |
| Smartphone | Running the dashboard |

## 💻 Software and Tech Used

- **Arduino IDE** with the ESP32 board package
- **C++ (Arduino framework)** for the ESP32 firmware
- Libraries: `WiFi.h`, `WebServer.h`, `ESPmDNS.h`
- **HTML, CSS and JavaScript** for the single-page dashboard, embedded inside the ESP32 firmware
- REST-style HTTP endpoints with JSON responses
- A phone serial terminal app for debugging

## 🔌 Wiring

| Relay Module | ESP32 |
|---|---|
| IN1 (Fan) | GPIO 13 |
| IN2 (Main Light) | GPIO 14 |
| IN3 (Mandir Light) | GPIO 27 |
| VCC | 5V / VIN |
| GND | GND |

The relay module is **active-LOW**. If the relays work in reverse (they turn off when they should turn on), swap `RELAY_ON` and `RELAY_OFF` in the code.

## 🚀 Setup

1. Install the ESP32 board package in Arduino IDE and select **ESP32 Dev Module**.
2. Open `smart_room_dashboard.ino` and change these lines:
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_NAME";
   const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
   ```
3. Set `local_IP` and `gateway` to match your router. The first three numbers should match your router, and the last number of `local_IP` should be unused (for example 200).
4. Upload the code. The ESP32 only connects to **2.4GHz** WiFi.
5. Keep your phone on the same WiFi and open this in a browser:
   ```
   http://192.168.1.200
   ```
   (or `http://room.local`, which only works on some phones)
6. Use the browser menu to **Add to Home Screen**.

## 🌐 API Endpoints

| Endpoint | Description |
|---|---|
| `/` | Dashboard page |
| `/state` | Current state of everything (JSON) |
| `/set?d=fan&v=1` | Turn one device on or off (`d` = fan / main / mandir, `v` = 1 / 0) |
| `/mode?m=night` | Run a scene (`night`, `morning`, `pooja`, `movie`, `party`, `alloff`) |

Example response from `/state`:
```json
{"fan":false,"main":false,"mandir":false,"mode":"alloff","party":false}
```

## ⚠️ Safety

- The fan and lights run on **220V mains**. Keep all relay wiring inside an insulated box and never leave exposed wires.
- Test with a low-voltage LED or bulb first.
- The dashboard has no login and works on your home WiFi only, so use a strong router password.
- Never upload your WiFi password to GitHub.

## 🔮 Future Ideas

- Control from anywhere (MERN dashboard with MQTT or an API)
- Voice commands such as "Jarvis, good night"
- Timers and schedules (lights on automatically in the morning)
- OTA (wireless) code updates
- Door sensor and light sensor automation

## 🎬 The Reel

Mom asks *"What are you building?"*. You show her the dashboard and control the light and fan from bed, and she says *"You're so lazy"* 😂

---
Made with ESP32, chai and a little bit of laziness.
