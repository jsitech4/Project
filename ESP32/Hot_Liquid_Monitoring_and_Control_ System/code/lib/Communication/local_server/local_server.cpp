#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "local_server.h"
#include "maintenance_manager/maintenance_manager.h"
#include "temp_sensor/temp_sensor.h"
#include "ultrasonic_sensor/ultrasonic_sensor.h"
#include "load_relay/load_relay.h"
#include "storage/storage.h"

namespace local_server
{
  static WebServer server(80);
  static bool running = false;
  static String ipAddress = "0.0.0.0";

  static const char *apSsid = "Hot Liquid Monitoring and Control System";
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
<title>Hot Liquid Monitoring and Control System</title>
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
.status.NORMAL{background:#dcfce7;color:#166534}.status.WARNING{background:#fef9c3;color:#854d0e}
.bar{width:100%;max-width:520px;height:12px;background:rgba(100,116,139,.22);border-radius:999px;overflow:hidden;margin-top:10px}
.fill{height:100%;width:0%;background:var(--accent);transition:width .35s ease;border-radius:999px}
.fill.good{background:var(--good)}.fill.warn{background:var(--warn)}.fill.bad{background:var(--bad)}.fill.orange{background:var(--orange)}
.tank-wrap{display:flex;align-items:center;justify-content:center;gap:28px;width:100%;padding:8px 0 14px}.tank{position:relative;width:170px;height:250px;border:4px solid var(--text);border-radius:18px 18px 28px 28px;background:rgba(148,163,184,.10);overflow:hidden;box-shadow:inset 0 0 0 2px rgba(255,255,255,.35)}.tank:before{content:'';position:absolute;left:24px;right:24px;top:12px;height:7px;border:2px solid var(--text);border-radius:8px}.liquid{position:absolute;left:0;right:0;bottom:0;height:0%;background:linear-gradient(180deg,#38bdf8,#0284c7);transition:height .5s ease}.liquid:before{content:'';position:absolute;left:-10%;top:-8px;width:120%;height:16px;border-radius:50%;background:rgba(186,230,253,.72)}.tank-label{position:absolute;inset:0;display:flex;align-items:center;justify-content:center;z-index:1;font-size:27px;font-weight:800;color:#fff;text-shadow:0 2px 4px rgba(0,0,0,.5)}.valve{display:flex;flex-direction:column;align-items:center;gap:10px;min-width:150px}.valve-body{width:74px;height:74px;border:5px solid var(--text);transform:rotate(45deg);background:var(--bad);transition:background .25s ease}.valve-body.open{background:var(--good)}.valve-stem{width:8px;height:38px;background:var(--text);margin-top:-32px;z-index:1}.valve-title{font-size:18px;font-weight:800}.valve-state{font-size:13px;color:var(--muted)}
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
    <h1>Hot Liquid Monitoring and Control System</h1>
    <p>Live tank level, PT100 temperature, and valve control.</p>
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
      <p class="small">PT100 through MAX31865, 3-wire mode</p>
    </div>

    <div class="card span3">
      <div class="label">Ultrasonic Distance</div>
      <div class="value" id="distance">-- <span class="unit">cm</span></div>
      <p class="small">Sensor distance from the liquid surface</p>
    </div>

    <div class="card span3">
      <div class="label">Tank Level</div>
      <div class="value" id="levelPercent">-- <span class="unit">%</span></div>
      <p class="small">Calibrated between 5 cm full and 40 cm empty</p>
    </div>

    <div class="card span3">
      <div class="label">Condition</div>
      <span class="status NORMAL" id="level">NORMAL</span>
      <p class="small" id="backend">Backend: --</p>
    </div>

    <div class="card span12">
      <div class="label">Tank And Valve</div>
      <div class="tank-wrap">
        <div class="tank"><div class="liquid" id="liquidFill"></div><div class="tank-label" id="tankLabel">--%</div></div>
        <div class="valve"><div class="valve-body" id="valveBody"></div><div class="valve-stem"></div><div class="valve-title">Outlet Valve</div><div class="valve-state" id="valveState">Relay OFF</div></div>
      </div>
      <div class="bar"><div class="fill good" id="tankFill"></div></div>
      <p class="small">The valve symbol follows the physical relay state.</p>
    </div>

    <div class="card span6">
      <div class="label">Sensor Status</div>
      <div class="value" id="sensorStatus">READY</div>
      <p class="small" id="sensorStatusText">PT100 and ultrasonic online</p>
    </div>

    <div class="card span7">
      <div class="label">Tank Recommendation</div>
      <div class="value" id="worst" style="font-size:20px">Worst Metric: --</div>
      <p class="small" id="recommendation">Waiting for data...</p>
      <div class="metric-row"><strong>Maintenance Decision</strong><span id="decision">--</span></div>
    </div>

    <div class="card span5">
      <div class="label">Valve Control</div>
      <p class="small" id="relay">Valve relay: --</p>
      <div class="actions">
        <button class="btn good" onclick="cmd('/api/relay?state=on')">Valve OPEN</button>
        <button class="btn secondary" onclick="cmd('/api/relay?state=off')">Valve CLOSE</button>
      </div>
      <p class="small">The relay directly controls the valve output.</p>
    </div>

    <div class="card span12">
      <div class="label">Tank Automation Settings</div>
      <div class="metric-row"><strong>Full level threshold (%)</strong><span><input id="fullThreshold" type="number" min="1" max="100" step="1" value="90"></span></div>
      <div class="metric-row"><strong>Low level threshold (%)</strong><span><input id="lowThreshold" type="number" min="0" max="99" step="1" value="20"></span></div>
      <div class="metric-row"><strong>Relay latch trigger</strong><span><select id="latchMode"><option value="0">Disabled</option><option value="1">Latch at full level</option><option value="2">Latch at low level</option></select></span></div>
      <div class="actions"><button class="btn good" onclick="saveSettings()">Save settings</button><span class="small" id="settingsMessage">Saved settings load automatically at boot.</span></div>
    </div>

    <div class="card span4"><div class="label">Acceleration X</div><div class="value" id="x">-- <span class="unit">g</span></div></div>
    <div class="card span4"><div class="label">Acceleration Y</div><div class="value" id="y">-- <span class="unit">g</span></div></div>
    <div class="card span4"><div class="label">Acceleration Z</div><div class="value" id="z">-- <span class="unit">g</span></div></div>

    <div class="card span12">
      <div class="label">Trend Graph</div>
      <canvas id="chart" width="1000" height="260"></canvas>
      <p class="small">Orange: PT100 temperature, green: tank level.</p>
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
    document.getElementById('distance').innerHTML=s.level_valid?fmt(s.distance_cm,1)+' <span class="unit">cm</span>':'N/A';
    document.getElementById('levelPercent').innerHTML=s.level_valid?fmt(s.level_percent,0)+' <span class="unit">%</span>':'N/A';
    document.getElementById('liquidFill').style.height=(s.level_valid?clamp(s.level_percent,0,100):0)+'%';
    document.getElementById('tankFill').style.width=(s.level_valid?clamp(s.level_percent,0,100):0)+'%';
    document.getElementById('tankLabel').textContent=s.level_valid?fmt(s.level_percent,0)+'%':'N/A';

    document.getElementById('backend').textContent='Backend: '+s.backend+' | Internal: '+(s.internal_ready?'ready':'not ready');
    document.getElementById('backendPill').textContent='Backend: '+s.backend;

    document.getElementById('sensorStatus').textContent=(s.temp_valid&&s.level_valid)?'READY':'WAITING';
    document.getElementById('sensorStatusText').textContent=(s.temp_valid&&s.level_valid)?'PT100 and ultrasonic online':'Waiting for sensor readings';

    document.getElementById('worst').textContent='Tank monitoring';
    document.getElementById('recommendation').textContent='Live tank measurements are available.';

    document.getElementById('relay').textContent='Valve relay: '+(s.relay_on?'ON':'OFF')+' | Requested: '+(s.relay_requested?'ON':'OFF');
    document.getElementById('valveState').textContent=s.relay_on?'Relay ON / Valve OPEN':'Relay OFF / Valve CLOSED';
    document.getElementById('valveBody').className='valve-body'+(s.relay_on?' open':'');

    points.push({temp:Number(s.temperature_c)||0,level:Number(s.level_percent)||0});
    if(points.length>70)points.shift();
    drawChart();
    document.getElementById('fullThreshold').value=s.full_level_percent;
    document.getElementById('lowThreshold').value=s.low_level_percent;
    document.getElementById('latchMode').value=s.relay_latch_mode;
  }catch(e){}
}

async function saveSettings(){
  const full=Number(document.getElementById('fullThreshold').value);
  const low=Number(document.getElementById('lowThreshold').value);
  const mode=document.getElementById('latchMode').value;
  const message=document.getElementById('settingsMessage');
  if(!Number.isFinite(full)||!Number.isFinite(low)||full<=low||full>100||low<0){
    message.textContent='Full level must be higher than low level.';
    return;
  }
  try{
    const response=await fetch('/api/settings/save?full='+encodeURIComponent(full)+'&low='+encodeURIComponent(low)+'&mode='+encodeURIComponent(mode));
    const result=await response.json();
    message.textContent=result.ok?'Settings saved to internal flash.':'Settings were not saved.';
  }catch(e){message.textContent='Dashboard could not save settings.';}
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

  drawLine(ctx,points.map(p=>p.temp),20,90,'#ea580c');
  drawLine(ctx,points.map(p=>p.level),0,100,'#16a34a');
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
    String json;
    json.reserve(1200);

    json += "{";
    json += "\"uptime_ms\":";
    json += String(millis());
    json += ",";

    json += "\"temperature_c\":";
    json += snap.tempValid ? String(snap.temperatureC, 2) : String("null");
    json += ",";

    json += "\"temp_valid\":";
    json += snap.tempValid ? "true" : "false";
    json += ",";

    json += "\"distance_cm\":";
    json += snap.levelValid ? String(snap.distanceCm, 1) : String("null");
    json += ",";

    json += "\"level_percent\":";
    json += snap.levelValid ? String(snap.levelPercent, 1) : String("null");
    json += ",";

    json += "\"level_valid\":";
    json += snap.levelValid ? "true" : "false";
    json += ",";

    json += "\"relay_on\":";
    json += load_relay::isOn() ? "true" : "false";
    json += ",";

    json += "\"relay_requested\":";
    json += load_relay::getRequestedState() ? "true" : "false";
    json += ",";

    json += "\"full_level_percent\":";
    json += String(maintenance_manager::getFullLevelPercent(), 1);
    json += ",";

    json += "\"low_level_percent\":";
    json += String(maintenance_manager::getLowLevelPercent(), 1);
    json += ",";

    json += "\"relay_latch_mode\":";
    json += String(static_cast<uint8_t>(maintenance_manager::getRelayLatchMode()));
    json += ",";

    json += "\"sd_ready\":";
    json += storage::isSdReady() ? "true" : "false";
    json += ",";

    json += "\"internal_ready\":";
    json += storage::isInternalReady() ? "true" : "false";
    json += ",";

    json += "\"backend\":\"";
    json += jsonEscape(storage::getBackendName());
    json += "\",";

    json += "\"motor_log_size\":";
    json += String(storage::getFileSize(storage::getLiquidLogFileName()));
    json += ",";

    json += "\"analysis_log_size\":";
    json += String(storage::getFileSize(storage::getAnalysisLogFileName()));
    json += ",";

    json += "\"event_log_size\":";
    json += String(storage::getFileSize(storage::getEventLogFileName()));

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
    json += jsonEscape(storage::readTail(storage::getLiquidLogFileName(), 5000));
    json += "\",";

    json += "\"analysis_log\":\"";
    json += jsonEscape(storage::readTail(storage::getAnalysisLogFileName(), 4000));
    json += "\",";

    json += "\"event_log\":\"";
    json += jsonEscape(storage::readTail(storage::getEventLogFileName(), 3000));
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
        storage::logEvent("RELAY", "Relay requested ON from dashboard.");
      }
      else if (state == "off")
      {
        load_relay::turnOff();
        storage::logEvent("RELAY", "Relay requested OFF from dashboard.");
      }
    }

