#include <Arduino.h>
#include <WebServer.h>
#include <FS.h>
#include "local_server.h"
#include "attendance_manager/attendance_manager.h"
#include "sd_card/sd_card.h"
#include "wifi_manager/wifi_manager.h"
#include "battery_level/battery_level.h"
#include "error_handling/error_handling.h"

namespace local_server
{
  static WebServer server(80);
  static bool ready = false;

  static String htmlEscape(String value)
  {
    value.replace("&", "&amp;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");
    value.replace("\"", "&quot;");
    value.replace("'", "&#39;");
    return value;
  }

  static String jsonEscape(String value)
  {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    value.replace("\n", "\\n");
    value.replace("\r", "\\r");
    value.replace("\t", "\\t");
    return value;
  }

  static String enrollmentStateText()
  {
    switch (attendance_manager::getEnrollmentState())
    {
    case attendance_manager::ENROLL_WAIT_RFID:
      return "WAIT_RFID";
    case attendance_manager::ENROLL_WAIT_FINGER:
      return "WAIT_FINGER";
    case attendance_manager::ENROLL_SAVING:
      return "SAVING";
    default:
      return "IDLE";
    }
  }

  static String storageBadgeClass()
  {
    if (sd_card::isSdCardReady())
      return "ok";

    if (sd_card::isInternalReady())
      return "warn";

    return "bad";
  }

  static String pageHead(const String &title)
  {
    String html;
    html.reserve(12000);
    html += "<!doctype html><html><head>";
    html += "<meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1,maximum-scale=1'>";
    html += "<meta name='theme-color' content='#090d1f'>";
    html += "<title>" + htmlEscape(title) + "</title>";
    html += "<style>";
    html += ":root{--bg:#090d1f;--panel:#111832;--panel2:#172044;--card:#121a38;--line:#263152;--text:#eef4ff;--muted:#9da9c7;--blue:#4f8cff;--cyan:#22d3ee;--green:#22c55e;--red:#ef4444;--amber:#f59e0b}";
    html += "*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;min-height:100vh;font-family:Arial,Helvetica,sans-serif;background:radial-gradient(circle at top,#18295a 0,#090d1f 46%,#050816 100%);color:var(--text);text-align:center}";
    html += ".wrap{width:min(1040px,94vw);margin:0 auto;padding:24px 0 42px}.top{display:flex;align-items:center;justify-content:center;gap:12px;flex-wrap:wrap;margin-bottom:16px}";
    html += ".mark{width:62px;height:62px;display:grid;place-items:center;border-radius:22px;background:linear-gradient(135deg,#24386f,#0d132c);border:1px solid #33416b;box-shadow:0 14px 35px #0007}";
    html += ".mark svg{width:38px;height:38px}.title{margin:0;font-size:clamp(24px,5vw,38px);letter-spacing:.4px}.sub{margin:6px auto 0;color:var(--muted);max-width:720px;line-height:1.45}";
    html += ".nav{display:flex;justify-content:center;gap:10px;flex-wrap:wrap;margin:18px 0}.nav a,.pill{color:var(--text);text-decoration:none;border:1px solid var(--line);background:#101735cc;padding:10px 14px;border-radius:999px;font-size:14px}";
    html += ".nav a:hover{border-color:var(--cyan)}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(215px,1fr));gap:14px;margin:18px 0}.card{background:linear-gradient(180deg,var(--card),#0d1430);border:1px solid var(--line);border-radius:20px;padding:18px;margin:14px 0;box-shadow:0 18px 42px #0005;text-align:center}";
    html += ".mini{min-height:124px;display:flex;flex-direction:column;align-items:center;justify-content:center}.label{color:var(--muted);font-size:13px;margin-bottom:8px}.value{font-size:23px;font-weight:800;word-break:break-word}.muted{color:var(--muted);font-size:13px;line-height:1.45}.ok{color:var(--green)}.warn{color:var(--amber)}.bad{color:var(--red)}";
    html += "form{width:min(560px,100%);margin:0 auto;text-align:left}label{display:block;margin:11px 0 6px;color:#c9d4ee;font-size:14px}input,select,button{width:100%;border-radius:14px;border:1px solid #314066;background:#0b1025;color:var(--text);padding:13px 14px;font-size:15px;outline:none}";
    html += "input:focus,select:focus{border-color:var(--cyan);box-shadow:0 0 0 3px #22d3ee22}button{margin-top:12px;border:0;background:linear-gradient(135deg,var(--blue),var(--cyan));color:white;font-weight:800;cursor:pointer}button:hover{filter:brightness(1.08)}button.danger{background:linear-gradient(135deg,#b91c1c,var(--red))}";
    html += ".split{display:grid;grid-template-columns:repeat(auto-fit,minmax(290px,1fr));gap:14px;align-items:start}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px}.toast{min-height:24px;margin-top:10px;color:var(--cyan);font-weight:700;text-align:center}.tablebox,pre{white-space:pre-wrap;text-align:left;background:#080d20;border:1px solid var(--line);border-radius:16px;padding:14px;overflow:auto;font-size:12px;line-height:1.45}";
    html += ".download{display:flex;justify-content:center;gap:10px;flex-wrap:wrap}.download a{color:var(--text);text-decoration:none;background:#182347;border:1px solid var(--line);padding:11px 14px;border-radius:13px}.footer{margin-top:20px;color:var(--muted);font-size:12px}";
    html += "@media(max-width:520px){.wrap{width:94vw;padding-top:18px}.card{padding:15px;border-radius:16px}.actions{grid-template-columns:1fr}.nav a{width:calc(50% - 8px)}}";
    html += "</style></head><body><main class='wrap'>";
    html += "<div class='top'><div class='mark' aria-label='fingerprint rfid icon'>";
    html += "<svg viewBox='0 0 64 64' fill='none' xmlns='http://www.w3.org/2000/svg'><path d='M32 8c12.2 0 22 9.8 22 22' stroke='#22d3ee' stroke-width='5' stroke-linecap='round'/><path d='M10 30c0-12.2 9.8-22 22-22' stroke='#4f8cff' stroke-width='5' stroke-linecap='round'/><path d='M20 55c5-6 8-13 8-22 0-2.4 1.8-4.3 4-4.3s4 1.9 4 4.3c0 8.5-2.1 16.3-6.3 23.2' stroke='#eef4ff' stroke-width='5' stroke-linecap='round'/><path d='M42 53c2.7-6.4 4-13 4-20 0-8.3-6.3-15-14-15s-14 6.7-14 15c0 5.5-1.6 10.3-4.8 14.4' stroke='#22c55e' stroke-width='5' stroke-linecap='round'/></svg>";
    html += "</div><div><h1 class='title'>" + htmlEscape(title) + "</h1><p class='sub'></p></div></div>";
    html += "<nav class='nav'><a href='/'>Home</a><a href='/users'>Users</a><a href='/attendance'>Attendance</a><a href='/config'>Config</a><a href='/api/status'>Status API</a></nav>";
    return html;
  }

