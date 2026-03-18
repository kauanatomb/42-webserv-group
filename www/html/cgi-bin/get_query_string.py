#!/usr/bin/env python3

import os
import html
import urllib.parse

method = os.environ.get("REQUEST_METHOD", "")
query = os.environ.get("QUERY_STRING", "")
params = urllib.parse.parse_qs(query)

print("Content-Type: text/html; charset=utf-8")
print()

print("""<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>GET Query | CGI</title>
<style>
body{margin:0;min-height:100vh;display:grid;place-items:center;background:#0d1117;color:#e5e7eb;font-family:Arial,sans-serif;padding:20px}
.card{width:100%;max-width:580px;background:#111827;border:1px solid rgba(52,211,153,.25);border-radius:16px;padding:40px 28px}
h1{margin:0 0 6px;font-size:1.5rem;text-align:center}
.sub{text-align:center;color:#9ca3af;margin:0 0 24px;font-size:.9rem}
h2{font-size:1rem;color:#34d399;margin:20px 0 8px;font-weight:600}
.badge{display:inline-block;background:rgba(52,211,153,.12);color:#34d399;padding:4px 10px;border-radius:6px;font-size:.85rem;font-family:monospace}
table{width:100%;border-collapse:collapse;border:1px solid rgba(52,211,153,.15);border-radius:10px;overflow:hidden;background:#1a2233;margin-top:6px}
th,td{padding:10px 12px;text-align:left;border-bottom:1px solid rgba(52,211,153,.1)}
th{background:rgba(52,211,153,.08);color:#34d399;font-size:.8rem;text-transform:uppercase;letter-spacing:.05em}
td{font-size:.9rem}
.mono{font-family:monospace;color:#9ca3af;word-break:break-all}
.empty{color:#9ca3af;font-style:italic}
.greeting{background:rgba(52,211,153,.08);border:1px solid rgba(52,211,153,.2);border-radius:10px;padding:14px;text-align:center;margin:12px 0;font-size:1.1rem}
.actions{margin-top:20px;text-align:center}
.actions a{text-decoration:none;color:#e5e7eb;border:1px solid rgba(52,211,153,.25);border-radius:10px;padding:10px 14px;font-size:.85rem;transition:border-color .2s}
.actions a:hover{border-color:#34d399}
</style>
</head>
<body>
<main class="card">""")

print("<h1>GET Query String</h1>")
print("<p class='sub'>CGI script parsed your query parameters</p>")

print("<h2>Request</h2>")
print(f"<p>Method: <span class='badge'>{html.escape(method)}</span></p>")
if query:
    print(f"<p>Query: <span class='mono'>{html.escape(query)}</span></p>")
else:
    print("<p class='empty'>No query string provided</p>")

if "name" in params and params["name"]:
    name = html.escape(params["name"][0])
    print(f"<div class='greeting'>Hello, <strong>{name}</strong>!</div>")

print("<h2>Parameters</h2>")
if params:
    print("<table><tr><th>Key</th><th>Value</th></tr>")
    for key, values in params.items():
        safe_key = html.escape(key)
        safe_val = ", ".join(html.escape(v) for v in values)
        print(f"<tr><td>{safe_key}</td><td>{safe_val}</td></tr>")
    print("</table>")
else:
    print("<p class='empty'>No parameters to display</p>")

print("<div class='actions'><a href='/'>Return to Control Panel</a></div>")
print("</main></body></html>")
