#include <TinyGPS++.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// =====================================================
// WIFI
// =====================================================
const char* ssid     = "NAMA WIFI";
const char* password = "PASSWORD";

// =====================================================
// PENYIMPANAN LINK WEB
// =====================================================
Preferences preferences;
String baseUrl = "";
String localBaseUrl = "";

// =====================================================
// PIN
// =====================================================
#define PIN_SENSOR 27
#define PIN_RELAY  26
#define PIN_BUZZER 25

// GPS
#define GPS_RX 16
#define GPS_TX 17

// =====================================================
// GPS
// =====================================================
TinyGPSPlus gps;
HardwareSerial GPS_Serial(2);

// =====================================================
// STATUS SISTEM
// =====================================================
int statusGetaran = LOW;

bool relayTerputus = false;
volatile bool relayManualTerputus = false;
volatile bool overrideGetaran = false;
unsigned long overrideGetaranMulai = 0;
const unsigned long OVERRIDE_GETARAN_MS = 5000;

volatile bool getaranInterrupt = false;
volatile unsigned long getaranInterruptTerakhir = 0;
const unsigned long GETARAN_DEBOUNCE = 150;
unsigned long getaranNormalSejak = 0;
const unsigned long GETARAN_NORMAL_DELAY = 500;

void IRAM_ATTR deteksiGetaranInterrupt() {

  unsigned long sekarang = millis();

  if (
    !overrideGetaran &&
    !relayManualTerputus &&
    digitalRead(PIN_SENSOR) == HIGH &&
    sekarang - getaranInterruptTerakhir >= GETARAN_DEBOUNCE
  ) {

    getaranInterruptTerakhir = sekarang;
    relayTerputus = true;
    digitalWrite(PIN_RELAY, HIGH);
    digitalWrite(PIN_BUZZER, HIGH);
    getaranInterrupt = true;
  }
}

// Sensor belum aktif selama startup
bool sensorAktif = false;

// =====================================================
// STARTUP ANTI FALSE-TRIGGER
// =====================================================
const unsigned long STARTUP_DELAY = 3000;
unsigned long waktuMulai = 0;

// =====================================================
// TIMER
// =====================================================
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 50;

unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 1000;

unsigned long lastPost = 0;
const unsigned long POST_INTERVAL = 5000;

unsigned long lastPoll = 0;
const unsigned long POLL_INTERVAL = 1000;

// =====================================================
// WIFI RECONNECT
// =====================================================
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 5000;

// =====================================================
// HTTP
// =====================================================
WiFiClientSecure secureClient;
WiFiClient localClient;

bool mulaiHttp(HTTPClient &http, const String &path, String &urlAktif) {

  String urls[2] = {baseUrl, localBaseUrl};

  for (int i = 0; i < 2; i++) {

    if (urls[i].length() == 0 || (i == 1 && urls[i] == urls[0])) {
      continue;
    }

    urlAktif = urls[i] + path;
    bool berhasil;

    if (urlAktif.startsWith("https://")) {
      berhasil = http.begin(secureClient, urlAktif);
    } else {
      berhasil = http.begin(localClient, urlAktif);
    }

    if (berhasil) {
      if (i == 1) {
        Serial.println("[Server] memakai alamat lokal");
      }
      return true;
    }
  }

  return false;
}

bool mulaiHttpLokal(HTTPClient &http, const String &path, String &urlAktif) {

  if (localBaseUrl.length() == 0) {
    return false;
  }

  urlAktif = localBaseUrl + path;
  return http.begin(localClient, urlAktif);
}

// =====================================================
// SERIAL COMMAND
// =====================================================
String serialBuffer = "";

