#  Sistem Keamanan Kendaraan Berbasis ESP32

Sistem keamanan kendaraan yang mendeteksi getaran/guncangan tidak wajar pada kendaraan, otomatis memutus aliran listrik (relay), membunyikan alarm, dan melacak lokasi kendaraan lewat GPS — semua bisa dipantau dan dikontrol dari dashboard web secara real-time.

##  Fitur

- **Deteksi getaran real-time** menggunakan sensor getaran (SW-420) yang dibaca lewat interrupt, sehingga respons terjadi seketika tanpa delay dari loop program.
- **Pemutus relay otomatis** — begitu getaran terdeteksi, relay langsung memutus aliran listrik kendaraan dan buzzer alarm menyala.
- **Bekerja tanpa internet** — respons keamanan (relay + buzzer) sepenuhnya diproses lokal di ESP32, tidak bergantung pada koneksi ke server.
- **Pelacakan lokasi GPS** menggunakan modul GPS NEO-6M (via TinyGPS++), ditampilkan di peta interaktif (Leaflet + OpenStreetMap) pada dashboard.
- **Dashboard web real-time** (PHP + MySQL) menampilkan status getaran, status relay, status koneksi ESP32 (heartbeat), dan lokasi GPS, dengan polling otomatis tiap 2 detik.
- **Kontrol relay jarak jauh** — relay bisa disambungkan kembali dari dashboard, dikirim sebagai command yang diambil (poll) oleh ESP32 dan dikonfirmasi lewat sistem ACK.
- **Dual endpoint (server utama + lokal)** — ESP32 otomatis mencoba alamat lokal (fallback) jika server utama tidak bisa dihubungi.
- **Migrasi database otomatis** — tabel MySQL dibuat dan diperbaiki otomatis oleh `db.php`, tidak perlu import SQL manual.

##  Arsitektur & Teknologi

| Bagian | Teknologi |
|---|---|
| Firmware | ESP32 (Arduino, C++), TinyGPS++, ArduinoJson, HTTPClient |
| Sensor | Sensor getaran SW-420, GPS NEO-6M |
| Aktuator | Relay module, buzzer DC |
| Backend | PHP + MySQL (mysqli, prepared statements) |
| Frontend | HTML/CSS/JS vanilla + Leaflet.js (peta) |
| Komunikasi | HTTP polling (bukan MQTT) antara ESP32 ↔ server |

Alur singkat: ESP32 membaca sensor getaran & GPS → mengirim status ke server via `update_status.php` tiap ±5 detik → dashboard mem-poll `get_status.php` tiap 2 detik untuk menampilkan status terkini → perintah dari dashboard (misal sambungkan relay) disimpan lewat `send_command.php`, diambil ESP32 lewat `get_command.php`, lalu dikonfirmasi lewat `ack_command.php`.

##  Struktur Project

```
├── ESP32/
│   └── sistem_keamanan.ino   # Firmware ESP32
├── api/
│   ├── update_status.php     # Terima status dari ESP32
│   ├── get_status.php        # Kirim status terkini ke dashboard
│   ├── send_command.php      # Dashboard mengirim perintah relay
│   ├── get_command.php       # ESP32 mengambil perintah tertunda
│   ├── ack_command.php       # ESP32 konfirmasi perintah sudah dijalankan
│   └── command_status.php    # Cek status eksekusi sebuah perintah
├── db.php                    # Koneksi & migrasi database otomatis
├── schema.sql                # Dokumentasi struktur tabel (opsional, tidak wajib di-import)
├── index.php                 # Halaman dashboard
├── app.js                    # Logic frontend (polling, peta, kontrol relay)
└── style.css                 # Styling dashboard
```

##  Instalasi & Setup

### 1. Server (dashboard)
1. Clone repo ini ke folder `htdocs` XAMPP (atau hosting PHP + MySQL lain).
2. Buat database MySQL, lalu sesuaikan kredensial di `db.php` (`$DB_HOST`, `$DB_USER`, `$DB_PASS`, `$DB_NAME`). Tabel akan dibuat otomatis saat pertama kali diakses.
3. Jalankan Apache & MySQL, lalu akses `index.php` lewat browser.

### 2. Firmware ESP32
1. Buka `ESP32/sistem_keamanan.ino` di Arduino IDE.
2. Install library: `TinyGPS++`, `ArduinoJson`, `Preferences` (built-in ESP32 core).
3. Isi `ssid` dan `password` WiFi kamu di bagian atas file.
4. Upload ke board ESP32.
5. Set alamat server lewat Serial Monitor dengan perintah:
   ```
   link="https://domain-kamu.com/nama-folder/api"
   ```
   atau untuk alamat lokal jaringan:
   ```
   linklocal="http://192.168.1.10/nama-folder/api"
   ```

### 3. Pemetaan pin
| Komponen | Pin ESP32 |
|---|---|
| Sensor getaran | 27 |
| Relay | 26 |
| Buzzer | 25 |
| GPS RX / TX | 16 / 17 |

##  Akses dari luar jaringan lokal (opsional)

Jika server berjalan di localhost/XAMPP, gunakan tunnel seperti `localhost.run` agar bisa diakses ESP32 dari luar:
```
ssh -R 80:localhost:80 nokey@localhost.run
```

##  Catatan

- Perintah dari dashboard hanya diperbolehkan untuk kontrol relay (`command = "relay"`), demi keamanan.
- Getaran yang terdeteksi selalu diproses langsung di ESP32 lewat interrupt — tidak menunggu respons server, sehingga sistem tetap aman meski internet mati.
- ESP32 dianggap offline oleh dashboard jika tidak mengirim heartbeat selama lebih dari 20 detik.

##  Author

Dikembangkan oleh **Syaepuddin** — Mahasiswa Teknik Komputer, Universitas Hamzanwadi.
