# Predictive Maintenance System for Industrial Motor

## Overview

This project is a complete **Predictive Maintenance System for Industrial Motor Monitoring** built around the **ESP32-S3 WROOM-1U** using a modular **PlatformIO** firmware structure.

The system is designed to monitor an industrial motor in real time, detect abnormal behaviour, estimate future fault risk, store large amounts of data for later analysis, and provide a local web dashboard for live viewing.

The system monitors:

- Motor vibration using an **ADXL345 accelerometer**
- Motor current using a **ZMCT103C current sensor**
- Motor temperature using a **DS18B20 temperature sensor**
- Motor operating state using a relay protection system

The system then calculates:

- Motor health score
- Motor risk score
- Fault level
- Worst operating metric
- Future fault estimate
- Maintenance recommendation

The project is designed so that the **SD card acts as the main backend and storage device**. If the SD card is not available, the firmware automatically uses the ESP32-S3 internal flash memory through **LittleFS** as fallback storage.

---

# Main Goal

The goal of this project is not only to display sensor values, but to behave like a real predictive maintenance prototype.

It should be able to:

1. Monitor motor operating condition
2. Log sensor and analysis data
3. Detect abnormal current, temperature, and vibration
4. Predict possible fault trends
5. Protect the motor using a relay
6. Notify the user using buzzer and LED indication
7. Display live status on LCD
8. Provide a web dashboard for future analysis
9. Store enough data for later engineering analysis

---

# Features

- ESP32-S3 WROOM-1U based controller
- ADXL345 vibration monitoring
- ZMCT103C current monitoring
- DS18B20 temperature monitoring
- 20x4 I2C LCD display
- Rotary encoder navigation
- Relay-based motor protection
- Active buzzer alarm
- Breathing onboard LED indicator
- SD card backend storage
- Internal LittleFS fallback storage
- CSV data logging
- Event logging
- Predictive maintenance analysis
- Motor health score calculation
- Risk score calculation
- Future fault estimation
- Local Wi-Fi Access Point
- Web dashboard for live monitoring
- Downloadable CSV logs from web dashboard
- Modular PlatformIO project structure
- Non-blocking firmware design

---

# Hardware Used

## Main Controller

- ESP32-S3 WROOM-1U

## Display

- 20x4 I2C LCD screen

## Sensors

- ADXL345 vibration sensor
- ZMCT103C current sensor
- DS18B20 temperature sensor

## Storage

- MicroSD card module
- ESP32-S3 internal flash memory using LittleFS as fallback

## User Input

- Rotary encoder with push button

## Output / Protection

- Relay module
- Active buzzer
- Onboard status LED

## Programming Interface

- CH340C USB-to-serial converter

---

# Pin Configuration

## I2C Bus

The LCD and ADXL345 share the same I2C bus.

| Function | ESP32-S3 GPIO |
|---|---|
| I2C SDA | GPIO8 |
| I2C SCL | GPIO9 |
| LCD SDA | GPIO8 |
| LCD SCL | GPIO9 |
| ADXL345 SDA | GPIO8 |
| ADXL345 SCL | GPIO9 |

---

## ADXL345 Vibration Sensor

| Function | ESP32-S3 GPIO |
|---|---|
| SDA | GPIO8 |
| SCL | GPIO9 |
| INT | GPIO10 |

---

## ZMCT103C Current Sensor

| Function | ESP32-S3 GPIO |
|---|---|
| Analog Output | GPIO1 |

> The ZMCT103C output must be conditioned properly and must not exceed the ESP32-S3 ADC input range.

---

## DS18B20 Temperature Sensor

| Function | ESP32-S3 GPIO |
|---|---|
| DATA | GPIO4 |

> Use a 4.7k pull-up resistor between DATA and 3.3V.

---

## SD Card Module

The SD card uses SPI mode.

| Function | ESP32-S3 GPIO |
|---|---|
| CS | GPIO5 |
| MOSI | GPIO11 |
| SCK | GPIO12 |
| MISO | GPIO13 |

---

## Rotary Encoder

| Function | ESP32-S3 GPIO |
|---|---|
| CLK | GPIO16 |
| DT | GPIO17 |
| SW | GPIO18 |

---

## Output Devices

| Function | ESP32-S3 GPIO |
|---|---|
| Buzzer | GPIO14 |
| Relay | GPIO21 |
| Status LED | GPIO48 |

