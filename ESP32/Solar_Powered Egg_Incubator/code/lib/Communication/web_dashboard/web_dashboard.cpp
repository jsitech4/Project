#include <Arduino.h>
#include <WebServer.h>
#include "web_dashboard.h"
#include "wifi_manager.h"
#include "battery_level.h"
#include "solar_level.h"
#include "temp_hum.h"
#include "ultrasonic.h"
#include "heater.h"
#include "spinner.h"
#include "humidifier.h"
#include "buzzer.h"
#include "lcd_screen.h"
#include "rtc_clock.h"

namespace web_dashboard
{
  static WebServer server(80);

  static String relayState(bool state)
  {
    return state ? "ON" : "OFF";
  }

  static String buzzerState()
  {
    return buzzer::isActive() ? "ACTIVE" : "IDLE";
  }

  static void showDeviceChange(const char *deviceName, bool state)
  {
    char line1[24];
    char line2[24];
    snprintf(line1, sizeof(line1), "%s updated", deviceName);
    snprintf(line2, sizeof(line2), "State: %s", state ? "ON" : "OFF");
    lcd_screen::showMessage("Dashboard", line1, line2, "", 1800, lcd_screen::PRIORITY_INFO);
  }

  static String page()
  {
    String html;
    html.reserve(30000);

    html += R"rawliteral(
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0">
  <title>Egg Incubator Dashboard</title>

  <style>
    :root {
      --text: #102033;
      --muted: #64748b;
      --white: #ffffff;
      --danger: #ef4444;
      --off: #64748b;
      --shadow: 0 12px 30px rgba(15, 23, 42, 0.12);
      --soft-shadow: 0 8px 24px rgba(15, 23, 42, 0.12);
      --radius: 22px;
      --page-width: 1080px;
      --header-bg: rgba(15, 23, 42, 0.38);
      --header-border: rgba(255, 255, 255, 0.24);
      --heading-color: #ffffff;
      --footer-color: rgba(255, 255, 255, 0.9);
      --page-overlay: rgba(255, 255, 255, 0.10);
      --card-bg: rgba(255, 255, 255, 0.90);
      --card-border: rgba(255, 255, 255, 0.70);
      --card-title: #64748b;
      --value-color: #0f172a;
      --card-accent: #2563eb;
    }

    body.dark-mode {
      --text: #f8fafc;
      --muted: #cbd5e1;
      --shadow: 0 16px 38px rgba(0, 0, 0, 0.28);
      --soft-shadow: 0 10px 28px rgba(0, 0, 0, 0.28);
      --header-bg: rgba(2, 6, 23, 0.64);
      --header-border: rgba(148, 163, 184, 0.26);
      --heading-color: #ffffff;
      --footer-color: rgba(226, 232, 240, 0.88);
      --page-overlay: rgba(2, 6, 23, 0.22);
      --card-bg: rgba(15, 23, 42, 0.82);
      --card-border: rgba(148, 163, 184, 0.18);
      --card-title: #cbd5e1;
      --value-color: #f8fafc;
      --card-accent: #60a5fa;
    }

    * {
      box-sizing: border-box;
    }

    html {
      width: 100%;
      min-height: 100%;
      scroll-behavior: smooth;
    }

    body {
      width: 100%;
      min-height: 100vh;
      font-family: Arial, sans-serif;
      margin: 0;
      color: var(--text);
      display: flex;
      flex-direction: column;
      align-items: center;
      background:
        radial-gradient(circle at top left, rgba(255, 214, 102, 0.45), transparent 28%),
        radial-gradient(circle at top right, rgba(56, 189, 248, 0.42), transparent 30%),
        radial-gradient(circle at bottom left, rgba(34, 197, 94, 0.35), transparent 30%),
        linear-gradient(135deg, #7c3aed 0%, #2563eb 42%, #14b8a6 100%);
      background-attachment: fixed;
      overflow-x: hidden;
      transition: background 0.25s ease, color 0.25s ease;
    }

    body.dark-mode {
      background:
        radial-gradient(circle at top left, rgba(147, 51, 234, 0.34), transparent 30%),
        radial-gradient(circle at top right, rgba(14, 165, 233, 0.24), transparent 32%),
        radial-gradient(circle at bottom left, rgba(20, 184, 166, 0.22), transparent 34%),
        linear-gradient(135deg, #020617 0%, #0f172a 46%, #111827 100%);
      background-attachment: fixed;
    }

    body::before {
      content: "";
      position: fixed;
      inset: 0;
      background: var(--page-overlay);
      pointer-events: none;
      z-index: -1;
      transition: background 0.25s ease;
    }

    header {
      width: 100%;
      color: white;
      padding: 20px 16px;
      display: flex;
      justify-content: center;
      position: relative;
    }

    .header-wrap {
      width: 100%;
      max-width: var(--page-width);
      text-align: center;
      background: var(--header-bg);
      border: 1px solid var(--header-border);
      border-radius: 0 0 28px 28px;
      padding: 18px 74px 18px 74px;
      box-shadow: var(--soft-shadow);
      backdrop-filter: blur(14px);
      position: relative;
      transition: background 0.25s ease, border 0.25s ease, box-shadow 0.25s ease;
    }

    .theme-toggle-wrap {
      position: absolute;
      top: 16px;
      right: 16px;
      display: flex;
      align-items: center;
      gap: 8px;
      z-index: 5;
    }

    .theme-label {
      font-size: 12px;
      font-weight: 700;
      color: rgba(255, 255, 255, 0.9);
      user-select: none;
    }

    .theme-switch {
      position: relative;
      display: inline-block;
      width: 54px;
      height: 28px;
    }

    .theme-switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }

    .slider {
      position: absolute;
      cursor: pointer;
      inset: 0;
      background: rgba(255, 255, 255, 0.92);
      border-radius: 999px;
      box-shadow: 0 6px 16px rgba(15, 23, 42, 0.22);
      transition: 0.25s;
    }

    .slider::before {
      content: "☀️";
      position: absolute;
      height: 22px;
      width: 22px;
      left: 3px;
      top: 3px;
      border-radius: 50%;
      background: #facc15;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 12px;
      transition: 0.25s;
    }

    .theme-switch input:checked+.slider {
      background: rgba(15, 23, 42, 0.96);
    }

    .theme-switch input:checked+.slider::before {
      content: "🌙";
      transform: translateX(26px);
      background: #6366f1;
    }

    h2 {
      margin: 0 0 7px 0;
      font-size: clamp(18px, 4vw, 60px);
      line-height: 1.2;
      font-weight: 800;
      letter-spacing: 0.2px;
      color: var(--heading-color);
    }

    h3 {
      width: 100%;
      max-width: var(--page-width);
      margin: 26px auto 14px auto;
      font-size: clamp(18px, 3.5vw, 23px);
      text-align: center;
      color: var(--heading-color);
      text-shadow: 0 2px 10px rgba(15, 23, 42, 0.25);
    }

    .sub {
      font-size: clamp(12px, 2.6vw, 14px);
      color: rgba(255, 255, 255, 0.86);
      line-height: 1.5;
      word-break: break-word;
    }

    main {
      width: 100%;
      max-width: var(--page-width);
      margin: 0 auto;
      padding: 8px 16px 30px 16px;
      display: flex;
      flex-direction: column;
      align-items: center;
    }

    .grid {
      width: 100%;
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
      gap: 16px;
      justify-content: center;
      align-items: stretch;
    }

    .card {
      width: 100%;
      min-height: 124px;
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: var(--radius);
      padding: 17px 16px;
      box-shadow: var(--shadow);
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      text-align: center;
      overflow: hidden;
      position: relative;
      color: var(--value-color);
      isolation: isolate;
      backdrop-filter: blur(12px);
      transition: transform 0.18s ease, box-shadow 0.18s ease, background 0.25s ease, border 0.25s ease;
    }

    .card::before {
      content: "";
      position: absolute;
      left: 0;
      top: 0px;
      width: 100%;
      height: 6px;
      background: var(--card-accent);
      z-index: -1;
    }

    .card::after {
      content: "";
      position: absolute;
      width: 92px;
      height: 92px;
      right: -40px;
      top: -49px;
      border-radius: 50%;
      background: color-mix(in srgb, var(--card-accent) 18%, transparent);
      z-index: -1;
    }

    .card:hover {
      transform: translateY(-3px);
      box-shadow: 0 18px 42px rgba(15, 23, 42, 0.18);
    }

    body.dark-mode .card:hover {
      box-shadow: 0 18px 42px rgba(0, 0, 0, 0.38);
    }

    .temperature-card {
      --card-accent: #f97316;
    }

    .humidity-card {
      --card-accent: #0ea5e9;
    }

    .battery-card {
      --card-accent: #22c55e;
    }

    .solar-card {
      --card-accent: #eab308;
    }

    .ultrasonic-card {
      --card-accent: #8b5cf6;
    }

    .lcd-card {
      --card-accent: #64748b;
    }

    .heater-card {
      --card-accent: #ef4444;
    }

    .spinner-card {
      --card-accent: #3b82f6;
    }

    .humidifier-card {
      --card-accent: #14b8a6;
    }

    .buzzer-card {
      --card-accent: #f43f5e;
    }

    .lcd-control-card {
      --card-accent: #6366f1;
    }

    .card-title {
      width: 100%;
      font-size: 14px;
      color: var(--card-title);
      margin-bottom: 8px;
      line-height: 1.3;
      font-weight: 700;
    }

    .v {
      width: 100%;
      font-size: clamp(25px, 5vw, 36px);
      font-weight: 800;
      line-height: 1.1;
      margin: 0;
      white-space: nowrap;
      color: var(--value-color);
    }

    .unit {
      font-size: clamp(13px, 3vw, 16px);
      font-weight: 700;
      color: var(--muted);
    }

    .percent {
      margin-top: 8px;
      color: var(--muted);
      font-size: 14px;
      line-height: 1.3;
      font-weight: 600;
    }

    .control-card {
      min-height: 138px;
      justify-content: space-between;
      gap: 14px;
    }

    .state-row {
      width: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      flex-direction: column;
      gap: 9px;
    }

    .state-row span {
      font-weight: 700;
      letter-spacing: 0.2px;
      color: var(--value-color);
    }

    .state {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 78px;
      padding: 7px 13px;
      border-radius: 999px;
      background: color-mix(in srgb, var(--card-accent) 14%, white);
      color: #111827;
      font-size: 13px;
      font-weight: 800;
      line-height: 1;
      box-shadow: 0 4px 12px rgba(15, 23, 42, 0.10);
    }

    body.dark-mode .state {
      background: color-mix(in srgb, var(--card-accent) 24%, #e2e8f0);
      color: #020617;
    }

    .btn-row {
      width: 100%;
      display: flex;
      justify-content: center;
      align-items: center;
      flex-wrap: wrap;
      gap: 8px;
    }

    .btn {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      min-width: 94px;
      min-height: 42px;
      padding: 10px 14px;
      border-radius: 13px;
      text-decoration: none;
      color: white;
      background: var(--card-accent);
      border: 1px solid rgba(255, 255, 255, 0.35);
      cursor: pointer;
      font-size: 14px;
      font-weight: 800;
      text-align: center;
      transition: transform 0.15s ease, opacity 0.15s ease, filter 0.15s ease;
      box-shadow: 0 6px 16px rgba(15, 23, 42, 0.12);
    }

    .btn:hover {
      filter: brightness(1.05);
    }

    .btn:active {
      transform: scale(0.97);
      opacity: 0.9;
    }

    .off {
      background: var(--off);
      color: white;
    }

    .danger {
      background: #ef4444;
      color: white;
    }

    .footer-note {
      width: 100%;
      max-width: var(--page-width);
      color: var(--footer-color);
      font-size: 13px;
      margin: 22px auto 0 auto;
      line-height: 1.5;
      text-align: center;
      text-shadow: 0 2px 8px rgba(15, 23, 42, 0.25);
    }

    .space {
      display: inline-block;
      width: 150px;
    }

    @media (min-width: 900px) {
      .sensor-grid {
        grid-template-columns: repeat(3, minmax(0, 1fr));
      }

      .control-grid {
        grid-template-columns: repeat(5, minmax(0, 1fr));
      }
    }

    @media (min-width: 600px) and (max-width: 899px) {
      .sensor-grid {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }

      .control-grid {
        grid-template-columns: repeat(2, minmax(0, 1fr));
      }
    }

    @media (max-width: 599px) {
      header {
        padding: 14px 12px;
      }

      .header-wrap {
        border-radius: 0 0 22px 22px;
        padding: 54px 12px 16px 12px;
      }

      .theme-toggle-wrap {
        top: 12px;
        right: 12px;
      }

      .theme-label {
        font-size: 11px;
      }

      main {
        padding: 12px 12px 24px 12px;
      }

      .grid,
      .sensor-grid,
      .control-grid {
        grid-template-columns: 1fr;
        gap: 12px;
      }

      .card {
        min-height: 108px;
        padding: 16px 14px;
        border-radius: 18px;
      }

      .control-card {
        min-height: 122px;
      }

      .btn-row {
        flex-direction: column;
      }

      .btn {
        width: 100%;
      }

      .space {
        display: inline-block;
        width: 30px;
      }
    }

    @media (max-width: 360px) {
      .v {
        font-size: 24px;
      }

      .card {
        padding: 14px 12px;
      }

      .theme-label {
        display: none;
      }

      .space {
        display: inline-block;
        width: 30px;
      }
    }
  </style>
</head>

<body>
  <header>
    <div class="header-wrap">
      <div class="theme-toggle-wrap">
        <span class="theme-label" id="themeText">Light</span>
        <label class="theme-switch" aria-label="Toggle dark and light mode">
          <input type="checkbox" id="themeToggle" onchange="toggleTheme()">
          <span class="slider"></span>
        </label>
      </div>

      <h2>Solar Powered Egg Incubator</h2>
      <div class="sub">
        IP: <span id="ip">)rawliteral";

    html += wifi_manager::getIp();

    html += R"rawliteral(</span><span class="space"></span> Time: <span id="time">)rawliteral";

    html += rtc_clock::getDateTime();

    html += R"rawliteral(</span>
      </div>
    </div>
  </header>

  <main>
    <section class="grid sensor-grid">
      <div class="card temperature-card">
        <div class="card-title">Temperature</div>
        <div class="v"><span id="temp">)rawliteral";

    html += String(temp_hum::getTemperature(), 1);

    html += R"rawliteral(</span> <span class="unit">&deg;C</span></div>
      </div>

      <div class="card humidity-card">
        <div class="card-title">Humidity</div>
        <div class="v"><span id="hum">)rawliteral";

