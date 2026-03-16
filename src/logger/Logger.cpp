#include "logger/Logger.hpp"

void Logger::serverInit(const std::string& ip, int port) {
    std::cout << "[SERVER] Listening on " << ip << ":" << port << std::endl;
}

void Logger::newConnection(int fd, const std::string& clientIp) {
    std::cout << "[CONNECT] New connection fd=" << fd
                << " from " << clientIp << std::endl;
}

void Logger::request(int fd, const std::string& method, const std::string& uri) {
    std::cout << "[REQUEST] fd=" << fd
                << " " << method << " " << uri << std::endl;
}

void Logger::response(int fd, int status) {
    std::cout << "[RESPONSE] fd=" << fd
                << " status=" << status << std::endl;
}

void Logger::cgiLaunch(int fd, const std::string& script) {
    std::cout << "[CGI] fd=" << fd
                << " launched " << script << std::endl;
}

void Logger::cgiDone(int fd, int status) {
    std::cout << "[CGI] fd=" << fd
                << " finished status=" << WEXITSTATUS(status) << std::endl;
}

void Logger::timeout(int fd, const std::string& reason) {
    std::cout << "[TIMEOUT] fd=" << fd
                << " reason=" << reason << std::endl;
}

void Logger::disconnection(int fd) {
    std::cout << "[DISCONNECT] fd=" << fd << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cout << "[ERROR] " << msg << std::endl;
}