<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../db.php';

$id = isset($_GET['id']) ? (int)$_GET['id'] : 0;
$stmt = $conn->prepare("SELECT executed, result FROM commands WHERE id=? LIMIT 1");
$stmt->bind_param('i', $id);
$stmt->execute();
$row = $stmt->get_result()->fetch_assoc();

if (!$row) {
    http_response_code(404);
    echo json_encode(['status'=>'error','error'=>'Command tidak ditemukan']);
    exit;
}

echo json_encode([
    'status' => 'ok',
    'executed' => (int)$row['executed'] === 1,
    'result' => $row['result']
]);