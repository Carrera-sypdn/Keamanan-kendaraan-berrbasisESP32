<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../db.php';

$id = isset($_POST['id']) ? (int)$_POST['id'] : 0;
$result = isset($_POST['result']) ? trim((string)$_POST['result']) : '';

if ($id <= 0 || !in_array($result, ['applied', 'rejected'], true)) {
    http_response_code(400);
    echo json_encode(['status'=>'error','error'=>'ACK tidak valid']);
    exit;
}

$stmt = $conn->prepare("UPDATE commands SET executed=1, result=? WHERE id=? AND executed=0");
$stmt->bind_param('si', $result, $id);
$stmt->execute();

echo json_encode(['status'=>'ok','updated'=>$stmt->affected_rows > 0]);