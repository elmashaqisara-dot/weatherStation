/*
  Smart Home — self-hosted, offline ESP32 access point.
  Board: ESP32 Dev Module / Arduino ESP32 core
  Required external library: DHT11 by Dhruba Saha, version 2.1.0.

  Wiring (all signals must be 3.3 V-safe at ESP32 inputs):
    DHT11 DATA          GPIO 4
    Flame detector DO   GPIO 23 (active LOW)
    Living-room LED     GPIO 18 (use a series resistor)
    Bedroom LED         GPIO 17 (use a series resistor)
    Relay control IN    GPIO 2 (strapping pin; see safety notes below)

  IMPORTANT SAFETY NOTES:
  * GPIO 2 is a boot strapping pin. A relay input/pull resistor can prevent
    boot, or a relay can pulse before the sketch runs. Output initialization
    cannot guarantee glitch-free operation during power-up or reset.
    If hardware permits changing the wiring later, GPIO 25 or GPIO 26 is
    generally a better relay output, but this sketch intentionally uses 2.
  * Verify relay logic level and OFF polarity WITHOUT a mains/high-energy load.
    For an active-HIGH relay, change RELAY_ACTIVE_LOW to false.
  * Use a correctly rated, isolated relay driver; never power a relay coil
    directly from an ESP32 GPIO. Do not connect 5 V signals to ESP32 GPIOs.
  * A hobby flame module is NOT a certified fire alarm or life-safety device.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DHT11.h>
#include <math.h>

static const uint8_t DHT_PIN = 4;
static const uint8_t FLAME_PIN = 23;
static const uint8_t LIGHT1_PIN = 18;
static const uint8_t LIGHT2_PIN = 17;
static const uint8_t RELAY_PIN = 2;
static const bool RELAY_ACTIVE_LOW = true;

static const char AP_SSID[] = "Smart_Home";
static const char AP_PASSWORD[] = "12345678";
static const unsigned long DHT_INTERVAL_MS = 2000UL;

DHT11 dht(DHT_PIN);
WebServer server(80);

bool light1On = false;
bool light2On = false;
bool relayOn = false;
float lastTemperature = NAN;
float lastHumidity = NAN;
unsigned long lastDhtSampleMs = 0;
bool hasDhtSample = false;

static uint8_t relayLevel(bool turnOn) {
  return (turnOn == RELAY_ACTIVE_LOW) ? LOW : HIGH;
}

void setRelay(bool turnOn) {
  digitalWrite(RELAY_PIN, relayLevel(turnOn));
  relayOn = turnOn;
}

void setLight1(bool turnOn) {
  digitalWrite(LIGHT1_PIN, turnOn ? HIGH : LOW);
  light1On = turnOn;
}

void setLight2(bool turnOn) {
  digitalWrite(LIGHT2_PIN, turnOn ? HIGH : LOW);
  light2On = turnOn;
}

void sampleDhtIfDue() {
  const unsigned long now = millis();
  if (hasDhtSample && (unsigned long)(now - lastDhtSampleMs) < DHT_INTERVAL_MS) return;
  hasDhtSample = true;
  lastDhtSampleMs = now;
  int t = 0;
  int h = 0;
  // The Dhruba Saha library reads both metrics in a single transaction.
  // A nonzero result indicates a timeout/checksum error.
  const int result = dht.readTemperatureHumidity(t, h);
  if (result == 0 && t >= -20 && t <= 80 && h >= 0 && h <= 100) {
    lastTemperature = static_cast<float>(t);
    lastHumidity = static_cast<float>(h);
  } else {
    // Do not display outdated values when communication fails.
    lastTemperature = NAN;
    lastHumidity = NAN;
  }
}

static void sendJson(int code, const String& json) {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.send(code, "application/json; charset=utf-8", json);
}

static void sendError(int code, const char* description) {
  String payload;
  payload.reserve(100);
  payload = "{\"success\":false,\"error\":\"";
  payload += description;
  payload += "\"}";
  sendJson(code, payload);
}

// Embedded HTML/CSS/JavaScript: no remote resources of any kind.
static const char PAGE_HTML[] PROGMEM = R"SMART_HOME_HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0B1120">
<title>Smart Home | Welcome Home</title>
<style>
:root{color-scheme:dark;--bg:#0B1120;--card:#151E30;--ink:#F8FAFC;--muted:#94A3B8;--line:rgba(194,209,240,.11);--violet:#6366F1;--green:#22C55E;--red:#EF4444;--shadow:0 20px 55px rgba(0,0,0,.23)}
*{box-sizing:border-box}html{scroll-behavior:smooth}body{margin:0;min-height:100vh;background:radial-gradient(ellipse 65% 40% at 92% -12%,rgba(99,102,241,.24),transparent 80%),radial-gradient(ellipse 50% 40% at -15% 60%,rgba(31,77,122,.17),transparent 80%),var(--bg);color:var(--ink);font-family:system-ui,-apple-system,"Segoe UI",Roboto,Arial,sans-serif;-webkit-font-smoothing:antialiased}
.app{width:min(1100px,calc(100% - 32px));margin:0 auto;padding:28px 0 64px}header{display:flex;justify-content:space-between;align-items:center;gap:14px;margin-bottom:36px}.brand{display:flex;align-items:center;gap:12px;font-weight:800;font-size:1.05rem;letter-spacing:.02em}.brand-mark{width:43px;height:43px;display:grid;place-items:center;border-radius:14px;background:linear-gradient(145deg,#818CF8,#4F46E5);box-shadow:0 8px 25px rgba(99,102,241,.3)}.brand-mark svg{width:24px;height:24px}.connection{display:inline-flex;align-items:center;gap:8px;padding:10px 14px;border:1px solid rgba(239,68,68,.23);background:rgba(239,68,68,.08);border-radius:100px;font-size:.81rem;color:#FCA5A5;font-weight:700;white-space:nowrap}.connection.online{color:#86EFAC;border-color:rgba(34,197,94,.25);background:rgba(34,197,94,.08)}.dot{width:8px;height:8px;background:currentColor;border-radius:50%;box-shadow:0 0 12px currentColor}
.intro{margin-bottom:30px}.eyebrow{display:inline-block;margin-bottom:9px;color:#A5B4FC;font-weight:700;letter-spacing:.14em;font-size:.74rem;text-transform:uppercase}.intro h1{font-size:clamp(2.2rem,6vw,3.9rem);line-height:1.1;letter-spacing:-.055em;margin:0 0 12px}.intro p{color:var(--muted);font-size:1rem;line-height:1.6;margin:0;max-width:580px}.section-header{display:flex;align-items:center;justify-content:space-between;margin:34px 0 15px}.section-header h2{font-size:1.1rem;letter-spacing:-.025em;margin:0}.section-header span{font-size:.77rem;color:var(--muted)}.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:16px}.card{position:relative;overflow:hidden;background:linear-gradient(135deg,rgba(255,255,255,.045),rgba(255,255,255,.012)),var(--card);border:1px solid var(--line);border-radius:23px;padding:23px;box-shadow:var(--shadow);backdrop-filter:blur(16px);min-width:0}.card-top{display:flex;justify-content:space-between;align-items:flex-start;gap:8px}.icon{width:47px;height:47px;border-radius:15px;display:grid;place-items:center;background:rgba(99,102,241,.14);color:#A5B4FC}.icon svg{width:24px;height:24px;stroke:currentColor;stroke-width:1.8;fill:none;stroke-linecap:round;stroke-linejoin:round}.icon.blue{background:rgba(56,189,248,.12);color:#7DD3FC}.icon.orange{background:rgba(251,146,60,.12);color:#FDBA74}.icon.green{background:rgba(34,197,94,.12);color:#86EFAC}.metric-label{margin:24px 0 6px;color:#BCC9DA;font-size:.9rem}.metric{font-size:2.6rem;line-height:1.15;font-weight:800;letter-spacing:-.06em}.unit{font-size:1.1rem;color:var(--muted);font-weight:600;letter-spacing:0}.caption{margin-top:11px;color:var(--muted);font-size:.81rem}.safe-badge{display:inline-flex;gap:8px;align-items:center;border-radius:100px;background:rgba(34,197,94,.12);color:#86EFAC;padding:9px 12px;font-weight:800;font-size:.85rem;margin-top:11px}.safe-badge::before{content:"";width:8px;height:8px;border-radius:50%;background:currentColor}.fire-card.danger{background:linear-gradient(135deg,rgba(239,68,68,.23),rgba(127,29,29,.16)),var(--card);border-color:rgba(239,68,68,.7);animation:alertPulse 1.25s infinite}.danger .icon{background:rgba(239,68,68,.22);color:#FCA5A5}.danger .safe-badge{background:rgba(239,68,68,.19);color:#FECACA}.danger .caption{color:#FECACA}@keyframes alertPulse{0%,100%{box-shadow:0 0 0 0 rgba(239,68,68,.16),var(--shadow)}50%{box-shadow:0 0 0 7px rgba(239,68,68,.09),var(--shadow)}}
.device-card{display:flex;flex-direction:column;justify-content:space-between;min-height:197px}.device-header{display:flex;justify-content:space-between;align-items:flex-start}.toggle{position:relative;display:inline-block;width:54px;height:31px;flex:none;cursor:pointer}.toggle input{position:absolute;opacity:0;width:100%;height:100%;margin:0;cursor:pointer}.track{display:block;width:54px;height:31px;background:#334155;border:1px solid rgba(255,255,255,.11);border-radius:100px;transition:background .23s,box-shadow .23s}.track:after{content:"";position:absolute;top:4px;left:4px;width:23px;height:23px;background:#E2E8F0;border-radius:100%;box-shadow:0 2px 5px rgba(0,0,0,.3);transition:transform .23s}.toggle input:checked+.track{background:var(--violet)}.toggle input:checked+.track:after{transform:translateX(23px);background:white}.toggle input:focus-visible+.track{outline:3px solid #C7D2FE;outline-offset:3px}.toggle input:disabled{cursor:not-allowed}.toggle:has(input:disabled){opacity:.55;cursor:wait}.device-title{margin:20px 0 5px;font-size:1.06rem;font-weight:750}.device-sub{color:var(--muted);font-size:.85rem}.state{font-weight:800;color:#94A3B8}.state.on{color:#86EFAC}.error-text{min-height:1.1em;margin:9px 0 0;color:#FCA5A5;font-size:.76rem}
footer{margin-top:32px;text-align:center;color:#65758E;font-size:.77rem}.offline-note{margin-top:13px;display:none;padding:12px 15px;border-radius:13px;background:rgba(239,68,68,.12);color:#FECACA;border:1px solid rgba(239,68,68,.2);font-size:.84rem}.offline-note.show{display:block}@media(max-width:780px){.grid{grid-template-columns:repeat(2,minmax(0,1fr))}.fire-card{grid-column:span 2}}@media(max-width:520px){.app{width:min(100% - 28px,450px);padding-top:19px}.grid{grid-template-columns:1fr;gap:13px}.fire-card{grid-column:auto}.card{padding:21px}.intro{margin-bottom:24px}header{margin-bottom:30px}.metric-label{margin-top:20px}.connection{padding:9px 11px}.brand{font-size:.95rem}}
@media(prefers-reduced-motion:reduce){*,*:before,*:after{animation:none!important;transition:none!important;scroll-behavior:auto!important}}
</style>
</head>
<body>
<main class="app">
<header><div class="brand"><div class="brand-mark"><svg viewBox="0 0 24 24" fill="none" stroke="white" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true"><path d="m3 10 9-7 9 7v10a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1Z"/><path d="M9 21v-8h6v8"/></svg></div>Smart Home</div><div class="connection" id="connection"><i class="dot"></i><span id="connectionText">Connecting...</span></div></header>
<div class="intro"><span class="eyebrow">Your space, your way</span><h1>Welcome Home<span style="color:#818CF8">.</span></h1><p>Comfort, lighting, and peace of mind — all in one place.</p><div class="offline-note" id="offlineNotice" role="status">Connection lost. Check your Wi-Fi and try again.</div></div>
<div class="section-header"><h2>Home Overview</h2><span>Live conditions</span></div>
<section class="grid" aria-label="Home conditions">
<article class="card"><div class="card-top"><div class="icon orange"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M14 14.8V5a3 3 0 0 0-6 0v9.8a5 5 0 1 0 6 0Z"/><path d="M11 11v7"/></svg></div></div><p class="metric-label">Temperature</p><div class="metric"><span id="temperature">--</span> <span class="unit">°C</span></div><div class="caption">Indoor temperature</div></article>
<article class="card"><div class="card-top"><div class="icon blue"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 2C9 6 5 10.4 5 15a7 7 0 0 0 14 0c0-4.6-4-9-7-13Z"/><path d="M9 17a3 3 0 0 0 3 3"/></svg></div></div><p class="metric-label">Humidity</p><div class="metric"><span id="humidity">--</span> <span class="unit">%</span></div><div class="caption">Indoor humidity</div></article>
<article class="card fire-card" id="fireCard"><div class="card-top"><div class="icon green"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 22c-4.5 0-8-3.2-8-7.4 0-2.6 1.4-5.1 4-7 .2 2.2 1.4 3.5 2.6 4.1C11 7.3 13.2 4.2 15 2c0 4.2 5 6.7 5 12.6 0 4.2-3.5 7.4-8 7.4Z"/><path d="M12 22c-2 0-3.5-1.5-3.5-3.5 0-1.7 1.2-3.2 3.5-5.5 2.3 2.3 3.5 3.8 3.5 5.5A3.5 3.5 0 0 1 12 22Z"/></svg></div></div><p class="metric-label">Fire Safety</p><div id="fireStatus" class="safe-badge" role="status" aria-live="assertive">Checking...</div><div id="fireCaption" class="caption">Monitoring your home</div></article>
</section>
<div class="section-header"><h2>Devices & Controls</h2><span>Your home, in control</span></div>
<section class="grid" aria-label="Devices">
<article class="card device-card"><div class="device-header"><div class="icon"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 18h6m-5 3h4M8 14c-1.3-1.2-2-2.6-2-4.5a6 6 0 0 1 12 0c0 1.9-.7 3.3-2 4.5-.6.6-1 1.4-1 2H9c0-.6-.4-1.4-1-2Z"/></svg></div><label class="toggle" aria-label="Living Room light"><input type="checkbox" id="light1" disabled><span class="track"></span></label></div><div><h3 class="device-title">Living Room</h3><div class="device-sub">Lighting · <span class="state" id="light1State">Off</span></div><p class="error-text" id="light1Error" role="status"></p></div></article>
<article class="card device-card"><div class="device-header"><div class="icon"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M9 18h6m-5 3h4M8 14c-1.3-1.2-2-2.6-2-4.5a6 6 0 0 1 12 0c0 1.9-.7 3.3-2 4.5-.6.6-1 1.4-1 2H9c0-.6-.4-1.4-1-2Z"/></svg></div><label class="toggle" aria-label="Bedroom light"><input type="checkbox" id="light2" disabled><span class="track"></span></label></div><div><h3 class="device-title">Bedroom</h3><div class="device-sub">Lighting · <span class="state" id="light2State">Off</span></div><p class="error-text" id="light2Error" role="status"></p></div></article>
<article class="card device-card"><div class="device-header"><div class="icon"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M12 2v10M6.2 6.2a8 8 0 1 0 11.6 0"/></svg></div><label class="toggle" aria-label="Power Control"><input type="checkbox" id="relay" disabled><span class="track"></span></label></div><div><h3 class="device-title">Power Control</h3><div class="device-sub">Power · <span class="state" id="relayState">Off</span></div><p class="error-text" id="relayError" role="status"></p></div></article>
</section>
<footer>Made for your home • Works without the internet</footer>
</main>
<script>
'use strict';
const names=['light1','light2','relay'];
const controls=Object.fromEntries(names.map(name=>[name,document.getElementById(name)]));
const pending=new Set();
let connected=false;
let lastStates={light1:false,light2:false,relay:false};
const el=id=>document.getElementById(id);
function showConnection(online){
  connected=online;
  el('connection').classList.toggle('online',online);
  el('connectionText').textContent=online?'Connected':'Disconnected';
  el('offlineNotice').classList.toggle('show',!online);
  for(const name of names) controls[name].disabled=!online||pending.has(name);
}
function drawDevice(name,state){
  lastStates[name]=Boolean(state);
  if(pending.has(name)) return;
  controls[name].checked=lastStates[name];
  const label=el(name+'State');
  label.textContent=lastStates[name]?'On':'Off';
  label.classList.toggle('on',lastStates[name]);
}
function updateFromStatus(data){
  el('temperature').textContent=typeof data.temperature==='number'&&Number.isFinite(data.temperature)?data.temperature.toFixed(1):'--';
  el('humidity').textContent=typeof data.humidity==='number'&&Number.isFinite(data.humidity)?data.humidity.toFixed(1):'--';
  const fire=data.flameDetected===true;
  el('fireCard').classList.toggle('danger',fire);
  el('fireStatus').textContent=fire?'FIRE DETECTED':'Safe';
  el('fireCaption').textContent=fire?'Warning: possible fire detected!':'No flame detected';
  for(const name of names) drawDevice(name,data[name]===true);
  showConnection(true);
}
async function timedFetch(url,options={}){
  const ctrl=new AbortController();
  const timeout=setTimeout(()=>ctrl.abort(),3500);
  try{return await fetch(url,{...options,signal:ctrl.signal,cache:'no-store'});}
  finally{clearTimeout(timeout);}
}
async function poll(){
  try{
    const response=await timedFetch('/api/status');
    if(!response.ok)throw Error('Status unavailable');
    const data=await response.json();
    if(typeof data!=='object'||data===null||typeof data.flameDetected!=='boolean')throw Error('Invalid status');
    updateFromStatus(data);
  }catch(_){showConnection(false);}
  finally{setTimeout(poll,2000);}
}
async function sendControl(name,desired){
  if(!connected||pending.has(name))return;
  pending.add(name);
  controls[name].disabled=true;
  controls[name].checked=lastStates[name]; // Do not optimistically show ON.
  el(name+'Error').textContent='';
  try{
    const response=await timedFetch('/api/'+name,{
      method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body:'state='+(desired?'on':'off')
    });
    const result=await response.json();
    if(!response.ok||result.success!==true||typeof result.state!=='boolean')throw Error('Could not update');
    lastStates[name]=result.state;
    showConnection(true);
  }catch(_){
    el(name+'Error').textContent='Could not update. Please try again.';
    showConnection(false);
  }finally{
    pending.delete(name);
    drawDevice(name,lastStates[name]);
    controls[name].disabled=!connected;
  }
}
for(const name of names){
  controls[name].addEventListener('change',event=>sendControl(name,event.target.checked));
}
poll();
</script>
</body>
</html>
)SMART_HOME_HTML";

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store, max-age=0");
  server.sendHeader("X-Content-Type-Options", "nosniff");
  server.send_P(200, "text/html; charset=utf-8", PAGE_HTML);
}

void handleStatus() {
  // DHT sampling is serviced by loop(), not by individual web requests.
  const bool flameDetected = (digitalRead(FLAME_PIN) == LOW);
  String json;
  json.reserve(160);
  json = "{\"temperature\":";
  json += isfinite(lastTemperature) ? String(lastTemperature, 1) : "null";
  json += ",\"humidity\":";
  json += isfinite(lastHumidity) ? String(lastHumidity, 1) : "null";
  json += ",\"flameDetected\":";
  json += flameDetected ? "true" : "false";
  json += ",\"light1\":";
  json += light1On ? "true" : "false";
  json += ",\"light2\":";
  json += light2On ? "true" : "false";
  json += ",\"relay\":";
  json += relayOn ? "true" : "false";
  json += '}';
  sendJson(200, json);
}

void handleControl(uint8_t device) {
  if (!server.hasArg("state") || server.args() != 1) {
    sendError(400, "Exactly one state parameter is required");
    return;
  }
  const String requested = server.arg("state");
  if (requested != "on" && requested != "off") {
    sendError(400, "state must be on or off");
    return;
  }
  const bool turnOn = (requested == "on");
  if (device == 1) setLight1(turnOn);
  else if (device == 2) setLight2(turnOn);
  else setRelay(turnOn);
  String response = "{\"success\":true,\"state\":";
  response += turnOn ? "true" : "false";
  response += '}';
  sendJson(200, response);
}

void setup() {
  Serial.begin(115200);

  // Prepare latch levels before changing each GPIO to output. NOTE: GPIO 2
  // still cannot be kept safe by software while the ESP32 is resetting.
  digitalWrite(LIGHT1_PIN, LOW);
  digitalWrite(LIGHT2_PIN, LOW);
  digitalWrite(RELAY_PIN, relayLevel(false));
  pinMode(LIGHT1_PIN, OUTPUT);
  pinMode(LIGHT2_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  setLight1(false);
  setLight2(false);
  setRelay(false);
  pinMode(FLAME_PIN, INPUT); // Flame module must provide a driven 3.3 V-safe DO.
  // DHT11 by Dhruba Saha needs no begin() call.

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAPConfig(IPAddress(192,168,4,1),
                         IPAddress(192,168,4,1),
                         IPAddress(255,255,255,0))) {
    Serial.println("WARNING: Access Point IP configuration failed.");
  }
  const bool apStarted = WiFi.softAP(AP_SSID, AP_PASSWORD);
  if (apStarted) {
    Serial.println("Smart Home access point started.");
    Serial.print("Wi-Fi network: "); Serial.println(AP_SSID);
    Serial.print("Local dashboard: http://"); Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("ERROR: Could not start Wi-Fi access point.");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/light1", HTTP_POST, [](){ handleControl(1); });
  server.on("/api/light2", HTTP_POST, [](){ handleControl(2); });
  server.on("/api/relay", HTTP_POST, [](){ handleControl(3); });
  server.onNotFound([](){ sendError(404, "Not found"); });
  server.begin();
  Serial.println("Local web server ready.");
}

void loop() {
  server.handleClient();
  sampleDhtIfDue();
}