  static String pageEnd()
  {
    return "<div class='footer'>Lead City University, Ibadan</div></main></body></html>";
  }

  static String optionTag(const String &value, const String &label, const String &selected)
  {
    String html = "<option value='" + htmlEscape(value) + "'";

    if (value == selected)
      html += " selected";

    html += ">" + htmlEscape(label) + "</option>";
    return html;
  }

  static void sendNoCache()
  {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    server.sendHeader("Pragma", "no-cache");
  }

  static void sendActionJson(bool ok, const String &message)
  {
    String json = "{";
    json += "\"ok\":";
    json += (ok ? "true" : "false");
    json += ",";
    json += "\"message\":\"" + jsonEscape(message) + "\"";
    json += "}";

    sendNoCache();
    server.send(ok ? 200 : 409, "application/json", json);
  }

  static void handleRoot()
  {
    String systemName = sd_card::getConfigValue("system_name", "Fingerprint RFID Attendance");
    String workspaceName = sd_card::getConfigValue("workspace_name", "Main");

    String html = pageHead("Attendance Dashboard");

    html += "<section class='grid'>";
    html += "<div class='card mini'><div class='label'>System</div><div class='value' id='systemName'>" + htmlEscape(systemName) + "</div><div class='muted' id='workspaceName'>" + htmlEscape(workspaceName) + "</div></div>";
    html += "<div class='card mini'><div class='label'>Mode</div><div class='value' id='mode'>" + htmlEscape(attendance_manager::getModeText()) + "</div><div class='muted'></div></div>";
    html += "<div class='card mini'><div class='label'>Storage</div><div class='value " + storageBadgeClass() + "' id='storage'>" + htmlEscape(sd_card::getStorageName()) + "</div><div class='muted'></div></div>";
    html += "<div class='card mini'><div class='label'>Users / Records</div><div class='value'><span id='users'>" + String(attendance_manager::getUserCount()) + "</span> / <span id='records'>" + String(attendance_manager::getAttendanceCount()) + "</span></div><div class='muted'>Registered users / logs</div></div>";
    html += "</section>";

    html += "<section class='card'><div class='label'>Last Event</div><div class='value' id='lastMessage'>" + htmlEscape(attendance_manager::getLastMessage()) + "</div><p class='muted'><span id='enrollment'>Enrollment: " + enrollmentStateText() + "</span> &nbsp; Battery: <span id='battery'>" + String(battery_level::getPercentage()) + "%</span></p></section>";

    html += "<section class='split'>";
    html += "<div class='card'><h3>Register User</h3><form id='enrollForm' action='/enroll' method='get'>";
    html += "<label>User ID</label><input name='id' placeholder='e.g. STU001' autocomplete='off' required>";
    html += "<label>Full Name</label><input name='name' placeholder='Full name' autocomplete='name' required>";
    html += "<label>Class / Department / Workspace</label><input name='workspace' placeholder='Main' value='Main' autocomplete='off'>";
    html += "<button type='submit'>Start Registration</button><div class='toast' id='enrollToast'></div></form>";
    html += "<p class='muted'></p></div>";

    html += "<div class='card'><h3>Attendance Mode</h3><div class='actions'>";
    html += "<form class='modeForm' data-mode='IN'><button type='submit'>Clock In</button></form>";
    html += "<form class='modeForm' data-mode='OUT'><button type='submit'>Clock Out</button></form>";
    html += "</div><div class='toast' id='modeToast'></div>";
    html += "<hr style='border:0;border-top:1px solid #263152;margin:18px 0'>";
    html += "<h3>Remove User</h3><form id='deleteForm' action='/delete-user' method='get'><label>User ID</label><input name='id' placeholder='e.g. STU001' autocomplete='off' required><button class='danger' type='submit'>Delete User</button><div class='toast' id='deleteToast'></div></form></div>";
    html += "</section>";

    html += "<section class='card'><h3>Downloads</h3><div class='download'><a href='/download?file=users'>Users CSV</a><a href='/download?file=attendance'>Attendance CSV</a><a href='/download?file=config'>Config</a></div></section>";

    html += "<script>";
    html += "const $=id=>document.getElementById(id);";
    html += "const put=(id,v)=>{const e=$(id);if(e)e.textContent=v;};";
    html += "async function api(path,params){const r=await fetch(path+(params?'?'+params:''),{cache:'no-store'});return await r.json();}";
    html += "function cls(el,state){if(!el)return;el.classList.remove('ok','warn','bad');el.classList.add(state);}";
    html += "async function refreshStatus(){try{const s=await api('/api/status','');put('systemName',s.system_name);put('workspaceName',s.workspace_name);put('mode',s.mode);put('users',s.users);put('records',s.attendance_records);put('storage',s.storage);cls($('storage'),s.storage_class);put('lastMessage',s.last_message);put('battery',s.battery_percent+'%');put('enrollment','Enrollment: '+s.enrollment_state);}catch(e){}}";
    html += "setInterval(refreshStatus,2500);refreshStatus();";
    html += "$('enrollForm').addEventListener('submit',async e=>{e.preventDefault();const p=new URLSearchParams(new FormData(e.target));$('enrollToast').textContent='Starting registration...';try{const r=await api('/api/enroll',p.toString());$('enrollToast').textContent=r.message;refreshStatus();}catch(x){$('enrollToast').textContent='Could not contact device';}});";
    html += "document.querySelectorAll('.modeForm').forEach(f=>f.addEventListener('submit',async e=>{e.preventDefault();const p=new URLSearchParams();p.set('mode',f.dataset.mode);$('modeToast').textContent='Updating mode...';try{const r=await api('/api/set-mode',p.toString());$('modeToast').textContent=r.message;refreshStatus();}catch(x){$('modeToast').textContent='Could not contact device';}}));";
    html += "$('deleteForm').addEventListener('submit',async e=>{e.preventDefault();const p=new URLSearchParams(new FormData(e.target));$('deleteToast').textContent='Deleting user...';try{const r=await api('/api/delete-user',p.toString());$('deleteToast').textContent=r.message;refreshStatus();}catch(x){$('deleteToast').textContent='Could not contact device';}});";
    html += "</script>";

    html += pageEnd();
    sendNoCache();
    server.send(200, "text/html", html);
  }

