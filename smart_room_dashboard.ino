/*
  Smart Room Dashboard
  Control a fan and lights from your phone using an ESP32 and a relay module.
  The dashboard is hosted on the ESP32 itself (works on the same WiFi network).

  Wiring (active-LOW relay module):
    IN1 -> GPIO 13  (Fan)
    IN2 -> GPIO 14  (Main Light)
    IN3 -> GPIO 27  (Mandir Light)
    VCC -> 5V/VIN, GND -> GND
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ====== Enter your details here ======
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

// Static IP (change to match your router)
IPAddress local_IP(192, 168, 1, 200);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(8, 8, 8, 8);
// =====================================

const int  PINS[3]   = {13, 14, 27};
const char* NAMES[3] = {"fan", "main", "mandir"};

// If the relays behave in reverse, swap these two values
const int RELAY_ON  = LOW;
const int RELAY_OFF = HIGH;

bool st[3] = {false, false, false};
WebServer server(80);

// Scenes: {fan, main, mandir}
struct Mode { const char* id; bool s[3]; };
const Mode MODES[] = {
  {"night",   {true,  false, false}},
  {"morning", {false, true,  true }},
  {"pooja",   {false, false, true }},
  {"movie",   {true,  false, true }},
  {"alloff",  {false, false, false}}
};
const int NMODES = 5;

// Party mode: only the lights flash, the fan is left untouched
bool party = false;
unsigned long partyStart = 0, lastStep = 0;
bool flipState = false, savedMain = false, savedMandir = false;

const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#070a14">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Smart Room</title>
<style>
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;min-height:100vh;font-family:system-ui,-apple-system,Segoe UI,Roboto,sans-serif;color:#eef2ff;
background:radial-gradient(900px 500px at 10% -10%,#1e2a5a 0,transparent 60%),radial-gradient(700px 500px at 110% 10%,#3b1d5e 0,transparent 55%),#070a14;
background-attachment:fixed;padding:calc(env(safe-area-inset-top) + 20px) 16px 90px;user-select:none;-webkit-user-select:none}
.wrap{max-width:480px;margin:auto}
.top{display:flex;justify-content:space-between;align-items:flex-start}
h1{margin:0;font-size:26px}
.sub{opacity:.6;font-size:14px;margin-top:4px}
.pill{display:flex;align-items:center;gap:6px;font-size:12px;padding:6px 11px;border-radius:99px;background:rgba(74,222,128,.12);color:#4ade80;border:1px solid rgba(74,222,128,.3)}
.pill i{width:7px;height:7px;border-radius:50%;background:currentColor;animation:pulse 1.6s infinite}
.pill.off{background:rgba(248,113,113,.12);color:#f87171;border-color:rgba(248,113,113,.3)}
.sec{margin:28px 4px 10px;font-size:12px;letter-spacing:.12em;text-transform:uppercase;opacity:.55}
.devs{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
.tile{background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.1);border-radius:20px;padding:18px 6px;text-align:center;transition:.25s;cursor:pointer}
.tile:active{transform:scale(.95)}
.ic{font-size:36px;transition:.25s;filter:grayscale(1) opacity(.5);display:inline-block}
.tn{font-size:13px;margin-top:8px;font-weight:600}
.ts{font-size:11px;margin-top:3px;opacity:.5;letter-spacing:.1em}
.tile.on{background:linear-gradient(160deg,rgba(74,222,128,.22),rgba(34,211,238,.1));border-color:rgba(74,222,128,.6);box-shadow:0 0 22px rgba(74,222,128,.25)}
.tile.on .ic{filter:none}
.tile.on .ts{color:#4ade80;opacity:1}
.fan.on .ic{animation:spin 1s linear infinite}
.main.on .ic{filter:drop-shadow(0 0 12px #fde047)}
.mandir.on .ic{filter:drop-shadow(0 0 12px #fb923c);animation:flick 1.8s ease-in-out infinite}
.modes{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.mode{display:flex;gap:12px;align-items:center;padding:14px;border-radius:18px;background:rgba(255,255,255,.06);border:1px solid rgba(255,255,255,.1);transition:.25s;cursor:pointer}
.mode:active{transform:scale(.97)}
.me{font-size:28px}
.mn{font-size:14px;font-weight:700}
.md{font-size:11px;opacity:.6;margin-top:2px}
.mode.act{background:linear-gradient(135deg,rgba(129,140,248,.38),rgba(236,72,153,.26));border-color:#a5b4fc;box-shadow:0 0 22px rgba(129,140,248,.4)}
#toast{position:fixed;left:50%;bottom:calc(env(safe-area-inset-bottom) + 24px);transform:translate(-50%,30px);background:#1f2747;border:1px solid rgba(255,255,255,.15);padding:11px 20px;border-radius:99px;font-size:14px;opacity:0;transition:.3s;pointer-events:none;white-space:nowrap}
#toast.show{opacity:1;transform:translate(-50%,0)}
@keyframes spin{to{transform:rotate(360deg)}}
@keyframes pulse{50%{opacity:.3}}
@keyframes flick{0%,100%{opacity:1}50%{opacity:.7}}
</style></head><body>
<div class="wrap">
  <div class="top">
    <div><h1 id="hello">Hello 👋</h1><div class="sub" id="sum">My Smart Room</div></div>
    <div class="pill" id="pill"><i></i><span id="pt">Online</span></div>
  </div>
  <div class="sec">Devices</div>
  <div class="devs" id="devs"></div>
  <div class="sec">Scenes</div>
  <div class="modes" id="modes"></div>
</div>
<div id="toast"></div>
<script>
const DEV=[
 {k:'fan',e:'🌀',n:'Fan'},
 {k:'main',e:'💡',n:'Main Light'},
 {k:'mandir',e:'🪔',n:'Mandir'}
];
const MODES=[
 {id:'night',e:'🌙',n:'Good Night',d:'Fan on, lights off'},
 {id:'morning',e:'☀️',n:'Good Morning',d:'Lights on, fan off'},
 {id:'pooja',e:'🙏',n:'Pooja',d:'Mandir light only'},
 {id:'movie',e:'🎬',n:'Movie Time',d:'Fan + soft glow'},
 {id:'party',e:'🎉',n:'Party',d:'Light show, 12 sec'},
 {id:'alloff',e:'⏻',n:'All Off',d:'Everything off'}
];
let S={fan:false,main:false,mandir:false,mode:'',party:false};
const $=id=>document.getElementById(id);

function greet(){
  const h=new Date().getHours();
  return h<5?'Night owl 🌙':h<12?'Good Morning ☀️':h<17?'Good Afternoon 🌤️':h<21?'Good Evening 🌆':'Good Night 🌙';
}
function build(){
  $('devs').innerHTML=DEV.map(d=>`<div class="tile ${d.k}" id="t_${d.k}" onclick="tog('${d.k}')">
    <div class="ic">${d.e}</div><div class="tn">${d.n}</div><div class="ts" id="s_${d.k}">OFF</div></div>`).join('');
  $('modes').innerHTML=MODES.map(m=>`<div class="mode" id="m_${m.id}" onclick="setMode('${m.id}')">
    <div class="me">${m.e}</div><div><div class="mn">${m.n}</div><div class="md">${m.d}</div></div></div>`).join('');
}
function update(){
  $('hello').textContent=greet();
  let on=0;
  DEV.forEach(d=>{
    if(S[d.k]) on++;
    $('t_'+d.k).classList.toggle('on',!!S[d.k]);
    $('s_'+d.k).textContent=S[d.k]?'ON':'OFF';
  });
  MODES.forEach(m=>$('m_'+m.id).classList.toggle('act',S.mode===m.id));
  $('sum').textContent=on+' of 3 devices ON';
}
function online(ok){
  $('pill').className=ok?'pill':'pill off';
  $('pt').textContent=ok?'Online':'Offline';
}
let tt;
function toast(t){
  const el=$('toast'); el.textContent=t; el.classList.add('show');
  clearTimeout(tt); tt=setTimeout(()=>el.classList.remove('show'),1600);
}
async function call(url,msg){
  try{
    const r=await fetch(url);
    S=await r.json(); update(); online(true);
    if(msg){ toast(msg); if(navigator.vibrate) navigator.vibrate(15); }
  }catch(e){ online(false); }
}
function tog(k){
  const d=DEV.find(x=>x.k===k);
  const next=S[k]?0:1;
  call('/set?d='+k+'&v='+next, d.n+(next?' ON ✅':' OFF'));
}
function setMode(id){
  const m=MODES.find(x=>x.id===id);
  call('/mode?m='+id, m.e+' '+m.n);
}
build(); update();
setInterval(()=>call('/state'),1500);
call('/state');
</script></body></html>
)rawliteral";

void apply(int i, bool on) {
  st[i] = on;
  digitalWrite(PINS[i], on ? RELAY_ON : RELAY_OFF);
}

String currentMode() {
  if (party) return "party";
  for (int m = 0; m < NMODES; m++) {
    if (st[0] == MODES[m].s[0] && st[1] == MODES[m].s[1] && st[2] == MODES[m].s[2]) {
      return MODES[m].id;
    }
  }
  return "";
}

void sendState() {
  String j = "{";
  for (int i = 0; i < 3; i++) {
    j += "\"" + String(NAMES[i]) + "\":" + (st[i] ? "true" : "false") + ",";
  }
  j += "\"mode\":\"" + currentMode() + "\",";
  j += "\"party\":" + String(party ? "true" : "false") + "}";
  server.send(200, "application/json", j);
}

void stopParty() {
  if (party) {
    party = false;
    apply(1, savedMain);
    apply(2, savedMandir);
  }
}

void startParty() {
  if (!party) {
    savedMain = st[1];
    savedMandir = st[2];
  }
  party = true;
  partyStart = millis();
  lastStep = 0;
}

void handleSet() {
  String d = server.arg("d");
  bool v = server.arg("v") == "1";
  stopParty();
  for (int i = 0; i < 3; i++) if (d == NAMES[i]) apply(i, v);
  sendState();
}

void handleMode() {
  String m = server.arg("m");
  if (m == "party") {
    startParty();
  } else {
    for (int k = 0; k < NMODES; k++) {
      if (m == MODES[k].id) {
        party = false;
        for (int i = 0; i < 3; i++) apply(i, MODES[k].s[i]);
      }
    }
  }
  sendState();
}

void setup() {
  Serial.begin(115200);

  // Write OFF before setting OUTPUT so the relays don't glitch at boot
  for (int i = 0; i < 3; i++) {
    digitalWrite(PINS[i], RELAY_OFF);
    pinMode(PINS[i], OUTPUT);
  }

  WiFi.mode(WIFI_STA);
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("Static IP config failed");
  }
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println();
  Serial.print("Dashboard: http://");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("room")) Serial.println("Or try: http://room.local");

  server.on("/", []() { server.send_P(200, "text/html", PAGE); });
  server.on("/state", sendState);
  server.on("/set", handleSet);
  server.on("/mode", handleMode);
  server.begin();
}

void loop() {
  server.handleClient();

  if (party) {
    unsigned long now = millis();
    if (now - partyStart > 12000) {
      stopParty();
    } else if (now - lastStep > 300) {
      lastStep = now;
      flipState = !flipState;
      apply(1, flipState);
      apply(2, !flipState);
    }
  }
}
