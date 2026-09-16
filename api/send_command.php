<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../db.php';

$raw = file_get_contents('php://input');
$data = json_decode($raw, true);
$command = isset($data['command']) ? trim((string)$data['command']) : '';
$value = !empty($data['value']) ? 1 : 0;

// Web hanya boleh mengontrol relay.
if ($command !== 'relay') {
    http_response_code(400);
    echo json_encode(['status'=>'error','error'=>'Perintah tidak diizinkan']);
    exit;
}

$conn->query("UPDATE commands SET executed=1, result='replaced' WHERE command='relay' AND executed=0");

$stmt = $conn->prepare("INSERT INTO commands (command,value,executed,result,created_at) VALUES ('relay',?,0,NULL,NOW())");
$stmt->bind_param('i', $value);
$stmt->execute();
$commandId = $stmt->insert_id;

echo json_encode(['status'=>'ok','command'=>'relay','value'=>$value,'command_id'=>$commandId]);