  static void handleSetMode()
  {
    String mode = server.arg("mode");
    mode.toUpperCase();

    if (mode == "OUT")
      attendance_manager::setMode(attendance_manager::MODE_OUT);
    else
      attendance_manager::setMode(attendance_manager::MODE_IN);

    server.sendHeader("Location", "/");
    server.send(303, "text/plain", "");
  }

  static void handleSetModeApi()
  {
    String mode = server.arg("mode");
    mode.toUpperCase();

    if (mode == "OUT")
      attendance_manager::setMode(attendance_manager::MODE_OUT);
    else
      attendance_manager::setMode(attendance_manager::MODE_IN);

    sendActionJson(true, "Mode set to " + attendance_manager::getModeText());
  }

  static void handleEnroll()
  {
    String userId = server.arg("id");
    String name = server.arg("name");
    String workspace = server.arg("workspace");

    bool ok = attendance_manager::startEnrollment(userId, name, workspace);

    String html = pageHead("Registration");
    html += "<div class='card'>";

    if (ok)
      html += "<h3>Registration started</h3><p>Tap RFID card, then place finger when requested.</p>";
    else
      html += "<h3>Registration could not start</h3><p>" + htmlEscape(attendance_manager::getLastMessage()) + "</p>";

    html += "<a class='pill' href='/'>Back to Dashboard</a></div>";
    html += pageEnd();

    sendNoCache();
    server.send(ok ? 200 : 409, "text/html", html);
  }

