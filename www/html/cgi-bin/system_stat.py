#!/usr/bin/env python3
import os
import sys
from datetime import datetime

# CGI requires standard HTTP headers separated by a blank line
print("Content-Type: text/plain")
print("X-Powered-By: Sentinel-CGI")
print("")

# Output dynamic system status
print("--- SYSTEM DIAGNOSTICS ---")
print(f"Timestamp : {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
print(f"Method    : {os.environ.get('REQUEST_METHOD', 'UNKNOWN')}")
print(f"Protocol  : {os.environ.get('SERVER_PROTOCOL', 'UNKNOWN')}")
print(f"Query     : {os.environ.get('QUERY_STRING', 'NONE')}")

print("\n--- CLIENT HEADERS ---")
# Extract and print HTTP_ variables passed by your C++ engine
for key, value in sorted(os.environ.items()):
    if key.startswith("HTTP_"):
        header_name = key[5:].replace("_", "-").title()
        print(f"{header_name}: {value}")

# If it's a POST request, read stdin
if os.environ.get("REQUEST_METHOD") == "POST":
    print("\n--- POST BODY ---")
    body = sys.stdin.read(1024)  # Read up to 1KB for safety
    print(body if body else "<Empty>")