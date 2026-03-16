#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <sstream>

class Logger {
    public:
        static void serverInit(const std::string& ip, int port);
        static void newConnection(int fd, const std::string& clientIp);
        static void request(int fd, const std::string& method, const std::string& uri);
        static void response(int fd, int status);
        static void cgiLaunch(int fd, const std::string& script);
        static void cgiDone(int fd, int status);
        static void timeout(int fd, const std::string& reason);
        static void disconnection(int fd);
        static void error(const std::string& msg);
};

#endif