// =====================================================
// SETUP
// =====================================================
void setup() {

  Serial.begin(115200);
  delay(300);

  // ---------------------------------------------------
  // PIN
  // ---------------------------------------------------
  pinMode(PIN_SENSOR, INPUT_PULLDOWN);

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // ---------------------------------------------------
  // KONDISI AWAL
  // ---------------------------------------------------
  // Relay TERHUBUNG
  digitalWrite(PIN_RELAY, LOW);

  // Buzzer MATI
  digitalWrite(PIN_BUZZER, LOW);

  relayTerputus = false;
  relayManualTerputus = false;
  overrideGetaran = false;
  overrideGetaranMulai = 0;
  sensorAktif = false;

  waktuMulai = millis();

  // ---------------------------------------------------
  // GPS
  // ---------------------------------------------------
  GPS_Serial.begin(
    9600,
    SERIAL_8N1,
    GPS_RX,
    GPS_TX
  );

  // ---------------------------------------------------
  // PREFERENCES
  // ---------------------------------------------------
  preferences.begin("cfg", false);

  baseUrl = preferences.getString(
    "baseUrl",
    ""
  );

  localBaseUrl = preferences.getString(
    "localUrl",
    ""
  );

  // ---------------------------------------------------
  // HTTPS
  // ---------------------------------------------------
  secureClient.setInsecure();

  // ---------------------------------------------------
  // SERIAL INFO
  // ---------------------------------------------------
  Serial.println();
  Serial.println("========================================");
  Serial.println("   SISTEM KEAMANAN KENDARAAN");
  Serial.println("========================================");

  Serial.println("Startup:");
  Serial.println("Relay    : TERHUBUNG");
  Serial.println("Buzzer   : MATI");
  Serial.println("Sensor   : MENUNGGU STABIL");
  Serial.println("Delay    : 3 detik");

  if (baseUrl.length() == 0) {

    Serial.println();
    Serial.println("[INFO] Link web belum diatur.");

  } else {

    Serial.println();
    Serial.println("Server   : " + baseUrl);
  }

  if (localBaseUrl.length() > 0) {
    Serial.println("Local    : " + localBaseUrl);
  }

  Serial.println();
  Serial.println("Format link:");
  Serial.println(
    "link=\"https://domain.com/kendaraanfinal/api\""
  );

  Serial.println("========================================");

  // ---------------------------------------------------
  // WIFI
  // ---------------------------------------------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("WiFi mulai...");
}

