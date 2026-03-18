#include "httpCore/CgiHandler.hpp"
#include "httpCore/ErrorHandler.hpp"
#include "network/ServerEngine.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <cctype>

static std::string takeBinary(std::string path, const RuntimeLocation* loc) {
    size_t pos = path.rfind(".");
    if (pos == std::string::npos)
        return "";
    std::string ext = path.substr(pos);
    const std::map<std::string, std::string>& cgi_exec = loc->getCGIExec();
    std::map<std::string, std::string>::const_iterator it = cgi_exec.find(ext);
    if (it == cgi_exec.end())
        return "";
    return it->second;
}

static std::string intToStr(int n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

CgiHandler::CgiHandler(const HttpRequest& req, const std::string& scriptFsPath, const RuntimeLocation* loc)
    : _req(req), _loc(loc), _scriptPath(scriptFsPath)
{
    _cgiBinary = takeBinary(_scriptPath, _loc);
}

bool CgiHandler::matchCgiExtension(const std::string& fsPath, const RuntimeLocation* loc) {
    size_t pos = fsPath.rfind('.');
    if (pos == std::string::npos)
        return false;
    std::string ext = fsPath.substr(pos);
    const std::map<std::string, std::string>& cgi_exec = loc->getCGIExec();
    return (cgi_exec.find(ext) != cgi_exec.end());
}

// CGI/1.1 environment variables (RFC 3875)
char** CgiHandler::buildEnv() {
    std::vector<std::string> env;

    env.push_back("AUTH_TYPE=");
    env.push_back("REDIRECT_STATUS=200");
    env.push_back("REQUEST_METHOD=" + _req.method);
    env.push_back("QUERY_STRING=" + _req.query);
    env.push_back("SCRIPT_NAME=" + _req.path);
    env.push_back("SCRIPT_FILENAME=" + _scriptPath);
    env.push_back("PATH_INFO=" + _req.pathInfo);
    env.push_back("PATH_TRANSLATED=" + (_req.pathInfo.empty() ? "" : _loc->getRoot() + _req.pathInfo));
    env.push_back("SERVER_PROTOCOL=" + _req.version);
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("SERVER_SOFTWARE=webserv/1.0");
    env.push_back("SERVER_PORT=" + intToStr(_req.serverPort));
    env.push_back("REMOTE_ADDR=" + _req.clientIp);
    env.push_back("REMOTE_HOST=" + _req.clientIp);
    env.push_back("REMOTE_IDENT=");
    env.push_back("REMOTE_USER=");

    std::string ct = _req.getHeader("Content-Type");
    if (!ct.empty())
        env.push_back("CONTENT_TYPE=" + ct);

    std::string cl = _req.getHeader("Content-Length");
    if (!cl.empty())
        env.push_back("CONTENT_LENGTH=" + cl);
    else if (!_req.body.empty())
        env.push_back("CONTENT_LENGTH=" + intToStr(_req.body.size()));

    std::string host = _req.getHeader("Host");
    if (!host.empty())
        env.push_back("SERVER_NAME=" + host);

    // Forward all HTTP headers as HTTP_* meta-variables (RFC 3875 4.1.18)
    for (std::map<std::string, std::string>::const_iterator it = _req.headers.begin();
            it != _req.headers.end(); ++it) {
        std::string name = it->first;
        // Content-Type and Content-Length are passed without HTTP_ prefix
        if (name == "Content-Type" || name == "Content-Length")
            continue;
        // Convert header name: uppercase, replace '-' with '_', prepend HTTP_
        std::string var = "HTTP_";
        for (size_t i = 0; i < name.size(); ++i) {
            if (name[i] == '-')
                var += '_';
            else
                var += static_cast<char>(std::toupper(name[i]));
        }
        env.push_back(var + "=" + it->second);
    }

    char** envp = new char*[env.size() + 1];
    for (size_t i = 0; i < env.size(); ++i)
        envp[i] = strdup(env[i].c_str());
    envp[env.size()] = NULL;
    return envp;
}

void CgiHandler::freeEnv(char** envp) {
    if (!envp)
        return;
    for (int i = 0; envp[i]; ++i)
        free(envp[i]);
    delete[] envp;
}

HttpResponse CgiHandler::parseCgiOutput(const std::string& out) {
    HttpResponse resp;

    // find header/body separator (\r\n\r\n or \n\n)
    size_t sep = out.find("\r\n\r\n");
    size_t sepLen = 4;
    if (sep == std::string::npos) {
        sep = out.find("\n\n");
        sepLen = 2;
    }
    if (sep == std::string::npos) {
        resp.status_code = 200;
        resp.reason_phrase = "OK";
        resp.headers["Content-Type"] = "text/html";
        resp.body = out;
        return resp;
    }

    std::string headerBlock = out.substr(0, sep);
    resp.body = out.substr(sep + sepLen);

    std::istringstream iss(headerBlock);
    std::string line;
    resp.status_code = 200;
    resp.reason_phrase = "OK";

    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        if (!val.empty() && val[0] == ' ')
            val = val.substr(1);

        // CGI "Status:" header overrides HTTP status
        if (key == "Status") {
            std::istringstream ss(val);
            ss >> resp.status_code;
            std::getline(ss, resp.reason_phrase);
            if (!resp.reason_phrase.empty() && resp.reason_phrase[0] == ' ')
                resp.reason_phrase = resp.reason_phrase.substr(1);
        } else {
            resp.headers[key] = val;
        }
    }

    if (resp.headers.find("Content-Length") == resp.headers.end())
        resp.headers["Content-Length"] = intToStr(resp.body.size());

    return resp;
}

