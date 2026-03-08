#!/usr/bin/env python3

import os
import sys

# CGI headers
print("Content-Type: application/json")

method = os.environ.get("REQUEST_METHOD", "")
path_info = os.environ.get("PATH_INFO", "")

# Only accept DELETE
if method != "DELETE":
    print("Status: 405 Method Not Allowed")
    print()
    print('{"error": "Only DELETE method is allowed"}')
    sys.exit(0)

# Extract item ID from PATH_INFO
# Example: /cgi-bin/delete_item.py/123 -> path_info = "/123"
if not path_info or path_info == "/":
    print("Status: 400 Bad Request")
    print()
    print('{"error": "Item ID is required in path"}')
    sys.exit(0)

item_id = path_info.lstrip("/")

# Simulate deletion logic
# In a real app, you would:
# - Check if item exists in database
# - Verify user permissions
# - Delete the item
# - Return appropriate status

print("Status: 200 OK")
print()
print(f'{{"message": "Item {item_id} deleted successfully", "id": "{item_id}"}}')
