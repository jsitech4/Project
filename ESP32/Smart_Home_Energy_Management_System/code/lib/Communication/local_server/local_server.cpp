#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <pgmspace.h>

#include "local_server.h"
#include "config_manager/config_manager.h"
#include "pzem_sensor/pzem_sensor.h"
#include "load_manager/load_manager.h"
#include "nepa_sense/nepa_sense.h"
#include "inverter_sense/inverter_sense.h"
#include "load_relay/load_relay.h"

namespace local_server
{
  WebServer server(80);

  const char *apSsid = "SHEMS-Controller";
  const char *apPassword = "12345678";

  String ipAddress = "0.0.0.0";

  const char HOME_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Smart Home Energy Management System</title>
  <style>
    :root{
      --bg:#edf4ff;
      --bgGlow1:#dbeafe;
      --bgGlow2:#dcfce7;
      --surface:#ffffff;
      --surface2:#f8fafc;
      --box:#f1f5ff;
      --text:#102033;
      --muted:#64748b;
      --border:#dbe4f0;
      --shadow:0 16px 36px rgba(30,64,175,.12);
      --accent:#2563eb;
      --accent2:#14b8a6;
      --accent3:#f59e0b;
      --accentText:#ffffff;
      --ok:#16a34a;
      --bad:#dc2626;
      --inputBg:#ffffff;
    }

    body.dark{
      --bg:#07111f;
      --bgGlow1:#0f2a47;
      --bgGlow2:#12302c;
      --surface:#101b2d;
      --surface2:#16243a;
      --box:#13233a;
      --text:#f8fafc;
      --muted:#b6c3d5;
      --border:#263b57;
      --shadow:0 18px 42px rgba(0,0,0,.38);
      --accent:#60a5fa;
      --accent2:#2dd4bf;
      --accent3:#fbbf24;
      --accentText:#08111f;
      --ok:#4ade80;
      --bad:#fb7185;
      --inputBg:#0f172a;
    }

    *{
      box-sizing:border-box;
    }

    body{
      font-family:Arial,Helvetica,sans-serif;
      background:
        radial-gradient(circle at top left,var(--bgGlow1),transparent 32%),
        radial-gradient(circle at top right,var(--bgGlow2),transparent 30%),
        var(--bg);
      margin:0;
      padding:0;
      color:var(--text);
      transition:background .25s,color .25s;
    }

    .page{
      min-height:100vh;
      display:flex;
      justify-content:center;
      align-items:flex-start;
      padding:20px;
    }

    .wrap{
      width:100%;
      max-width:980px;
      margin:0 auto;
    }

    .topbar{
      display:flex;
      align-items:center;
      justify-content:space-between;
      gap:14px;
      margin:4px auto 16px;
      width:100%;
    }

    .titleBlock{
      min-width:0;
      text-align:center;
      flex:1;
    }

    h1{
      font-size:clamp(21px,4vw,31px);
      line-height:1.15;
      margin:0;
      letter-spacing:-.03em;
      color:var(--text);
    }

    .subtitle{
      margin-top:6px;
      color:var(--muted);
      font-size:13px;
    }

    .themeSwitch{
      flex-shrink:0;
      display:flex;
      align-items:center;
      justify-content:center;
      gap:8px;
      background:var(--surface);
      border:1px solid var(--border);
      border-radius:999px;
      padding:8px 10px;
      box-shadow:var(--shadow);
    }

    .themeText{
      font-size:12px;
      font-weight:bold;
      color:var(--muted);
      white-space:nowrap;
    }

    .switch{
      position:relative;
      width:52px;
      height:28px;
      display:inline-block;
    }

    .switch input{
      opacity:0;
      width:0;
      height:0;
    }

