#include "network/Connection.hpp"
#include "resolver/ServerResolver.hpp"
#include <sys/socket.h>

#ifdef DEBUG_LOG
# include "resolver/SocketKeyUtils.hpp"
# include <iostream>
#endif

// TEMP only allow calling handler without final parser
#ifdef TEMP_NO_PARSER
# include "httpCore/HttpRequest.hpp"
# include "httpCore/HttpResponse.hpp"
# include "httpCore/RequestHandler.hpp"
# include <string>
#endif

Connection::Connection(int fd, const RuntimeConfig& config, const SocketKey& socket_key)
    : _socket_fd(fd),
      _socket_key(socket_key),
      _read_buffer(),
      _write_buffer(),
      _config(config),
      _state(READING),
      _keep_alive(false) {}

bool Connection::wantsWrite() const {
    return _state == WRITING;
}

bool Connection::isClosed() const {
    return _state == CLOSED;
}

#ifdef TEMP_NO_PARSER
static std::string extractHostHeader(const std::string& buf)
{
    std::string host = "";
    std::string::size_type pos = buf.find("Host:");
    if (pos == std::string::npos)
        return host;

    pos += 5;
    while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == '\t'))
        ++pos;

    std::string::size_type end = buf.find("\r\n", pos);
    if (end == std::string::npos)
        return host;

    host = buf.substr(pos, end - pos);
    return host;
}

static void fillStubRequestFromStartLine(HttpRequest& req, const std::string& buf)
{
    // Defaults
    req.method = "GET";
    req.uri = "/";
    req.version = "HTTP/1.1";

    // Parse "METHOD SP URI SP VERSION\r\n"
    std::string::size_type lineEnd = buf.find("\r\n");
    if (lineEnd == std::string::npos)
        return;

    std::string startLine = buf.substr(0, lineEnd);

    std::string::size_type sp1 = startLine.find(' ');
    if (sp1 == std::string::npos)
        return;

    std::string::size_type sp2 = startLine.find(' ', sp1 + 1);
    if (sp2 == std::string::npos)
        return;

    req.method = startLine.substr(0, sp1);
    req.uri = startLine.substr(sp1 + 1, sp2 - (sp1 + 1));
    req.version = startLine.substr(sp2 + 1);
}
#endif

void Connection::onReadable() {
    (void)_keep_alive;

    char buffer[4096];
    ssize_t bytes = recv(_socket_fd, buffer, sizeof(buffer), 0);

    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }

    _read_buffer.append(buffer, bytes);

#ifdef STUB_RESPONSE
    _write_buffer =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "OK";
    _state = WRITING;
    return;
#endif

#ifdef TEMP_NO_PARSER
    // TEMP: bypass full parser to unblock Handler + location matching
    HttpRequest req;
    fillStubRequestFromStartLine(req, _read_buffer);

    std::string host = extractHostHeader(_read_buffer);

    const RuntimeServer* server = ServerResolver::resolve(_config, _socket_key, host);
    //handle null server here and remove it from handler

    if (!server)
    {
        HttpResponse res;
        res.status_code = 500;
        res.reason_phrase = "Internal Server Error";
        res.body = "Resolver returned NULL server \n";
        _write_buffer = res.serialize();
        _state = WRITING;
        return;
    }
    RequestHandler handler;
    HttpResponse res = handler.handle(req, server);

    _write_buffer = res.serialize();
    _state = WRITING;
    return;
#endif

    _state = PARSING;

    // Uncomment when Fran parser is merged:
    // if (_parser.parse(_read_buffer, _request)) {
    //     if (_parser.getHasError()) {
    //         int status = _parser.getErrorStatus();
    //         _response = HttpResponse::fromStatus(status);
    //     } else {
    //         _request.print();
    //         const RuntimeServer* server =
    //             ServerResolver::resolve(_config, _socket_key, _request.getHeader("Host"));
    //         if (!server) {
    //             _response = HttpResponse::fromStatus(500);
    //         } else {
    //             // RequestHandler handler;
    //             // _response = handler.handle(_request, server);
    //         }
    //     }
    //     _write_buffer = _response.serialize();
    //     _state = WRITING;
    // }
}

void Connection::onWritable() {
    ssize_t bytes = send(_socket_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
    if (bytes <= 0) {
        _state = CLOSED;
        return;
    }

    _write_buffer.erase(0, bytes);

    if (_write_buffer.empty()) {
        // if (_keep_alive) resetForNextRequest();
        // else
        _state = CLOSED;
    }
}