const POLL_MS = 2000;
let map, marker;
let firstGps = true;
let relayState = null;
let commandPending = false;

function initMap() {
  map = L.map('map').setView([-8.65, 116.42], 10);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: '&copy; OpenStreetMap contributors'
  }).addTo(map);
}

function setStatusBox(el, type, text) {
  el.classList.remove('green','red','gray','yellow');
  el.classList.add(type);
  el.innerHTML = '<i></i>' + text;
}

function appendLog(msg) {
  const log = document.getElementById('log');
  const time = new Date().toLocaleTimeString('id-ID');
  log.textContent = `[${time}] ${msg}\n` + log.textContent;
}

function setConnection(connected) {
  const dot = document.getElementById('dot');
  const conn = document.getElementById('conn');
  dot.className = connected ? 'dot on' : 'dot off';
  conn.textContent = connected ? 'ESP32 TERHUBUNG' : 'ESP32 TIDAK TERHUBUNG';
  conn.className = connected ? 'online-text' : 'offline-text';
}

async function refreshStatus() {
  try {
    const res = await fetch('api/get_status.php?t=' + Date.now(), {cache:'no-store'});
    if (!res.ok) throw new Error('HTTP ' + res.status);
    const data = await res.json();
    const connected = data.connected === true;
    setConnection(connected);

    if (!connected) {
      setStatusBox(document.getElementById('vibration'),'gray','OFFLINE');
      setStatusBox(document.getElementById('relay'),'gray','OFFLINE');
      document.getElementById('gps').textContent = 'OFFLINE';
      document.getElementById('updated').textContent = 'ESP32 tidak mengirim heartbeat';
      updateRelaySwitch(null, true);
      return;
    }

    updateRelaySwitch(data.relay === 1, false);

    if (data.getaran === 1) {
      setStatusBox(document.getElementById('vibration'),'red','TERDETEKSI');
    } else {
      setStatusBox(document.getElementById('vibration'),'green','NORMAL');
    }

    setStatusBox(document.getElementById('relay'), data.relay === 0 ? 'green':'red', data.relay === 0 ? 'TERHUBUNG':'TERPUTUS');
    document.getElementById('gps').textContent = data.gps_online === 1 ? 'ONLINE' : 'OFFLINE';

    if (data.lat !== null && data.lon !== null && Number.isFinite(Number(data.lat)) && Number.isFinite(Number(data.lon))) {
      document.getElementById('lat').textContent = Number(data.lat).toFixed(6);
      document.getElementById('lon').textContent = Number(data.lon).toFixed(6);
      document.getElementById('updated').textContent = 'Update terakhir: ' + (data.updated_at || '-');
      const latlng = [Number(data.lat), Number(data.lon)];
      if (!marker) {
        marker = L.marker(latlng).addTo(map).bindPopup('Lokasi kendaraan').openPopup();
      } else marker.setLatLng(latlng);
      if (firstGps) { map.setView(latlng,16); firstGps = false; }
    }
  } catch (err) {
    setConnection(false);
    setStatusBox(document.getElementById('vibration'),'gray','OFFLINE');
    setStatusBox(document.getElementById('relay'),'gray','OFFLINE');
    document.getElementById('gps').textContent = 'OFFLINE';
    updateRelaySwitch(null, true);
    appendLog('Gagal mengambil status: ' + err.message);
  }
}

function updateRelaySwitch(isCut, unavailable) {
  const switchInput = document.getElementById('relaySwitch');
  const label = document.getElementById('relaySwitchLabel');
  const hint = document.getElementById('relaySwitchHint');
  switchInput.disabled = unavailable || commandPending;
  if (isCut === null) {
    switchInput.checked = false;
    label.textContent = 'Relay tidak tersedia';
    hint.textContent = 'Menunggu koneksi ESP32';
    return;
  }
  relayState = isCut;
  switchInput.checked = !isCut;
  label.textContent = isCut ? 'RELAY PUTUS' : 'RELAY TERHUBUNG';
  hint.textContent = isCut ? 'Switch OFF - arus kendaraan diputus' : 'Switch ON - arus kendaraan tersambung';
}

async function toggleRelay(isConnected) {
  if (commandPending || relayState === null) return;
  const isCut = !isConnected;
  const previousState = relayState;
  commandPending = true;
  updateRelaySwitch(isCut, false);
  document.getElementById('relaySwitchHint').textContent = 'Mengirim perintah...';
  try {
    const res = await fetch('api/send_command.php', {
      method:'POST', headers:{'Content-Type':'application/json'},
      body:JSON.stringify({command:'relay', value:Boolean(isCut)})
    });
    const data = await res.json();
    if (!res.ok || data.status !== 'ok') throw new Error(data.error || 'Perintah gagal');
    appendLog(isCut ? 'Perintah PUTUS RELAY dikirim, menunggu ESP32...' : 'Perintah SAMBUNG RELAY dikirim, menunggu ESP32...');
    await waitForCommand(data.command_id, isCut);
  } catch(err) {
    updateRelaySwitch(previousState, false);
    appendLog('Gagal mengirim perintah relay: ' + err.message);
  } finally {
    commandPending = false;
    if (relayState !== null) updateRelaySwitch(relayState, false);
  }
}

async function waitForCommand(commandId, isCut) {
  const deadline = Date.now() + 10000;

  while (Date.now() < deadline) {
    await new Promise(resolve => setTimeout(resolve, 700));
    const res = await fetch('api/command_status.php?id=' + encodeURIComponent(commandId) + '&t=' + Date.now(), {cache:'no-store'});
    if (!res.ok) throw new Error('Status command HTTP ' + res.status);
    const data = await res.json();

    if (data.executed) {
      if (data.result !== 'applied') throw new Error('ESP32 menolak perintah karena sensor masih mendeteksi getaran');
      appendLog(isCut ? 'ESP32 mengonfirmasi relay PUTUS' : 'ESP32 mengonfirmasi relay TERHUBUNG');
      await refreshStatus();
      return;
    }
  }

  throw new Error('ESP32 belum mengonfirmasi perintah setelah 10 detik');
}

initMap();
refreshStatus();
setInterval(refreshStatus, POLL_MS);
