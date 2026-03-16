#include "network/Connection.hpp"
#include "logger/Logger.hpp"
#include "resolver/HandlerResolver.hpp"
#include "httpCore/ErrorHandler.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include "httpCore/RequestHandler.hpp"
#include "httpCore/CgiHandler.hpp"
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <ctime>
#include <signal.h>

Connection::Connection(int fd, const RuntimeConfig& config, const SocketKey& socket_key)
    : _socket_fd(fd),
      _socket_key(socket_key),
      _read_buffer(),
      _write_buffer(),
      _active_loc(NULL),
      _config(config),
      _state(READING),
      _last_activity(time(NULL)),
      _cgi_start_time(0) {}

void Connection::markActivity() {
    _last_activity = time(NULL);
}

bool Connection::isIdleSince(time_t now, int seconds) const {
    return difftime(now, _last_activity) >= seconds;
}

time_t Connection::getCgiStartTime() const { return _cgi_start_time; }

void Connection::handleCgiTimeout() {
    if (_cgi.pid > 0) {
        kill(_cgi.pid, SIGKILL);
        waitpid(_cgi.pid, NULL, WNOHANG);
    }
    if (_cgi.stdin_fd != -1) { close(_cgi.stdin_fd); _cgi.stdin_fd = -1; }
    if (_cgi.stdout_fd != -1) { close(_cgi.stdout_fd); _cgi.stdout_fd = -1; }
    _cgi.pid = -1;
    _cgi.done = true;
    _cgi_start_time = 0;

    Logger::timeout(_socket_fd, "CGI timeout");
    _response = ErrorHandler::build(504, _active_loc);
    _write_buffer = _response.serialize();
    _state = WRITING;
}

bool Connection::wantsWrite() const {
    return _state == WRITING;
}

bool Connection::isClosed() const {
    return _state == CLOSED;
}

void Connection::processRequest() {
    Logger::request(_socket_fd, _request.method, _request.path);
    const RuntimeLocation* loc = HandlerResolver::resolve(_config, _socket_key, _request, _socket_fd);
    if (!loc) {
        _response = ErrorHandler::build(500, "Resolver returned NULL location \n", loc);
        _write_buffer = _response.serialize();
        _state = WRITING;
        return;
    }
    _active_loc = loc;
    std::string fsPath = HandlerResolver::resolvePath(_request, loc);
    if (fsPath.empty()) {
        _response = ErrorHandler::build(400, loc);
        _write_buffer = _response.serialize();
        _state = WRITING;
        return;
    }
    if (loc->getHasCGI() && CgiHandler::matchCgiExtension(fsPath, loc)) {
        _cgi = CgiHandler(_request, fsPath, loc).launch();
        Logger::cgiLaunch(_socket_fd, fsPath);
        if (_cgi.error != 0) {
            _response = ErrorHandler::build(_cgi.error, loc);
            _write_buffer = _response.serialize();
            _state = WRITING;
        } else {
            _cgi_start_time = time(NULL);
            if (_cgi.input.empty()) {
                close(_cgi.stdin_fd);
                _cgi.stdin_fd = -1;
                _state = CGI_READING;
            } else {
                _state = CGI_WRITING;
            }
        }
        return;
    }

    RequestHandler handler;
    _response = handler.handle(_request, loc);
    _write_buffer = _response.serialize();
    _state = WRITING;
}

void Connection::onReadable() {
    char buffer[4096];
    ssize_t bytes = recv(_socket_fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
        markActivity();
        _read_buffer.append(buffer, static_cast<size_t>(bytes));
    } else {
        _state = CLOSED;
        return;
    }

    _state = PARSING;

    if (!_parser.parse(_read_buffer, _request)) {

        _state = READING;
        return;
    }

    if (_parser.hasError()) {
        int status = _parser.getErrorStatus();
        _response = ErrorHandler::build(status, (const RuntimeLocation*)NULL);
        _write_buffer = _response.serialize();
        _state = WRITING;
        return;
    }

    processRequest();
}

void Connection::onWritable() {
    if (_write_buffer.empty())
        return;

    ssize_t bytes = send(_socket_fd, _write_buffer.c_str(), _write_buffer.size(), 0);
    if (bytes > 0) {
        markActivity();
        _write_buffer.erase(0, static_cast<size_t>(bytes));
    } else {
        _state = CLOSED;
        return;
    }

    if (_write_buffer.empty()) {
        Logger::response(_socket_fd, _response.status_code);
        _state = CLOSED;
    }
}

int  Connection::getCgiStdinFd()  const { return _cgi.stdin_fd; }
int  Connection::isCgiDone()  const { return _cgi.done; }
int  Connection::getCgiStdoutFd() const { return _cgi.stdout_fd; }
bool Connection::hasCgi()         const { return _cgi.active(); }

void Connection::onCgiWritable() {
    if (_cgi.stdin_fd == -1)
        return;

    if (_cgi.input_offset >= _cgi.input.size()) {
        close(_cgi.stdin_fd);
        _cgi.stdin_fd = -1;
        _state = CGI_READING;
        return;
    }

    const char* data = _cgi.input.c_str() + _cgi.input_offset;
    size_t remaining = _cgi.input.size() - _cgi.input_offset;

    ssize_t w = write(_cgi.stdin_fd, data, remaining);
    if (w > 0) {
        _cgi.input_offset += static_cast<size_t>(w);
        markActivity();
        return;
    } else {
        close(_cgi.stdin_fd);
        _cgi.stdin_fd = -1;
        _state = CGI_READING;
    }
}

void Connection::onCgiReadable() {
    if (_cgi.stdout_fd == -1)
        return;

    char buffer[4096];
    ssize_t n = read(_cgi.stdout_fd, buffer, sizeof(buffer));

    if (n > 0) {
        _cgi.output.append(buffer, static_cast<size_t>(n));
        markActivity();
        return;
    } else
        _cgi.done = true; // EOF
}

void Connection::finalizeCgi() {
    int status = 0;
    waitpid(_cgi.pid, &status, WNOHANG);

    Logger::cgiDone(_socket_fd, status);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0 && _cgi.output.empty())
        _response = ErrorHandler::build(502, _active_loc);
    else
        _response = CgiHandler::parseCgiOutput(_cgi.output);

    if (_cgi.stdout_fd != -1) {
        close(_cgi.stdout_fd);
        _cgi.stdout_fd = -1;
    }
    if (_cgi.stdin_fd != -1) {
        close(_cgi.stdin_fd);
        _cgi.stdin_fd = -1;
    }
    _cgi.pid = -1;
    _cgi_start_time = 0;

    _write_buffer = _response.serialize();
    _state = WRITING;
}