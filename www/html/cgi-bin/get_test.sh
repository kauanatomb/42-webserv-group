#!/bin/bash

echo "Content-Type: text/html; charset=utf-8"
echo ""

cat <<'HEAD'
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Bash CGI</title>
<style>
body{margin:0;min-height:100vh;display:grid;place-items:center;background:#0d1117;color:#e5e7eb;font-family:Arial,sans-serif;padding:20px}
.card{width:100%;max-width:580px;background:#111827;border:1px solid rgba(52,211,153,.25);border-radius:16px;padding:40px 28px}
h1{margin:0 0 6px;font-size:1.5rem;text-align:center}
.sub{text-align:center;color:#9ca3af;margin:0 0 24px;font-size:.9rem}
h2{font-size:1rem;color:#34d399;margin:20px 0 8px;font-weight:600}
table{width:100%;border-collapse:collapse;border:1px solid rgba(52,211,153,.15);border-radius:10px;overflow:hidden;background:#1a2233;margin-top:6px}
th,td{padding:10px 12px;text-align:left;border-bottom:1px solid rgba(52,211,153,.1)}
th{background:rgba(52,211,153,.08);color:#34d399;font-size:.8rem;text-transform:uppercase;letter-spacing:.05em}
td{font-size:.9rem}
td.mono{font-family:monospace;color:#9ca3af}
.actions{margin-top:20px;text-align:center}
.actions a{text-decoration:none;color:#e5e7eb;border:1px solid rgba(52,211,153,.25);border-radius:10px;padding:10px 14px;font-size:.85rem;transition:border-color .2s}
.actions a:hover{border-color:#34d399}
</style>
</head>
<body>
<main class="card">
<h1>Bash CGI Script</h1>
<p class="sub">Environment variables and system info from a shell script</p>
<h2>CGI Environment</h2>
<table>
<tr><th>Variable</th><th>Value</th></tr>
HEAD

echo "<tr><td>REQUEST_METHOD</td><td class='mono'>$REQUEST_METHOD</td></tr>"
echo "<tr><td>QUERY_STRING</td><td class='mono'>$QUERY_STRING</td></tr>"
echo "<tr><td>SCRIPT_NAME</td><td class='mono'>$SCRIPT_NAME</td></tr>"
echo "<tr><td>PATH_INFO</td><td class='mono'>$PATH_INFO</td></tr>"
echo "<tr><td>SERVER_PROTOCOL</td><td class='mono'>$SERVER_PROTOCOL</td></tr>"
echo "</table>"

echo "<h2>System Info</h2>"
echo "<table>"
echo "<tr><th>Field</th><th>Value</th></tr>"
echo "<tr><td>Date</td><td class='mono'>$(date)</td></tr>"
echo "<tr><td>Hostname</td><td class='mono'>$(hostname)</td></tr>"
echo "</table>"

cat <<'FOOT'
<div class="actions"><a href="/">Return to Control Panel</a></div>
</main>
</body>
</html>
FOOT
