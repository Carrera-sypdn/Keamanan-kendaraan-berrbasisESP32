<?php

header('Content-Type: application/json; charset=utf-8');

require_once __DIR__ . '/../db.php';

/*
|--------------------------------------------------------------------------
| Ambil status terakhir ESP32
|--------------------------------------------------------------------------
| Perhitungan heartbeat dilakukan langsung oleh MySQL menggunakan
| TIMESTAMPDIFF(), sehingga tidak terpengaruh perbedaan timezone PHP.
*/

$sql = "
    SELECT
        id,
        sistem_aktif,
        getaran,
        relay,
        gps_lat,
        gps_lon,
        gps_online,
        updated_at,
        CASE
            WHEN updated_at IS NULL THEN 999999
            ELSE TIMESTAMPDIFF(SECOND, updated_at, NOW())
        END AS heartbeat_age
    FROM status
    ORDER BY id DESC
    LIMIT 1
";

$res = $conn->query($sql);

if (!$res) {
    http_response_code(500);

    echo json_encode([
        'connected' => false,
        'status' => 'error',
        'error' => $conn->error
    ]);

    exit;
}

$row = $res->fetch_assoc();

if (!$row) {

    echo json_encode([
        'connected' => false,
        'status' => 'no_data'
    ]);

    exit;
}

/*
|--------------------------------------------------------------------------
| HEARTBEAT
|--------------------------------------------------------------------------
| ESP32 mengirim POST setiap ±5 detik.
| Jika tidak ada data lebih dari 12 detik, dianggap offline.
*/

$age = (int)$row['heartbeat_age'];

$connected = (
    $age >= 0 &&
    $age <= 20
);

/*
|--------------------------------------------------------------------------
| RESPONSE
|--------------------------------------------------------------------------
*/

echo json_encode([
    'connected' => $connected,

    'sistem_aktif' => (int)$row['sistem_aktif'],

    'getaran' => $connected
        ? (int)$row['getaran']
        : 0,

    'relay' => (int)$row['relay'],

    'gps_online' => $connected
        ? (int)$row['gps_online']
        : 0,

    'lat' => (
        $row['gps_lat'] !== null
        ? (float)$row['gps_lat']
        : null
    ),

    'lon' => (
        $row['gps_lon'] !== null
        ? (float)$row['gps_lon']
        : null
    ),

    'updated_at' => $row['updated_at'],

    'heartbeat_age' => $age
]);