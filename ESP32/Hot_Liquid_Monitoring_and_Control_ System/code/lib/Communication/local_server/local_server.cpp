#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "local_server.h"
#include "maintenance_manager/maintenance_manager.h"
#include "temp_sensor/temp_sensor.h"
#include "current_sensor/current_sensor.h"
#include "vibration_sensor/vibration_sensor.h"
#include "load_relay/load_relay.h"
#include "sd_card/sd_card.h"

namespace local_server
{
  static WebServer server(80);
  static bool running = false;
  static String ipAddress = "0.0.0.0";

  static const char *apSsid = "MOTOR_PM_SYSTEM";
  static const char *apPassword = "12345678";

  static String jsonEscape(const String &input)
  {
    String out;
    out.reserve(input.length() + 8);

    for (size_t i = 0; i < input.length(); i++)
    {
      char c = input[i];

      if (c == '"')
        out += "\\\"";
      else if (c == '\\')
        out += "\\\\";
      else if (c == '\n')
        out += "\\n";
      else if (c == '\r')
        out += "";
      else
        out += c;
    }

    return out;
  }

  static String forecastText(float minutes)
  {
    if (minutes < 0.0f)
      return "No rising fault trend detected";

    if (minutes < 1.0f)
      return "Fault condition is active or imminent";

    if (minutes < 60.0f)
      return String(minutes, 1) + " minutes to estimated fault limit";

    return String(minutes / 60.0f, 1) + " hours to estimated fault limit";
  }

  static void sendCors()
  {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Cache-Control", "no-store");
  }

