_This project has been created as part of the 42 curriculum by ktombola, falatrac, mcamaren._

# Webserv

## Description

Webserv is a custom HTTP/1.0 web server written in C++ as part of the 42 curriculum.
The goal of the project is to understand how a real web server works internally by reimplementing core networking and HTTP features from scratch, without relying on existing web frameworks.

## The server:

- Listens on one or multiple ports and hostnames
- Handles multiple concurrent client connections using non-blocking I/O
- Parses and validates HTTP/1.1 requests including chunked transfer encoding
- Serves static files and directory listings
- Executes CGI scripts asynchronously with timeout enforcement
- Applies configuration rules defined in a nginx-inspired configuration file

This project focuses on low-level networking (sockets, poll), HTTP protocol semantics, request parsing, process management, and event-driven architecture.

## Features

- HTTP/1.1 compliant request parsing (start line, headers, body)
- GET, POST, DELETE methods
- Static file serving with MIME type detection
- Directory listing (optional per location block)
- Custom error pages per server block
- File upload support
- Asynchronous CGI execution with configurable timeout (Python, Bash, PHP, etc.)
- Full HTTP header forwarding to CGI processes (RFC 3875)
- CGI processes isolated in their own process group
- CGI scripts executed in their own working directory for relative path resolution
- Multiple server blocks with independent ports, hostnames, and configurations
- Single event loop with non-blocking I/O across all connections and CGI pipes

## Architecture Overview

The server is structured around the following core components:

**ServerEngine**  
The central orchestrator. Responsible for:
- Creating and binding listening sockets
- Accepting client connections
- Running the main event loop via `poll`
- Dispatching read/write events to connections and CGI pipes
- Enforcing CGI and idle connection timeouts
- Managing graceful shutdown on SIGINT/SIGTERM via self-pipe pattern
- Cleaning up all file descriptors and child processes on exit

**Connection**  
Represents a single client connection. Responsible for:
- Buffered reading and writing on the client socket
- Owning the request parser state machine
- Owning the active CGI state (pid, pipes, I/O buffers)
- Driving CGI stdin writes and stdout reads as poll events fire
- Finalizing CGI output into an HTTP response
- Handling CGI timeout and process termination

**RequestParser**  
- Parses start line, headers, and body incrementally
- Handles `Content-Length` and chunked transfer encoding
- Validates HTTP format and enforces `client_max_body_size`

**HandlerResolver**  
- Matches request URI against configured location blocks
- Resolves file system paths from root and alias directives

**RequestHandler**  
- Applies method restrictions per location
- Deals with methods and errors

**CgiHandler**  
- Validates script path and binary existence before fork
- Forks child process with redirected stdin/stdout via pipes
- Isolates child in a new process group (`setpgid`)
- Closes all inherited file descriptors in the child before `execve`
- Sets working directory to the script's directory (`chdir`)
- Builds CGI environment per RFC 3875 including `HTTP_*` meta-variables
- Returns a `CgiState` struct tracked by the owning `Connection`

**Response Builder**
- Generates proper HTTP responses
- Adds required headers

**ErrorHandler**  
- Builds HTTP error responses for standard status codes
- Serves custom error pages when configured

**RuntimeConfig / ConfigParser**  
- Parses nginx-inspired configuration file
- Validates directives and builds per-server, per-location configuration trees
- Exposes typed accessors used throughout the server at runtime

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

if program is closed incorrectly use the following commands 

lsof -i :8080
kill -9 <PID>
and rerun 

## Configuration File

The configuration file defines:

- `server` blocks with independent `listen`, `server_name`, and `root`
- `location` blocks with route-specific rules
- `allowed_methods` per location
- `autoindex` for directory listing
- `error_page` for custom error responses
- `client_max_body_size` for upload limits
- `cgi_exec` to map file extensions to CGI binaries
- `upload_store` for file upload destination

Its structure is inspired by nginx configuration syntax.

## Technical Choices

- C++98 to enforce manual memory management and explicit object design
- Non-blocking sockets with a single `poll` loop to handle concurrency without threads
- Self-pipe pattern for safe signal handling within the event loop
- CGI process isolation via `setpgid` to prevent terminal signal propagation
- Clear separation between parsing, routing, CGI execution, and response generation

### Example Request Flow

**Static Request:**
1. Client connects to server socket
2. Server accepts connection (non-blocking)
3. `poll` detects readable fd
4. Raw HTTP request is read into buffer
5. RequestParser parses headers/body
6. RequestHandler resolves route
7. Response is built from static file
8. Event loop sends response
9. Connection is closed

**CGI Request:**
1. Client connects to server socket (same as steps 1-5 in Static Request)
2. Connection detects CGI location and → `CgiHandler::launch()` forks child
3. Child: redirects pipes, closes inherited fds, `chdir`, `execve`
4. Parent: `poll` monitors CGI stdout, CGI stdin, client socket, and timeout
5. Server writes request body to CGI stdin and reads CGI stdout as data becomes available
6. On completion: CGI output is parsed into an HTTP response and sent to the client
7. On timeout: child killed with `SIGKILL`, `waitpid`, pipes closed, 504 returned
8. Connection is closed

## Resources

**HTTP & Networking**

- RFC 7230 — Hypertext Transfer Protocol (HTTP/1.1)
- Beej’s Guide to Network Programming
- Linux man 2 socket
- Linux man 2 poll
- Linux man 2 chdir

**CGI**

- RFC 3875
- CGI/1.1 Specification
- Python CGI documentation


**Upload**

- RFC 7578

**Virtual Hosting**

- RFC 7151

### Use of AI

AI tools were used for:
- Clarifying HTTP specification details
- Understanding edge cases in request parsing
- Reviewing architectural decisions
- Improving documentation clarity

### Learning Outcomes

- Deep understanding of HTTP/1.1 HTTP/1.0 internals
- Practical socket programming experience
- Event-driven system design
- Robust parsing and error handling
- Configuration-driven architecture