  static void handleEnrollApi()
  {
    String userId = server.arg("id");
    String name = server.arg("name");
    String workspace = server.arg("workspace");

    bool ok = attendance_manager::startEnrollment(userId, name, workspace);
    String message = ok ? "Registration started. Tap RFID card, then scan finger on the device." : attendance_manager::getLastMessage();
    sendActionJson(ok, message);
  }

  static void handleUsers()
  {
    String html = pageHead("Users");

    if (!sd_card::isReady())
      html += "<div class='card'>Storage not ready.</div>";
    else
      html += "<div class='card'><h3>Registered Users</h3><pre>" + htmlEscape(sd_card::readFile("/users.csv", 22000)) + "</pre></div>";

    html += "<div class='card'><h3>Add User</h3><form id='usersEnrollForm' action='/enroll' method='get'>";
    html += "<label>User ID</label><input name='id' placeholder='User ID e.g. STU001' autocomplete='off' required>";
    html += "<label>Full Name</label><input name='name' placeholder='Full name' autocomplete='name' required>";
    html += "<label>Class / Department / Workspace</label><input name='workspace' placeholder='Main' value='Main' autocomplete='off'>";
    html += "<button type='submit'>Start Registration</button><div class='toast' id='usersEnrollToast'></div>";
    html += "</form></div>";

    html += "<div class='card'><h3>Remove User</h3><form id='usersDeleteForm' action='/delete-user' method='get'>";
    html += "<label>User ID</label><input name='id' placeholder='User ID e.g. STU001' autocomplete='off' required>";
    html += "<button class='danger' type='submit'>Delete User</button><div class='toast' id='usersDeleteToast'></div>";
    html += "</form><p class='muted'>This removes the user, RFID card link, and fingerprint template.</p></div>";

    html += "<script>const $=id=>document.getElementById(id);async function api(path,params){const r=await fetch(path+(params?'?'+params:''),{cache:'no-store'});return await r.json();}";
    html += "$('usersEnrollForm').addEventListener('submit',async e=>{e.preventDefault();const p=new URLSearchParams(new FormData(e.target));$('usersEnrollToast').textContent='Starting registration...';try{const r=await api('/api/enroll',p.toString());$('usersEnrollToast').textContent=r.message;}catch(x){$('usersEnrollToast').textContent='Could not contact device';}});";
    html += "$('usersDeleteForm').addEventListener('submit',async e=>{e.preventDefault();const p=new URLSearchParams(new FormData(e.target));$('usersDeleteToast').textContent='Deleting user...';try{const r=await api('/api/delete-user',p.toString());$('usersDeleteToast').textContent=r.message;}catch(x){$('usersDeleteToast').textContent='Could not contact device';}});</script>";

    html += pageEnd();
    sendNoCache();
    server.send(200, "text/html", html);
  }