// =====================================================
// LOOP
// =====================================================
void loop() {

  // ===================================================
  // 0. SERIAL COMMAND
  // ===================================================
  bacaPerintahSerial();

  // ===================================================
  // 1. GPS
  // ===================================================
  while (GPS_Serial.available() > 0) {

    gps.encode(
      GPS_Serial.read()
    );
  }

  // ===================================================
  // 2. AKTIFKAN SENSOR SETELAH STARTUP
  // ===================================================
  if (!sensorAktif) {

    if (millis() - waktuMulai >= STARTUP_DELAY) {

      sensorAktif = true;

      // Baca kondisi sensor setelah stabil
      statusGetaran = digitalRead(PIN_SENSOR);
      getaranNormalSejak = statusGetaran == LOW ? millis() : 0;

      attachInterrupt(
        digitalPinToInterrupt(PIN_SENSOR),
        deteksiGetaranInterrupt,
        RISING
      );

      Serial.println();
      Serial.println(
        "[OK] Startup selesai."
      );

      Serial.println(
        "[OK] Sensor getaran sekarang AKTIF."
      );

      Serial.println(
        "[OK] Relay tetap TERHUBUNG."
      );

      Serial.println();
    }
  }

  // ===================================================
  // 3. SENSOR GETARAN
  // ===================================================
  if (
    sensorAktif &&
    millis() - lastSensorRead >= SENSOR_INTERVAL
  ) {

    lastSensorRead = millis();

    if (
      overrideGetaran &&
      millis() - overrideGetaranMulai >= OVERRIDE_GETARAN_MS
    ) {
      overrideGetaran = false;
      Serial.println("[Sensor] Override selesai - sensor aktif kembali");
    }

    statusGetaran = digitalRead(PIN_SENSOR);

    if (statusGetaran == LOW) {

      if (getaranNormalSejak == 0) {
        getaranNormalSejak = millis();
      }

    } else {

      getaranNormalSejak = 0;
    }

    if (getaranInterrupt) {

      getaranInterrupt = false;

      Serial.println();
      Serial.println(
        "!!! GETARAN TERDETEKSI - RELAY LANGSUNG TERPUTUS !!!"
      );
      Serial.println();
    }

    // -------------------------------------------------
    // GETARAN TERDETEKSI
    // -------------------------------------------------
    if (
      statusGetaran == HIGH &&
      !overrideGetaran &&
      !relayManualTerputus
    ) {

      // Hanya lakukan sekali
      if (!relayTerputus) {

        relayTerputus = true;

        // RELAY PUTUS SEKARANG
        digitalWrite(
          PIN_RELAY,
          HIGH
        );

        // BUZZER MENYALA
        digitalWrite(
          PIN_BUZZER,
          HIGH
        );

        Serial.println();
        Serial.println(
          "!!! GETARAN TERDETEKSI !!!"
        );

        Serial.println(
          "!!! RELAY LANGSUNG TERPUTUS !!!"
        );

        Serial.println(
          "!!! BUZZER MENYALA !!!"
        );

        Serial.print("[Sensor] PIN_SENSOR = ");
        Serial.println(statusGetaran);

        Serial.println();
      }
    }

    // -------------------------------------------------
    // NORMAL
    // -------------------------------------------------
    else {

      /*
       * Jika relay sudah terputus karena getaran,
       * JANGAN sambungkan kembali otomatis.
       *
       * Relay hanya dapat kembali terhubung jika
       * sistem di-reset/restart.
       */

      if (!relayTerputus) {

        digitalWrite(
          PIN_RELAY,
          LOW
        );

        digitalWrite(
          PIN_BUZZER,
          LOW
        );
      }
    }
  }

  // ===================================================
  // 4. STATUS SERIAL
  // ===================================================
  if (
    millis() - lastPrint >= PRINT_INTERVAL
  ) {

    lastPrint = millis();

    Serial.println();

    if (!sensorAktif) {

      Serial.println(
        "Status   : STARTUP"
      );

      Serial.println(
        "Sensor   : MENUNGGU STABIL"
      );

    } else {

      if (statusGetaran == HIGH) {

        Serial.println(
          "Kondisi  : !!! GETARAN !!!"
        );

      } else {

        Serial.println(
          "Kondisi  : normal"
        );
      }

      Serial.print("Sensor   : ");

      if (sensorAktif) {
        Serial.println("AKTIF");
      } else {
        Serial.println("NONAKTIF");
      }

      Serial.print("Relay    : ");

      if (relayTerputus) {
        Serial.println("TERPUTUS");
      } else {
        Serial.println("TERHUBUNG");
      }
    }

    Serial.print("WiFi     : ");

    if (WiFi.status() == WL_CONNECTED) {

      Serial.println("TERHUBUNG");

    } else {

      Serial.println("TIDAK TERHUBUNG");
    }

    Serial.print("Server   : ");

    if (baseUrl.length() > 0) {

      Serial.println(baseUrl);

    } else {

      Serial.println("belum diatur");
    }

    Serial.print("GPS      : ");

    if (gps.location.isValid()) {

      Serial.print(
        gps.location.lat(),
        6
      );

      Serial.print(", ");

      Serial.println(
        gps.location.lng(),
        6
      );

    } else {

      Serial.println(
        "belum mendapatkan lokasi"
      );
    }

    Serial.println(
      "----------------------------------------"
    );
  }

  // ===================================================
  // 5. WIFI RECONNECT
  // ===================================================
  if (
    millis() - lastWiFiCheck >= WIFI_CHECK_INTERVAL
  ) {

    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {

      WiFi.disconnect();

      WiFi.begin(
        ssid,
        password
      );
    }
  }

  // ===================================================
  // 6. KIRIM STATUS KE SERVER
  // ===================================================
  if (
    millis() - lastPost >= POST_INTERVAL
  ) {

    lastPost = millis();

    kirimStatus();
  }

  // ===================================================
  // 7. CEK PERINTAH DASHBOARD
  // ===================================================
  if (
    millis() - lastPoll >= POLL_INTERVAL
  ) {

    lastPoll = millis();

    cekPerintah();
  }
}

// =====================================================
// BACA PERINTAH SERIAL
// =====================================================
void bacaPerintahSerial() {

  while (Serial.available() > 0) {

    char c = Serial.read();

    if (
      c == '\n' ||
      c == '\r'
    ) {

      if (serialBuffer.length() > 0) {

        prosesPerintah(
          serialBuffer
        );

        serialBuffer = "";
      }

    } else {

      serialBuffer += c;
    }
  }
}