  static const char indexPage[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Industrial Motor Predictive Maintenance</title>
<style>
:root{--bg:#eef3f8;--card:#fff;--text:#18212f;--muted:#667085;--accent:#2563eb;--good:#16a34a;--warn:#ca8a04;--bad:#dc2626;--orange:#ea580c;--line:rgba(100,116,139,.22);--shadow:0 14px 35px rgba(15,23,42,.12)}
[data-theme=dark]{--bg:#0b1220;--card:#111c2f;--text:#e5edf8;--muted:#98a2b3;--accent:#60a5fa;--line:rgba(148,163,184,.18);--shadow:0 14px 35px rgba(0,0,0,.35)}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;min-height:100vh;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at top left,rgba(37,99,235,.14),transparent 32%),var(--bg);color:var(--text);display:flex;justify-content:center}
.page{width:100%;max-width:1220px;margin:0 auto}
.header{padding:22px 18px;display:flex;justify-content:center;align-items:center;gap:14px;max-width:1180px;margin:auto;flex-wrap:wrap;text-align:center}
.title{flex:1 1 560px;display:flex;flex-direction:column;align-items:center}
.title h1{margin:0;font-size:25px;letter-spacing:-.4px}
.title p{margin:6px auto 0;color:var(--muted);line-height:1.4;max-width:760px}
.top-actions{display:flex;justify-content:center;align-items:center;gap:10px;flex-wrap:wrap;flex:1 1 100%}
.pill{border:0;border-radius:999px;padding:10px 14px;background:var(--card);color:var(--text);box-shadow:var(--shadow);font-weight:700}
.theme-btn{width:44px;height:44px;border:0;border-radius:50%;background:var(--card);color:var(--text);box-shadow:var(--shadow);cursor:pointer;display:inline-flex;justify-content:center;align-items:center;font-size:20px;line-height:1}
.container{max-width:1180px;margin:auto;padding:0 16px 30px}
.grid{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:16px;justify-content:center}
.card{background:var(--card);border:1px solid var(--line);border-radius:22px;padding:18px;box-shadow:var(--shadow);text-align:center;display:flex;flex-direction:column;align-items:center}
.span3{grid-column:span 3}.span4{grid-column:span 4}.span5{grid-column:span 5}.span6{grid-column:span 6}.span7{grid-column:span 7}.span8{grid-column:span 8}.span12{grid-column:span 12}
.label{font-size:13px;color:var(--muted);margin-bottom:8px;font-weight:700;text-transform:uppercase;letter-spacing:.04em}
.value{font-size:29px;font-weight:800;letter-spacing:-.5px}
.unit{font-size:14px;color:var(--muted);font-weight:500}
.small{color:var(--muted);font-size:13px;line-height:1.5;max-width:760px}
.status{display:inline-block;padding:9px 13px;border-radius:999px;font-weight:800;font-size:13px;background:#e5e7eb;color:#111827}
.status.NORMAL{background:#dcfce7;color:#166534}.status.WARNING{background:#fef9c3;color:#854d0e}.status.CRITICAL{background:#ffedd5;color:#9a3412}.status.FAULT{background:#fee2e2;color:#991b1b}
.bar{width:100%;max-width:520px;height:12px;background:rgba(100,116,139,.22);border-radius:999px;overflow:hidden;margin-top:10px}
.fill{height:100%;width:0%;background:var(--accent);transition:width .35s ease;border-radius:999px}
.fill.good{background:var(--good)}.fill.warn{background:var(--warn)}.fill.bad{background:var(--bad)}.fill.orange{background:var(--orange)}
.actions{display:flex;justify-content:center;gap:10px;flex-wrap:wrap;width:100%}
.btn{border:0;border-radius:14px;padding:11px 14px;font-weight:800;background:var(--accent);color:#fff;cursor:pointer}
.btn.secondary{background:#64748b}.btn.danger{background:var(--bad)}.btn.good{background:var(--good)}
pre{width:100%;background:rgba(100,116,139,.12);border:1px solid var(--line);border-radius:16px;padding:12px;overflow:auto;max-height:330px;font-size:12px;white-space:pre-wrap;text-align:left}
.metric-row{width:100%;display:flex;justify-content:space-between;align-items:center;gap:12px;border-top:1px solid var(--line);padding:12px 0;text-align:left}
.metric-row:first-of-type{border-top:0}
.metric-row strong{font-size:14px}
.metric-row span{color:var(--muted);font-size:13px;text-align:right}
.footer{margin-top:16px;text-align:center;color:var(--muted);font-size:12px}
canvas{width:100%;height:260px;display:block}
@media(max-width:900px){.span3,.span4,.span5,.span6,.span7,.span8{grid-column:span 12}.value{font-size:24px}.header{padding:16px}.title h1{font-size:20px}.metric-row{flex-direction:column;text-align:center}.metric-row span{text-align:center}}
</style>
</head>
<body>
<div class="page">
<div class="header">
  <div class="title">
    <h1>Industrial Motor Predictive Maintenance</h1>
    <p>Live condition monitoring, risk scoring, future fault estimate, SD/Internal backend logging.</p>
  </div>
  <div class="top-actions">
    <span class="pill" id="backendPill">Backend: --</span>
    <button class="theme-btn" id="themeIcon" onclick="toggleTheme()" aria-label="Toggle dark and light mode" title="Toggle theme">🌙</button>
  </div>
</div>

<div class="container">
  <div class="grid">
    <div class="card span3">
      <div class="label">Temperature</div>
      <div class="value" id="temp">-- <span class="unit">°C</span></div>
      <p class="small">DS18B20 motor body temperature</p>
    </div>

    <div class="card span3">
      <div class="label">Current</div>
      <div class="value" id="current">-- <span class="unit">A</span></div>
      <p class="small">ZMCT103C load current estimate</p>
    </div>

    <div class="card span3">
      <div class="label">Vibration RMS</div>
      <div class="value" id="vibration">-- <span class="unit">g</span></div>
      <p class="small">ADXL345 vibration severity</p>
    </div>

    <div class="card span3">
      <div class="label">Condition</div>
      <span class="status NORMAL" id="level">NORMAL</span>
      <p class="small" id="backend">Backend: --</p>
    </div>

    <div class="card span6">
      <div class="label">Risk Score</div>
      <div class="value" id="risk">-- <span class="unit">%</span></div>
      <div class="bar"><div class="fill good" id="riskFill"></div></div>
      <p class="small">Higher score means higher probability of fault based on current values and trend.</p>
    </div>

    <div class="card span6">
      <div class="label">Motor Health</div>
      <div class="value" id="health">-- <span class="unit">%</span></div>
      <div class="bar"><div class="fill good" id="healthFill"></div></div>
      <p class="small" id="forecast">Forecast: --</p>
    </div>

    <div class="card span7">
      <div class="label">Future Analysis / Recommendation</div>
      <div class="value" id="worst" style="font-size:20px">Worst Metric: --</div>
      <p class="small" id="recommendation">Waiting for data...</p>
      <div class="metric-row"><strong>Maintenance Decision</strong><span id="decision">--</span></div>
      <div class="metric-row"><strong>Estimated Time-To-Fault</strong><span id="ttf">--</span></div>
      <div class="metric-row"><strong>Shutdown Status</strong><span id="shutdown">--</span></div>
    </div>

    <div class="card span5">
      <div class="label">Motor Control</div>
      <p class="small" id="relay">Relay: --</p>
      <div class="actions">
        <button class="btn good" onclick="cmd('/api/relay?state=on')">Relay ON</button>
        <button class="btn secondary" onclick="cmd('/api/relay?state=off')">Relay OFF</button>
        <button class="btn danger" onclick="cmd('/api/clear_fault')">Clear Fault</button>
      </div>
      <p class="small">Relay ON will be blocked automatically while a fault is still active.</p>
    </div>

    <div class="card span4"><div class="label">Acceleration X</div><div class="value" id="x">-- <span class="unit">g</span></div></div>
    <div class="card span4"><div class="label">Acceleration Y</div><div class="value" id="y">-- <span class="unit">g</span></div></div>
    <div class="card span4"><div class="label">Acceleration Z</div><div class="value" id="z">-- <span class="unit">g</span></div></div>

    <div class="card span12">
      <div class="label">Trend Graph</div>
      <canvas id="chart" width="1000" height="260"></canvas>
      <p class="small">Blue: current, orange: temperature, green: vibration.</p>
    </div>

    <div class="card span12">
      <div class="label">Recent Motor Data Log</div>
      <pre id="motorLog">Loading...</pre>
      <div class="actions">
        <button class="btn secondary" onclick="loadLogs()">Refresh Logs</button>
        <button class="btn secondary" onclick="cmd('/api/log_now')">Log Now</button>
        <button class="btn" onclick="location.href='/download/motor_log.csv'">Download Motor CSV</button>
        <button class="btn" onclick="location.href='/download/analysis_log.csv'">Download Analysis CSV</button>
        <button class="btn" onclick="location.href='/download/event_log.csv'">Download Events CSV</button>
      </div>
    </div>

    <div class="card span6"><div class="label">Recent Analysis Log</div><pre id="analysisLog">Loading...</pre></div>
    <div class="card span6"><div class="label">Recent Event Log</div><pre id="eventLog">Loading...</pre></div>
  </div>
  <div class="footer">ESP32-S3 WROOM-1U Predictive Maintenance Dashboard</div>
</div>
</div>

<script>
let points=[];
document.documentElement.setAttribute('data-theme',localStorage.getItem('theme')||'light');
updateThemeIcon();

function updateThemeIcon(){
  const icon=document.getElementById('themeIcon');
  if(!icon)return;
  icon.textContent=document.documentElement.getAttribute('data-theme')==='dark'?'☀️':'🌙';
}

function toggleTheme(){
  const d=document.documentElement;
  const theme=d.getAttribute('data-theme')==='dark'?'light':'dark';
  d.setAttribute('data-theme',theme);
  localStorage.setItem('theme',theme);
  updateThemeIcon();
}

function fmt(v,d){
  v=Number(v);
  if(!isFinite(v))return '--';
  return v.toFixed(d);
}

function clamp(v,a,b){
  return Math.max(a,Math.min(b,v));
}

function setFill(id,v,reverse){
  const e=document.getElementById(id);
  v=clamp(Number(v)||0,0,100);
  e.style.width=v+'%';
  let cls;
  if(reverse)cls=v>=75?'good':v>=50?'warn':'bad';
  else cls=v>=75?'bad':v>=50?'warn':'good';
  e.className='fill '+cls;
}

function decision(level){
  if(level==='FAULT')return 'Stop motor and inspect immediately';
  if(level==='CRITICAL')return 'Plan urgent maintenance';
  if(level==='WARNING')return 'Monitor closely';
  return 'Continue operation';
}

async function cmd(url){
  try{
    await fetch(url);
    await loadStatus();
    await loadLogs();
  }catch(e){}
}

async function loadStatus(){
  try{
    const r=await fetch('/api/status');
    const s=await r.json();

    document.getElementById('temp').innerHTML=s.temp_valid?fmt(s.temperature_c,1)+' <span class="unit">°C</span>':'N/A';
    document.getElementById('current').innerHTML=fmt(s.current_a,2)+' <span class="unit">A</span>';
    document.getElementById('vibration').innerHTML=fmt(s.vibration_rms_g,3)+' <span class="unit">g</span>';

    const lev=document.getElementById('level');
    lev.textContent=s.level;
    lev.className='status '+s.level;

    document.getElementById('backend').textContent='Backend: '+s.backend+' | SD: '+(s.sd_ready?'ready':'not ready')+' | Internal: '+(s.internal_ready?'ready':'not ready');
    document.getElementById('backendPill').textContent='Backend: '+s.backend;

    document.getElementById('risk').innerHTML=fmt(s.risk_score,1)+' <span class="unit">%</span>';
    document.getElementById('health').innerHTML=fmt(s.health_score,1)+' <span class="unit">%</span>';
    setFill('riskFill',s.risk_score,false);
    setFill('healthFill',s.health_score,true);

    document.getElementById('forecast').textContent='Forecast: '+s.forecast_text;
    document.getElementById('worst').textContent='Worst Metric: '+s.worst_metric;
    document.getElementById('recommendation').textContent=s.recommendation;
    document.getElementById('decision').textContent=decision(s.level);
    document.getElementById('ttf').textContent=s.forecast_text;
    document.getElementById('shutdown').textContent=s.fault?'Relay forced OFF by protection logic':'No shutdown active';

    document.getElementById('relay').textContent='Relay: '+(s.relay_on?'ON':'OFF')+' | Requested: '+(s.relay_requested?'ON':'OFF')+' | Fault: '+(s.fault?'YES':'NO');

    document.getElementById('x').innerHTML=fmt(s.x_g,2)+' <span class="unit">g</span>';
    document.getElementById('y').innerHTML=fmt(s.y_g,2)+' <span class="unit">g</span>';
    document.getElementById('z').innerHTML=fmt(s.z_g,2)+' <span class="unit">g</span>';

    points.push({current:Number(s.current_a)||0,temp:Number(s.temperature_c)||0,vib:Number(s.vibration_rms_g)||0});
    if(points.length>70)points.shift();
    drawChart();
  }catch(e){}
}

async function loadLogs(){
  try{
    const r=await fetch('/api/logs');
    const s=await r.json();
    document.getElementById('motorLog').textContent=s.motor_log||'No motor log yet';
    document.getElementById('analysisLog').textContent=s.analysis_log||'No analysis log yet';
    document.getElementById('eventLog').textContent=s.event_log||'No event log yet';
  }catch(e){}
}

function drawChart(){
  const c=document.getElementById('chart');
  const ctx=c.getContext('2d');
  const w=c.width,h=c.height;
  ctx.clearRect(0,0,w,h);
  ctx.strokeStyle=getComputedStyle(document.documentElement).getPropertyValue('--line');
  ctx.lineWidth=1;

  for(let i=0;i<5;i++){
    const y=30+i*45;
    ctx.beginPath();
    ctx.moveTo(30,y);
    ctx.lineTo(w-20,y);
    ctx.stroke();
  }

  drawLine(ctx,points.map(p=>p.current),0,6,'#2563eb');
  drawLine(ctx,points.map(p=>p.temp),20,90,'#ea580c');
  drawLine(ctx,points.map(p=>p.vib),0,3,'#16a34a');
}

function drawLine(ctx,arr,min,max,color){
  if(arr.length<2)return;
  const w=ctx.canvas.width,h=ctx.canvas.height;
  ctx.strokeStyle=color;
  ctx.lineWidth=3;
  ctx.beginPath();

  arr.forEach((v,i)=>{
    const x=30+(i/69)*(w-55);
    const y=h-25-((v-min)/(max-min))*(h-55);
    if(i===0)ctx.moveTo(x,y);
    else ctx.lineTo(x,y);
  });

  ctx.stroke();
}

loadStatus();
loadLogs();
setInterval(loadStatus,2000);
setInterval(loadLogs,10000);
</script>
</body>
</html>
)HTML";

  static void handleRoot()
  {
    sendCors();
    server.send_P(200, "text/html", indexPage);
  }

  static void handleStatus()
  {
    sendCors();

    maintenance_manager::Snapshot snap = maintenance_manager::getSnapshot();
    String forecast = forecastText(snap.forecastMinutes);

    String json;
    json.reserve(1200);

    json += "{";
    json += "\"uptime_ms\":";
    json += String(millis());
    json += ",";

    json += "\"temperature_c\":";
    json += String(snap.temperatureC, 2);
    json += ",";

    json += "\"temp_valid\":";
    json += snap.tempValid ? "true" : "false";
    json += ",";

    json += "\"current_a\":";
    json += String(snap.currentA, 3);
    json += ",";

    json += "\"current_vrms\":";
    json += String(current_sensor::getVoltageRMS(), 4);
    json += ",";

    json += "\"vibration_rms_g\":";
    json += String(snap.vibrationRmsG, 3);
    json += ",";

    json += "\"x_g\":";
    json += String(snap.xG, 3);
    json += ",";

    json += "\"y_g\":";
    json += String(snap.yG, 3);
    json += ",";

    json += "\"z_g\":";
    json += String(snap.zG, 3);
    json += ",";

    json += "\"risk_score\":";
    json += String(snap.riskScore, 1);
    json += ",";

    json += "\"health_score\":";
    json += String(snap.healthScore, 1);
    json += ",";

    json += "\"forecast_minutes\":";
    json += String(snap.forecastMinutes, 1);
    json += ",";

    json += "\"forecast_text\":\"";
    json += jsonEscape(forecast);
    json += "\",";

    json += "\"level\":\"";
    json += jsonEscape(maintenance_manager::getLevelText());
    json += "\",";

    json += "\"worst_metric\":\"";
    json += jsonEscape(maintenance_manager::getWorstMetric());
    json += "\",";

    json += "\"recommendation\":\"";
    json += jsonEscape(maintenance_manager::getRecommendation());
    json += "\",";

    json += "\"relay_on\":";
    json += load_relay::isOn() ? "true" : "false";
    json += ",";

    json += "\"relay_requested\":";
    json += load_relay::getRequestedState() ? "true" : "false";
    json += ",";

    json += "\"fault\":";
    json += maintenance_manager::isFault() ? "true" : "false";
    json += ",";

    json += "\"sd_ready\":";
    json += sd_card::isSdReady() ? "true" : "false";
    json += ",";

    json += "\"internal_ready\":";
    json += sd_card::isInternalReady() ? "true" : "false";
    json += ",";

    json += "\"backend\":\"";
    json += jsonEscape(sd_card::getBackendName());
    json += "\",";

    json += "\"internal_ready\":";
    json += sd_card::isInternalReady() ? "true" : "false";
    json += ",";

    json += "\"backend\":\"";
    json += jsonEscape(sd_card::getBackendName());
    json += "\",";

    json += "\"motor_log_size\":";
    json += String(sd_card::getFileSize(sd_card::getMotorLogFileName()));
    json += ",";

    json += "\"analysis_log_size\":";
    json += String(sd_card::getFileSize(sd_card::getAnalysisLogFileName()));
    json += ",";

    json += "\"event_log_size\":";
    json += String(sd_card::getFileSize(sd_card::getEventLogFileName()));

    json += "}";

    server.send(200, "application/json", json);
  }

  static void handleLogs()
  {
    sendCors();

    String json;
    json.reserve(14000);

    json += "{";

    json += "\"motor_log\":\"";
    json += jsonEscape(sd_card::readTail(sd_card::getMotorLogFileName(), 5000));
    json += "\",";

    json += "\"analysis_log\":\"";
    json += jsonEscape(sd_card::readTail(sd_card::getAnalysisLogFileName(), 4000));
    json += "\",";

    json += "\"event_log\":\"";
    json += jsonEscape(sd_card::readTail(sd_card::getEventLogFileName(), 3000));
    json += "\"";

    json += "}";

    server.send(200, "application/json", json);
  }

  static void handleRelay()
  {
    sendCors();

    if (server.hasArg("state"))
    {
      String state = server.arg("state");
      state.toLowerCase();

      if (state == "on")
      {
        load_relay::turnOn();
        sd_card::logEvent("RELAY", "Relay requested ON from dashboard.");
      }
      else if (state == "off")
      {
        load_relay::turnOff();
        sd_card::logEvent("RELAY", "Relay requested OFF from dashboard.");
      }
    }

    server.send(200, "application/json", "{\"ok\":true}");
  }

  static void handleClearFault()
  {
    sendCors();

    maintenance_manager::clearFault();
    sd_card::logEvent("FAULT", "Fault cleared from dashboard.");

    server.send(200, "application/json", "{\"ok\":true}");
  }

  static void handleLogNow()
  {
    sendCors();

    sd_card::logNow();

    server.send(200, "application/json", "{\"ok\":true}");
  }

  static void streamCsv(const char *path)
  {
    sendCors();

    File file = sd_card::openRead(path);

    if (!file)
    {
      server.send(404, "text/plain", "File not found");
      return;
    }

    server.streamFile(file, "text/csv");
    file.close();
  }

  static void handleMotorDownload()
  {
    streamCsv(sd_card::getMotorLogFileName());
  }

  static void handleAnalysisDownload()
  {
    streamCsv(sd_card::getAnalysisLogFileName());
  }

  static void handleEventDownload()
  {
    streamCsv(sd_card::getEventLogFileName());
  }

  void begin()
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    ipAddress = WiFi.softAPIP().toString();

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/logs", HTTP_GET, handleLogs);
    server.on("/api/relay", HTTP_GET, handleRelay);
    server.on("/api/clear_fault", HTTP_GET, handleClearFault);
    server.on("/api/log_now", HTTP_GET, handleLogNow);
    server.on("/download/motor_log.csv", HTTP_GET, handleMotorDownload);
    server.on("/download/analysis_log.csv", HTTP_GET, handleAnalysisDownload);
    server.on("/download/event_log.csv", HTTP_GET, handleEventDownload);

    server.begin();

    running = true;
  }

  void update()
  {
    if (running)
    {
      server.handleClient();
    }
  }

  bool isRunning()
  {
    return running;
  }

  String getIp()
  {
    return ipAddress;
  }

  String getSsid()
  {
    return String(apSsid);
  }
}