    html += String(temp_hum::getHumidity(), 0);

    html += R"rawliteral(</span> <span class="unit">%</span></div>
      </div>

      <div class="card battery-card">
        <div class="card-title">Battery</div>
        <div class="v"><span id="battv">)rawliteral";

    html += String(battery_level::getVoltage(), 2);

    html += R"rawliteral(</span> <span class="unit">V</span></div>
        <div class="percent"><span id="battp">)rawliteral";

    html += String(battery_level::getPercentage());

    html += R"rawliteral(</span>% remaining</div>
      </div>

      <div class="card solar-card">
        <div class="card-title">Solar</div>
        <div class="v"><span id="solarv">)rawliteral";

    html += String(solar_level::getVoltage(), 2);

    html += R"rawliteral(</span> <span class="unit">V</span></div>
        <div class="percent"><span id="solarp">)rawliteral";

    html += String(solar_level::getPercentage());

    html += R"rawliteral(</span>% input</div>
      </div>

      <div class="card ultrasonic-card">
        <div class="card-title">Ultrasonic</div>
        <div class="v"><span id="dist">)rawliteral";

    html += String(ultrasonic::getDistanceCm(), 1);

    html += R"rawliteral(</span> <span class="unit">cm</span></div>
      </div>

      <div class="card lcd-card">
        <div class="card-title">LCD</div>
        <div class="v"><span id="lcd">)rawliteral";

    html += String(lcd_screen::isAwake() ? "AWAKE" : "SLEEP");

    html += R"rawliteral(</span></div>
      </div>
    </section>

    <h3>Controls</h3>

    <section class="grid control-grid">
      <div class="card control-card heater-card">
        <div class="state-row">
          <span>Heater</span>
          <b class="state" id="heater">)rawliteral";

    html += relayState(heater::isOn());

    html += R"rawliteral(</b>
        </div>
        <div class="btn-row">
          <button class="btn" onclick="toggleDevice('heater')">Toggle</button>
        </div>
      </div>

      <div class="card control-card spinner-card">
        <div class="state-row">
          <span>Spinner</span>
          <b class="state" id="spinner">)rawliteral";

    html += relayState(spinner::isOn());

    html += R"rawliteral(</b>
        </div>
        <div class="btn-row">
          <button class="btn" onclick="toggleDevice('spinner')">Toggle</button>
        </div>
      </div>

      <div class="card control-card humidifier-card">
        <div class="state-row">
          <span>Humidifier</span>
          <b class="state" id="humidifier">)rawliteral";

    html += relayState(humidifier::isOn());

    html += R"rawliteral(</b>
        </div>
        <div class="btn-row">
          <button class="btn" onclick="toggleDevice('humidifier')">Toggle</button>
        </div>
      </div>

      <div class="card control-card buzzer-card">
        <div class="state-row">
          <span>Buzzer</span>
          <b class="state" id="buzzer">)rawliteral";

    html += buzzerState();

    html += R"rawliteral(</b>
        </div>
        <div class="btn-row">
          <button class="btn danger" onclick="beep()">Beep</button>
        </div>
      </div>

      <div class="card control-card lcd-control-card">
        <div class="state-row">
          <span>LCD Control</span>
          <b class="state">Screen</b>
        </div>
        <div class="btn-row">
          <button class="btn" onclick="setLcd(true)">Wake</button>
          <button class="btn off" onclick="setLcd(false)">Sleep</button>
        </div>
      </div>
    </section>

    <p class="footer-note">
      This dashboard is a prototype for the solar powered egg incubator project. Sensor values are dynamically
      updated from the actual hardware readings and controls.
    </p>
  </main>

  <script>
    function toggleTheme() {
      const checked = document.getElementById("themeToggle").checked;
      document.body.classList.toggle("dark-mode", checked);
      document.getElementById("themeText").innerHTML = checked ? "Dark" : "Light";
      localStorage.setItem("eggTheme", checked ? "dark" : "light");
    }

    function loadTheme() {
      const savedTheme = localStorage.getItem("eggTheme");
      const isDark = savedTheme === "dark";
      document.getElementById("themeToggle").checked = isDark;
      document.body.classList.toggle("dark-mode", isDark);
      document.getElementById("themeText").innerHTML = isDark ? "Dark" : "Light";
    }

    async function updateDashboard() {
      try {
        const response = await fetch("/api");
        const data = await response.json();

        for (const key in data) {
          const element = document.getElementById(key);

          if (element) {
            element.innerHTML = data[key];
          }
        }
      } catch (error) {
        console.log("Dashboard update failed:", error);
      }
    }

    async function toggleDevice(device) {
      await fetch(`/toggle?dev=${device}`);
      updateDashboard();
    }

    async function beep() {
      await fetch("/beep");
      updateDashboard();
    }

    async function setLcd(value) {
      await fetch(`/lcd?wake=${value ? "1" : "0"}`);
      updateDashboard();
    }

    loadTheme();
    setInterval(updateDashboard, 1000);
    updateDashboard();
  </script>
