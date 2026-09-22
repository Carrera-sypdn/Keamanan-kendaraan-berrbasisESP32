#  Sistem Keamanan Kendaraan Berbasis ESP32

Sistem keamanan kendaraan yang mendeteksi getaran/percobaan pencurian, memutus tegangan kendaraan secara otomatis, mengirim posisi GPS, dan bisa dipantau/dikontrol lewat dashboard web — dengan respons alarm yang **tetap jalan secara lokal di ESP32** walaupun internet atau server sedang mati.

##  Fitur

- **Deteksi getaran** via sensor (pin 27) — saat getaran terdeteksi, ESP32 langsung memutus relay (pin 26) dan membunyikan buzzer (pin 25) tanpa menunggu respons server.
- **GPS real-time** menggunakan modul NEO-6M (TinyGPS++) untuk melacak lokasi kendaraan.
- **Kontrol relay jarak jauh** dari dashboard web (nyala/matikan aliran listrik kendaraan).
- **Remote RF (RX500)** sebagai kontrol tambahan selain dashboard.
- **Dashboard web** (PHP + MySQL) menampilkan status getaran, relay, koneksi ESP32, dan posisi GPS di peta (Leaflet).
- **Auto-migrasi database** — tabel dibuat/diperbaiki otomatis oleh `db.php`, tidak perlu import `schema.sql` manual.
- **Link server dinamis** — ESP32 membaca base URL dashboard dari GitHub Gist, jadi tidak perlu flash ulang firmware setiap kali link tunnel (ngrok/localhost.run) berubah.
- **Komunikasi via HTTP polling**, bukan MQTT — dipilih karena hosting gratis (mis. AeonFree) memblokir koneksi MQTT ke ESP32 lewat JS-challenge anti-bot.

##  Struktur Folder

```
├── ESP32/
│   └── sistem_keamanan/
│       └── sistem_keamanan.ino   # Firmware ESP32 (Arduino)
├── api/
│   ├── get_status.php            # Ambil status terbaru ESP32 untuk dashboard
│   ├── update_status.php         # ESP32 mengirim status (getaran, relay, GPS) ke server
│   ├── send_command.php          # Dashboard mengirim perintah relay ke ESP32
│   ├── get_command.php           # ESP32 mengambil perintah relay yang tertunda
│   ├── ack_command.php           # ESP32 mengonfirmasi perintah sudah dijalankan
│   └── command_status.php        # Cek status eksekusi sebuah perintah
├── db.php                        # Koneksi database + auto-migrasi skema
├── schema.sql                    # Dokumentasi struktur tabel (tidak wajib di-import)
├── index.php                     # Halaman dashboard
├── app.js                        # Logika frontend dashboard (polling status, kontrol relay, peta)
├── style.css                     # Styling dashboard
├── start_all.bat                 # Menyalakan Apache/MySQL (XAMPP) + SSH tunnel
├── stop_all.bat                  # Mematikan semuanya
└── cmd.txt                       # Contoh perintah SSH tunnel (localhost.run)
```

##  Cara Kerja Singkat

1. ESP32 membaca sensor getaran & GPS, lalu setiap ±5 detik mengirim status ke `api/update_status.php` (heartbeat).
2. Dashboard (`index.php` + `app.js`) polling `api/get_status.php` untuk menampilkan status terkini dan lokasi di peta.
3. Saat getaran terdeteksi: **relay langsung diputus dan buzzer dibunyikan oleh ESP32 sendiri** (di dalam interrupt handler), tidak menunggu request ke server — ini yang membuat alarm tetap bekerja walau internet mati.
4. Kontrol relay manual dari dashboard dikirim lewat `api/send_command.php`, lalu ESP32 mengambilnya via `api/get_command.php` dan mengonfirmasi lewat `api/ack_command.php`.
5. Karena link tunnel (ngrok/localhost.run) berubah setiap restart, base URL server disimpan di GitHub Gist dan dibaca otomatis oleh ESP32 (`Preferences`) — tidak perlu flash ulang firmware setiap ganti link.

##  Kebutuhan

**Hardware:**
- ESP32
- Sensor getaran (SW-420 atau sejenis) di pin 27
- Modul relay di pin 26
- Buzzer DC di pin 25
- Modul GPS NEO-6M (RX/TX di pin 16/17)
- Modul penerima RF RX500 (opsional, untuk remote)

**Software:**
- Arduino IDE / PlatformIO dengan library: `TinyGPS++`, `WiFi`, `WiFiClientSecure`, `HTTPClient`, `ArduinoJson`, `Preferences`, `RCSwitch`
- XAMPP (Apache + MySQL/PHP) untuk menjalankan backend secara lokal
- SSH client (untuk tunnel `localhost.run`) atau ngrok, agar ESP32 bisa mengakses server lokal dari luar jaringan

##  Instalasi & Menjalankan

1. **Database** — buat database MySQL (nama sesuai `db.php`, default: `keamanan_kendaraan`). Struktur tabel dibuat otomatis saat `db.php` pertama kali diakses, jadi `schema.sql` tidak wajib di-import.
2. **Konfigurasi `db.php`** — sesuaikan `$DB_HOST`, `$DB_USER`, `$DB_PASS`, `$DB_NAME` dengan kredensial database kamu. **Jangan commit kredensial database produksi ke repo publik.**
3. **Firmware ESP32** — buka `ESP32/sistem_keamanan/sistem_keamanan.ino` di Arduino IDE, isi SSID/password WiFi sendiri, lalu upload ke board. Base URL dashboard diambil otomatis dari Gist yang dikonfigurasi di kode.
4. **Jalankan server** — di Windows, edit path `XAMPP_DIR` di `start_all.bat` sesuai lokasi instalasi XAMPP, lalu jalankan `start_all.bat` (otomatis menyalakan Apache, MySQL, dan membuka SSH tunnel ke `localhost.run`). Gunakan `stop_all.bat` untuk menghentikan semuanya.
5. **Akses dashboard** — buka URL tunnel yang muncul (atau `http://localhost/`) di browser.

## 🔌 Ringkasan API

| Endpoint | Method | Dipanggil oleh | Fungsi |
|---|---|---|---|
| `api/update_status.php` | POST | ESP32 | Mengirim status getaran, relay, GPS (heartbeat) |
| `api/get_status.php` | GET | Dashboard | Mengambil status terbaru untuk ditampilkan |
| `api/send_command.php` | POST | Dashboard | Mengirim perintah kontrol relay |
| `api/get_command.php` | GET | ESP32 | Mengambil perintah relay yang belum dieksekusi |
| `api/ack_command.php` | POST | ESP32 | Konfirmasi perintah sudah/tidak dijalankan |
| `api/command_status.php` | GET | Dashboard | Cek status eksekusi sebuah perintah |

##  Catatan Keamanan

- Jangan commit SSID/password WiFi asli maupun kredensial database produksi ke repository publik — gunakan nilai placeholder di kode yang diunggah.
- `send_command.php` sengaja dibatasi hanya menerima perintah `relay` dari web, untuk mencegah penyalahgunaan endpoint.

##  Status

Proyek dalam pengembangan aktif — kontribusi/perubahan berikutnya dicatat lewat commit history di repo ini.
##  Author

Dikembangkan oleh **Syaepuddin** — Mahasiswa Teknik Komputer, Universitas Hamzanwadi.