---

## CH340C USB-to-Serial

| CH340C Pin | ESP32-S3 GPIO |
|---|---|
| TXD | GPIO44 RX0 |
| RXD | GPIO43 TX0 |
| GND | GND |

> TX and RX must be crossed.

---

## Boot and Reset Pins

| Function | Pin |
|---|---|
| BOOT | GPIO0 |
| EN / RESET | EN pin |

Recommended hardware:

- EN pulled up to 3.3V using 10k resistor
- EN connected to GND through 0.1uF capacitor
- BOOT button pulls GPIO0 to GND
- RESET button pulls EN to GND

---

# System Architecture

The firmware is arranged into independent modules.

```text
lib/
├── Actuators/
│   └── buzzer/
├── Communication/
│   └── local_server/
├── Control/
│   ├── error_handling/
│   ├── load_relay/
│   ├── maintenance_manager/
│   └── sleep_wake/
├── Display/
│   ├── lcd_screen/
│   └── led_indicator/
├── Input/
│   └── rotary_encoder/
├── Pins/
├── Sensors/
│   ├── current_sensor/
│   ├── temp_sensor/
│   └── vibration_sensor/
├── Storage/
│   └── sd_card/
├── System/
│   └── reset/
src/
└── main.cpp
```

---

# Firmware Flow

## Startup

On power-up, the firmware initializes:

1. Serial monitor
2. Pin configuration
3. Sensors
4. LCD
5. SD card
6. Internal LittleFS fallback storage
7. Relay protection module
8. Maintenance manager
9. Local web dashboard
10. LED and buzzer indication

---

## Main Loop

During operation, the loop updates all modules continuously:

1. Read current sensor
2. Read temperature sensor
3. Read vibration sensor
4. Calculate motor risk and health
5. Update relay protection logic
6. Update LCD display
7. Update local web dashboard
8. Log data to storage
9. Update buzzer and LED indicators

The design follows a non-blocking style as much as possible.

---

# Predictive Maintenance Logic

The predictive maintenance module uses live sensor values to estimate motor condition.

It monitors:

- temperature severity
- current severity
- vibration severity
- rate of change
- worst metric
- fault trend

The output of the analysis includes:

| Output | Description |
|---|---|
| Health Score | Estimated motor health in percent |
| Risk Score | Estimated fault risk in percent |
| Level | NORMAL, WARNING, CRITICAL, or FAULT |
| Worst Metric | The sensor value contributing most to the risk |
| Forecast | Estimated time before a fault condition |
| Recommendation | Maintenance advice based on condition |

---

# Fault Levels

## NORMAL

The motor is operating within acceptable range.

Typical behaviour:

- Relay remains ON
- Buzzer is OFF
- LED breathes normally
- Data continues logging

---

## WARNING

The motor is beginning to show abnormal behaviour.

Typical causes:

- rising vibration
- increasing temperature
- current approaching limit

Action:

- Continue monitoring
- Prepare maintenance inspection
- Check trend from dashboard logs

---

## CRITICAL

The motor is close to a fault limit.

Action:

- Schedule urgent inspection
- Check bearing, load, cooling, and supply condition
- Avoid prolonged operation

---

## FAULT

A fault threshold has been reached.

System response:

- Relay turns OFF
- Motor is disconnected
- Buzzer alarm starts
- LED changes behaviour
- Fault event is logged
- Dashboard shows fault condition

---

# Default Protection Thresholds

| Parameter | Threshold |
|---|---|
| Current | 5.0 A |
| Temperature | 70.0 °C |
| Vibration RMS | 2.5 g |

These values are firmware defaults and should be calibrated based on the actual motor rating and installation environment.

---

# Relay Protection Logic

The relay is used to protect the motor.

If any of the following conditions occur:

- current is above maximum limit
- temperature is above maximum limit
- vibration RMS is above maximum limit

Then:

- relay turns OFF
- buzzer alarm starts
- fault state is displayed
- event is stored in the log

The relay can also be controlled from the web dashboard, but the system will block unsafe relay ON operation while a fault is active.

---

# LCD Display

The 20x4 LCD displays real-time information.

LCD screens include:

## Screen 1: Live Motor Data

- Temperature
- Current
- Vibration RMS

## Screen 2: System Status

- Relay state
- Fault state
- SD card status
- Backend storage status

## Screen 3: Acceleration Data