  static void handleDeleteUser()
  {
    String userId = server.arg("id");
    userId.trim();

    bool ok = attendance_manager::deleteUser(userId);

    String html = pageHead("Delete User");
    html += "<div class='card'>";

    if (ok)
      html += "<h3>User deleted successfully.</h3>";
    else
      html += "<h3>Delete failed</h3><p>" + htmlEscape(attendance_manager::getLastMessage()) + "</p>";

    html += "<a class='pill' href='/users'>Back to Users</a></div>";
    html += pageEnd();

    sendNoCache();
    server.send(ok ? 200 : 409, "text/html", html);
  }

  static void handleDeleteUserApi()
  {
    String userId = server.arg("id");
    userId.trim();

    bool ok = attendance_manager::deleteUser(userId);
    String message = ok ? "User deleted successfully." : attendance_manager::getLastMessage();
    sendActionJson(ok, message);
  }

  static void handleAttendance()
  {
    String html = pageHead("Attendance Log");

    if (!sd_card::isReady())
      html += "<div class='card'>Storage not ready.</div>";
    else
      html += "<div class='card'><h3>Attendance Records</h3><pre>" + htmlEscape(sd_card::readFile("/attendance.csv", 30000)) + "</pre></div>";

    html += "<div class='card'><form action='/clear-attendance' method='get'>";
    html += "<input type='hidden' name='confirm' value='yes'>";
    html += "<button class='danger' type='submit'>Clear Attendance Log</button>";
    html += "</form></div>";

    html += pageEnd();
    sendNoCache();
    server.send(200, "text/html", html);
  }

  static void handleConfig()
  {
    String systemName = sd_card::getConfigValue("system_name", "Fingerprint RFID Attendance System");
    String workspaceName = sd_card::getConfigValue("workspace_name", "Prototype Workspace");
    String workspaceType = sd_card::getConfigValue("workspace_type", "school");
    String apSsid = sd_card::getConfigValue("ap_ssid", "AttendanceSystem");
    String apPassword = sd_card::getConfigValue("ap_password", "12345678");
    String defaultMode = sd_card::getConfigValue("default_mode", "IN");
    defaultMode.toUpperCase();

    String html = pageHead("System Configuration");
    html += "<div class='card'><form action='/save-config' method='get'>";
    html += "<label>System Name</label><input name='system_name' value='" + htmlEscape(systemName) + "'>";
    html += "<label>Workspace / School Name</label><input name='workspace_name' value='" + htmlEscape(workspaceName) + "'>";
    html += "<label>System Type</label><select name='workspace_type'>";
    html += optionTag("school", "School", workspaceType);
    html += optionTag("workspace", "Workspace", workspaceType);
    html += "</select>";
    html += "<label>Default Attendance Mode</label><select name='default_mode'>";
    html += optionTag("IN", "Clock In", defaultMode);
    html += optionTag("OUT", "Clock Out", defaultMode);
    html += "</select>";
    html += "<label>Access Point SSID</label><input name='ap_ssid' value='" + htmlEscape(apSsid) + "'>";
    html += "<label>Access Point Password</label><input name='ap_password' value='" + htmlEscape(apPassword) + "'>";
    html += "<button type='submit'>Save Configuration</button>";
    html += "</form><p class='muted'>If Wi-Fi name or password is changed, restart the device after saving.</p></div>";
    html += pageEnd();

    sendNoCache();
    server.send(200, "text/html", html);
  }

  static void handleSaveConfig()
  {
    bool ok = sd_card::saveConfig(server.arg("system_name"),
                                  server.arg("workspace_name"),
                                  server.arg("workspace_type"),
                                  server.arg("ap_ssid"),
                                  server.arg("ap_password"),
                                  server.arg("default_mode"));

    String html = pageHead("Save Configuration");
    html += "<div class='card'>";
    html += ok ? "<h3>Configuration saved.</h3>" : "<h3>Configuration save failed.</h3>";
    html += "<p class='muted'>Restart the device if AP settings were changed.</p>";
    html += "<a class='pill' href='/config'>Back to Config</a></div>";
    html += pageEnd();

    sendNoCache();
    server.send(ok ? 200 : 500, "text/html", html);
  }

