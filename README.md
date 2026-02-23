*This project has been created as part of the 42 curriculum by ktombola, falatrac, mcamaren.*

# Webserv

## Description

Webserv is a custom HTTP/1.0 web server written in C++ as part of the 42 curriculum.
The goal of the project is to understand how a real web server works internally by reimplementing core networking and HTTP features from scratch, without relying on existing web frameworks.

**The server:**

- Listens on one or multiple ports
- Handles multiple client connections using non-blocking I/O
- Parses and validates HTTP requests
- Serves static files
- Supports CGI execution
- Applies configuration rules defined in a configuration file (similar to nginx)

This project focuses on low-level networking (sockets, poll), HTTP protocol semantics, request parsing, and event-driven architecture.

## Features

- HTTP/1.0 compliant request parsing
- GET, POST, DELETE methods
- Static file serving
- Directory listing (optional per configuration)
- Custom error pages
- File upload support
- CGI execution (e.g. Python, PHP)
- Multiple server blocks with different ports and hostnames
- Non-blocking I/O with a single event loop

## Architecture Overview

The server is structured around the following core components:

**Server / Socket Layer** Responsible for:
- Creating listening sockets
- Binding to IP/port
- Accepting client connections
- Managing file descriptors in non-blocking mode

**Event Loop** Uses poll to:
- Monitor multiple connections
- Dispatch read/write events
- Avoid blocking operations

**Request Parser**
- Parses start line, headers, and body
- Handles Content-Length and chunked transfer encoding
- Validates HTTP format

**Request Handler**
- Matches request against configuration
- Resolves routes and locations
- Applies method restrictions
- Determines static response or CGI execution

**Response Builder**
- Generates proper HTTP responses
- Adds required headers
- Handles error codes

## Instructions

**Requirements**
- C++98 compliant compiler
- make
- Unix-like system (Linux or macOS)

**Compilation**

- `make`- Build automation tool

This generates the executable:
```bash
./webserv
```

**Execution**
```bash
./webserv [configuration_file]
```
If no configuration file is provided, a default one is used.

Example:
```bash
./webserv config/default.conf
```

**Testing**

Open a browser and access:

```bash
http://localhost:8080
```
Or use curl:
```bash
curl -v http://localhost:8080/
```

## Configuration File

The configuration file defines:
- server blocks
- listen directives
- server_name
- root
- location
- allowed_methods
- error_page
- client_max_body_size
- cgi configuration

Its structure is inspired by nginx configuration syntax.

## Technical Choices

- C++98 to enforce deep understanding of manual memory and object design
- Non-blocking sockets to handle concurrency without threads
- Event-driven architecture for scalability
- Clear separation between parsing, routing, and response generation

### Example Request Flow

1. Client connects to server socket
2. Server accepts connection (non-blocking)
3. Event loop detects readable FD
4. Raw HTTP request is read into buffer
5. RequestParser parses headers/body
6. RequestHandler resolves route
7. Response is built
8. Event loop sends response
9. Connection is closed

## Resources
**HTTP & Networking**
- RFC 7230 — Hypertext Transfer Protocol (HTTP/1.1)
- Beej’s Guide to Network Programming
- Linux man 2 socket
- Linux man 2 poll
- Linux man 2 epoll

**CGI**
- CGI/1.1 Specification
- Python CGI documentation

### Use of AI

AI tools were used for:
- Clarifying HTTP specification details
- Understanding edge cases in request parsing
- Reviewing architectural decisions
- Improving documentation clarity

> AI was not used to generate the final production code without understanding. All architectural decisions and implementations were validated and adapted manually.

### Learning Outcomes
- Deep understanding of HTTP/1.1 HTTP/1.0 internals
- Practical socket programming experience
- Event-driven system design
- Robust parsing and error handling
- Configuration-driven architecture