</body>

</html>
)rawliteral";

    return html;
  }

  static void sendApi()
  {
    String json = "{";
    json += "\"ip\":\"" + wifi_manager::getIp() + "\",";
    json += "\"time\":\"" + rtc_clock::getDateTime() + "\",";
    json += "\"temp\":\"" + String(temp_hum::getTemperature(), 1) + "\",";
    json += "\"hum\":\"" + String(temp_hum::getHumidity(), 0) + "\",";
    json += "\"battv\":\"" + String(battery_level::getVoltage(), 2) + "\",";
    json += "\"battp\":\"" + String(battery_level::getPercentage()) + "\",";
    json += "\"solarv\":\"" + String(solar_level::getVoltage(), 2) + "\",";
    json += "\"solarp\":\"" + String(solar_level::getPercentage()) + "\",";
    json += "\"dist\":\"" + String(ultrasonic::getDistanceCm(), 1) + "\",";
    json += "\"lcd\":\"" + String(lcd_screen::isAwake() ? "AWAKE" : "SLEEP") + "\",";
    json += "\"heater\":\"" + relayState(heater::isOn()) + "\",";
    json += "\"spinner\":\"" + relayState(spinner::isOn()) + "\",";
    json += "\"humidifier\":\"" + relayState(humidifier::isOn()) + "\",";
    json += "\"buzzer\":\"" + buzzerState() + "\"";
    json += "}";

    server.send(200, "application/json", json);
  }

  static void sendStateJson()
  {
    String json = "{";
    json += "\"ok\":true,";
    json += "\"ip\":\"" + wifi_manager::getIp() + "\",";
    json += "\"time\":\"" + rtc_clock::getDateTime() + "\",";
    json += "\"temp\":\"" + String(temp_hum::getTemperature(), 1) + "\",";
    json += "\"hum\":\"" + String(temp_hum::getHumidity(), 0) + "\",";
    json += "\"battv\":\"" + String(battery_level::getVoltage(), 2) + "\",";
    json += "\"battp\":\"" + String(battery_level::getPercentage()) + "\",";
    json += "\"solarv\":\"" + String(solar_level::getVoltage(), 2) + "\",";
    json += "\"solarp\":\"" + String(solar_level::getPercentage()) + "\",";
    json += "\"dist\":\"" + String(ultrasonic::getDistanceCm(), 1) + "\",";
    json += "\"lcd\":\"" + String(lcd_screen::isAwake() ? "AWAKE" : "SLEEP") + "\",";
    json += "\"heater\":\"" + relayState(heater::isOn()) + "\",";
    json += "\"spinner\":\"" + relayState(spinner::isOn()) + "\",";
    json += "\"humidifier\":\"" + relayState(humidifier::isOn()) + "\",";
    json += "\"buzzer\":\"" + buzzerState() + "\"";
    json += "}";

    server.send(200, "application/json", json);
  }

  void begin()
  {
    server.on("/", []()
              { server.send(200, "text/html", page()); });

    server.on("/api", []()
              { sendApi(); });

    server.on("/toggle", []()
              {
                String dev = server.arg("dev");

                if (dev == "heater")
                {
                  heater::toggle();
                  showDeviceChange("Heater", heater::isOn());
                }
                else if (dev == "spinner")
                {
                  spinner::toggle();
                  showDeviceChange("Spinner", spinner::isOn());
                }
                else if (dev == "humidifier")
                {
                  humidifier::toggle();
                  showDeviceChange("Humidifier", humidifier::isOn());
                }

                sendStateJson(); });

    server.on("/beep", []()
              {
                buzzer::beep(100, 100, 2);
                lcd_screen::showMessage("Dashboard", "Buzzer test", "Running", "", 1500, lcd_screen::PRIORITY_INFO);
                sendStateJson(); });

    server.on("/lcd", []()
              {
                if (server.arg("wake") == "1")
                {
                  lcd_screen::wake();
                  lcd_screen::showMessage("LCD", "Screen awake", "", "", 1200, lcd_screen::PRIORITY_INFO);
                }
                else
                {
                  lcd_screen::sleep();
                }

                sendStateJson(); });

    server.begin();
  }

  void update()
  {
    server.handleClient();
  }
}
