#!/usr/bin/env python3
import os
import sys
import html
import urllib.parse

length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body = sys.stdin.read(length)
content_type = os.environ.get("CONTENT_TYPE", "")

fields = {}
if "application/x-www-form-urlencoded" in content_type and body:
	fields = urllib.parse.parse_qs(body, keep_blank_values=True)

print("Content-Type: text/html; charset=utf-8")
print()

print("<!doctype html>")
print("<html lang='en'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'>")
print("<title>POST Echo</title><link rel='stylesheet' href='/cgi-bin/form.css'></head><body>")
print("<main class='card'>")
print("<h1>CGI POST received</h1>")
print("<p>Your profile was submitted and parsed by the CGI script.</p>")

print("<h2>Meta</h2>")
print("<ul class='meta'>")
print(f"<li><b>REQUEST_METHOD:</b> {html.escape(os.environ.get('REQUEST_METHOD', ''))}</li>")
print(f"<li><b>CONTENT_TYPE:</b> {html.escape(content_type)}</li>")
print(f"<li><b>CONTENT_LENGTH:</b> {html.escape(os.environ.get('CONTENT_LENGTH', ''))}</li>")
print(f"<li><b>PATH_INFO:</b> {html.escape(os.environ.get('PATH_INFO', ''))}</li>")
print(f"<li><b>PATH_TRANSLATED:</b> {html.escape(os.environ.get('PATH_TRANSLATED', ''))}</li>")
print("</ul>")

print("<h2>Sent fields</h2>")
if fields:
	print("<table>")
	print("<tr><th>Field</th><th>Value</th></tr>")
	for key, values in fields.items():
		safe_key = html.escape(key)
		safe_values = ", ".join(html.escape(v) for v in values)
		print(f"<tr><td>{safe_key}</td><td>{safe_values}</td></tr>")
	print("</table>")
else:
	print("<p>No field parsed (or content-type is not x-www-form-urlencoded).</p>")

print("<h2>Body as came</h2>")
print(f"<pre>{html.escape(body)}</pre>")
print("<div class='actions'><a class='button-link' href='/cgi-bin/form.html'>Back to form</a></div>")
print("</main></body></html>")