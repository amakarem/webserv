#!/usr/bin/env python3

import os
import uuid

# Get cookies from environment (forwarded by webserver)
cookies = os.environ.get("HTTP_COOKIE", "")

session_id = None

# Parse cookie safely
for part in cookies.split(";"):
    part = part.strip()
    if "=" in part:
        key, val = part.split("=", 1)
        if key == "session":
            session_id = val
            break

# If no session, create new and send cookie
new_session = False
if not session_id:
    session_id = str(uuid.uuid4())
    new_session = True

# Session storage (file based)
session_file = f"/tmp/sess_{session_id}"

# Load existing counter
counter = 1
if os.path.exists(session_file):
    try:
        with open(session_file, "r") as f:
            data = f.read()
            if data.isdigit():
                counter = int(data) + 1
    except:
        counter = 1

# Save updated counter
with open(session_file, "w") as f:
    f.write(str(counter))

# Output headers (cookie only if new session)
if new_session:
    print(f"Set-Cookie: session={session_id}")
print("Content-Type: text/html")
print()

# Output page
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>Python CGI Session</title></head>")
print("<body>")
print("<h1>Python CGI Session Demo ✅</h1>")
print(f"<p>Session ID: {session_id}</p>")
print(f"<p>Visit Count: {counter}</p>")
print("</body>")
print("</html>")

print("<pre>")
print("COOKIES:", cookies)
print("SESSION:", session_id)
print("</pre>")