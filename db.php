<?php
// ============================================================
// db.php - koneksi + migrasi database otomatis
// ============================================================
// Isi sesuai database hosting kamu.
$DB_HOST = "localhost";
$DB_USER = "root";
$DB_PASS = "";
$DB_NAME = "keamanan_kendaraan";

mysqli_report(MYSQLI_REPORT_OFF);
$conn = @new mysqli($DB_HOST, $DB_USER, $DB_PASS, $DB_NAME);

if ($conn->connect_error) {
    header('Content-Type: application/json; charset=utf-8');
    http_response_code(500);
    echo json_encode(["status"=>"error", "error"=>"Koneksi database gagal. Periksa db.php."]);
    exit;
}
$conn->set_charset("utf8mb4");

// ------------------------------------------------------------
// Migrasi otomatis. Aman dijalankan setiap request.
// Tidak membutuhkan import SQL manual.
// ------------------------------------------------------------
function ensure_column($conn, $table, $column, $definition) {
    $table = preg_replace('/[^a-zA-Z0-9_]/', '', $table);
    $column = preg_replace('/[^a-zA-Z0-9_]/', '', $column);
    $check = $conn->query("SHOW COLUMNS FROM `$table` LIKE '$column'");
    if ($check && $check->num_rows === 0) {
        $conn->query("ALTER TABLE `$table` ADD COLUMN `$column` $definition");
    }
}

$conn->query("CREATE TABLE IF NOT EXISTS status (
    id INT PRIMARY KEY AUTO_INCREMENT,
    sistem_aktif TINYINT(1) NOT NULL DEFAULT 1,
    getaran TINYINT(1) NOT NULL DEFAULT 0,
    relay TINYINT(1) NOT NULL DEFAULT 0,
    gps_lat DOUBLE DEFAULT NULL,
    gps_lon DOUBLE DEFAULT NULL,
    gps_online TINYINT(1) NOT NULL DEFAULT 0,
    updated_at DATETIME NULL DEFAULT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

$conn->query("CREATE TABLE IF NOT EXISTS commands (
    id INT PRIMARY KEY AUTO_INCREMENT,
    command VARCHAR(50) NOT NULL,
    value TINYINT(1) NOT NULL DEFAULT 0,
    executed TINYINT(1) NOT NULL DEFAULT 0,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

$conn->query("CREATE TABLE IF NOT EXISTS logs (
    id INT PRIMARY KEY AUTO_INCREMENT,
    pesan VARCHAR(255) NOT NULL,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4");

// Struktur database lama diperbaiki otomatis jika kolom belum ada.
ensure_column($conn, 'status', 'sistem_aktif', 'TINYINT(1) NOT NULL DEFAULT 1');
ensure_column($conn, 'status', 'getaran', 'TINYINT(1) NOT NULL DEFAULT 0');
ensure_column($conn, 'status', 'relay', 'TINYINT(1) NOT NULL DEFAULT 0');
ensure_column($conn, 'status', 'gps_lat', 'DOUBLE DEFAULT NULL');
ensure_column($conn, 'status', 'gps_lon', 'DOUBLE DEFAULT NULL');
ensure_column($conn, 'status', 'gps_online', 'TINYINT(1) NOT NULL DEFAULT 0');
ensure_column($conn, 'status', 'updated_at', 'DATETIME NULL DEFAULT NULL');
ensure_column($conn, 'commands', 'result', "VARCHAR(30) NULL DEFAULT NULL");

// Pastikan hanya ada minimal satu baris status.
$count = 0;
$r = $conn->query("SELECT COUNT(*) AS c FROM status");
if ($r) { $count = (int)$r->fetch_assoc()['c']; }
if ($count === 0) {
    $conn->query("INSERT INTO status (sistem_aktif,getaran,relay,gps_lat,gps_lon,gps_online,updated_at)
                  VALUES (1,0,0,NULL,NULL,0,NULL)");
}
