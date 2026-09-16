<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../db.php';

$getaran = isset($_POST['getaran']) ? (int)$_POST['getaran'] : 0;
$relay = isset($_POST['relay']) ? (int)$_POST['relay'] : 0;
$gps_online = isset($_POST['gps_online']) ? (int)$_POST['gps_online'] : 0;
$lat = (isset($_POST['lat']) && $_POST['lat'] !== '') ? (float)$_POST['lat'] : null;
$lon = (isset($_POST['lon']) && $_POST['lon'] !== '') ? (float)$_POST['lon'] : null;

$getaran = $getaran ? 1 : 0;
$relay = $relay ? 1 : 0;
$gps_online = $gps_online ? 1 : 0;

$res = $conn->query("SELECT id, sistem_aktif FROM status ORDER BY id DESC LIMIT 1");
$row = $res ? $res->fetch_assoc() : null;
if (!$row) {
    $conn->query("INSERT INTO status (sistem_aktif,getaran,relay,gps_online,updated_at) VALUES (1,0,0,0,NOW())");
    $id = $conn->insert_id;
    $sistem_aktif = 1;
} else {
    $id = (int)$row['id'];
    $sistem_aktif = (int)$row['sistem_aktif'];
}

$stmt = $conn->prepare("UPDATE status SET getaran=?, relay=?, gps_lat=?, gps_lon=?, gps_online=?, updated_at=NOW() WHERE id=?");
$stmt->bind_param('iiddii', $getaran, $relay, $lat, $lon, $gps_online, $id);
$stmt->execute();

if ($getaran === 1) {
    $conn->query("INSERT INTO logs (pesan) VALUES ('Getaran terdeteksi - relay diputus lokal oleh ESP32')");
}

echo json_encode(['status'=>'ok']);
