#!/usr/bin/php-cgi
<?php
// Enforce plain text output for the Sentinel OS terminal
header("Content-Type: text/plain");
header("X-Powered-By: Sentinel-PHP-Engine");

echo "--- PHP SYSTEM DIAGNOSTICS ---\n";
echo "Timestamp : " . date('Y-m-d H:i:s') . "\n";
echo "PHP Core  : " . phpversion() . "\n";
echo "Method    : " . ($_SERVER['REQUEST_METHOD'] ?? 'UNKNOWN') . "\n";
echo "Query     : " . ($_SERVER['QUERY_STRING'] ?? 'NONE') . "\n";

echo "\n--- CLIENT HEADERS ---\n";
// Extract and format HTTP headers provided by the C++ engine
foreach ($_SERVER as $key => $value) {
    if (strpos($key, 'HTTP_') === 0) {
        $headerName = str_replace(' ', '-', ucwords(strtolower(str_replace('_', ' ', substr($key, 5)))));
        echo "$headerName: $value\n";
    }
}

// Read raw body for POST requests
if (($_SERVER['REQUEST_METHOD'] ?? '') === 'POST') {
    echo "\n--- POST BODY ---\n";
    $body = file_get_contents("php://input");
    echo $body ? $body : "<Empty>";
}
?>