- X axis
- Y axis
- Z axis

## Screen 4: Predictive Analysis

- Health score
- Risk score
- Worst metric
- Fault level

The rotary encoder is used to switch between screens.

---

# Web Dashboard

The ESP32-S3 creates a local Wi-Fi Access Point.

## Access Point

```text
SSID: MOTOR_PM_SYSTEM
Password: 12345678
```

## Dashboard Address

```text
http://192.168.4.1
```

The dashboard provides:

- live temperature
- live current
- live vibration
- acceleration values
- motor health score
- risk score
- condition level
- future fault estimate
- maintenance recommendation
- relay control
- fault clear button
- log now button
- CSV download buttons
- trend graph
- dark and light theme toggle

The dashboard is responsive and works on:

- phone
- tablet
- laptop
- desktop

---

# Web API Endpoints

## Root Dashboard

```text
/
```

Opens the main dashboard page.

---

## Live Status

```text
/api/status
```

Returns live sensor and analysis data as JSON.

Example fields:

```json
{
  "temperature_c": 35.50,
  "current_a": 1.250,
  "vibration_rms_g": 0.210,
  "risk_score": 18.5,
  "health_score": 81.5,
  "level": "NORMAL",
  "worst_metric": "Vibration",
  "forecast_text": "No rising fault trend detected",
  "relay_on": true,
  "fault": false,
  "backend": "SD CARD"
}
```

---

## Logs

```text
/api/logs
```

Returns recent motor, analysis, and event logs.

---

## Relay Control

```text
/api/relay?state=on
/api/relay?state=off
```

Controls the relay from the dashboard.

---

## Clear Fault

```text
/api/clear_fault
```

Clears the current latched fault condition.

---

## Log Immediately

```text
/api/log_now
```

Forces immediate log writing.

---

## Download CSV Logs

```text
/download/motor_log.csv
/download/analysis_log.csv
/download/event_log.csv
```

Downloads stored CSV files from the active backend.

---

# Storage Backend

The project uses two storage options.

## Primary Backend

```text
SD Card
```

The SD card is the main storage backend and is used for large data logging.

## Fallback Backend

```text
Internal LittleFS
```

If the SD card is missing or fails to initialize, the firmware automatically logs to internal flash memory.

This allows the system to continue operating even when the SD card is unavailable.

---

# Log Files

## Motor Log

```text
/motor_log.csv
```

Stores raw motor operating data.

Example:

```csv
millis,temp_c,current_a,vibration_rms_g,relay_on,fault
1000,32.40,1.220,0.120,1,0
6000,33.10,1.250,0.115,1,0
11000,34.00,1.300,0.140,1,0
```

---

## Analysis Log

```text
/analysis_log.csv
```

Stores predictive maintenance result.

Example:

```csv
millis,health_score,risk_score,level,worst_metric,forecast_minutes,recommendation
1000,91.5,8.5,NORMAL,Vibration,-1,Motor condition is normal
6000,78.0,22.0,NORMAL,Temperature,-1,Motor condition is normal
11000,54.0,46.0,WARNING,Vibration,180,Monitor closely
```

---

## Event Log

```text
/event_log.csv
```

Stores important system events.

Example:

```csv
millis,event,message
5000,BOOT,System started
24000,WARNING,Vibration trend increasing
45000,FAULT,Relay shutdown due to high temperature
```

---

# LED Behaviour

The onboard LED uses a PWM breathing effect.

## Normal Condition

- Breathing LED pattern
- Indicates firmware is running normally

## Fault Condition

- Fast blinking LED
- Indicates alarm or protection state

---

# Buzzer Behaviour

## Normal

- Buzzer OFF

## Fault

- Repeating alarm beep pattern

## Manual Feedback

The buzzer can also be used for short beep feedback in future menu actions.

---

# Sensor Calibration Notes

## ZMCT103C Current Sensor

The current value depends heavily on calibration.

The firmware uses a calibration factor to convert measured AC RMS voltage into current.

You should calibrate using:

1. A known load
2. A clamp meter
3. Serial monitor reading
4. Adjustment of calibration factor

Do not connect AC mains directly to the ESP32-S3.

---

## ADXL345 Vibration Sensor

The ADXL345 measures acceleration on X, Y, and Z axes.

The firmware calculates vibration RMS by comparing the measured acceleration magnitude against a base magnitude.

For better accuracy:

