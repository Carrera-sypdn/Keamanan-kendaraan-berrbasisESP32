<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sistem Keamanan Kendaraan</title>
<link rel="stylesheet" href="style.css">
<link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">
</head>
<body>
<header class="topbar">
  <div class="brand"><div class="logo">🏍️</div><div>
    <h1>SISTEM KEAMANAN KENDARAAN</h1>
    <p>Monitoring • Lokasi • GPS • Kontrol Relay</p>
  </div></div>
  <div class="connection"><i id="dot" class="dot off"></i><span id="conn">ESP32 TIDAK TERHUBUNG</span></div>
</header>

<main>
<section class="cards">
  <article class="card"><div class="icon green">〽</div><h2>Getaran</h2><div id="vibration" class="status gray"><i></i>OFFLINE</div></article>
  <article class="card"><div class="icon red">⚡</div><h2>Relay</h2><div id="relay" class="status gray"><i></i>OFFLINE</div></article>
</section>

<section class="panel controls">
  <h2>⚡ KONTROL RELAY</h2>
  <p class="warning">Getaran selalu diproses langsung oleh ESP32. Jika getaran terdeteksi, relay diputus meskipun internet/server mati.</p>
  <div class="relay-control">
    <label class="switch-row" for="relaySwitch">
      <span><b id="relaySwitchLabel">Relay tidak tersedia</b><small id="relaySwitchHint">Menunggu koneksi ESP32</small></span>
      <input id="relaySwitch" type="checkbox" role="switch" onchange="toggleRelay(this.checked)" disabled>
      <span class="switch-slider" aria-hidden="true"></span>
    </label>
  </div>
</section>

<section class="location">
  <article class="panel"><h2>📍 LOKASI & GPS</h2><div class="gps-lines">
    <div><b>Status GPS:</b> <span id="gps">OFFLINE</span></div>
    <div><b>Latitude:</b> <span id="lat">-</span></div>
    <div><b>Longitude:</b> <span id="lon">-</span></div>
    <p id="updated">Belum ada data GPS</p>
  </div><div id="map"></div></article>
</section>

<section class="panel"><h2>📋 STATUS KONEKSI</h2><pre id="log">Menunggu koneksi ESP32...</pre></section>
</main>
<footer>ESP32 + SW-420 + GPS NEO-6M + Relay + PHP/MySQL</footer>
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<script src="app.js"></script>
</body></html>
