<?php
header('Content-Type: application/json; charset=utf-8');
require_once __DIR__ . '/../db.php';

$res = $conn->query("SELECT id, command, value FROM commands WHERE executed=0 ORDER BY id DESC LIMIT 1");
$row = $res ? $res->fetch_assoc() : null;
if (!$row) {
    echo json_encode(['command'=>null]);
    exit;
}

echo json_encode(['id'=>(int)$row['id'], 'command'=>$row['command'], 'value'=>(int)$row['value']]);