- mount the sensor firmly on the motor body
- avoid loose wires
- avoid placing it on flexible surfaces
- calibrate the baseline when the motor is in normal condition

---

## DS18B20 Temperature Sensor

For better temperature reading:

- attach the sensor to the motor body
- use thermal paste if possible
- keep the cable away from high-voltage switching lines
- use a 4.7k pull-up resistor

---

# Power and Safety Notes

- Use a stable 3.3V supply for the ESP32-S3
- Relay coils must not be driven directly from ESP32 GPIO
- Use a transistor, optocoupler, or relay module
- Use flyback protection for relay coils
- Keep AC and DC sections isolated
- Use common ground only where safe and required
- Add fuses and protection for industrial motor wiring
- Keep sensor signal wires away from motor power wires

---

# PlatformIO Configuration

Example environment:

```ini
[env:esp32-s3-devkitm-1]
platform = espressif32
board = esp32-s3-devkitm-1
framework = arduino
monitor_speed = 115200
```

Required libraries may include:

```ini
lib_deps =
  marcoschwartz/LiquidCrystal_I2C
  adafruit/Adafruit ADXL345
  adafruit/Adafruit Unified Sensor
  paulstoffregen/OneWire
  milesburton/DallasTemperature
```

Depending on your exact project structure, the library names may need small adjustment inside PlatformIO.

---

# Build and Upload

## Build

```bash
pio run
```

## Upload

```bash
pio run --target upload
```

## Serial Monitor

```bash
pio device monitor
```

or:

```bash
pio device monitor -b 115200
```

---

# Operating Guide

## Power On

1. Power the ESP32-S3 board
2. Wait for the LCD startup screen
3. Check that the status LED starts breathing
4. Confirm SD card status on LCD
5. Connect to the Wi-Fi AP if dashboard monitoring is needed

---

## View Dashboard

1. Open Wi-Fi settings on your phone or laptop
2. Connect to:

```text
MOTOR_PM_SYSTEM
```

3. Enter password:

```text
12345678
```

4. Open browser and go to:

```text
http://192.168.4.1
```

---

## Monitor Motor

The dashboard will show:

- temperature
- current
- vibration
- health score
- risk score
- fault forecast
- recommendation
- trend graph

---

## Download Logs

From the dashboard, use:

- Download Motor CSV
- Download Analysis CSV
- Download Events CSV

These files can be opened in:

- Excel
- Google Sheets
- Python
- MATLAB
- Power BI

---

# Data Analysis Use

The logged CSV data can be used for:

- trend analysis
- fault history review
- machine learning dataset creation
- motor condition classification
- vibration behaviour analysis
- current overload study
- maintenance scheduling

---

# Future Improvements

- Machine learning model for motor fault classification
- Web-based threshold calibration
- MQTT support
- Cloud sync
- OTA firmware updates
- Email or SMS fault notification
- More vibration features such as peak, crest factor, and FFT
- Motor runtime counter
- Bearing fault frequency analysis
- Multi-motor monitoring
- User login for dashboard
- Historical charts from stored CSV
- Automatic report generation

---

# Troubleshooting

## LCD Not Showing Text

Check:

- LCD I2C address
- SDA and SCL wiring
- 5V or 3.3V compatibility
- I2C pull-up resistors

Common LCD addresses:

```text
0x27
0x3F
```

---

## SD Card Not Detected

Check:

- SD CS pin
- SPI wiring
- SD card formatting
- 3.3V compatibility
- card inserted properly

If SD card is missing, the system should fall back to internal LittleFS storage.

---

## Current Reading Is Wrong

Check:

- ZMCT103C calibration factor
- ADC input voltage range
- burden resistor and conditioning circuit
- real load current using clamp meter

---

## Vibration Reading Is Too High

Check:

- sensor mounting
- loose wires
- motor frame vibration
- ADXL345 orientation
- baseline calibration

---

## Dashboard Not Opening

Check:

- phone is connected to `MOTOR_PM_SYSTEM`
- open `http://192.168.4.1`
- ensure local server module is started in `main.cpp`
- check serial monitor for AP IP address

---

# Author

Egbe Raymond

Electrical / Electronic Engineer  
Embedded Systems Developer  
Full Stack Developer

---

# License

This project is intended for educational, industrial prototype, and development purposes.

Use proper electrical safety practices when testing with industrial motors and mains-powered equipment.
