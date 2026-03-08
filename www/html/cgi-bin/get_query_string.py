#!/usr/bin/env python3

import os
import urllib.parse

# CGI requires headers first
print("Content-Type: text/html")
print()

method = os.environ.get("REQUEST_METHOD", "")
query = os.environ.get("QUERY_STRING", "")

params = urllib.parse.parse_qs(query)

print("<html>")
print("<body>")
print("<h1>CGI script working with query params</h1>")

print("<h2>Request info</h2>")
print(f"<p>Method: {method}</p>")
print(f"<p>Query string: {query}</p>")

print("<h2>Parsed parameters</h2>")
if "name" in params and params["name"]:
    print(f"<p>Hello, {params['name'][0]}</p>")
    
for key, value in params.items():
    print(f"<p>{key}, {value}</p>")
print("</body>")
print("</html>")