-- Tidak wajib di-import.
-- Sistem sekarang membuat/memperbaiki tabel otomatis melalui db.php.
-- File ini hanya dokumentasi struktur database.

CREATE TABLE status (
  id INT PRIMARY KEY AUTO_INCREMENT,
  sistem_aktif TINYINT(1) NOT NULL DEFAULT 1,
  getaran TINYINT(1) NOT NULL DEFAULT 0,
  relay TINYINT(1) NOT NULL DEFAULT 0,
  gps_lat DOUBLE DEFAULT NULL,
  gps_lon DOUBLE DEFAULT NULL,
  gps_online TINYINT(1) NOT NULL DEFAULT 0,
  updated_at DATETIME NULL DEFAULT NULL
);

CREATE TABLE commands (
  id INT PRIMARY KEY AUTO_INCREMENT,
  command VARCHAR(50) NOT NULL,
  value TINYINT(1) NOT NULL DEFAULT 0,
  executed TINYINT(1) NOT NULL DEFAULT 0,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE logs (
  id INT PRIMARY KEY AUTO_INCREMENT,
  pesan VARCHAR(255) NOT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
);