// =====================================================
// PROSES PERINTAH
// =====================================================
void prosesPerintah(String perintah) {

  perintah.trim();

  // ---------------------------------------------------
  // FORMAT:
  // link="https://domain.com/api"
  // ---------------------------------------------------
  if (perintah.startsWith("link=")) {

    String nilai =
      perintah.substring(5);

    nilai.trim();

    // Hapus tanda kutip
    if (
      nilai.startsWith("\"") &&
      nilai.endsWith("\"") &&
      nilai.length() >= 2
    ) {

      nilai =
        nilai.substring(
          1,
          nilai.length() - 1
        );
    }

    // Hapus slash terakhir
    while (
      nilai.endsWith("/")
    ) {

      nilai =
        nilai.substring(
          0,
          nilai.length() - 1
        );
    }

    if (nilai.length() == 0) {

      Serial.println(
        "[ERROR] Link kosong."
      );

      return;
    }

    baseUrl = nilai;

    // Simpan permanen
    preferences.putString(
      "baseUrl",
      baseUrl
    );

    Serial.println();
    Serial.println(
      "[OK] Link web disimpan:"
    );

    Serial.println(baseUrl);

    Serial.println();
  }

  else if (perintah.startsWith("linklocal=")) {

    String nilai = perintah.substring(10);
    nilai.trim();

    if (
      nilai.startsWith("\"") &&
      nilai.endsWith("\"") &&
      nilai.length() >= 2
    ) {
      nilai = nilai.substring(1, nilai.length() - 1);
    }

    while (nilai.endsWith("/")) {
      nilai = nilai.substring(0, nilai.length() - 1);
    }

    localBaseUrl = nilai;
    preferences.putString("localUrl", localBaseUrl);

    Serial.println();
    Serial.println("[OK] Link lokal disimpan:");
    Serial.println(localBaseUrl);
    Serial.println();
  }

  else {

    Serial.println();
    Serial.println(
      "[INFO] Perintah tidak dikenal."
    );

    Serial.println(
      "Gunakan:"
    );

    Serial.println(
      "link=\"https://domain.com/api\""
    );

    Serial.println(
      "linklocal=\"http://192.168.1.10/kendaraan1/api\""
    );

    Serial.println();
  }
}

// =====================================================
// KIRIM STATUS KE SERVER
// =====================================================
void kirimStatus() {

  // Tidak ada WiFi
  if (
    WiFi.status() != WL_CONNECTED
  ) {

    return;
  }

  // Link belum ada
  if (baseUrl.length() == 0 && localBaseUrl.length() == 0) {

    return;
  }

  HTTPClient http;

  String url;

  http.setConnectTimeout(3000);
  http.setTimeout(3000);

  if (!mulaiHttp(http, "/update_status.php", url)) {

    Serial.println(
      "[HTTP POST] begin gagal"
    );

    return;
  }

  http.addHeader(
    "Content-Type",
    "application/x-www-form-urlencoded"
  );

  // GPS
  bool gpsOnline =
    gps.location.isValid();

  // ---------------------------------------------------
  // BODY
  // ---------------------------------------------------
  String body =
    "getaran=" +
    String(
      statusGetaran == HIGH ? 1 : 0
    );

  body +=
    "&relay=" +
    String(
      relayTerputus ? 1 : 0
    );

  body +=
    "&gps_online=" +
    String(
      gpsOnline ? 1 : 0
    );

  if (gpsOnline) {

    body +=
      "&lat=" +
      String(
        gps.location.lat(),
        6
      );

    body +=
      "&lon=" +
      String(
        gps.location.lng(),
        6
      );
  }

  // ---------------------------------------------------
  // POST
  // ---------------------------------------------------
  int httpCode =
    http.POST(body);

  if (
    (httpCode < 200 || httpCode >= 300) &&
    localBaseUrl.length() > 0 &&
    !url.startsWith(localBaseUrl)
  ) {
    http.end();
    if (mulaiHttpLokal(http, "/update_status.php", url)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      httpCode = http.POST(body);
      Serial.println("[Server] status dicoba melalui alamat lokal");
    }
  }

  if (httpCode >= 200 && httpCode < 300) {

    Serial.print(
      "[HTTP POST] OK: "
    );

    Serial.println(
      httpCode
    );

  } else if (httpCode > 0) {

    Serial.print(
      "[HTTP POST] HTTP ERROR: "
    );

    Serial.println(
      httpCode
    );

    String response =
      http.getString();

    if (response.length() > 0) {
      Serial.println(response);
    }

  } else {

    Serial.print(
      "[HTTP POST] gagal: "
    );

    Serial.println(
      http.errorToString(
        httpCode
      )
    );
  }

  http.end();
}