int CgiHandler::validateCgiPreconditions() {
    struct stat st;

    if (_req.body.size() > _loc->getClientMaxBodySize())
        return 413;
    if (stat(_scriptPath.c_str(), &st) < 0 || !S_ISREG(st.st_mode))
        return 404;
    if (stat(_cgiBinary.c_str(), &st) < 0)
        return 500;
    return 0;
}

// Child process: setup pipes, exec CGI script (never returns)
void CgiHandler::executeChild(int outfd[2], int infd[2]) {
    // Create new process group to isolate from terminal signals
    setpgid(0, 0);
    
    close(outfd[0]);  // close read-end of stdout pipe
    dup2(outfd[1], STDOUT_FILENO);  // redirect stdout -> pipe
    close(outfd[1]);

    close(infd[1]); // close write-end of stdin pipe
    dup2(infd[0], STDIN_FILENO); // redirect pipe -> stdin
    close(infd[0]);

    // Close all inherited FDs (listening sockets, signal pipe, etc.)
    for (int fd = 3; fd < 1024; ++fd)
        close(fd);

    // chdir to the script's directory for relative path file access
    std::string dir = _scriptPath.substr(0, _scriptPath.rfind('/'));
    if (!dir.empty())
        chdir(dir.c_str());

    char* argv[3];
    argv[0] = strdup(_cgiBinary.c_str());
    argv[1] = strdup(_scriptPath.c_str());
    argv[2] = NULL;

    char** envp = buildEnv();
    execve(_cgiBinary.c_str(), argv, envp);

    // execve only returns on failure
    free(argv[0]);
    free(argv[1]);
    freeEnv(envp);
    _exit(1);
}

CgiState CgiHandler::launch() {
    int preCheck = validateCgiPreconditions();
    
    CgiState state;
    state.error = preCheck; // 0 = ok
    if (preCheck != 0)
        return state;

    int outfd[2], infd[2];
    if (pipe(outfd) < 0) {
        state.error = 500;
        return state;
    }

    if (pipe(infd) < 0) {
        close(outfd[0]); close(outfd[1]);
        state.error = 500;
        return state;
    }
    pid_t pid = fork();
    if (pid < 0) {
        close(outfd[0]); close(outfd[1]);
        close(infd[0]);  close(infd[1]);
        state.error = 500;
        return state;
    }
    if (pid == 0)
        executeChild(outfd, infd); // never returns

    // parent
    close(outfd[1]);
    close(infd[0]);

    fcntl(infd[1],  F_SETFL, O_NONBLOCK);
    fcntl(outfd[0], F_SETFL, O_NONBLOCK);

    state.pid          = pid;
    state.stdin_fd     = infd[1];   // server writes here
    state.stdout_fd    = outfd[0];  // server reads here
    state.input        = _req.body;
    state.done         = false;
    state.input_offset = 0;
    state.error        = 0;
    return state;
}