  static void handleDownload()
  {
    String requested = server.arg("file");
    String path;
    String name;
    String type;

    if (requested == "users")
    {
      path = "/users.csv";
      name = "users.csv";
      type = "text/csv";
    }
    else if (requested == "attendance")
    {
      path = "/attendance.csv";
      name = "attendance.csv";
      type = "text/csv";
    }
    else if (requested == "config")
    {
      path = "/config.txt";
      name = "config.txt";
      type = "text/plain";
    }
    else
    {
      server.send(400, "text/plain", "Invalid file request");
      return;
    }

    if (!sd_card::isReady() || !sd_card::exists(path))
    {
      server.send(404, "text/plain", "File not found");
      return;
    }

    File file = sd_card::openRead(path);

    if (!file)
    {
      server.send(500, "text/plain", "Could not open file");
      return;
    }

    server.sendHeader("Content-Disposition", String("attachment; filename=") + name);
    server.streamFile(file, type);
    file.close();
  }

  static void handleClearAttendance()
  {
    if (server.arg("confirm") == "yes")
    {
      sd_card::clearAttendance();
      attendance_manager::refreshCounts();
    }

    server.sendHeader("Location", "/attendance");
    server.send(303, "text/plain", "");
  }

  static void handleStatusApi()
  {
    String systemName = sd_card::getConfigValue("system_name", "Fingerprint RFID Attendance");
    String workspaceName = sd_card::getConfigValue("workspace_name", "Main");

    String json = "{";
    json += "\"system_name\":\"" + jsonEscape(systemName) + "\",";
    json += "\"workspace_name\":\"" + jsonEscape(workspaceName) + "\",";
    json += "\"ip\":\"" + jsonEscape(wifi_manager::getIpString()) + "\",";
    json += "\"mode\":\"" + jsonEscape(attendance_manager::getModeText()) + "\",";
    json += "\"users\":" + String(attendance_manager::getUserCount()) + ",";
    json += "\"attendance_records\":" + String(attendance_manager::getAttendanceCount()) + ",";
    json += "\"storage_ready\":";
    json += (sd_card::isReady() ? "true" : "false");
    json += ",";
    json += "\"storage\":\"" + jsonEscape(sd_card::getStorageName()) + "\",";
    json += "\"storage_class\":\"" + storageBadgeClass() + "\",";
    json += "\"enrollment_busy\":";
    json += (attendance_manager::isEnrollmentBusy() ? "true" : "false");
    json += ",";
    json += "\"enrollment_state\":\"" + jsonEscape(enrollmentStateText()) + "\",";
    json += "\"battery_percent\":" + String(battery_level::getPercentage()) + ",";
    json += "\"battery_voltage\":" + String(battery_level::getVoltage(), 2) + ",";
    json += "\"error\":\"" + jsonEscape(error_handling::getLastError()) + "\",";
    json += "\"last_message\":\"" + jsonEscape(attendance_manager::getLastMessage()) + "\"";
    json += "}";

    sendNoCache();
    server.send(200, "application/json", json);
  }

  static void handleNotFound()
  {
    server.send(404, "text/plain", "Not found");
  }

  void begin()
  {
    server.on("/", handleRoot);
    server.on("/set-mode", handleSetMode);
    server.on("/api/set-mode", handleSetModeApi);
    server.on("/enroll", handleEnroll);
    server.on("/api/enroll", handleEnrollApi);
    server.on("/users", handleUsers);
    server.on("/delete-user", handleDeleteUser);
    server.on("/api/delete-user", handleDeleteUserApi);
    server.on("/attendance", handleAttendance);
    server.on("/config", handleConfig);
    server.on("/save-config", handleSaveConfig);
    server.on("/download", handleDownload);
    server.on("/clear-attendance", handleClearAttendance);
    server.on("/api/status", handleStatusApi);
    server.onNotFound(handleNotFound);

    server.begin();
    ready = true;
  }

  void update()
  {
    if (ready)
      server.handleClient();
  }

  bool isReady()
  {
    return ready;
  }
}