    server.send(200, "application/json", "{\"ok\":true}");
  }

  static void handleLogNow()
  {
    sendCors();

    storage::logNow();

    server.send(200, "application/json", "{\"ok\":true}");
  }

  static void handleSaveSettings()
  {
    sendCors();

    if (!server.hasArg("full") || !server.hasArg("low") || !server.hasArg("mode"))
    {
      server.send(400, "application/json", "{\"ok\":false}");
      return;
    }

    float fullPercent = server.arg("full").toFloat();
    float lowPercent = server.arg("low").toFloat();
    int mode = server.arg("mode").toInt();

    if (fullPercent <= lowPercent || fullPercent > 100.0f || lowPercent < 0.0f || mode < 0 || mode > 2)
    {
      server.send(400, "application/json", "{\"ok\":false}");
      return;
    }

    maintenance_manager::setLevelThresholds(fullPercent, lowPercent);
    maintenance_manager::setRelayLatchMode(static_cast<maintenance_manager::RelayLatchMode>(mode));
    bool saved = storage::saveTankSettings(fullPercent, lowPercent, static_cast<uint8_t>(mode));
    server.send(saved ? 200 : 500, "application/json", saved ? "{\"ok\":true}" : "{\"ok\":false}");
  }

  static void streamCsv(const char *path)
  {
    sendCors();

    File file = storage::openRead(path);

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
    streamCsv(storage::getLiquidLogFileName());
  }

  static void handleAnalysisDownload()
  {
    streamCsv(storage::getAnalysisLogFileName());
  }

  static void handleEventDownload()
  {
    streamCsv(storage::getEventLogFileName());
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
    server.on("/api/log_now", HTTP_GET, handleLogNow);
    server.on("/api/settings/save", HTTP_GET, handleSaveSettings);
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
