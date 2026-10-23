#pragma once
// Pages served by the board. SETTINGS_HTML at http://skyframe.local, SETUP_HTML on the first-time hotspot.

static const char SETTINGS_HTML[] = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#000000"><title>SkyFrame</title>

<style>
@font-face{font-family:Inter;font-weight:300;font-display:swap;src:url(/font/inter300.ttf) format("truetype")}
@font-face{font-family:Inter;font-weight:400;font-display:swap;src:url(/font/inter400.ttf) format("truetype")}
@font-face{font-family:Inter;font-weight:500;font-display:swap;src:url(/font/inter500.ttf) format("truetype")}
:root{--bg:#000;--panel:#0f1217;--panel2:#161a22;--line:#232831;--fg:#f4f5f7;--soft:#c9d6e6;--dim:#8a93a0;--sun:#f6c945;--good:#4cd38a;--bad:#ff6b63;--kofi:#FF5E5B}
*{box-sizing:border-box}
[hidden]{display:none!important}
html,body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;-webkit-font-smoothing:antialiased}
button,input,select{font:inherit;color:inherit}
button{cursor:pointer}
:focus-visible{outline:2px solid var(--sun);outline-offset:2px}
.app{max-width:1100px;margin:0 auto;padding:16px 16px 100px}
header{display:flex;align-items:center;justify-content:space-between;gap:10px;flex-wrap:wrap;margin-bottom:16px}
h1{font-size:22px;font-weight:600;margin:0;letter-spacing:-.01em}
h2{font-size:12px;letter-spacing:.14em;text-transform:uppercase;color:var(--dim);font-weight:600;margin:22px 4px 8px}
h2:first-child{margin-top:4px}
.chips{display:flex;gap:6px;flex-wrap:wrap}
.chip.ok .dot{background:var(--good)}.chip.new{background:var(--sun);color:#000;font-weight:600}.chip.new .dot{background:#000}
.updbar{position:sticky;top:0;z-index:60;display:flex;align-items:center;gap:10px 14px;flex-wrap:wrap;background:var(--sun);color:#000;border-radius:12px;padding:10px 12px 10px 16px;margin-bottom:14px;box-shadow:0 6px 24px rgba(0,0,0,.5)}
.updbar p{margin:0;flex:1 1 220px;font-size:14.5px;line-height:1.35}.updbar p b{font-weight:700}
.updbar .acts{display:flex;gap:8px;flex-wrap:wrap}
.updbar a,.updbar button{min-height:38px;padding:0 14px;border-radius:9px;font-weight:600;font-size:14px;display:inline-flex;align-items:center;text-decoration:none;border:1px solid rgba(0,0,0,.35);background:transparent;color:#000}
.updbar button{background:#000;color:var(--sun);border-color:#000}
.updlock{position:fixed;inset:0;z-index:90;background:rgba(0,0,0,.92);display:flex;align-items:center;justify-content:center;padding:16px}
.updlock .card{width:100%;max-width:420px;padding:26px 22px;text-align:center}
.updlock h3{margin:0 0 6px;font-size:21px;font-weight:600}.updlock .big{font-size:44px;font-weight:300;margin:10px 0 4px;color:var(--sun)}
.updlock .bar{height:8px;border-radius:4px;background:#2a2f38;overflow:hidden;margin:14px 0 12px}.updlock .bar i{display:block;height:100%;background:var(--sun);width:0;transition:width .5s}
.updlock .spin{width:30px;height:30px;border:3px solid #2a2f38;border-top-color:var(--sun);border-radius:50%;margin:14px auto;animation:sp 1s linear infinite}@keyframes sp{to{transform:rotate(360deg)}}
.updbar .bar{flex-basis:100%;height:5px;border-radius:3px;background:rgba(0,0,0,.18);overflow:hidden}.updbar .bar i{display:block;height:100%;background:#000;width:0;transition:width .4s}
.chip{background:var(--panel2);border-radius:999px;padding:5px 11px;font-size:12.5px;color:var(--soft);display:inline-flex;align-items:center;gap:6px;white-space:nowrap}
.dot{width:8px;height:8px;border-radius:50%;background:var(--bad);flex:none}.dot.on{background:var(--good)}
.grid{display:grid;gap:20px}
.view{display:none}.view.on{display:block}
@media(min-width:900px){.grid{grid-template-columns:370px minmax(0,1fr);align-items:start}#now{display:block!important;position:sticky;top:16px}.tnow{display:none!important}}
.tabs{display:flex;gap:8px;margin:0 0 14px}
.tab{min-height:44px;padding:0 16px;border-radius:12px;background:var(--panel);border:1px solid var(--line);color:var(--dim);display:inline-flex;align-items:center;gap:8px;font-size:14px;font-weight:500}
.tab svg{width:20px;height:20px}
.tab[aria-selected=true]{color:var(--sun);border-color:#4a3f1a}
@media(max-width:899px){
 .tabs{position:fixed;left:0;right:0;bottom:0;margin:0;z-index:30;background:rgba(9,11,15,.97);border-top:1px solid var(--line);justify-content:space-around;padding:6px 8px calc(6px + env(safe-area-inset-bottom))}
 .tab{flex:1;max-width:150px;flex-direction:column;gap:2px;font-size:11.5px;border:0;background:none;min-height:52px;padding:0}
}
.card{background:var(--panel);border:1px solid var(--line);border-radius:16px}
.pad{padding:16px}
.preview{position:relative;background:#000;border-radius:12px;overflow:hidden;border:1px solid var(--line)}
.preview canvas{display:block;width:100%;aspect-ratio:1;image-rendering:auto}
.bigcap{margin:0 0 10px;color:var(--dim);font-size:13px}
.enl{position:absolute;right:8px;bottom:8px;width:40px;height:40px;border-radius:10px;border:0;background:rgba(0,0,0,.6);color:var(--soft);display:grid;place-items:center}
.navrow{display:flex;align-items:center;justify-content:space-between;gap:8px;margin:12px 0 4px}
.ibtn{width:48px;height:48px;border-radius:12px;border:1px solid var(--line);background:var(--panel2);color:var(--fg);display:grid;place-items:center}
.ibtn svg,.enl svg{width:22px;height:22px}
.cur{text-align:center}.cur b{display:block;font-weight:600;font-size:16px}.cur small{color:var(--dim)}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;min-height:48px}
.row+.row{border-top:1px solid var(--line)}
.row .t{display:flex;flex-direction:column}.row .t small{color:var(--dim);font-size:12.5px}
.field{padding:12px 16px}
.field+.field{border-top:1px solid var(--line)}
.lab{display:flex;justify-content:space-between;align-items:baseline;gap:8px;margin-bottom:8px;font-size:14px}
.lab output{color:var(--soft);font-variant-numeric:tabular-nums}
input[type=range]{width:100%;accent-color:var(--sun);height:28px}
input[type=text],input[type=datetime-local],select{width:100%;min-height:44px;background:#000;border:1px solid var(--line);border-radius:10px;padding:10px 12px;outline:none;color-scheme:dark}
input:focus,select:focus{border-color:#3d4452}
.seg{display:flex;background:#000;border:1px solid var(--line);border-radius:10px;padding:3px;gap:3px}
.seg button{flex:1;min-height:38px;border:0;border-radius:7px;background:none;color:var(--dim);font-weight:500}
.seg button[aria-pressed=true]{background:#232835;color:var(--fg)}
.sw{position:relative;width:46px;height:28px;flex:none}
.sw input{position:absolute;inset:0;opacity:0;margin:0;cursor:pointer}
.sw span{position:absolute;inset:0;border-radius:14px;background:#2b303a;transition:.2s;pointer-events:none}
.sw span:after{content:"";position:absolute;left:3px;top:3px;width:22px;height:22px;border-radius:50%;background:#7d8590;transition:.2s}
.sw input:checked+span{background:var(--sun)}.sw input:checked+span:after{left:21px;background:#000}
.sw input:focus-visible+span{outline:2px solid var(--sun);outline-offset:2px}
.list{display:flex;flex-direction:column;gap:8px}
.scr{background:var(--panel);border:1px solid var(--line);border-radius:14px;touch-action:pan-y}
.scr.drag{border-color:var(--sun);position:relative;z-index:5;box-shadow:0 10px 30px rgba(0,0,0,.6)}
.scr.off .sbody b{color:var(--dim)}
.shead{display:flex;align-items:center;gap:4px;padding:4px 10px 4px 4px;min-height:60px}
.handle{width:40px;height:52px;border:0;background:none;color:#5b6370;display:grid;place-items:center;touch-action:none;cursor:grab;flex:none}
.handle svg{width:20px;height:20px}
.sbody{flex:1;min-width:0;text-align:left;background:none;border:0;padding:8px 4px;display:flex;flex-direction:column}
.sbody b{font-weight:600;font-size:15px}
.sbody small{color:var(--dim);font-size:12.5px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.more{width:44px;height:44px;border:0;background:none;color:var(--dim);display:grid;place-items:center;flex:none;border-radius:10px}
.more svg{width:20px;height:20px;transition:.2s}.more[aria-expanded=true] svg{transform:rotate(180deg)}
.sset{border-top:1px solid var(--line);padding:12px 14px 14px;display:flex;flex-direction:column;gap:12px}
.cur-badge{font-size:11px;color:#000;background:var(--sun);border-radius:6px;padding:1px 6px;margin-left:6px;font-weight:600;vertical-align:1px}
.search{position:relative}
.results{position:absolute;left:0;right:0;top:calc(100% + 4px);background:#0b0d11;border:1px solid var(--line);border-radius:12px;overflow:hidden;z-index:40;display:none;box-shadow:0 16px 36px rgba(0,0,0,.7)}
.results.open{display:block}
.results button{display:flex;width:100%;text-align:left;gap:10px;align-items:center;background:none;border:0;border-top:1px solid #171a20;padding:11px 12px;min-height:48px}
.results button:first-child{border-top:0}.results button:hover,.results button.hl{background:#161a21}
.results small{color:var(--dim);display:block;font-size:12px}
.cc{font:600 11px/1 system-ui,sans-serif;letter-spacing:.05em;color:var(--soft);background:#1a1e26;border:1px solid #272c36;border-radius:6px;padding:5px 0;width:30px;text-align:center;flex:none}
details{border-top:1px solid var(--line)}
details:first-of-type{border-top:0}
summary{list-style:none;display:flex;align-items:center;justify-content:space-between;gap:12px;min-height:56px;padding:6px 16px;cursor:pointer}
summary::-webkit-details-marker{display:none}
summary .t{display:flex;align-items:center;gap:10px}
summary .t svg{width:20px;height:20px;color:var(--dim)}
summary .v{color:var(--dim);font-size:13.5px;display:flex;align-items:center;gap:8px;text-align:right}
summary .v svg{width:18px;height:18px;transition:.2s}
details[open] summary .v svg{transform:rotate(90deg)}
.dbody{padding:0 16px 16px;display:flex;flex-direction:column;gap:12px}
.btn{min-height:44px;padding:0 16px;border-radius:10px;border:1px solid #2d333e;background:var(--panel2);color:var(--fg);font-weight:500;display:inline-flex;align-items:center;justify-content:center;gap:8px;text-decoration:none}
.btn svg{width:18px;height:18px}
.btn.danger{color:var(--bad);border-color:#4a2a2a;background:none}
.btn.kofi{background:var(--kofi);border-color:var(--kofi);color:#fff}
.btns{display:flex;gap:8px;flex-wrap:wrap}
.btns.eq{display:grid;grid-auto-flow:column;grid-auto-columns:minmax(0,1fr)}
@media (max-width:560px){.btns.eq.stack{grid-auto-flow:row}}
.btns.eq .btn{width:100%;white-space:nowrap;padding:0 10px}
.btn.primary{background:var(--sun);border-color:var(--sun);color:#000}
.note{color:var(--dim);font-size:13px;line-height:1.5;margin:0}
.ok{color:var(--good)}.warn{color:var(--sun)}.bad{color:var(--bad)}
.saved{font-size:12px;color:var(--good);opacity:0;transition:opacity .3s;margin-left:6px}.saved.show{opacity:1}
.toast{position:fixed;left:50%;bottom:24px;transform:translate(-50%,20px);background:#1d212b;border:1px solid #2d333e;border-radius:12px;padding:10px 10px 10px 16px;display:flex;align-items:center;gap:12px;opacity:0;pointer-events:none;transition:.25s;z-index:60;max-width:calc(100% - 32px)}
@media(max-width:899px){.toast{bottom:84px}}
.toast.show{opacity:1;transform:translate(-50%,0);pointer-events:auto}
.toast button{background:none;border:0;color:var(--sun);font-weight:600;min-height:36px;padding:0 8px}
.modal{position:fixed;inset:0;background:rgba(0,0,0,.88);display:none;align-items:center;justify-content:center;padding:16px;z-index:70}
.modal.open{display:flex}
.modal .card{width:100%;max-width:420px;padding:22px}
.modal h3{margin:0 0 6px;font-size:20px;font-weight:600}
.big{width:100%;max-width:520px}
.big canvas{width:100%;aspect-ratio:1;border-radius:12px;display:block;background:#000}
.pinin{font-size:28px;letter-spacing:.3em;text-align:center}
.sr{position:absolute;width:1px;height:1px;overflow:hidden;clip:rect(0 0 0 0)}
</style></head><body>
<div class="app">
 <div class="updbar" id="updbar" role="status" aria-live="polite" hidden>
  <p id="updbarText"></p>
  <div class="acts" id="updbarActs"><a id="updbarNotes" href="#" target="_blank" rel="noopener">What's new</a><button id="updbarGo">Install now</button></div>
  <div class="bar" id="updbarBar" hidden><i id="updbarFill"></i></div>
 </div>
 <header>
  <h1>SkyFrame</h1>
  <div class="chips" aria-live="polite"><span class="chip"><span class="dot" id="dot"></span><span id="online">Connecting…</span></span><span class="chip" id="ver0" hidden><span class="dot"></span><span id="ver0t"></span></span><span class="chip" id="upd1" hidden></span><span class="chip" id="upd2" hidden></span></div>
 </header>
 <div class="grid">
  <section id="now" class="view on" aria-label="Now">
   <div class="card pad">
    <div class="preview"><canvas id="cv" width="480" height="480" role="img" aria-label="What the display is showing"></canvas>
     <button class="enl" id="enl" aria-label="Show exact picture" title="Show exact picture"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M15 3h6v6M9 21H3v-6M21 3l-7 7M3 21l7-7"/></svg></button></div>
    <div class="navrow">
     <button class="ibtn" id="prev" aria-label="Previous screen"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><path d="M15 18l-6-6 6-6"/></svg></button>
     <div class="cur"><b id="curName">—</b><small id="curPos"></small></div>
     <button class="ibtn" id="next" aria-label="Next screen"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><path d="M9 18l6-6-6-6"/></svg></button>
    </div>
    <div class="field" style="padding:12px 0 4px"><div class="lab"><label for="bNow" id="bNowLab">Brightness</label><span><output id="bNowOut"></output><span class="saved" data-for="bNow">Saved ✓</span></span></div><input type="range" id="bNow" min="5" max="100" step="5"></div>
    <div class="row"><div class="t"><span id="holdLab">Hold this screen</span><small>Stops changing screens until you turn it off</small></div><label class="sw"><input type="checkbox" id="hold" role="switch" aria-labelledby="holdLab"><span></span></label></div>
   </div>
  </section>

  <div>
   <nav class="tabs" role="tablist" aria-label="Sections">
    <button class="tab tnow" role="tab" data-tab="now" aria-selected="true"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><rect x="3" y="4" width="18" height="13" rx="2"/><path d="M8 21h8M12 17v4"/></svg>Now</button>
    <button class="tab" role="tab" data-tab="screens" aria-selected="false"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="7" height="7" rx="1.5"/><rect x="14" y="3" width="7" height="7" rx="1.5"/><rect x="3" y="14" width="7" height="7" rx="1.5"/><rect x="14" y="14" width="7" height="7" rx="1.5"/></svg>Screens</button>
    <button class="tab" role="tab" data-tab="device" aria-selected="false"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.7 1.7 0 0 0 .3 1.8l.1.1a2 2 0 1 1-2.8 2.8l-.1-.1a1.7 1.7 0 0 0-1.8-.3 1.7 1.7 0 0 0-1 1.5V21a2 2 0 1 1-4 0v-.1a1.7 1.7 0 0 0-1.1-1.5 1.7 1.7 0 0 0-1.8.3l-.1.1a2 2 0 1 1-2.8-2.8l.1-.1a1.7 1.7 0 0 0 .3-1.8 1.7 1.7 0 0 0-1.5-1H3a2 2 0 1 1 0-4h.1a1.7 1.7 0 0 0 1.5-1.1 1.7 1.7 0 0 0-.3-1.8l-.1-.1a2 2 0 1 1 2.8-2.8l.1.1a1.7 1.7 0 0 0 1.8.3H9a1.7 1.7 0 0 0 1-1.5V3a2 2 0 1 1 4 0v.1a1.7 1.7 0 0 0 1 1.5 1.7 1.7 0 0 0 1.8-.3l.1-.1a2 2 0 1 1 2.8 2.8l-.1.1a1.7 1.7 0 0 0-.3 1.8V9a1.7 1.7 0 0 0 1.5 1H21a2 2 0 1 1 0 4h-.1a1.7 1.7 0 0 0-1.5 1z"/></svg>Device</button>
   </nav>

   <section id="screens" class="view" aria-label="Screens">
    <h2 id="scrCount">Screens</h2>
    <p class="note" style="margin:0 4px 10px">Drag ⠿ to change the order. Tap a screen to show it now.</p>
    <div class="list" id="list"></div>
   </section>

   <section id="device" class="view" aria-label="Device">
    <h2>Display</h2>
    <div class="card">
     <div class="field"><div class="lab"><label for="rotate">Change screen every</label><span><output id="rotateOut"></output><span class="saved" data-for="rotate">Saved ✓</span></span></div><input type="range" id="rotate" min="0" max="60" step="5"></div>
     <div class="field"><div class="lab"><span>Rotation</span><span class="saved" data-for="rotation">Saved ✓</span></div><div class="seg" id="rotation" role="group" aria-label="Rotation"><button data-v="auto">Auto</button><button data-v="0">0°</button><button data-v="90">90°</button><button data-v="180">180°</button><button data-v="270">270°</button></div></div><p class="note" style="margin-top:6px">Auto turns the picture with the display, using its motion sensor. Pick an angle to fix it.</p>
     <details><summary><span class="t">Night mode</span><span class="v"><span id="nightSum"></span><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></span></summary>
      <div class="dbody">
       <div class="lab" style="margin:0"><span>From</span><span>To</span></div>
       <div style="display:grid;grid-template-columns:1fr 1fr;gap:10px"><select id="nFrom" aria-label="Night starts"></select><select id="nTo" aria-label="Night ends"></select></div>
       <div><div class="lab"><label for="bNight">Night brightness</label><span><output id="bNightOut"></output><span class="saved" data-for="bNight">Saved ✓</span></span></div><input type="range" id="bNight" min="0" max="100" step="5"></div>
       <div class="row" style="min-height:44px"><span class="t"><span id="sWakeLab">Wake on sound</span><small>At night, use the day brightness for a minute when there's sound around. Only loudness is measured; nothing is recorded.</small></span><label class="sw"><input type="checkbox" id="sWake" role="switch" aria-labelledby="sWakeLab"><span></span></label></div>
       <div class="field" id="sSenseRow"><div class="lab"><span>Sound sensitivity</span><span class="saved" data-for="soundSense">Saved ✓</span></div><div class="seg" id="sSense" role="group" aria-label="Sound sensitivity"><button data-v="0">Low</button><button data-v="1">Medium</button><button data-v="2">High</button></div></div>
       <div><div class="lab"><label for="bDay">Day brightness</label><span><output id="bDayOut"></output><span class="saved" data-for="bDay">Saved ✓</span></span></div><input type="range" id="bDay" min="5" max="100" step="5"></div>
      </div></details>
    </div>

    <h2>Location</h2>
    <div class="card"><div class="field"><div class="lab"><label for="city">City for weather, sun, moon and sky</label><span class="saved" data-for="city">Saved ✓</span></div>
     <div class="search"><input type="text" id="city" placeholder="Type a city…" autocomplete="off"><div class="results" id="cityResults"></div></div>
     <p class="note" id="citySum" style="margin-top:8px"></p></div></div>

    <h2>Connection</h2>
    <div class="card">
     <details><summary><span class="t"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M5 12.5a10 10 0 0 1 14 0M8.5 16a5 5 0 0 1 7 0M2 9a15 15 0 0 1 20 0"/><circle cx="12" cy="19.5" r="1"/></svg>Wi-Fi</span><span class="v"><span id="wifiSum"></span><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></span></summary>
      <div class="dbody"><p class="note" id="wifiInfo"></p><p class="note">Wi-Fi switches off between updates. An open settings page keeps it on.</p>
       <div class="btns"><button class="btn danger" id="forget">Change Wi-Fi network</button></div></div></details>
     <details><summary><span class="t"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M21 12a9 9 0 1 1-3-6.7L21 8M21 3v5h-5"/></svg>Updates</span><span class="v"><span id="updSum"></span><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></span></summary>
      <div class="dbody"><p class="note">SkyFrame checks GitHub for new versions every night between 3 and 5 and installs signed versions by itself. It also looks (without installing) each time this page opens. If a new version doesn't start properly, it goes back to the previous one.</p>
       <div class="row" style="min-height:44px"><span id="autoLab">Install new versions automatically</span><label class="sw"><input type="checkbox" id="auto" role="switch" aria-labelledby="autoLab"><span></span></label></div>
       <p class="note" id="updState" aria-live="polite"></p>
       <div class="btns eq"><button class="btn" id="check">Check for updates now</button><a class="btn gh" href="#" target="_blank" rel="noopener"><svg viewBox="0 0 16 16"><path fill="currentColor" d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0016 8c0-4.42-3.58-8-8-8z"/></svg>View on GitHub</a></div></div></details>
     <details><summary><span class="t"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"><path d="M12 3l8 3v6c0 4.5-3.4 8.3-8 9-4.6-.7-8-4.5-8-9V6l8-3z"/><path d="M9 12l2 2 4-4"/></svg>Security</span><span class="v"><span id="secSum"></span><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M9 18l6-6-6-6"/></svg></span></summary>
      <div class="dbody"><p class="note" id="pwInfo"></p><p class="note">This browser is paired with the display. Hold the display for 1 second to see the pairing code.</p>
       <div class="btns"><button class="btn danger" id="newpin">Change pairing code</button></div></div></details>
    </div>

    <h2>About</h2>
    <div class="card pad">
     <p style="margin:0 0 8px"><b>SkyFrame</b> <span id="ver" style="color:var(--dim)"></span></p>
     <p class="note">A proof of concept of what this little screen can do on its own. Shared as is, with no support. Free and open source under the MIT license: use it, change it and share it, as long as the license notice stays with it. No warranty.</p>
     <p class="note" style="margin-top:8px">Weather and air quality by <a href="https://open-meteo.com/" target="_blank" rel="noopener" style="color:var(--soft)">Open-Meteo.com</a> (CC BY 4.0) · Space Station data from wheretheiss.at · World map from Natural Earth · Font: Inter (SIL OFL 1.1)</p>
     <div class="btns eq stack" style="margin-top:14px">
      <a class="btn gh" href="#" target="_blank" rel="noopener"><svg viewBox="0 0 16 16"><path fill="currentColor" d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.01 8.01 0 0016 8c0-4.42-3.58-8-8-8z"/></svg>Project on GitHub</a>
      <a class="btn kofi" id="kofi" href="#" target="_blank" rel="noopener"><svg viewBox="0 0 24 24"><path fill="currentColor" d="M3 6h14v6a5 5 0 0 1-5 5H8a5 5 0 0 1-5-5V6zm14 1h1.5a3 3 0 0 1 0 6H17v-2h1.5a1 1 0 0 0 0-2H17V7zM3 19h14v2H3z"/><path fill="#FF5E5B" d="M10 9.2c.9-1.2 3-.8 3 .9 0 1.5-3 3.4-3 3.4s-3-1.9-3-3.4c0-1.7 2.1-2.1 3-.9z"/></svg>Support on Ko-fi</a>
      <a class="btn" id="lic" href="#" target="_blank" rel="noopener"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z"/><path d="M14 3v5h5M9 13h6M9 17h6"/></svg>MIT license</a>
     </div>
    </div>
   </section>
  </div>
 </div>
</div>

<div class="toast" id="toast" role="status"><span id="toastMsg"></span><button id="toastUndo" hidden>Undo</button></div>

<div class="modal" id="pinbox" role="dialog" aria-modal="true" aria-labelledby="pinTitle"><div class="card">
 <h3 id="pinTitle">Enter pairing code</h3>
 <p class="note" id="pinmsg" style="margin-bottom:14px">Hold your finger on the display for 1 second. It shows a 6-digit code, or scan its QR code.</p>
 <input type="text" id="pin" class="pinin" inputmode="numeric" maxlength="6" autocomplete="one-time-code" aria-label="Pairing code">
 <button class="btn" id="pingo" style="width:100%;margin-top:12px">Unlock</button></div></div>

<div class="modal" id="welcome" role="dialog" aria-modal="true" aria-labelledby="wTitle"><div class="card">
 <h3 id="wTitle">Welcome to SkyFrame</h3>
 <p class="note" style="margin-bottom:16px">Three quick questions. You can change everything later.</p>
 <div class="lab"><label for="wcity">Your city</label></div>
 <div class="search"><input type="text" id="wcity" placeholder="Type a city…" autocomplete="off"><div class="results" id="wcityResults"></div></div>
 <p class="note" id="wcitySum" style="margin:6px 0 14px"></p>
 <div class="lab"><span>Temperature</span></div><div class="seg" id="wunits" role="group" aria-label="Temperature"><button data-v="c">°C</button><button data-v="f">°F</button></div>
 <div class="lab" style="margin-top:14px"><span>Clock</span></div><div class="seg" id="wh12" role="group" aria-label="Clock"><button data-v="0">24-hour</button><button data-v="1">12-hour</button></div>
 <div class="btns" style="margin-top:20px;justify-content:flex-end"><button class="btn" id="wskip" style="background:none">Skip</button><button class="btn" id="wdone" style="background:var(--sun);color:#000;border-color:var(--sun)">Done</button></div>
</div></div>

<div class="updlock" id="updlock" role="alertdialog" aria-modal="true" aria-labelledby="ulTitle" aria-live="polite" hidden><div class="card">
 <h3 id="ulTitle">Updating SkyFrame</h3>
 <div class="big" id="ulPct"></div><div class="spin" id="ulSpin" hidden></div>
 <div class="bar" id="ulBar"><i id="ulFill"></i></div>
 <p class="note" id="ulMsg"></p>
 <p class="note" id="ulNote" style="margin-top:10px">Please keep the display powered and this page open. It goes back to the previous version by itself if anything goes wrong.</p>
 <div class="btns eq" style="margin-top:14px"><button class="btn" id="ulCancel">Cancel</button><button class="btn primary" id="ulGo">Install</button><button class="btn" id="ulClose" hidden>Close</button></div>
</div></div>
<div class="modal" id="bigbox" role="dialog" aria-modal="true" aria-label="Exact picture"><div class="big"><p class="bigcap" id="bigCap">Exact picture from the display</p><canvas id="cvBig" width="480" height="480"></canvas><button class="btn" id="bigClose" style="width:100%;margin-top:12px">Close</button></div></div>

<script>
/* SkyFrame screen renderer: draws the display's screens in the browser from /api/data.
   A line-by-line port of firmware/src/screens.cpp (same coordinates, sizes and colours). */
const SF = (() => {
const C = { BG: "#000", FG: "#f4f5f7", SOFT: "#c9d6e6", DIM: "#7d8590", FAINT: "#1c1f24", LINE: "#2b2f36", SUN: "#f6c945", CLOUD: "#a8c6e6", CLOUD2: "#8fa9c4",
  RAIN: "#5aa8ff", MOON: "#ece7d6", SNOW: "#e8f1fb", GOOD: "#4cd38a", MOD: "#f6c945", USG: "#ff9f43", BAD: "#ff5e57", VBAD: "#b56cff" };
const PI = Math.PI, TAU = 2 * PI, RAD = PI / 180, DEG = 180 / PI;
let D = null, LAND = null, skew = 0, X = null;

/* ---------- drawing helpers (same meaning as gfx.cpp) ---------- */
const font = (s, w) => { X.font = `${w} ${s}px Inter, system-ui, -apple-system, "Segoe UI", sans-serif`; };
function tw(s, size, w = 400, tr = 0) { font(size, w); X.letterSpacing = tr + "px"; return X.measureText(s).width - (s.length ? tr : 0); }
function txt(s, x, y, size, w, col, al = "left", tr = 0, a = 1) {
  const width = tw(s, size, w, tr);
  X.textAlign = "left"; X.fillStyle = col; X.globalAlpha = a;
  X.fillText(s, al === "center" ? x - width / 2 : al === "right" ? x - width : x, y);
  X.globalAlpha = 1; return width;
}
function circle(x, y, r, col, a = 1) { X.globalAlpha = a; X.fillStyle = col; X.beginPath(); X.arc(x, y, Math.max(r, 0), 0, TAU); X.fill(); X.globalAlpha = 1; }
function ring(x, y, r, t, col, a = 1) { X.globalAlpha = a; X.strokeStyle = col; X.lineWidth = t; X.beginPath(); X.arc(x, y, r, 0, TAU); X.stroke(); X.globalAlpha = 1; }
function arc(x, y, r, t, a0, a1, col, a = 1) { X.globalAlpha = a; X.strokeStyle = col; X.lineWidth = t; X.lineCap = "round"; X.beginPath(); X.arc(x, y, r, a0, a1); X.stroke(); X.globalAlpha = 1; }
function dashedArc(x, y, r, t, a0, a1, dash, gap, col) { const da = dash / r, ga = gap / r; for (let s = a0; s < a1; s += da + ga) arc(x, y, r, t, s, Math.min(s + da, a1), col); }
function line(x0, y0, x1, y1, t, col, a = 1) { X.globalAlpha = a; X.strokeStyle = col; X.lineWidth = t; X.lineCap = "round"; X.beginPath(); X.moveTo(x0, y0); X.lineTo(x1, y1); X.stroke(); X.globalAlpha = 1; }
function rrect(x, y, w, h, r, col, a = 1) { X.globalAlpha = a; X.fillStyle = col; X.beginPath(); X.roundRect ? X.roundRect(x, y, w, h, Math.min(r, w / 2, h / 2)) : X.rect(x, y, w, h); X.fill(); X.globalAlpha = 1; }
function rrectStroke(x, y, w, h, r, t, col) { X.strokeStyle = col; X.lineWidth = t; X.beginPath(); X.roundRect ? X.roundRect(x, y, w, h, r) : X.rect(x, y, w, h); X.stroke(); }
function poly(pts, col) { X.fillStyle = col; X.beginPath(); pts.forEach(([x, y], i) => i ? X.lineTo(x, y) : X.moveTo(x, y)); X.closePath(); X.fill(); }
const sep = y => { X.fillStyle = C.FAINT; X.fillRect(36, Math.floor(y), 408, 1); };
const label = (s, x = 36, y = 52, al = "left", col = C.DIM) => txt(s, x, y, 14, 500, col, al, 2.2);
function statCol(x, al, key, val, unit, vs = 28) {
  txt(key, x, 404, 13, 500, C.DIM, al, 1.8);
  const uw = unit ? tw(unit, 15) + 3 : 0, vw = tw(val, vs);
  const vx = al === "left" ? x : al === "center" ? x - (vw + uw) / 2 : x - vw - uw;
  txt(val, vx, 440, vs, 400, C.FG);
  if (unit) txt(unit, vx + vw + 3, 440, 15, 400, C.DIM);
}
function ellipsize(s, size, w, maxW) { if (tw(s, size, w) <= maxW) return s; let t = s; while (t.length > 1) { t = t.slice(0, -1); const u = t.trimEnd() + "…"; if (tw(u, size, w) <= maxW) return u; } return t; }
function wrap(s, size, w, maxW, max) { const out = []; let cur = ""; for (const word of s.split(" ")) { const tr = cur ? cur + " " + word : word; if (tw(tr, size, w) <= maxW || !cur) cur = tr; else { out.push(cur); cur = word; } if (out.length >= max) break; } if (cur && out.length < max) out.push(cur); return out; }
const mix = (a, b, t) => { t = Math.min(1, Math.max(0, t)); const p = h => [1, 3, 5].map(i => parseInt(h.slice(i, i + 2), 16)); const A = p(a), B = p(b); return `rgb(${A.map((v, i) => Math.round(v + (B[i] - v) * t)).join(",")})`; };

/* ---------- time (board clock, city time zone) ---------- */
const NOW = () => Math.floor(Date.now() / 1000 + skew);
function L(t, off) { const d = new Date((t + off) * 1000), y = d.getUTCFullYear(); return { y, mo: d.getUTCMonth() + 1, d: d.getUTCDate(), h: d.getUTCHours(), mi: d.getUTCMinutes(), s: d.getUTCSeconds(), wd: d.getUTCDay(), yd: Math.round((Date.UTC(y, d.getUTCMonth(), d.getUTCDate()) - Date.UTC(y, 0, 1)) / 864e5) }; }
const civil = (y, m, d) => Date.UTC(y, m - 1, d) / 864e5;
const localToUtc = (y, mo, d, h, mi, off) => Date.UTC(y, mo - 1, d, h, mi) / 1000 - off;
const p2 = n => String(n).padStart(2, "0");
function fmtHM(t, off, h12, ampm) { const l = L(t, off); if (h12) { const h = l.h % 12 || 12; return ampm ? `${h}:${p2(l.mi)} ${l.h < 12 ? "AM" : "PM"}` : `${h}:${p2(l.mi)}`; } return `${p2(l.h)}:${p2(l.mi)}`; }
const deg = v => v == null || isNaN(v) ? "--°" : Math.round(v) + "°";
const MON = ["Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"];
const MONL = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
const WDS = ["Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"];

/* ---------- astronomy (same formulas as astro.cpp) ---------- */
const EL = [[[0.38709927, 0.20563593, 7.00497902, 252.25032350, 77.45779628, 48.33076593], [0.00000037, 0.00001906, -0.00594749, 149472.67411175, 0.16047689, -0.12534081]],
  [[0.72333566, 0.00677672, 3.39467605, 181.97909950, 131.60246718, 76.67984255], [0.00000390, -0.00004107, -0.00078890, 58517.81538729, 0.00268329, -0.27769418]],
  [[1.00000261, 0.01671123, -0.00001531, 100.46457166, 102.93768193, 0], [0.00000562, -0.00004392, -0.01294668, 35999.37244981, 0.32327364, 0]],
  [[1.52371034, 0.09339410, 1.84969142, -4.55343205, -23.94362959, 49.55953891], [0.00001847, 0.00007882, -0.00813131, 19140.30268499, 0.44441088, -0.29257343]],
  [[5.20288700, 0.04838624, 1.30439695, 34.39644051, 14.72847983, 100.47390909], [-0.00011607, -0.00013253, -0.00183714, 3034.74612775, 0.21252668, 0.20469106]],
  [[9.53667594, 0.05386179, 2.48599187, 49.95424423, 92.59887831, 113.66242448], [-0.00125060, -0.00050991, 0.00193609, 1222.49362201, -0.41897216, -0.28867794]],
  [[19.18916464, 0.04725744, 0.77263783, 313.23810451, 170.95427630, 74.01692503], [-0.00196176, -0.00004397, -0.00242939, 428.48202785, 0.40805281, 0.04240589]],
  [[30.06992276, 0.00859048, 1.77004347, -55.12002969, 44.96476227, 131.78422574], [0.00026291, 0.00005105, 0.00035372, 218.45945325, -0.32241464, -0.01262724]]];
const PLANET = [["Mercury", "#bdb6ad", 3], ["Venus", "#f3dfae", 4.5], ["Earth", "#5aa8ff", 4.5], ["Mars", "#ff7a59", 3.8], ["Jupiter", "#e6c39a", 8], ["Saturn", "#ead9a8", 6.5], ["Uranus", "#9fe3e6", 5], ["Neptune", "#6f8cff", 5]];
const NAKED = [0, 1, 3, 4, 5];
const jd = t => t / 86400 + 2440587.5, n360 = a => ((a % 360) + 360) % 360, w180 = a => n360(a + 180) - 180;
function helio(p, t) {
  const T = (jd(t) - 2451545) / 36525, v = EL[p][0].map((x, i) => x + EL[p][1][i] * T);
  const [a, e, I0, Lm, wb, O] = v, I = I0 * RAD, w = (wb - O) * RAD, Om = O * RAD, M = w180(Lm - wb) * RAD;
  let E = M + e * Math.sin(M); for (let k = 0; k < 6; k++) E -= (E - e * Math.sin(E) - M) / (1 - e * Math.cos(E));
  const xp = a * (Math.cos(E) - e), yp = a * Math.sqrt(1 - e * e) * Math.sin(E), cw = Math.cos(w), sw = Math.sin(w), cO = Math.cos(Om), sO = Math.sin(Om), cI = Math.cos(I), sI = Math.sin(I);
  return [(cw * cO - sw * sO * cI) * xp + (-sw * cO - cw * sO * cI) * yp, (cw * sO + sw * cO * cI) * xp + (-sw * sO + cw * cO * cI) * yp, sw * sI * xp + cw * sI * yp];
}
const geo = (p, t) => { const a = helio(p, t), e = helio(2, t); return [a[0] - e[0], a[1] - e[1], a[2] - e[2]]; };
const sunGeo = t => helio(2, t).map(v => -v);
function moonGeo(t) { const d = jd(t) - 2451545, Lm = 218.316 + 13.176396 * d, M = (134.963 + 13.064993 * d) * RAD, F = (93.272 + 13.229350 * d) * RAD; const lo = (Lm + 6.289 * Math.sin(M)) * RAD, la = 5.128 * Math.sin(F) * RAD, r = (385001 - 20905 * Math.cos(M)) / 149597870.7; return [r * Math.cos(la) * Math.cos(lo), r * Math.cos(la) * Math.sin(lo), r * Math.sin(la)]; }
const OBL = 23.43928 * RAD;
function eq([x, y, z]) { const ye = y * Math.cos(OBL) - z * Math.sin(OBL), ze = y * Math.sin(OBL) + z * Math.cos(OBL); return [Math.atan2(ye, x), Math.atan2(ze, Math.hypot(x, ye))]; }
function altaz([ra, dec], t, lat, lon) { const lst = n360(280.46061837 + 360.98564736629 * (jd(t) - 2451545) + lon) * RAD, H = lst - ra, phi = lat * RAD; const alt = Math.asin(Math.sin(phi) * Math.sin(dec) + Math.cos(phi) * Math.cos(dec) * Math.cos(H)); const az = Math.atan2(Math.sin(H), Math.cos(H) * Math.sin(phi) - Math.tan(dec) * Math.cos(phi)) + PI; return [alt * DEG, n360(az * DEG)]; }
function moonPhase(t) { const syn = 29.530588853; let p = ((t - 947182440) / 86400) % syn; if (p < 0) p += syn; return p / syn; }
const STARS = [["Sirius",6.752,-16.716,-1.46],["Canopus",6.399,-52.696,-.74],["Arcturus",14.261,19.182,-.05],["Rigil Kent",14.660,-60.834,-.27],["Vega",18.616,38.784,.03],["Capella",5.278,45.998,.08],["Rigel",5.242,-8.202,.13],["Procyon",7.655,5.225,.34],["Achernar",1.629,-57.237,.46],["Betelgeuse",5.919,7.407,.5],["Hadar",14.064,-60.373,.61],["Altair",19.846,8.868,.76],["Acrux",12.443,-63.099,.76],["Aldebaran",4.599,16.509,.86],["Antares",16.490,-26.432,.96],["Spica",13.420,-11.161,.97],["Pollux",7.755,28.026,1.14],["Fomalhaut",22.961,-29.622,1.16],["Deneb",20.690,45.280,1.25],["Mimosa",12.795,-59.689,1.25],["Regulus",10.139,11.967,1.35],["Adhara",6.977,-28.972,1.5],["Castor",7.577,31.888,1.58],["Gacrux",12.519,-57.113,1.63],["Shaula",17.560,-37.104,1.62],["Bellatrix",5.419,6.350,1.64],["Elnath",5.438,28.608,1.65],["Alnilam",5.604,-1.202,1.69],["Alnitak",5.679,-1.943,1.74],["Mintaka",5.533,-.299,2.23],["Saiph",5.796,-9.670,2.06],["Alioth",12.900,55.960,1.77],["Dubhe",11.062,61.751,1.79],["Merak",11.031,56.382,2.37],["Phecda",11.897,53.695,2.44],["Megrez",12.257,57.033,3.31],["Mizar",13.399,54.925,2.23],["Alkaid",13.792,49.313,1.86],["Polaris",2.530,89.264,1.98],["Mirfak",3.405,49.861,1.79],["Alpheratz",.140,29.091,2.06],["Schedar",.675,56.537,2.24],["Caph",.153,59.150,2.28],["Navi",.945,60.717,2.47],["Ruchbah",1.430,60.235,2.68],["Segin",1.907,63.670,3.38],["Sadr",20.370,40.257,2.23],["Aljanah",20.770,33.970,2.48],["Albireo",19.512,27.960,3.08],["Fawaris",19.750,45.131,2.87],["Denebola",11.818,14.572,2.14],["Algieba",10.333,19.842,2.08],["Zosma",11.235,20.524,2.56],["Chertan",11.237,15.430,3.3],["Al Jabhah",10.122,16.763,3.5],["Adhafera",10.278,23.417,3.4],["Rasalas",9.879,26.007,3.88],["Algenubi",9.764,23.774,2.98],["Rasalhague",17.582,12.560,2.08],["Hamal",2.120,23.462,2.0],["Algol",3.136,40.956,2.1],["Markab",23.079,15.205,2.49],["Scheat",23.063,28.083,2.42],["Algenib",.221,15.184,2.83],["Kaus Australis",18.403,-34.385,1.85],["Nunki",18.921,-26.297,2.05],["Acrab",16.091,-19.806,2.62],["Dschubba",16.006,-22.622,2.29],["Paikauhale",16.598,-28.216,2.82],["Larawag",16.836,-34.293,2.29],["Xamidimura",16.864,-38.047,3.0],["Sargas",17.622,-42.998,1.86],["Girtab",17.708,-39.030,2.39],["Imai",12.252,-58.749,2.8],["Mirach",1.162,35.621,2.06],["Almach",2.065,42.330,2.1],["Alcyone",3.791,24.105,2.87]];
const CONS = [["Orion",[["Betelgeuse","Bellatrix"],["Betelgeuse","Alnitak"],["Bellatrix","Mintaka"],["Mintaka","Alnilam"],["Alnilam","Alnitak"],["Alnitak","Saiph"],["Mintaka","Rigel"]]],["Big Dipper",[["Dubhe","Merak"],["Merak","Phecda"],["Phecda","Megrez"],["Megrez","Dubhe"],["Megrez","Alioth"],["Alioth","Mizar"],["Mizar","Alkaid"]]],["Cassiopeia",[["Caph","Schedar"],["Schedar","Navi"],["Navi","Ruchbah"],["Ruchbah","Segin"]]],["Cygnus",[["Deneb","Sadr"],["Sadr","Albireo"],["Fawaris","Sadr"],["Sadr","Aljanah"]]],["Leo",[["Regulus","Al Jabhah"],["Al Jabhah","Algieba"],["Algieba","Adhafera"],["Adhafera","Rasalas"],["Rasalas","Algenubi"],["Algieba","Zosma"],["Zosma","Denebola"],["Denebola","Chertan"],["Chertan","Regulus"],["Zosma","Chertan"]]],["Scorpius",[["Acrab","Dschubba"],["Dschubba","Antares"],["Antares","Paikauhale"],["Paikauhale","Larawag"],["Larawag","Xamidimura"],["Xamidimura","Sargas"],["Sargas","Girtab"],["Girtab","Shaula"]]],["Pegasus",[["Markab","Scheat"],["Scheat","Alpheratz"],["Alpheratz","Algenib"],["Algenib","Markab"]]],["Andromeda",[["Alpheratz","Mirach"],["Mirach","Almach"]]],["Crux",[["Acrux","Gacrux"],["Mimosa","Imai"]]],["Gemini",[["Castor","Pollux"]]]];

/* ---------- weather icons ---------- */
function wmo(c) { if (c === 0) return ["clear", "Clear"]; if (c === 1) return ["partly", "Mostly clear"]; if (c === 2) return ["partly", "Partly cloudy"]; if (c === 3) return ["cloud", "Cloudy"]; if (c === 45 || c === 48) return ["fog", "Foggy"]; if (c >= 51 && c <= 57) return ["drizzle", "Drizzle"]; if (c >= 61 && c <= 67) return ["rain", c >= 65 ? "Heavy rain" : "Rain"]; if (c >= 71 && c <= 77) return ["snow", "Snow"]; if (c >= 80 && c <= 82) return ["rain", "Showers"]; if (c === 85 || c === 86) return ["snow", "Snow showers"]; if (c >= 95) return ["storm", "Thunderstorm"]; return ["cloud", "—"]; }
function icon(k, day, cx, cy, size) {
  const S = size / 64, ix = u => cx + (u - 32) * S, iy = v => cy + (v - 32) * S;
  const sun = (u, v, r) => { for (let i = 0; i < 8; i++) { const a = i * PI / 4; line(ix(u + Math.cos(a) * (r + 4)), iy(v + Math.sin(a) * (r + 4)), ix(u + Math.cos(a) * (r + 9)), iy(v + Math.sin(a) * (r + 9)), 3 * S, C.SUN); } circle(ix(u), iy(v), r * S, C.SUN); };
  const moon = (tx, ty, s) => { circle(ix(tx + 36 * s), iy(ty + 31 * s), 17 * s * S, C.MOON); circle(ix(tx + 46 * s), iy(ty + 23 * s), 14 * s * S, C.BG); };
  const cloud = (tx, ty, c) => { circle(ix(tx + 22), iy(ty + 40), 10 * S, c); circle(ix(tx + 34), iy(ty + 31), 14 * S, c); circle(ix(tx + 46), iy(ty + 40), 10 * S, c); rrect(ix(tx + 22), iy(ty + 36), 24 * S, 14 * S, 0, c); };
  if (k === "clear") day ? sun(32, 32, 12) : moon(-2, 2, 1);
  else if (k === "partly") { day ? sun(42, 20, 8) : moon(14, -6, .7); cloud(-4, 2, C.CLOUD); }
  else if (k === "cloud") { cloud(-2, -4, C.CLOUD2); cloud(-2, 4, C.CLOUD); }
  else if (k === "fog") { cloud(-2, -10, C.CLOUD); line(ix(12), iy(48), ix(52), iy(48), 3 * S, C.CLOUD, .7); line(ix(18), iy(56), ix(46), iy(56), 3 * S, C.CLOUD, .7); }
  else if (k === "rain" || k === "drizzle") { cloud(-2, -10, C.CLOUD); [[22, 46, 19, 54], [32, 46, 29, 54], [42, 46, 39, 54], [27, 56, 25, 61], [37, 56, 35, 61]].slice(0, k === "rain" ? 5 : 3).forEach(l => line(ix(l[0]), iy(l[1]), ix(l[2]), iy(l[3]), 3 * S, C.RAIN)); }
  else if (k === "snow") { cloud(-2, -10, C.CLOUD); [[21, 50, 2.6], [32, 54, 2.6], [43, 50, 2.6], [26, 59, 2.2], [38, 59, 2.2]].forEach(d => circle(ix(d[0]), iy(d[1]), d[2] * S, C.SNOW)); }
  else if (k === "storm") { cloud(-2, -10, C.CLOUD); poly([[34, 42], [26, 54], [33, 54], [29, 64], [40, 50], [33, 50], [37, 42]].map(([u, v]) => [ix(u), iy(v)]), C.SUN); }
}

/* ---------- screens ---------- */
const W = () => D.wx, OFF = () => D.off;
const stale = () => D.lastSync ? NOW() - D.lastSync > 3600 : false;

function home() {
  const w = W(), t = deg(w && w.temp); let [k, lbl] = w ? wmo(w.code) : [null, stale() ? "Offline" : "Loading…"];
  const tws = tw(t, 88, 400, -1.5), total = w ? 104 + 18 + tws : tws, x0 = 240 - total / 2;
  if (w) icon(k, w.isDay, x0 + 52, 103, 104);
  txt(t, w ? x0 + 122 : x0, 137, 88, 400, C.FG, "left", -1.5);
  txt(lbl, 240, 204, 34, 400, C.FG, "center");
  if (D.timeOk) { const n = NOW(), l = L(n, OFF()); txt(fmtHM(n, OFF(), D.h12), 240, 346, 132, 500, C.FG, "center", -4); txt(`${MON[l.mo - 1]} ${l.d}`, 240, 402, 30, 400, C.SOFT, "center"); }
  else txt("--:--", 240, 346, 132, 500, C.FAINT, "center", -4);
}
function forecast() {
  label(D.city.name.toUpperCase() + " · THIS WEEK");
  const w = W(); if (!w || w.d.length < 6) { txt("Waiting for the forecast…", 240, 260, 18, 400, C.DIM, "center"); return; }
  const ds = w.d.slice(0, 6), lo = Math.min(...ds.map(d => d[3])), hi = Math.max(...ds.map(d => d[2]));
  ds.forEach((d, i) => {
    const top = 70 + i * 62, cy = top + 31; if (i) sep(top);
    txt(i === 0 ? "Today" : WDS[L(d[0] + 43200, OFF()).wd], 36, cy + 8, 22, 500, C.FG);
    icon(wmo(d[1])[0], true, 145, cy, 38);
    if (d[4] >= 20) txt(d[4] + "%", 170, cy + 5, 14, 500, C.RAIN);
    txt(deg(d[3]), 250, cy + 7, 21, 400, C.DIM, "right"); txt(deg(d[2]), 408, cy + 7, 21, 400, C.FG);
    const bx0 = 260, bx1 = 398, a = (d[3] - lo) / Math.max(hi - lo, 1), b = (d[2] - lo) / Math.max(hi - lo, 1);
    rrect(bx0, cy - 3, bx1 - bx0, 6, 3, C.FAINT);
    const s0 = bx0 + a * (bx1 - bx0), s1 = bx0 + b * (bx1 - bx0), g = X.createLinearGradient(s0, 0, s1, 0);
    g.addColorStop(0, C.RAIN); g.addColorStop(1, C.SUN); X.fillStyle = g; X.fillRect(s0, cy - 3, s1 - s0, 6);
    circle(s0, cy, 3, C.RAIN); circle(s1, cy, 3, C.SUN);
  });
}
function hourly() {
  label("NEXT 12 HOURS"); const w = W();
  txt(deg(w && w.temp), 36, 128, 64, 300, C.FG);
  if (!w || w.h.length < 13) return;
  txt(`High ${deg(w.d[0][2])} · Low ${deg(w.d[0][3])}`, 444, 98, 18, 400, C.SOFT, "right");
  const h = w.h.slice(0, 13), mp = Math.max(...h.map(x => x[2]));
  txt(mp >= 20 ? `Rain chance up to ${mp}%` : "No rain expected", 444, 125, 18, 400, C.RAIN, "right");
  const tMin = Math.min(...h.map(x => x[1])) - 1, tMax = Math.max(...h.map(x => x[1])) + 1, top = 234, bot = 374;
  const Xk = k => 62 + k * (356 / 12), Y = v => bot - (v - tMin) / (tMax - tMin) * (bot - top);
  const pts = [[Xk(0), Y(h[0][1])]]; let minY = pts[0][1];
  for (let k = 1; k < 13; k++) { const x0 = Xk(k - 1), y0 = Y(h[k - 1][1]), x1 = Xk(k), y1 = Y(h[k][1]), cx = (x0 + x1) / 2;
    for (let s = 1; s <= 24; s++) { const t = s / 24, u = 1 - t; const x = u * u * u * x0 + 3 * u * u * t * cx + 3 * u * t * t * cx + t * t * t * x1, y = u * u * u * y0 + 3 * u * u * t * y0 + 3 * u * t * t * y1 + t * t * t * y1; pts.push([x, y]); minY = Math.min(minY, y); } }
  const g = X.createLinearGradient(0, minY, 0, bot); g.addColorStop(0, "rgba(246,201,69,.353)"); g.addColorStop(1, "rgba(246,201,69,0)");
  X.fillStyle = g; X.beginPath(); pts.forEach(([x, y], i) => i ? X.lineTo(x, y) : X.moveTo(x, y)); X.lineTo(pts[pts.length - 1][0], bot); X.lineTo(pts[0][0], bot); X.closePath(); X.fill();
  X.strokeStyle = C.SUN; X.lineWidth = 3; X.lineCap = "round"; X.lineJoin = "round"; X.beginPath(); pts.forEach(([x, y], i) => i ? X.lineTo(x, y) : X.moveTo(x, y)); X.stroke();
  h.forEach((x, k) => { if (x[2] > 0) rrect(Xk(k) - 5, bot + 4, 10, Math.max(2, x[2] / 100 * 28), 3, C.RAIN, .86); });
  for (let k = 0; k <= 12; k += 3) { const x = Xk(k), y = Y(h[k][1]); circle(x, y, 5.2, C.SUN); circle(x, y, 2.8, C.BG); txt(deg(h[k][1]), x, y - 14, 17, 400, C.FG, "center");
    const l = L(h[k][0], OFF()); txt(k === 0 ? "Now" : D.h12 ? `${l.h % 12 || 12} ${l.h < 12 ? "AM" : "PM"}` : p2(l.h), x, 440, 14, 400, C.DIM, "center"); }
}
function sun() {
  label("SUN & MOON"); const w = W(); if (!w) return;
  const n = NOW(), rise = w.d[0][5], set = w.d[0][6], t = Math.min(1, Math.max(0, (n - rise) / (set - rise))), up = n > rise && n < set, cx = 240, cy = 294, R = 180;
  dashedArc(cx, cy, R, 2, PI, TAU, 4, 7, C.LINE);
  if (t > 0) arc(cx, cy, R, 3, PI, PI + PI * t, C.SUN);
  X.fillStyle = C.LINE; X.fillRect(36, cy, 408, 1);
  const a = PI + PI * t, sx = cx + R * Math.cos(a), sy = cy + R * Math.sin(a);
  if (up) { circle(sx, sy, 26, C.SUN, .15); circle(sx, sy, 14, C.SUN); }
  else txt(n < rise ? `Sun rises in ${Math.ceil((rise - n) / 3600)}h` : "Sun has set", cx, cy - 60, 18, 400, C.DIM, "center");
  const len = Math.floor((set - rise) / 60);
  statCol(36, "left", "SUNRISE", fmtHM(rise, OFF(), D.h12, true), null, 30);
  statCol(240, "center", "DAYLIGHT", `${Math.floor(len / 60)}h ${p2(len % 60)}m`, null, 30);
  statCol(444, "right", "SUNSET", fmtHM(set, OFF(), D.h12, true), null, 30);
}
function moon() {
  const p = moonPhase(NOW()), cx = 240, cy = 192, r = 120, k = Math.cos(TAU * p);
  circle(cx, cy, r, "#15171b");
  const x0 = cx - r - 1, y0 = cy - r - 1, size = 2 * r + 3, img = X.getImageData(x0, y0, size, size), dd = img.data;
  for (let yy = 0; yy < size; yy++) { const y = y0 + yy, v = (y + .5 - cy) / r; if (Math.abs(v) > 1) continue; const s = Math.sqrt(1 - v * v);
    for (let xx = 0; xx < size; xx++) { const x = x0 + xx, u = (x + .5 - cx) / r, d = Math.hypot(u, v), edge = (1 - d) * r + .5; if (edge <= 0) continue;
      const term = p < .5 ? (u - k * s) * r : (-k * s - u) * r, cov = Math.min(1, edge) * Math.min(1, Math.max(0, term + .5)); if (cov <= 0) continue;
      const g = Math.min(1, Math.max(0, Math.hypot(u + .2, v + .3) / 1.3)), col = [251 + (201 - 251) * g, 248 + (195 - 248) * g, 238 + (174 - 238) * g], i = (yy * size + xx) * 4;
      for (let c = 0; c < 3; c++) dd[i + c] = dd[i + c] + (col[c] - dd[i + c]) * cov; } }
  X.putImageData(img, x0, y0);
  const names = ["New Moon", "Waxing Crescent", "First Quarter", "Waxing Gibbous", "Full Moon", "Waning Gibbous", "Last Quarter", "Waning Crescent"], cuts = [.034, .216, .284, .466, .534, .716, .784, .966];
  let ci = 0; while (ci < 8 && p >= cuts[ci]) ci++;
  txt(names[ci % 8], 240, 375, 32, 500, C.FG, "center");
  txt(`${Math.round((1 - Math.cos(TAU * p)) / 2 * 100)}% illuminated · Full moon in ${Math.round(((0.5 - p + 1) % 1) * 29.530588853)} days`, 240, 408, 18, 400, C.DIM, "center");
}
function sky() {
  const lat = D.city.lat, lon = D.city.lon, n = NOW(); let t = n, when = "Now";
  if (altaz(eq(sunGeo(t)), t, lat, lon)[0] > -6) { const l = L(n, OFF()); let tt = localToUtc(l.y, l.mo, l.d, 22, 0, OFF()); if (tt <= n) tt += 86400; t = tt; when = "Tonight at " + fmtHM(tt, OFF(), D.h12, true); }
  const cx = 240, cy = 252, R = 194, g = X.createRadialGradient(cx, cy, 0, cx, cy, R);
  g.addColorStop(0, "#0b1426"); g.addColorStop(1, "#03060c"); X.fillStyle = g; X.beginPath(); X.arc(cx, cy, R, 0, TAU); X.fill();
  ring(cx, cy, R, 1, "#1f2733");
  for (const a of [30, 60]) dashedArc(cx, cy, R * (90 - a) / 90, 1, 0, TAU, 2, 6, "#121c2c");
  X.save(); X.beginPath(); X.arc(cx, cy, R, 0, TAU); X.clip();
  const P = (alt, az) => { const r = R * (90 - alt) / 90; return [cx - r * Math.sin(az * RAD), cy - r * Math.cos(az * RAD)]; };
  const pos = {}; STARS.forEach(([nm, ra, dec, mag]) => { const [alt, az] = altaz([ra * 15 * RAD, dec * RAD], t, lat, lon); pos[nm] = { alt, mag, nm, xy: P(alt, az) }; });
  CONS.forEach(([cn, lines]) => { const seen = new Set(); lines.forEach(([a, b]) => { const A = pos[a], B = pos[b]; if (A.alt < -3 && B.alt < -3) return; line(A.xy[0], A.xy[1], B.xy[0], B.xy[1], 1, "#2a3d5c"); if (A.alt > 0) seen.add(A); if (B.alt > 0) seen.add(B); });
    if (seen.size >= 3) { const v = [...seen]; txt(cn.toUpperCase(), v.reduce((q, p) => q + p.xy[0], 0) / v.length, Math.max(...v.map(p => p.xy[1])) + 20, 10, 500, "#3e5880", "center", 1.6); } });
  Object.values(pos).forEach(p => { if (p.alt < -1) return; const r = Math.max(.7, 2.9 - .62 * p.mag), o = Math.min(1, .5 + (2.6 - p.mag) * .2); circle(p.xy[0], p.xy[1], r, "#f2f5ff", o);
    if ((p.mag < .9 || p.nm === "Polaris") && p.alt > 3) txt(p.nm, p.xy[0] + r + 4, p.xy[1] + 3.5, 10, 400, "#8593a8"); });
  NAKED.forEach(k => { const [alt, az] = altaz(eq(geo(k, t)), t, lat, lon); if (alt < -1) return; const [x, y] = P(alt, az), r = PLANET[k][2] * .45 + 2; circle(x, y, r, PLANET[k][1]); txt(PLANET[k][0], x + r + 5, y + 4, 11, 500, PLANET[k][1]); });
  const [ma, mz] = altaz(eq(moonGeo(t)), t, lat, lon); if (ma > -1) { const [x, y] = P(ma, mz); circle(x, y, 16, C.MOON, .12); circle(x, y, 7, C.MOON); txt("Moon", x + 12, y + 4, 11, 500, C.MOON); }
  X.restore();
  txt("N", cx, cy - R + 17, 13, 500, C.DIM, "center"); txt("S", cx, cy + R - 9, 13, 500, C.DIM, "center"); txt("E", cx - R + 14, cy + 4, 13, 500, C.DIM, "center"); txt("W", cx + R - 14, cy + 4, 13, 500, C.DIM, "center");
  label("SKY", 26, 40); txt(when, 454, 40, 14, 400, C.SOFT, "right");
}
function planets() {
  label("PLANETS TONIGHT"); if (!D.plan) { txt("Working out the sky…", 240, 260, 18, 400, C.DIM, "center"); return; }
  D.plan.forEach(([p, status, big, sub, kind, dist], n) => {
    const top = 64 + n * 76; if (n) sep(top); const [nm, col, size] = PLANET[p];
    circle(48, top + 38, Math.min(10, size + 2), col, kind ? 1 : .35);
    txt(nm, 72, top + 36, 22, 500, C.FG);
    const sw = txt(status.toUpperCase(), 72, top + 59, 12, 500, kind === 2 ? C.GOOD : kind === 1 ? C.SUN : C.DIM, "left", 1.2);
    const bigW = big ? tw(big, 26, 300) : 20;
    txt(ellipsize(" · " + sub, 14, 400, 444 - bigW - 16 - (72 + sw)), 72 + sw, top + 59, 14, 400, C.DIM);
    txt(big || "—", 444, top + 38, 26, 300, C.FG, "right");
    const lm = dist * 8.317; txt(lm < 60 ? `${Math.round(lm)} light-min away` : `${Math.floor(lm / 60)}h ${Math.round(lm % 60)}m light-time`, 444, top + 59, 13, 400, C.DIM, "right");
  });
}
function solar() {
  label("SOLAR SYSTEM TODAY"); const cx = 240, cy = 214, RADII = [22, 36, 51, 65, 90, 112, 131, 148];
  RADII.forEach(r => ring(cx, cy, r, 1, "#1a1f27"));
  for (let r = 17; r > 0; r--) circle(cx, cy, r, C.SUN, (16 + (17 - r) * 9) / 255);
  circle(cx, cy, 6.5, "#ffd866");
  const t = NOW(), e = helio(2, t); let nearP = -1, nearD = 1e9;
  PLANET.forEach(([nm, col, size], k) => {
    const p = helio(k, t), lam = Math.atan2(p[1], p[0]), r = RADII[k], x = cx + r * Math.cos(lam), y = cy - r * Math.sin(lam);
    if (k === 5) { X.save(); X.translate(x, y); X.rotate(-20 * RAD); X.globalAlpha = .78; X.strokeStyle = col; X.lineWidth = 1.4; X.beginPath(); X.ellipse(0, 0, size * 1.9, size * .6, 0, 0, TAU); X.stroke(); X.restore(); X.globalAlpha = 1; }
    if (k === 2) ring(x, y, size + 4, 1, C.RAIN, .55);
    circle(x, y, size, col);
    if (k >= 2) { const right = Math.cos(lam) >= 0; txt(nm, x + (right ? 1 : -1) * (size + 6), y + 4, 11, 400, k === 2 ? C.SOFT : C.DIM, right ? "left" : "right"); }
    if (k !== 2) { const d = Math.hypot(p[0] - e[0], p[1] - e[1], p[2] - e[2]); if (d < nearD) { nearD = d; nearP = k; } }
  });
  txt("positions true · distances not to scale", 444, 372, 10, 400, "#3a3f47", "right");
  const dS = Math.hypot(...e), lt = dS * 499.005;
  statCol(36, "left", "TO THE SUN", (dS * 149.598).toFixed(1), "M km", 22);
  statCol(220, "center", "LIGHT TIME", `${Math.floor(lt / 60)}m ${p2(Math.round(lt % 60))}s`, null, 22);
  txt("NEAREST", 444, 404, 13, 500, C.DIM, "right", 1.8);
  const b3 = `${Math.round(nearD * 149.6)}M km`, w3 = tw(b3, 15); txt(b3, 444, 440, 15, 400, C.DIM, "right"); txt(PLANET[nearP][0], 444 - w3 - 6, 440, 22, 400, C.FG, "right");
}
function events() {
  label("COMING UP IN THE SKY"); const COL = ["#9fd0ff", C.MOON, "#ff9f43", C.GOOD, C.SUN], n = NOW(), td = L(n, OFF()), tday = civil(td.y, td.mo, td.d);
  D.ev.slice(0, 6).forEach(([t, type, title, sub], i) => {
    const top = 64 + i * 64; if (i) sep(top); const d = L(t, OFF());
    txt(MON[d.mo - 1].toUpperCase(), 61, top + 26, 12, 500, COL[type], "center", 1.6); txt(String(d.d), 61, top + 53, 26, 400, C.FG, "center");
    const dd = Math.round(civil(d.y, d.mo, d.d) - tday), night = type <= 1;
    const when = dd <= 0 ? (night ? "Tonight" : "Today") : dd === 1 ? "Tomorrow" : dd < 60 ? `in ${dd} days` : `in ${Math.round(dd / 30.4)} months`;
    const ww = txt(when, 444, top + 38, 14, 400, C.SOFT, "right"), maxW = 444 - ww - 12 - 100;
    txt(ellipsize(title, 18, 500, maxW), 100, top + 30, 18, 500, C.FG); txt(ellipsize(sub, 13, 400, maxW), 100, top + 50, 13, 400, C.DIM);
  });
}
function iss() {
  circle(40, 47, 8, C.BAD, .2); circle(40, 47, 4, C.BAD);
  const I = D.iss, n = NOW(), tr = I.tr || []; let ilat = 0, ilon = 0, have = false;
  const vec = (la, lo) => [Math.cos(la * RAD) * Math.cos(lo * RAD), Math.cos(la * RAD) * Math.sin(lo * RAD), Math.sin(la * RAD)];
  for (let i = 0; i + 1 < tr.length && !have; i++) { if (n < tr[i][2] || n > tr[i + 1][2]) continue; const f = (n - tr[i][2]) / (tr[i + 1][2] - tr[i][2]), A = vec(tr[i][0], tr[i][1]), B = vec(tr[i + 1][0], tr[i + 1][1]), v = A.map((a, j) => a * (1 - f) + B[j] * f), m = Math.hypot(...v); ilat = Math.asin(v[2] / m) * DEG; ilon = Math.atan2(v[1], v[0]) * DEG; have = true; }
  label("SPACE STATION · LIVE", 54, 52);
  if (!have) { txt(I.ok ? "Waiting for the next update…" : "Finding the station…", 240, 250, 18, 400, C.DIM, "center"); return; }
  txt(`${Math.abs(ilat).toFixed(1)}°${ilat >= 0 ? "N" : "S"}  ${Math.abs(ilon).toFixed(1)}°${ilon >= 0 ? "E" : "W"}`, 444, 52, 14, 400, C.SOFT, "right");
  const cx = 240, cy = 226, R = 150, lat0 = ilat * RAD, lon0 = ilon * RAD;
  const proj = (la, lo) => { la *= RAD; lo *= RAD; const cl = Math.cos(la), dl = lo - lon0; return [cx + R * cl * Math.sin(dl), cy - R * (Math.cos(lat0) * Math.sin(la) - Math.sin(lat0) * cl * Math.cos(dl)), Math.sin(lat0) * Math.sin(la) + Math.cos(lat0) * cl * Math.cos(dl)]; };
  const g = X.createRadialGradient(cx - .24 * R, cy - .36 * R, 0, cx - .24 * R, cy - .36 * R, 1.5 * R); g.addColorStop(0, "#0e1a2c"); g.addColorStop(1, "#03060b");
  X.fillStyle = g; X.beginPath(); X.arc(cx, cy, R, 0, TAU); X.fill(); ring(cx, cy, R, 1, "#1d2a3c");
  const curve = (la0, lo0, la1, lo1, steps) => { let pv = false, px = 0, py = 0; for (let s = 0; s <= steps; s++) { const f = s / steps, [x, y, k] = proj(la0 + (la1 - la0) * f, lo0 + (lo1 - lo0) * f), v = k > 0; if (v && pv) line(px, py, x, y, 1, "#14213a"); px = x; py = y; pv = v; } };
  for (let lo = -180; lo < 180; lo += 30) curve(-90, lo, 90, lo, 45);
  for (let la = -60; la <= 60; la += 30) curve(la, -180, la, 180, 90);
  if (LAND) for (let i = 0; i < LAND.length; i += 2) { const [x, y, k] = proj(LAND[i] / 100, LAND[i + 1] / 100); if (k > .03) circle(x, y, 1.1 + .7 * k, "#4f7099", .25 + .75 * k); }
  for (let i = 0; i + 1 < tr.length; i++) {
    if (tr[i + 1][2] < n - 1800 || tr[i][2] > n + 2700) continue;
    const A = vec(tr[i][0], tr[i][1]), B = vec(tr[i + 1][0], tr[i + 1][1]); let px = 0, py = 0, pv = false;
    for (let s = 0; s <= 12; s++) { const f = s / 12, v = A.map((a, j) => a * (1 - f) + B[j] * f), m = Math.hypot(...v), [x, y, k] = proj(Math.asin(v[2] / m) * DEG, Math.atan2(v[1], v[0]) * DEG);
      const past = tr[i][2] + f * (tr[i + 1][2] - tr[i][2]) <= n, vis = k > 0; if (vis && pv) { if (past) line(px, py, x, y, 2, C.SUN, .45); else if (s & 1) line(px, py, x, y, 2, C.SUN); } px = x; py = y; pv = vis; }
  }
  const [ux, uy, uk] = proj(D.city.lat, D.city.lon);
  if (uk > 0) { circle(ux, uy, 4, C.RAIN); ring(ux, uy, 9, 1, C.RAIN, .5); txt("You", ux + 12, uy + 4, 11, 400, "#9cc8ff"); }
  circle(cx, cy, 16, C.FG, .1); circle(cx, cy, 5.5, C.FG);
  const p1 = D.city.lat * RAD, q2 = ilat * RAD, dl = (ilon - D.city.lon) * RAD, km = 6371 * 2 * Math.asin(Math.sqrt(Math.sin((q2 - p1) / 2) ** 2 + Math.cos(p1) * Math.cos(q2) * Math.sin(dl / 2) ** 2));
  const fmt = v => Math.round(v).toLocaleString("en-US");
  statCol(36, "left", "ALTITUDE", String(Math.round(I.alt)), "km", 22); statCol(240, "center", "SPEED", fmt(I.vel), "km/h", 22); statCol(444, "right", "FROM YOU", fmt(km), "km", 22);
}
function clock() {
  label("WORLD"); const n = NOW(), me = L(n, OFF());
  D.wc.forEach((w, i) => {
    const top = 66 + i * 126, cy = top + 63; if (i) sep(top); const l = L(n, w.off), day = l.h >= 7 && l.h < 19;
    circle(42, cy - 9, 4.5, day ? C.SUN : "#6f86a6"); txt(ellipsize(w.name, 24, 500, 200), 56, cy, 24, 500, C.FG);
    const diff = Math.round((w.off - OFF()) / 60), dayDiff = Math.round(civil(l.y, l.mo, l.d) - civil(me.y, me.mo, me.d));
    let b = !diff ? "Same time" : (diff > 0 ? "+" : "−") + Math.floor(Math.abs(diff) / 60) + (Math.abs(diff) % 60 ? ":" + p2(Math.abs(diff) % 60) : "") + "h";
    if (dayDiff) b += dayDiff > 0 ? " · tomorrow" : " · yesterday";
    txt(b, 42, cy + 26, 15, 400, C.DIM);
    const t = fmtHM(n, w.off, D.h12);
    if (D.h12) { const ap = l.h < 12 ? "AM" : "PM", aw = tw(ap, 18); txt(ap, 444, cy + 22, 18, 400, C.DIM, "right"); txt(t, 444 - aw - 6, cy + 22, 62, 300, C.FG, "right", -1); }
    else txt(t, 444, cy + 22, 62, 300, C.FG, "right", -1);
  });
}
function calendar() {
  const l = L(NOW(), OFF()); txt(MONL[l.mo - 1], 36, 74, 34, 500, C.FG); txt(String(l.y), 444, 74, 22, 400, C.DIM, "right");
  const cw = 408 / 7; ["M", "T", "W", "T", "F", "S", "S"].forEach((d, i) => txt(d, 36 + cw * i + cw / 2, 116, 13, 500, C.DIM, "center", 1.3));
  const first = civil(l.y, l.mo, 1), shift = (new Date(first * 864e5).getUTCDay() + 6) % 7, today = civil(l.y, l.mo, l.d);
  for (let i = 0; i < 42; i++) { const dd = first - shift + i, d = new Date(dd * 864e5), mo = d.getUTCMonth() + 1; if (i >= 35 && mo !== l.mo) break;
    const x = 36 + cw * (i % 7) + cw / 2, y = 152 + Math.floor(i / 7) * 48; let col = mo !== l.mo ? "#3a3f47" : i % 7 >= 5 ? C.SOFT : C.FG;
    if (dd === today) { circle(x, y, 22, C.SUN); col = C.BG; } txt(String(d.getUTCDate()), x, y + 8, 21, dd === today ? 500 : 400, col, "center"); }
}
function air() {
  label("AIR QUALITY");
  if (D.aqi != null) { const v = D.aqi, B = [[50, "Good", C.GOOD, "Air is clean. Great for a walk."], [100, "Moderate", C.MOD, "Acceptable for most people."], [150, "Sensitive", C.USG, "Sensitive groups should take it easy."], [200, "Unhealthy", C.BAD, "Limit time outdoors."], [1e9, "Very poor", C.VBAD, "Stay inside if you can."]];
    const b = B.find(x => v <= x[0]), nw = txt(String(v), 32, 196, 120, 300, b[2], "left", -4);
    txt(b[1], 32 + nw + 24, 146, 28, 500, C.FG); wrap(b[3], 16, 400, 444 - (32 + nw + 24), 3).forEach((s, i) => txt(s, 32 + nw + 24, 174 + i * 22, 16, 400, C.DIM));
    rrect(36, 234, 408, 8, 4, C.FAINT); const seg = [0, .16, .33, .5, .66, 1], sc = [C.GOOD, C.MOD, C.USG, C.BAD, C.VBAD];
    sc.forEach((c, i) => rrect(36 + 408 * seg[i], 234, 408 * (seg[i + 1] - seg[i]) + (i < 4 ? 4 : 0), 8, i === 0 || i === 4 ? 4 : 0, c));
    const mx = 36 + 408 * Math.min(1, v / 300); rrect(mx - 5, 225, 10, 26, 4, C.BG); rrect(mx - 2, 228, 4, 20, 2, C.FG); }
  else txt(stale() ? "No update yet" : "Loading…", 36, 196, 28, 400, C.DIM);
  const w = W(); if (w) { statCol(36, "left", "HUMIDITY", String(w.hum), "%"); statCol(240, "center", "WIND", String(Math.round(w.wind)), D.f ? "mph" : "km/h"); statCol(444, "right", "FEELS LIKE", deg(w.feels), null); }
}
function stargaze() {
  label("STARGAZING TONIGHT"); if (!D.timeOk) return;
  const lat = D.city.lat, lon = D.city.lon, off = OFF(); let t = NOW(), start = 0, end = 0;
  for (let i = 0; i < 120 && !end; i++, t += 900) { const dark = altaz(eq(sunGeo(t)), t, lat, lon)[0] < -12; if (dark && !start) start = t; if (!dark && start) end = t; }
  if (!start) { txt("No real darkness tonight", 240, 250, 22, 400, C.SOFT, "center"); return; }
  if (!end) end = start + 12 * 3600;
  const illum = (1 - Math.cos(TAU * moonPhase(start))) / 2, sl = [], cl = (W() && W().c) || []; let have = false;
  for (let h = start; h < end && sl.length < 16;) { const e = Math.min(end, Math.floor((h + off) / 3600) * 3600 + 3600 - off), mid = (h + e) / 2; let cloud = -1;
    cl.forEach(([ct, c]) => { if (ct <= mid && mid < ct + 3600 && c >= 0) { cloud = c; have = true; } });
    const up = altaz(eq(moonGeo(mid)), mid, lat, lon)[0] > 0, clear = cloud >= 0 ? 1 - cloud / 100 : .5; sl.push({ t: h, e, cloud, up, score: 10 * clear * (1 - .7 * illum * (up ? 1 : 0)) }); h = e; }
  let best = 0; sl.forEach((s, i) => { if (s.score > sl[best].score) best = i; });
  let w0 = best, w1 = best; while (w0 > 0 && sl[w0 - 1].score >= sl[best].score - 1) w0--; while (w1 + 1 < sl.length && sl[w1 + 1].score >= sl[best].score - 1) w1++;
  const sc = sl[best].score, col = sc >= 7 ? C.GOOD : sc >= 4 ? C.SUN : C.DIM, word = sc >= 8 ? "Excellent" : sc >= 6 ? "Good" : sc >= 4 ? "Fair" : sc >= 2 ? "Poor" : "Not tonight";
  let avg = 0, cn = 0; for (let i = w0; i <= w1; i++) if (sl[i].cloud >= 0) { avg += sl[i].cloud; cn++; } if (cn) avg = Math.floor(avg / cn);
  const nw = txt(String(Math.round(sc)), 32, 196, 120, 300, col, "left", -4); txt("/10", 32 + nw + 4, 196, 26, 400, C.DIM);
  const tx = 32 + nw + 70; txt(word, tx, 138, 28, 500, C.FG);
  txt(`Best ${fmtHM(sl[w0].t, off, D.h12, true)} – ${fmtHM(sl[w1].e, off, D.h12, true)}`, tx, 168, 16, 400, C.SOFT);
  txt(!have ? "No cloud forecast yet" : avg > 60 ? "Mostly cloudy" : (sl[best].up && illum > .5) ? "Bright Moon in the sky" : avg > 25 ? "Some clouds" : "Clear, dark skies", tx, 192, 16, 400, C.DIM);
  const x0 = 36, top = 238, base = 326, bw = 408 / Math.max(sl.length, 1);
  sl.forEach((s, i) => { const x = x0 + i * bw, clear = s.cloud >= 0 ? 1 - s.cloud / 100 : .5, inW = i >= w0 && i <= w1; rrect(x + 3, top, bw - 6, base - top, 3, C.FAINT);
    const hg = Math.max(4, (base - top) * clear); rrect(x + 3, base - hg, bw - 6, hg, 3, inW ? col : C.RAIN, inW ? 1 : .59);
    if (s.up) rrect(x + 3, base + 6, bw - 6, 4, 2, C.MOON, (80 + 175 * illum) / 255);
    const l = L(s.e, off); if ((l.h % 2 === 0 || sl.length <= 8) && s.e < end) txt(D.h12 ? `${l.h % 12 || 12}${l.h < 12 ? "a" : "p"}` : p2(l.h), x + bw / 2, 358, 13, 400, C.DIM, "center"); });
  const dm = Math.floor((end - start) / 60);
  statCol(36, "left", "CLOUDS", cn ? String(avg) : "--", "%", 22); statCol(240, "center", "MOON", String(Math.round(illum * 100)), "% lit", 22); statCol(444, "right", "DARKNESS", `${Math.floor(dm / 60)}h ${p2(dm % 60)}m`, null, 22);
}
function countdown() {
  const m = /(\d+)-(\d+)-(\d+)(?:T(\d+):(\d+))?/.exec(D.cdDate || ""), n = NOW(), target = m ? localToUtc(+m[1], +m[2], +m[3], +(m[4] || 0), +(m[5] || 0), OFF()) : n;
  let s = Math.max(0, target - n);
  txt((D.cdLabel || "").toUpperCase(), 240, 132, 16, 500, C.SUN, "center", 3.2);
  txt(String(Math.floor(s / 86400)), 240, 290, 170, 300, C.FG, "center", -8); txt("days to go", 240, 330, 22, 400, C.SOFT, "center");
  s %= 86400; txt(`${p2(Math.floor(s / 3600))} : ${p2(Math.floor(s % 3600 / 60))} : ${p2(s % 60)}`, 240, 388, 34, 300, C.DIM, "center", 1.3);
}
const QUOTES = [["The best time to plant a tree was 20 years ago. The second best time is now.", "Proverb"], ["You have power over your mind, not outside events.", "Marcus Aurelius"], ["Well begun is half done.", "Aristotle"], ["It is not that we have a short time to live, but that we waste a lot of it.", "Seneca"], ["Simplicity is the ultimate sophistication.", "Attributed to Leonardo da Vinci"], ["The journey of a thousand miles begins with one step.", "Lao Tzu"], ["Nothing is so painful to the human mind as a great and sudden change.", "Mary Shelley"], ["Little by little, one travels far.", "Proverb"], ["We suffer more often in imagination than in reality.", "Seneca"], ["He who has a why to live can bear almost any how.", "Friedrich Nietzsche"], ["Act as if what you do makes a difference. It does.", "William James"], ["Waste no more time arguing what a good person should be. Be one.", "Marcus Aurelius"]];
function quote() {
  const q = QUOTES[(L(NOW(), OFF()).yd + 1) % QUOTES.length], lines = wrap(q[0], 31, 300, 408, 6), h = 50 + lines.length * 42 + 40, y0 = 240 - h / 2;
  txt("“", 32, y0 + 70, 110, 400, C.SUN); lines.forEach((s, i) => txt(s, 36, y0 + 82 + i * 42, 31, 300, C.FG)); txt("— " + q[1], 36, y0 + 82 + lines.length * 42 + 18, 17, 400, C.DIM, "left", 1);
}
function other(msg) { txt(msg, 240, 250, 20, 400, C.SOFT, "center"); }
const SCREENS = { home, forecast, hourly, sun, moon, sky, planets, solar, events, iss, clock, calendar, air, stargaze, countdown, quote };

function dots(idx, count) { if (count <= 1 || idx < 0) return; let x = 240 - ((count - 1) * 12 + 14) / 2;
  for (let i = 0; i < count; i++) { if (i === idx) { rrect(x, 463, 14, 5, 2.5, C.DIM); x += 21; } else { circle(x + 2.5, 465.5, 2.5, C.LINE); x += 12; } } }

return {
  setData(d) { D = d; skew = d.now - Date.now() / 1000; },
  setLand(l) { LAND = l; },
  ready: () => !!D,
  draw(canvas, key, order) {
    X = canvas.getContext("2d"); X.setTransform(1, 0, 0, 1, 0, 0); X.fillStyle = C.BG; X.fillRect(0, 0, 480, 480); X.textBaseline = "alphabetic";
    if (!D) return;
    try {
      if (SCREENS[key]) { SCREENS[key](); const i = order.indexOf(key); dots(i, order.length); }
      else other(key === "" ? "The display is showing its settings screen" : "…");
      if (SCREENS[key] && stale()) txt("offline", 466, 20, 11, 400, "#444", "right");
    } catch (e) { other("Couldn't draw this screen"); console.error(e); }
  }
};
})();

const $=id=>document.getElementById(id);
const esc=s=>String(s).replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
const META={home:["Home","",1],forecast:["Forecast","Next 6 days"],hourly:["Hourly","Next 12 hours"],sun:["Sun","Sunrise and sunset"],moon:["Moon","Today's phase"],sky:["Sky map","Stars and planets tonight"],planets:["Planets tonight","When and where to look"],solar:["Solar system","Positions today"],events:["Sky events","Meteors, eclipses, full moons"],iss:["Space Station","Live track"],clock:["World clocks","",1],calendar:["Calendar","This month"],air:["Air quality","AQI, humidity, wind"],stargaze:["Stargazing tonight","Tonight's 0–10 score"],countdown:["Countdown","",1],quote:["Quote","One per day"]};
let cfg=null,st=null,PIN=(()=>{const m=location.hash.match(/pin=(\d{6})/);if(m){try{localStorage.setItem("pin",m[1])}catch(e){}history.replaceState(null,"",location.pathname);return m[1]}try{return localStorage.getItem("pin")||""}catch(e){return ""}})();

/* ---------- API ---------- */
async function api(path,opt={}){opt.headers=Object.assign({},opt.headers,{"X-Pin":PIN});const r=await fetch(path,opt);
 if(r.status===401){askPin();throw new Error("pin")}if(r.status===429){askPin("Too many wrong codes. Wait a minute and try again.");throw new Error("locked")}return r}
function post(path,body){return api(path,{method:"POST",body:JSON.stringify(body||{})})}
function askPin(msg){$("pinbox").classList.add("open");if(msg)$("pinmsg").textContent=msg;setTimeout(()=>$("pin").focus(),50)}

/* ---------- saving with "Saved" marks and undo ---------- */
let saveT=null,undoCfg=null,toastT=null,dirtyKeys=new Set();
function toast(msg,undo){$("toastMsg").textContent=msg;$("toastUndo").hidden=!undo;$("toastUndo").onclick=undo?()=>{undo();$("toast").classList.remove("show")}:null;
 $("toast").classList.add("show");clearTimeout(toastT);toastT=setTimeout(()=>$("toast").classList.remove("show"),undo?5000:2200)}
function markSaved(k){document.querySelectorAll(`.saved[data-for="${k}"]`).forEach(el=>{el.classList.add("show");setTimeout(()=>el.classList.remove("show"),1600)})}
function change(key,fn,delay=350){if(!undoCfg)undoCfg=JSON.parse(JSON.stringify(cfg));fn(cfg);dirtyKeys.add(key);clearTimeout(saveT);saveT=setTimeout(flush,delay)}
async function flush(){const before=undoCfg;undoCfg=null;const keys=[...dirtyKeys];dirtyKeys.clear();
 try{await post("/api/config",cfg);keys.forEach(markSaved);refreshSummaries();toast("Saved",()=>{cfg=before;post("/api/config",cfg).then(()=>{renderAll();toast("Change undone")})});setTimeout(loadData,1500)}catch(e){}}

/* ---------- small widgets ---------- */
function seg(el,get,set,key){el.querySelectorAll("button").forEach(b=>{b.setAttribute("aria-pressed",b.dataset.v===get());b.onclick=()=>{change(key,()=>set(b.dataset.v));el.querySelectorAll("button").forEach(x=>x.setAttribute("aria-pressed",x===b))}})}
function range(id,get,set,fmt,key){const el=$(id),out=$(id+"Out");el.value=get();out.textContent=fmt(get());el.oninput=()=>{out.textContent=fmt(+el.value);change(key||id,()=>set(+el.value),500)}}
function tzOffset(tz){try{const d=new Date();const p=Object.fromEntries(new Intl.DateTimeFormat("en-US",{timeZone:tz,hourCycle:"h23",year:"numeric",month:"numeric",day:"numeric",hour:"numeric",minute:"numeric",second:"numeric"}).formatToParts(d).map(x=>[x.type,x.value]));return Math.round((Date.UTC(p.year,p.month-1,p.day,p.hour,p.minute,p.second)-d.getTime())/60000)*60}catch(e){return 0}}
function citySearch(input,box,onPick){let t=null,items=[],hl=-1;
 const render=()=>{box.innerHTML=items.map((r,i)=>`<button type="button" class="${i===hl?"hl":""}" data-i="${i}"><span class="cc">${esc(r.country_code||"··")}</span><span><b style="font-weight:500">${esc(r.name)}</b><small>${esc([r.admin1,r.country].filter(Boolean).join(", "))}</small></span></button>`).join("");box.classList.toggle("open",items.length>0)};
 const msg=(c,m)=>{box.innerHTML=`<button type="button" disabled><span class="cc">${c}</span><span>${m}</span></button>`;box.classList.add("open")};
 input.oninput=()=>{clearTimeout(t);const q=input.value.trim();if(q.length<2){items=[];render();return}
  t=setTimeout(async()=>{try{const r=await fetch(`https://geocoding-api.open-meteo.com/v1/search?name=${encodeURIComponent(q)}&count=8&language=en`).then(r=>r.json());items=r.results||[];hl=0;render();if(!items.length)msg("?","No places found")}catch(e){msg("!","Search needs internet on this phone or computer")}},250)};
 input.onkeydown=e=>{if(!items.length)return;if(e.key==="ArrowDown"){hl=Math.min(items.length-1,hl+1);render();e.preventDefault()}if(e.key==="ArrowUp"){hl=Math.max(0,hl-1);render();e.preventDefault()}if(e.key==="Enter"&&hl>=0){pick(hl);e.preventDefault()}if(e.key==="Escape"){items=[];render()}};
 const tap=e=>{const b=e.target.closest("button[data-i]");if(b){e.preventDefault();pick(+b.dataset.i)}};
 box.onmousedown=tap;box.addEventListener("touchstart",tap,{passive:false});
 input.onblur=()=>setTimeout(()=>box.classList.remove("open"),200);
 function pick(i){const r=items[i];items=[];render();input.value="";onPick(r)}}
const placeOf=r=>({name:r.name,country:r.country||"",cc:r.country_code||"",lat:r.latitude,lon:r.longitude});
const cityText=c=>`${c.name}${c.country?", "+c.country:""}`;

/* ---------- Screens tab ---------- */
function summary(k){
 if(k==="home")return `${cfg.city.name} · °${cfg.units.toUpperCase()} · ${cfg.h12?"12":"24"}-hour`;
 if(k==="clock")return cfg.worldClocks.map(w=>w.city).join(", ");
 if(k==="countdown"){const d=new Date(cfg.countdown.date);return `${cfg.countdown.label} · ${isNaN(d)?cfg.countdown.date:d.toLocaleDateString(undefined,{day:"numeric",month:"short",year:"numeric"})}`}
 return META[k][1]}
function settingsHtml(k){
 if(k==="home")return `<div><div class="lab"><span>City</span><span class="saved" data-for="city">Saved ✓</span></div><div class="search"><input type="text" class="hcity" placeholder="Type a city…" autocomplete="off" aria-label="City"><div class="results"></div></div></div>
  <div><div class="lab"><span>Temperature</span><span class="saved" data-for="units">Saved ✓</span></div><div class="seg hunits" role="group" aria-label="Temperature"><button data-v="c">°C</button><button data-v="f">°F</button></div></div>
  <div><div class="lab"><span>Clock</span><span class="saved" data-for="h12">Saved ✓</span></div><div class="seg hh12" role="group" aria-label="Clock"><button data-v="0">24-hour</button><button data-v="1">12-hour</button></div></div>`;
 if(k==="clock")return cfg.worldClocks.map((w,i)=>`<div><div class="lab"><span>City ${i+1}</span><span class="saved" data-for="wc${i}">Saved ✓</span></div><div class="search"><input type="text" class="wci" data-i="${i}" value="${esc(w.city)}" autocomplete="off" aria-label="World clock city ${i+1}"><div class="results"></div></div></div>`).join("");
 if(k==="countdown")return `<div><div class="lab"><label>Event</label><span class="saved" data-for="cdl">Saved ✓</span></div><input type="text" class="cdl" maxlength="24" value="${esc(cfg.countdown.label)}" aria-label="Event name"></div>
  <div><div class="lab"><label>Date and time</label><span class="saved" data-for="cdd">Saved ✓</span></div><input type="datetime-local" class="cdd" value="${esc(cfg.countdown.date.slice(0,16))}" aria-label="Event date and time"></div>`;
 return ""}
let openSet=new Set();
function renderList(){
 const on=new Set(cfg.screens),cur=st&&st.current;
 $("scrCount").textContent=`Screens · ${cfg.screens.length} of ${cfg.order.length} on`;
 $("list").innerHTML=cfg.order.map(k=>{const m=META[k];if(!m)return "";const hasSet=!!m[2],open=openSet.has(k);
  return `<div class="scr${on.has(k)?"":" off"}" data-k="${k}">
   <div class="shead">
    <button class="handle" aria-label="Move ${esc(m[0])} (use arrow keys)"><svg viewBox="0 0 24 24" fill="currentColor"><circle cx="9" cy="6" r="1.6"/><circle cx="15" cy="6" r="1.6"/><circle cx="9" cy="12" r="1.6"/><circle cx="15" cy="12" r="1.6"/><circle cx="9" cy="18" r="1.6"/><circle cx="15" cy="18" r="1.6"/></svg></button>
    <button class="sbody" aria-label="Show ${esc(m[0])} on the display now"><b>${esc(m[0])}${k===cur?'<span class="cur-badge">On screen</span>':""}</b><small>${esc(summary(k))}</small></button>
    ${hasSet?`<button class="more" aria-expanded="${open}" aria-label="${esc(m[0])} settings"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M6 9l6 6 6-6"/></svg></button>`:""}
    <label class="sw"><input type="checkbox" role="switch" ${on.has(k)?"checked":""} aria-label="Show ${esc(m[0])} in the rotation"><span></span></label>
   </div>${hasSet?`<div class="sset"${open?"":" hidden"}>${settingsHtml(k)}</div>`:""}</div>`}).join("");
 $("list").querySelectorAll(".scr").forEach(bindCard)}
function bindCard(card){
 const k=card.dataset.k;
 card.querySelector(".sbody").onclick=async()=>{try{showCurrent(k);await post("/api/show",{screen:k});toast(`Showing ${META[k][0]} on the display`);follow(k)}catch(e){}};
 card.querySelector(".sw input").onchange=e=>{const v=e.target.checked;change("screens",c=>{const s=new Set(c.screens);v?s.add(k):s.delete(k);if(!s.size)s.add("home");c.screens=c.order.filter(x=>s.has(x))},0);card.classList.toggle("off",!v)};
 const more=card.querySelector(".more");
 if(more)more.onclick=()=>{const set=card.querySelector(".sset"),o=set.hidden;set.hidden=!o;more.setAttribute("aria-expanded",o);o?openSet.add(k):openSet.delete(k)};
 if(k==="home"){const hc=card.querySelector(".hcity");citySearch(hc,hc.nextElementSibling,r=>change("city",c=>{c.city=placeOf(r);c.off=tzOffset(r.timezone)},0));
  seg(card.querySelector(".hunits"),()=>cfg.units,v=>cfg.units=v,"units");seg(card.querySelector(".hh12"),()=>cfg.h12?"1":"0",v=>cfg.h12=v==="1","h12")}
 if(k==="clock")card.querySelectorAll(".wci").forEach(inp=>{const i=+inp.dataset.i;citySearch(inp,inp.nextElementSibling,r=>change("wc"+i,c=>{c.worldClocks[i]={city:r.name,lat:r.latitude,lon:r.longitude,off:tzOffset(r.timezone)}},0))});
 if(k==="countdown"){card.querySelector(".cdl").oninput=e=>change("cdl",c=>c.countdown.label=e.target.value||"Countdown",900);card.querySelector(".cdd").onchange=e=>{if(e.target.value)change("cdd",c=>c.countdown.date=e.target.value.slice(0,16),0)}}
 // reorder: drag the handle (mouse or touch), or arrow keys on the handle
 const h=card.querySelector(".handle");
 h.onkeydown=e=>{if(e.key!=="ArrowUp"&&e.key!=="ArrowDown")return;e.preventDefault();const sib=e.key==="ArrowUp"?card.previousElementSibling:card.nextElementSibling;if(!sib)return;
  e.key==="ArrowUp"?card.parentNode.insertBefore(card,sib):card.parentNode.insertBefore(sib,card);h.focus();commitOrder()};
 h.onpointerdown=e=>{e.preventDefault();h.setPointerCapture(e.pointerId);card.classList.add("drag");let y0=e.clientY;
  const move=ev=>{let dy=ev.clientY-y0;const prev=card.previousElementSibling,next=card.nextElementSibling;
   if(next&&dy>next.offsetHeight/2+4){card.parentNode.insertBefore(next,card);y0+=next.offsetHeight+8;dy=ev.clientY-y0}
   if(prev&&dy<-(prev.offsetHeight/2+4)){card.parentNode.insertBefore(card,prev);y0-=prev.offsetHeight+8;dy=ev.clientY-y0}
   card.style.transform=`translateY(${dy}px)`};
  const up=()=>{h.removeEventListener("pointermove",move);h.removeEventListener("pointerup",up);h.removeEventListener("pointercancel",up);card.style.transform="";card.classList.remove("drag");commitOrder()};
  h.addEventListener("pointermove",move);h.addEventListener("pointerup",up);h.addEventListener("pointercancel",up)}}
function commitOrder(){const ord=[...$("list").children].map(c=>c.dataset.k);if(ord.join()===cfg.order.join())return;change("order",c=>{c.order=ord;const s=new Set(c.screens);c.screens=ord.filter(x=>s.has(x))},0)}

/* ---------- Device tab ---------- */
const hh=h=>String(h).padStart(2,"0")+":00";
function renderDevice(){
 range("rotate",()=>cfg.rotate,v=>cfg.rotate=v,v=>v?`${v} s`:"Never");
 seg($("rotation"),()=>cfg.autoRotate!==false?"auto":String(cfg.rotation||0),v=>{if(v==="auto")cfg.autoRotate=true;else{cfg.autoRotate=false;cfg.rotation=+v}},"rotation");
 for(const id of ["nFrom","nTo"]){const el=$(id);el.innerHTML=[...Array(24).keys()].map(h=>`<option value="${h}">${hh(h)}</option>`).join("");el.value=cfg.brightness[id==="nFrom"?"nightFrom":"nightTo"];
  el.onchange=()=>change("night",c=>c.brightness[id==="nFrom"?"nightFrom":"nightTo"]=+el.value,0)}
 range("bNight",()=>cfg.brightness.night,v=>cfg.brightness.night=v,v=>v+"%");
 range("bDay",()=>cfg.brightness.day,v=>cfg.brightness.day=v,v=>v+"%");
 $("nightSum").textContent=`${hh(cfg.brightness.nightFrom)}–${hh(cfg.brightness.nightTo)} · ${cfg.brightness.night}%`;
 $("citySum").textContent=`Showing ${cityText(cfg.city)} · ${(+cfg.city.lat).toFixed(2)}, ${(+cfg.city.lon).toFixed(2)}`;
 $("sWake").checked=!!cfg.soundWake;$("sSenseRow").hidden=!cfg.soundWake;
 $("sWake").onchange=e=>{$("sSenseRow").hidden=!e.target.checked;change("soundWake",c=>c.soundWake=e.target.checked,0)};
 seg($("sSense"),()=>String(cfg.soundSense??1),v=>cfg.soundSense=+v,"soundSense");
 $("auto").checked=cfg.update.auto;$("auto").onchange=e=>change("auto",c=>c.update.auto=e.target.checked,0)}
function renderNow(){
 const night=st&&st.night,b=cfg.brightness;
 $("bNowLab").textContent=night?"Brightness (night)":"Brightness";
 range("bNow",()=>night?b.night:b.day,v=>{night?b.night=v:b.day=v},v=>v+"%","bNow")}
function renderAll(){if(!cfg)return;renderList();renderDevice();renderNow()}
function refreshSummaries(){$("scrCount").textContent=`Screens · ${cfg.screens.length} of ${cfg.order.length} on`;
 document.querySelectorAll(".scr").forEach(c=>{c.querySelector(".sbody small").textContent=summary(c.dataset.k);c.classList.toggle("off",!cfg.screens.includes(c.dataset.k))});
 $("nightSum").textContent=`${hh(cfg.brightness.nightFrom)}–${hh(cfg.brightness.nightTo)} · ${cfg.brightness.night}%`;
 $("citySum").textContent=`Showing ${cityText(cfg.city)} · ${(+cfg.city.lat).toFixed(2)}, ${(+cfg.city.lon).toFixed(2)}`}

/* ---------- status ---------- */
const ago=s=>s<90?"just now":s<3600?Math.round(s/60)+" min ago":Math.round(s/3600)+" h ago";
let pending=null,lastData=null;
function showCurrent(cur){const ord=cfg?cfg.order.filter(k=>cfg.screens.includes(k)):[],i=ord.indexOf(cur);
 $("curName").textContent=cur&&META[cur]?META[cur][0]:"—";$("curPos").textContent=i>=0?`${i+1} of ${ord.length}`:"";
 document.querySelectorAll(".scr").forEach(c=>{const b=c.querySelector(".sbody b"),has=b.querySelector(".cur-badge");if(c.dataset.k===cur&&!has)b.insertAdjacentHTML("beforeend",'<span class="cur-badge">On screen</span>');if(c.dataset.k!==cur&&has)has.remove()})}
// after a tap: show the expected screen at once, then confirm with the display quickly
async function follow(expect){pending=expect;if(expect){showCurrent(expect);drawLive()}for(let i=0;i<12;i++){await new Promise(r=>setTimeout(r,250));await poll();if(!pending)break}pending=null}
async function poll(){try{st=await api("/api/status").then(r=>r.json())}catch(e){$("dot").classList.remove("on");$("online").textContent="Display not reachable";updLockTick(false);return}
 updLockTick(true);
 $("dot").classList.add("on");$("online").textContent="Online";
 $("upd1").hidden=st.lastSyncAgo<0;$("upd1").textContent=`Updated ${ago(st.lastSyncAgo)}`;
 $("upd2").hidden=st.nextSyncIn<0;$("upd2").textContent=st.nextSyncIn>60?`Next in ${Math.round(st.nextSyncIn/60)} min`:"Updating soon";
 if(!pending||st.current===pending){pending=null;showCurrent(st.current)}
 drawLive();
 if(st.data!==lastData){lastData=st.data;loadData()}
 $("hold").checked=!!st.hold;
 const sig=st.rssi>-60?"Strong":st.rssi>-72?"Good":"Weak";
 $("wifiSum").textContent=`${st.ssid} · ${sig}`;$("wifiInfo").textContent=`Connected to ${st.ssid} · signal ${sig.toLowerCase()} (${st.rssi} dBm) · ${st.ip}`;
 $("updSum").textContent=`v${st.version} · ${cfg&&cfg.update.auto?"Automatic":"Manual"}`;$("updState").textContent=(st.update?st.update:"No check yet")+(st.updating>=0?` · installing ${st.updating}%`:"");
 const av=st.available&&st.updating<0;$("check").textContent=av?`Install ${st.available} now`:"Check for updates now";$("check").classList.toggle("primary",!!av);
 $("updSum").textContent=av?`${st.available} available`:$("updSum").textContent;
 showVersion();
 const pw={encrypted:["Protected","ok","Wi-Fi password encrypted with a key locked inside the chip."],pending:["Pending","warn","Wi-Fi password gets encrypted after the next successful connection."],unavailable:["Not encrypted","bad","This chip has no free key slot, so the Wi-Fi password is stored unencrypted."]}[st.pwState]||["","",""];
 $("secSum").innerHTML=`<span class="${pw[1]}">${pw[0]}</span>`;$("pwInfo").textContent=pw[2];
 document.querySelectorAll(".gh").forEach(a=>a.href=st.projectUrl);$("lic").href=st.projectUrl+"/blob/main/LICENSE";$("kofi").href=st.donateUrl;$("ver").textContent="v"+st.version;
 if(cfg)renderNow()}

/* ---------- live view (drawn here from the display's data, so it is instant and sharp) ---------- */
let dataBusy=false,dataAt=0;
async function loadData(){if(dataBusy)return;dataBusy=true;try{SF.setData(await api("/api/data").then(r=>r.json()));dataAt=Date.now();drawLive()}catch(e){}dataBusy=false}
async function loadLand(){try{SF.setLand(await api("/api/land").then(r=>r.json()));drawLive()}catch(e){}}
function liveKey(){return pending||(st?st.current:"")}
function drawLive(){if(!cfg||document.hidden)return;SF.draw($("cv"),liveKey(),cfg.order.filter(k=>cfg.screens.includes(k)))}
// the exact picture (a photo of the display's memory), only when asked: copying it costs the display a little
function paint(canvas,buf,n){const b=new Uint16Array(buf);if(b.length<n*n)return;const ctx=canvas.getContext("2d"),img=ctx.createImageData(n,n),d=img.data;
 for(let i=0;i<n*n;i++){const v=b[i];d[i*4]=(v>>8&0xF8)|(v>>13);d[i*4+1]=(v>>3&0xFC)|(v>>9&3);d[i*4+2]=(v<<3&0xF8)|(v>>2&7);d[i*4+3]=255}ctx.putImageData(img,0,0)}
$("enl").onclick=async()=>{const c=$("cvBig");c.getContext("2d").clearRect(0,0,480,480);$("bigCap").textContent="Taking a picture of the display…";$("bigbox").classList.add("open");
 try{paint(c,await api("/api/screen?full=1").then(r=>r.arrayBuffer()),480);$("bigCap").textContent="Exact picture from the display"}catch(e){$("bigCap").textContent="Couldn't get the picture"}};
$("bigClose").onclick=()=>$("bigbox").classList.remove("open");
const nav=async d=>{const ord=cfg.order.filter(k=>cfg.screens.includes(k));let i=ord.indexOf(st&&st.current);const exp=i<0?null:ord[(i+d+ord.length)%ord.length];
 try{if(exp)showCurrent(exp);await post("/api/nav",{dir:d});follow(exp)}catch(e){}};
$("prev").onclick=()=>nav(-1);$("next").onclick=()=>nav(1);
$("hold").onchange=async e=>{try{await post("/api/hold",{on:e.target.checked});toast(e.target.checked?"Holding this screen":"Screens change again")}catch(x){}};
/* ---------- version status (header chip) and the yellow update bar ---------- */
let seenVersion=null;
function showVersion(){const v="v"+st.version,u=st.update||"",inst=st.updating>=0,av=st.available;
 if(seenVersion&&seenVersion!==st.version){toast(`Updated to ${v}`);setTimeout(()=>location.reload(),2500);return}   // new firmware: load its page   // just restarted on a new version
 seenVersion=st.version;
 const chip=$("ver0");chip.hidden=false;chip.classList.toggle("ok",!inst&&!av&&/^Up to date|^No releases/.test(u));chip.classList.toggle("new",!!(inst||av));
 $("ver0t").textContent=inst?`Installing ${st.available||"update"}`:av?`${v} · ${av} available`:/^Checking/.test(u)?`${v} · checking for updates…`:/^Up to date|^No releases/.test(u)?`${v} · up to date`:u?`${v} · couldn't check`:v;
 const bar=$("updbar");bar.hidden=!(inst||av);if(bar.hidden)return;
 $("updbarNotes").href=`${st.projectUrl}/releases/tag/${encodeURIComponent(av||"")}`;
 $("updbarActs").hidden=inst;$("updbarBar").hidden=!inst;
 if(inst){const ph=st.phase|0;$("updbarFill").style.width=(ph>=2?100:st.updating)+"%";
  $("updbarText").innerHTML=ph>=3?`<b>Installed ${esc(av)}.</b> The display is restarting…`:ph==2?`<b>Installing ${esc(av)}.</b> The screen turns off for a few seconds, then restarts.`:`<b>Downloading ${esc(av)}</b> · ${st.updating}%. Keep the display powered.`}
 else $("updbarText").innerHTML=`<b>SkyFrame ${esc(av)} is available.</b> This display has ${esc(v)}.`}
// full-screen box for updates: asks first, then blocks the page with progress until the display is back on the
// new version; it can be cancelled while it checks or downloads (not while flash is written)
let ul=null;   // {target, from, stage:"confirm"|"run", t0, lost, cancelling}
const ulShow=(id,on)=>{$(id).hidden=!on};
function startUpdate(){const target=st&&st.available;if(!target)return;ul={target,from:st.version,stage:"confirm"};showLock()}
function showLock(){const L=$("updlock");L.hidden=false;ulShow("ulClose",false);
 if(ul.stage==="confirm"){
  $("ulTitle").textContent=`Install ${ul.target}?`;["ulPct","ulSpin","ulBar"].forEach(x=>ulShow(x,false));
  $("ulMsg").textContent=`This display has v${ul.from}. It downloads the new version, checks its signature, installs it and restarts, in about a minute. Your settings are kept.`;
  ulShow("ulNote",false);ulShow("ulGo",true);ulShow("ulCancel",true);$("ulCancel").textContent="Cancel";setTimeout(()=>$("ulGo").focus(),50);return}
 const ph=st&&st.phase|0,pct=st&&st.updating>=0?st.updating:0,indet=ul.lost||ph>=2||pct===0;
 $("ulTitle").textContent=`Updating to ${ul.target}`;ulShow("ulNote",true);ulShow("ulGo",false);
 ulShow("ulPct",!indet);ulShow("ulSpin",indet);ulShow("ulBar",!indet);$("ulPct").textContent=pct+"%";$("ulFill").style.width=pct+"%";
 ulShow("ulCancel",!ul.lost&&ph<2&&!ul.cancelling);$("ulCancel").textContent="Cancel update";
 $("ulMsg").textContent=ul.cancelling?"Cancelling…":ul.lost?"The display is restarting on the new version… reconnecting":ph>=3?"Installed. Restarting…":ph==2?"Installing. The display's screen turns off for a few seconds.":pct>0?"Downloading the new version":"Checking the new version…"}
function closeLock(msg){$("updlock").hidden=true;ul=null;if(msg)toast(msg)}
function lockFail(msg){$("ulTitle").textContent="The update didn't finish";["ulPct","ulSpin","ulBar","ulGo","ulCancel"].forEach(x=>ulShow(x,false));
 $("ulMsg").textContent=!msg||/^Version|^Up to date/.test(msg)?"The display didn't start the update. Please try again.":msg;ulShow("ulClose",true);ul=null}
$("ulGo").onclick=()=>{if(!ul)return;ul.stage="run";ul.t0=Date.now();ul.lost=false;showLock();post("/api/update/check").catch(()=>{});setTimeout(poll,800)};
$("ulCancel").onclick=async()=>{if(!ul)return;if(ul.stage==="confirm")return closeLock();
 ul.cancelling=true;showLock();try{await post("/api/update/cancel")}catch(e){}setTimeout(poll,600)};
$("ulClose").onclick=()=>{$("updlock").hidden=true};
document.addEventListener("keydown",e=>{if(e.key==="Escape"&&ul&&ul.stage==="confirm")closeLock()});
function updLockTick(ok){if(!ul&&ok&&st.updating>=0&&st.available)ul={target:st.available,from:st.version,stage:"run",t0:Date.now(),lost:false};   // page opened mid-update
 if(!ul||ul.stage!=="run")return;
 if(!ok){ul.lost=true;showLock();return}
 if("v"+st.version===ul.target||(st.version!==ul.from&&st.updating<0)){closeLock(`Updated to v${st.version}`);return}
 ul.lost=false;
 if(ul.cancelling&&st.updating<0){closeLock("Update cancelled");return}
 if(st.updating<0&&Date.now()-ul.t0>12000&&!/^Starting|^Checking/.test(st.update||"")){lockFail(st.update);return}
 showLock()}
$("updbarGo").onclick=startUpdate;
$("check").onclick=async()=>{if(st&&st.available)return startUpdate();try{await post("/api/update/check");$("updState").textContent="Checking GitHub…";setTimeout(poll,5000)}catch(e){}};
$("forget").onclick=async()=>{if(!confirm("Forget this Wi-Fi network and restart in setup mode? You'll set up Wi-Fi again from the display."))return;try{await post("/api/wifi/forget");toast("Restarting in setup mode…")}catch(e){}};
$("newpin").onclick=async()=>{if(!confirm("Make a new pairing code? Other phones and computers will need to enter it."))return;try{const r=await post("/api/pin/new").then(r=>r.json());PIN=r.pin;try{localStorage.setItem("pin",PIN)}catch(e){}toast("New pairing code: "+r.pin)}catch(e){}};
citySearch($("city"),$("cityResults"),r=>change("city",c=>{c.city=placeOf(r);c.off=tzOffset(r.timezone)},0));

/* ---------- tabs ---------- */
function showTab(t){document.querySelectorAll(".tab").forEach(b=>b.setAttribute("aria-selected",b.dataset.tab===t));
 const wide=matchMedia("(min-width:900px)").matches;if(wide&&t==="now")return showTab("screens");
 ["now","screens","device"].forEach(v=>$(v).classList.toggle("on",v===t));try{localStorage.setItem("tab",t)}catch(e){}}
document.querySelectorAll(".tab").forEach(b=>b.onclick=()=>showTab(b.dataset.tab));

/* ---------- welcome (first visit in this browser) ---------- */
function welcome(){let w;try{w=localStorage.getItem("welcomed")}catch(e){w="1"}if(w)return;
 const draft={city:null,units:cfg.units,h12:cfg.h12};$("welcome").classList.add("open");$("wcitySum").textContent=`Now: ${cityText(cfg.city)}`;
 citySearch($("wcity"),$("wcityResults"),r=>{draft.city=r;$("wcitySum").textContent=`Selected: ${r.name}${r.country?", "+r.country:""}`});
 const s1=$("wunits"),s2=$("wh12");
 s1.querySelectorAll("button").forEach(b=>{b.setAttribute("aria-pressed",b.dataset.v===draft.units);b.onclick=()=>{draft.units=b.dataset.v;s1.querySelectorAll("button").forEach(x=>x.setAttribute("aria-pressed",x===b))}});
 s2.querySelectorAll("button").forEach(b=>{b.setAttribute("aria-pressed",b.dataset.v===(draft.h12?"1":"0"));b.onclick=()=>{draft.h12=b.dataset.v==="1";s2.querySelectorAll("button").forEach(x=>x.setAttribute("aria-pressed",x===b))}});
 const close=()=>{$("welcome").classList.remove("open");try{localStorage.setItem("welcomed","1")}catch(e){}};
 $("wskip").onclick=close;
 $("wdone").onclick=()=>{change("welcome",c=>{if(draft.city){c.city=placeOf(draft.city);c.off=tzOffset(draft.city.timezone)}c.units=draft.units;c.h12=draft.h12},0);close()}}

/* ---------- start ---------- */
$("pingo").onclick=async()=>{PIN=$("pin").value.trim();try{localStorage.setItem("pin",PIN)}catch(e){}
 const r=await fetch("/api/auth",{headers:{"X-Pin":PIN}});
 if(r.ok){$("pinbox").classList.remove("open");start()}else $("pinmsg").textContent=r.status===429?"Too many wrong codes. Wait a minute and try again.":"That code isn't right. Hold the display for 1 second to see it."};
$("pin").onkeydown=e=>{if(e.key==="Enter")$("pingo").click()};
let running=false;
async function start(){if(running)return;try{await api("/api/auth")}catch(e){return}running=true;
 cfg=await api("/api/config").then(r=>r.json());await poll();renderAll();
 let t;try{t=localStorage.getItem("tab")}catch(e){}showTab(t||"now");
 await loadData();loadLand();welcome();post("/api/update/peek").then(()=>setTimeout(poll,6000)).catch(()=>{});if(document.fonts)Promise.all([300,400,500].map(w=>document.fonts.load(`${w} 16px Inter`))).then(drawLive,()=>{});
 setInterval(()=>{if(!document.hidden&&!pending)poll()},2000);setInterval(()=>{if(ul)poll()},1000);setInterval(drawLive,1000);setInterval(()=>{if(Date.now()-dataAt>60000)loadData()},10000);
 document.addEventListener("visibilitychange",()=>{if(!document.hidden){poll();loadData()}})}
start();
</script></body></html>
)HTML";

static const char SETUP_HTML[] = R"HTML(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>SkyFrame setup</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#000;color:#f4f5f7;font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
.wrap{max-width:440px;margin:0 auto;padding:32px 16px}
h1{font-size:26px;margin:0 0 6px}p{color:#7d8590;margin:0 0 22px}
.net{display:flex;justify-content:space-between;align-items:center;width:100%;background:#0d0f13;border:1px solid #1f232b;border-radius:12px;padding:14px 16px;margin-bottom:8px;color:#f4f5f7;font:inherit;cursor:pointer;text-align:left}
.net.on{border-color:#f6c945}.net small{color:#7d8590}
input{width:100%;background:#000;border:1px solid #1f232b;color:#f4f5f7;border-radius:10px;padding:14px;font:inherit;margin:14px 0}
button.go{width:100%;background:#f6c945;color:#000;border:0;border-radius:10px;padding:14px;font:600 16px system-ui,sans-serif;cursor:pointer}
button.go:disabled{opacity:.4}
.msg{color:#c9d6e6;margin-top:16px}
</style></head><body><div class="wrap">
<h1>Connect SkyFrame</h1><p>Choose your home Wi-Fi network. The display remembers it.</p>
<div id="list">Looking for networks…</div>
<input type="text" id="ssid" placeholder="Network name" autocomplete="off" autocapitalize="none">
<input type="password" id="pass" placeholder="Wi-Fi password" autocomplete="off">
<button class="go" id="go">Connect</button>
<div class="msg" id="msg"></div>
</div><script>
const $=id=>document.getElementById(id);
const esc=s=>String(s).replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));
async function scan(){try{const n=await fetch("/api/scan").then(r=>r.json());
 $("list").innerHTML=n.length?n.map(x=>`<button class="net" data-s="${esc(x.ssid)}"><span>${esc(x.ssid)}</span><small>${x.rssi>-60?"strong":x.rssi>-72?"good":"weak"}${x.open?" · open":""}</small></button>`).join(""):"No networks found. Type the name below.";
 document.querySelectorAll(".net").forEach(b=>b.onclick=()=>{document.querySelectorAll(".net").forEach(x=>x.classList.remove("on"));b.classList.add("on");$("ssid").value=b.dataset.s;$("pass").focus()})}catch(e){setTimeout(scan,2000)}}
$("go").onclick=async()=>{const ssid=$("ssid").value.trim();if(!ssid)return;$("go").disabled=true;
 await fetch("/api/wifi",{method:"POST",body:JSON.stringify({ssid,pass:$("pass").value})}).catch(()=>{});
 $("msg").innerHTML="Saved. The display is restarting and joining <b>"+esc(ssid)+"</b>.<br><br>Reconnect this phone to your home Wi-Fi, then open <b>http://skyframe.local</b> to choose your city.";}
scan();
</script></body></html>)HTML";