void kirimAck(int commandId, const char* hasil) {

  HTTPClient http;
  String url;

  http.setConnectTimeout(3000);
  http.setTimeout(3000);

  if (!mulaiHttp(http, "/ack_command.php", url)) {
    Serial.println("[ACK] server tidak dapat dihubungi");
    return;
  }

  http.addHeader(
    "Content-Type",
    "application/x-www-form-urlencoded"
  );

  String body =
    "id=" + String(commandId) +
    "&result=" + String(hasil);

  int httpCode = http.POST(body);

  if (
    (httpCode < 200 || httpCode >= 300) &&
    localBaseUrl.length() > 0 &&
    !url.startsWith(localBaseUrl)
  ) {
    http.end();
    if (mulaiHttpLokal(http, "/ack_command.php", url)) {
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      httpCode = http.POST(body);
      Serial.println("[Server] ACK dicoba melalui alamat lokal");
    }
  }

  if (httpCode >= 200 && httpCode < 300) {
    Serial.println("[ACK] command diterima server");
  } else {
    Serial.print("[ACK] gagal: ");
    Serial.println(httpCode);
  }

  http.end();
}

// =====================================================
// CEK PERINTAH DASHBOARD
// =====================================================
void cekPerintah() {

  // Tidak ada WiFi
  if (
    WiFi.status() != WL_CONNECTED
  ) {

    return;
  }

  // Tidak ada server
  if (baseUrl.length() == 0 && localBaseUrl.length() == 0) {

    return;
  }

  HTTPClient http;

  String url;

  http.setConnectTimeout(3000);
  http.setTimeout(3000);

  if (!mulaiHttp(http, "/get_command.php", url)) {

    return;
  }

  int httpCode =
    http.GET();

  if (
    (httpCode < 200 || httpCode >= 300) &&
    localBaseUrl.length() > 0 &&
    !url.startsWith(localBaseUrl)
  ) {
    http.end();
    if (mulaiHttpLokal(http, "/get_command.php", url)) {
      httpCode = http.GET();
      Serial.println("[Server] command dicoba melalui alamat lokal");
    }
  }

  if (httpCode == 200) {

    String payload =
      http.getString();

    StaticJsonDocument<256> doc;

    DeserializationError err =
      deserializeJson(
        doc,
        payload
      );

    if (!err) {

      if (
        !doc["command"].isNull()
      ) {

        String cmd =
          doc["command"].as<String>();

        int val =
          doc["value"];

        int commandId =
          doc["id"];

        const char* hasil = "rejected";

        // ------------------------------------------------
        // HANYA PERINTAH RELAY
        // ------------------------------------------------
        if (
          cmd == "relay"
        ) {

          // Perintah PUTUS
          if (val == 1) {

            overrideGetaran = false;
            relayManualTerputus = true;
            relayTerputus = true;
            getaranInterrupt = false;

            digitalWrite(
              PIN_BUZZER,
              LOW
            );

            digitalWrite(
              PIN_RELAY,
              HIGH
            );

            Serial.println(
              "[Dashboard] Relay: TERPUTUS"
            );

            hasil = "applied";
          }

          // Perintah SAMBUNG
          else if (val == 0) {

            /*
             * Perintah sambung dari dashboard menjadi override manual.
             * Relay tetap hidup walaupun sensor sedang HIGH.
             */
            overrideGetaran = true;
            relayManualTerputus = false;
            overrideGetaranMulai = millis();
            getaranInterrupt = false;
            relayTerputus = false;

            digitalWrite(
              PIN_RELAY,
              LOW
            );

            digitalWrite(
              PIN_BUZZER,
              LOW
            );

            Serial.println(
              "[Dashboard] Relay: TERHUBUNG (override getaran aktif)"
            );

            hasil = "applied";
          }

          kirimAck(commandId, hasil);
        }
      }
    }
  }

  http.end();
}