    .slider{
      position:absolute;
      cursor:pointer;
      inset:0;
      background:linear-gradient(135deg,#93c5fd,#38bdf8);
      border-radius:999px;
      transition:.25s;
    }

    .slider:before{
      position:absolute;
      content:"";
      height:22px;
      width:22px;
      left:3px;
      bottom:3px;
      background:#ffffff;
      border-radius:50%;
      transition:.25s;
      box-shadow:0 3px 8px rgba(0,0,0,.22);
    }

    .switch input:checked + .slider{
      background:linear-gradient(135deg,#1e293b,#6366f1);
    }

    .switch input:checked + .slider:before{
      transform:translateX(24px);
      background:#f8fafc;
    }

    .card{
      background:var(--surface);
      border:1px solid var(--border);
      border-radius:22px;
      padding:18px;
      margin:15px auto;
      box-shadow:var(--shadow);
      transition:background .25s,border .25s;
      overflow:hidden;
      position:relative;
    }

    .card:before{
      content:"";
      position:absolute;
      top:0;
      left:0;
      width:100%;
      height:4px;
      background:linear-gradient(90deg,var(--accent),var(--accent2),var(--accent3));
    }

    h2{
      font-size:17px;
      margin:3px 0 14px;
      text-align:center;
    }

    .grid{
      display:grid;
      grid-template-columns:repeat(auto-fit,minmax(155px,1fr));
      gap:12px;
    }

    .box{
      background:var(--box);
      border:1px solid var(--border);
      border-radius:17px;
      padding:14px;
      min-height:80px;
      display:flex;
      flex-direction:column;
      justify-content:center;
      align-items:center;
      text-align:center;
      transition:transform .18s,background .25s,border .25s;
    }

    .box:hover{
      transform:translateY(-2px);
    }

    .box.reading:nth-child(1){border-left:5px solid var(--accent);}
    .box.reading:nth-child(2){border-left:5px solid var(--accent2);}
    .box.reading:nth-child(3){border-left:5px solid var(--accent3);}
    .box.reading:nth-child(4){border-left:5px solid #8b5cf6;}

    .label{
      font-size:12px;
      color:var(--muted);
      line-height:1.3;
      text-align:center;
    }

    .value{
      font-size:clamp(18px,4vw,24px);
      font-weight:800;
      margin-top:6px;
      word-break:break-word;
      text-align:center;
    }

    input{
      width:100%;
      text-align:center;
      padding:10px;
      border:1px solid var(--border);
      border-radius:11px;
      background:var(--inputBg);
      color:var(--text);
      font-size:15px;
      margin-top:7px;
      outline:none;
    }

    input:focus{
      border-color:var(--accent);
      box-shadow:0 0 0 3px rgba(59,130,246,.14);
    }

    button{
      display:block;
      margin:0 auto;
      width:auto;
      min-width:150px;
      padding:12px 16px;
      border:0;
      border-radius:13px;
      background:linear-gradient(135deg,var(--accent),var(--accent2));
      color:var(--accentText);
      font-weight:bold;
      cursor:pointer;
      box-shadow:0 10px 22px rgba(37,99,235,.22);
    }

    .tableWrap{
      width:100%;
      overflow:hidden;
      border:1px solid var(--border);
      border-radius:16px;
      background:var(--surface);
    }

    table{
      width:100%;
      border-collapse:collapse;
      table-layout:fixed;
    }

    td,th{
      padding:12px 8px;
      border-bottom:1px solid var(--border);
      text-align:center;
      font-size:14px;
      word-break:break-word;
      overflow-wrap:anywhere;
    }

    tr:last-child td{
      border-bottom:0;
    }

    th{
      background:var(--surface2);
      color:var(--muted);
      font-size:11px;
      text-transform:uppercase;
      letter-spacing:.03em;
    }

    .ok{
      color:var(--ok);
      font-weight:800;
    }

    .bad{
      color:var(--bad);
      font-weight:800;
    }

    .sourceBadge{
      display:inline-block;
      padding:6px 10px;
      border-radius:999px;
      background:linear-gradient(135deg,rgba(37,99,235,.16),rgba(20,184,166,.16));
      color:var(--text);
      font-weight:bold;
      border:1px solid var(--border);
    }

    @media(max-width:700px){
      .tableWrap{
        border:0;
        background:transparent;
      }

      table,
      thead,
      tbody,
      th,
      td,
      tr{
        display:block;
        width:100%;
      }

      thead{
        display:none;
      }

      tbody{
        display:grid;
        grid-template-columns:1fr;
        gap:10px;
      }

      tr{
        background:var(--box);
        border:1px solid var(--border);
        border-radius:16px;
        padding:10px;
      }

      td{
        border-bottom:1px solid var(--border);
        display:flex;
        align-items:center;
        justify-content:space-between;
        gap:12px;
        text-align:right;
        padding:9px 4px;
        font-size:14px;
      }

      td:last-child{
        border-bottom:0;
      }

      td:before{
        content:attr(data-label);
        font-size:12px;
        color:var(--muted);
        font-weight:bold;
        text-align:left;
      }

      .sourceBadge{
        white-space:normal;
        max-width:130px;
        text-align:center;
      }

      .topbar{
        flex-direction:column;
        align-items:center;
        text-align:center;
      }

      .page{
        padding:12px;
      }

      .themeSwitch{
        padding:7px 8px;
      }

      .themeText{
        display:none;
      }

      .card{
        border-radius:18px;
        padding:15px;
      }

      .grid{
        grid-template-columns:1fr;
      }

      button{
        width:100%;
      }
    }

    @media(min-width:701px) and (max-width:920px){
      .grid{
        grid-template-columns:repeat(2,1fr);
      }
    }
  </style>
</head>
<body>
  <div class="page">
    <div class="wrap">
      <div class="topbar">
        <div class="titleBlock">
          <h1>Smart Home Energy Management System</h1>
          <div class="subtitle">Local server dashboard</div>
        </div>

        <div class="themeSwitch">
          <span class="themeText">Day</span>
          <label class="switch">
            <input id="themeToggle" type="checkbox" onchange="toggleTheme()">
            <span class="slider"></span>
          </label>
          <span class="themeText">Night</span>
        </div>
      </div>

      <div class="card">
        <h2>Live Sensor Readings</h2>
        <div class="grid">
          <div class="box reading"><div class="label">Voltage</div><div class="value" id="voltage">0V</div></div>
          <div class="box reading"><div class="label">Current</div><div class="value" id="current">0A</div></div>
          <div class="box reading"><div class="label">Actual Load Power</div><div class="value" id="power">0W</div></div>
          <div class="box reading"><div class="label">Energy</div><div class="value" id="energy">0Wh</div></div>
        </div>
      </div>

      <div class="card">
        <h2>System Decision</h2>
        <div class="grid">
          <div class="box"><div class="label">NEPA / PHCN</div><div class="value" id="nepa">---</div></div>
          <div class="box"><div class="label">Inverter</div><div class="value" id="inverter">---</div></div>
          <div class="box"><div class="label">Effective Limit</div><div class="value" id="effectiveLimit">0W</div></div>
          <div class="box"><div class="label">Decision</div><div class="value" id="decision">---</div></div>
        </div>
      </div>

      <div class="card">
        <h2>Load Configuration</h2>
        <form action="/save" method="GET">
          <div class="grid">
            <div class="box">
              <div class="label">Inverter Power Rating W</div>
              <input id="invInput" name="inv" type="number" min="500" max="50000" value="5000">
            </div>

            <div class="box">
              <div class="label">NEPA / System Power Rating W</div>
              <input id="sysInput" name="sys" type="number" min="500" max="50000" value="10000">
            </div>

            <div class="box">
              <div class="label">Allowed Load Margin %</div>
              <input id="marginInput" name="margin" type="number" min="50" max="100" value="90">
            </div>

            <div class="box"><div class="label">Relay 1 Load Power W</div><input id="r1Input" name="r1" type="number" min="0" max="4000" value="0"></div>
            <div class="box"><div class="label">Relay 2 Load Power W</div><input id="r2Input" name="r2" type="number" min="0" max="4000" value="0"></div>
            <div class="box"><div class="label">Relay 3 Load Power W</div><input id="r3Input" name="r3" type="number" min="0" max="4000" value="0"></div>
            <div class="box"><div class="label">Relay 4 Load Power W</div><input id="r4Input" name="r4" type="number" min="0" max="4000" value="0"></div>
            <div class="box"><div class="label">Relay 5 Load Power W</div><input id="r5Input" name="r5" type="number" min="0" max="4000" value="0"></div>
            <div class="box"><div class="label">Relay 6 Load Power W</div><input id="r6Input" name="r6" type="number" min="0" max="4000" value="0"></div>
          </div>
          <br>
          <button type="submit">Save Settings</button>
        </form>
      </div>

      <div class="card">
        <h2>Relay Source Status</h2>
        <div class="tableWrap">
          <table>
            <thead><tr><th>Relay</th><th>Configured Load</th><th>Active Source</th></tr></thead>
            <tbody id="relayTable"></tbody>
          </table>
        </div>
      </div>
    </div>
  </div>

  <script>
    let formLoaded = false;

    function toggleTheme(){
      const isDark = document.getElementById('themeToggle').checked;
      document.body.classList.toggle('dark', isDark);
      localStorage.setItem('shemsTheme', isDark ? 'dark' : 'light');
    }

    function loadTheme(){
      const theme = localStorage.getItem('shemsTheme');
      const dark = theme === 'dark';
      document.getElementById('themeToggle').checked = dark;
      document.body.classList.toggle('dark', dark);
    }

    function setText(id,text){
      const el = document.getElementById(id);
      if(el) el.innerText = text;
    }

    function setInput(id,value){
      const el = document.getElementById(id);
      if(el) el.value = value;
    }

    function statusClass(text){
      return text === 'AVAILABLE' ? 'ok' : 'bad';
    }

    function refresh(){
      fetch('/status')
      .then(r => r.json())
      .then(d => {
        setText('voltage', Number(d.voltage).toFixed(1) + 'V');
        setText('current', Number(d.current).toFixed(2) + 'A');
        setText('power', Number(d.power).toFixed(0) + 'W');
        setText('energy', Number(d.energy).toFixed(3) + 'kWh');

        setText('effectiveLimit', d.effectiveLimit + 'W');
        setText('decision', d.decision);
        setText('nepa', d.nepa);
        setText('inverter', d.inverter);

        document.getElementById('nepa').className = 'value ' + statusClass(d.nepa);
        document.getElementById('inverter').className = 'value ' + statusClass(d.inverter);

        if(!formLoaded){
          setInput('invInput', d.inverterPower);
          setInput('sysInput', d.systemPower);
          setInput('marginInput', d.loadMargin);

          for(let i = 0; i < d.relays.length; i++){
            setInput('r' + (i + 1) + 'Input', d.relays[i].power);
          }

          formLoaded = true;
        }

        let rows = '';

        for(let i = 0; i < d.relays.length; i++){
          let r = d.relays[i];

          rows += '<tr>';
          rows += '<td data-label="Relay">Relay ' + r.id + '</td>';
          rows += '<td data-label="Configured Load">' + r.power + 'W</td>';
          rows += '<td data-label="Active Source"><span class="sourceBadge">' + r.source + '</span></td>';
          rows += '</tr>';
        }

        document.getElementById('relayTable').innerHTML = rows;
      });
    }

    loadTheme();
    refresh();
    setInterval(refresh, 2000);
  </script>
</body>
</html>
)rawliteral";

  String sourceText(bool state)
  {
    return state ? "AVAILABLE" : "NOT AVAILABLE";
  }

  String relaySourceText(bool onInverter, bool enabled)
  {
    if (!enabled)
      return "OFF";

    return onInverter ? "INVERTER" : "NEPA";
  }

  String statusJson()
  {
    String data = "{";

    data += "\"voltage\":";
    data += String(pzem_sensor::getVoltage(), 1);
    data += ",";

    data += "\"current\":";
    data += String(pzem_sensor::getCurrent(), 2);
    data += ",";

    data += "\"power\":";
    data += String(pzem_sensor::getPower(), 0);
    data += ",";

    data += "\"energy\":";
    data += String(pzem_sensor::getEnergy(), 2);
    data += ",";

    data += "\"totalPower\":";
    data += String(pzem_sensor::getTotalPower());
    data += ",";

    data += "\"inverterPower\":";
    data += String(config_manager::getInverterPower());
    data += ",";

    data += "\"systemPower\":";
    data += String(config_manager::getSystemPower());
    data += ",";

    data += "\"effectiveLimit\":";
    data += String(load_manager::getEffectiveLimit());
    data += ",";

    data += "\"effectiveInverterLimit\":";
    data += String(load_manager::getEffectiveInverterLimit());
    data += ",";

    data += "\"effectiveSystemLimit\":";
    data += String(load_manager::getEffectiveSystemLimit());
    data += ",";

    data += "\"loadMargin\":";
    data += String(config_manager::getLoadMarginPercent());
    data += ",";

    data += "\"loadRatio\":";
    data += String(load_manager::getLoadRatio(), 2);
    data += ",";

    data += "\"fuzzyRisk\":";
    data += String(load_manager::getFuzzyRisk(), 2);
    data += ",";

    data += "\"nepa\":\"";
    data += sourceText(nepa_sense::isAvailable());
    data += "\",";

    data += "\"inverter\":\"";
    data += sourceText(inverter_sense::isAvailable());
    data += "\",";

    data += "\"decision\":\"";
    data += load_manager::getDecisionText();
    data += "\",";

    data += "\"relays\":[";

    for (int i = 0; i < 6; i++)
    {
      data += "{";
      data += "\"id\":";
      data += String(i + 1);
      data += ",";
      data += "\"power\":";
      data += String(config_manager::getRelayPower(i));
      data += ",";
      data += "\"enabled\":";
      data += load_manager::isLoadEnabled(i) ? "true" : "false";
      data += ",";
      data += "\"source\":\"";
      data += relaySourceText(load_manager::isOnInverter(i), load_manager::isLoadEnabled(i));
      data += "\"";
      data += "}";

      if (i < 5)
        data += ",";
    }

    data += "]";
    data += "}";

    return data;
  }

  void sendStatus()
  {
    server.send(200, "application/json", statusJson());
  }

  void sendHome()
  {
    server.send_P(200, "text/html", HOME_PAGE);
  }

  void handleSave()
  {
    if (server.hasArg("inv"))
      config_manager::setInverterPower(server.arg("inv").toInt());

    if (server.hasArg("sys"))
      config_manager::setSystemPower(server.arg("sys").toInt());

    if (server.hasArg("margin"))
      config_manager::setLoadMarginPercent(server.arg("margin").toInt());

    for (int i = 0; i < 6; i++)
    {
      String key = "r" + String(i + 1);

      if (server.hasArg(key))
        config_manager::setRelayPower(i, server.arg(key).toInt());
    }

    config_manager::save();

    server.sendHeader("Location", "/");
    server.send(303);
  }


  void handleResetEnergy()
  {
    pzem_sensor::resetEnergy();
    server.sendHeader("Location", "/");
    server.send(303);
  }

  void handleDefaults()
  {
    config_manager::resetDefaults();
    server.sendHeader("Location", "/");
    server.send(303);
  }

  void begin()
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid, apPassword);

    ipAddress = WiFi.softAPIP().toString();

    server.on("/", sendHome);
    server.on("/status", sendStatus);
    server.on("/api/status", sendStatus);
    server.on("/save", handleSave);
    server.on("/resetEnergy", handleResetEnergy);
    server.on("/defaults", handleDefaults);

    server.begin();
  }

  void update()
  {
    server.handleClient();
  }

  const char *getIpAddress()
  {
    return ipAddress.c_str();
  }
}
