#include "httpCore/CgiHandler.hpp"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>

// ─── helpers ──────────────────────────────────────────────

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

// ─── constructor / matchCgiExtension ──────────────────────

CgiHandler::CgiHandler(const HttpRequest& req, std::string fsPath, const RuntimeLocation* loc)
    : _req(req), _loc(loc), _scriptPath(fsPath)
{
    _cgiBinary = takeBinary(fsPath, _loc);
}

bool CgiHandler::matchCgiExtension(std::string fsPath, const RuntimeLocation* loc) {
    size_t pos = fsPath.rfind(".");
    if (pos == std::string::npos)
        return false;
    std::string ext = fsPath.substr(pos);
    const std::map<std::string, std::string>& cgi_exec = loc->getCGIExec();
    std::map<std::string, std::string>::const_iterator it = cgi_exec.find(ext);
    return (it != cgi_exec.end());
}

// ─── buildEnv ─────────────────────────────────────────────
// CGI/1.1 environment variables (RFC 3875)

char** CgiHandler::buildEnv() {
    std::vector<std::string> env;

    env.push_back("REQUEST_METHOD=" + _req.method);
    env.push_back("QUERY_STRING=" + _req.query);
    env.push_back("SCRIPT_NAME=" + _req.path);
    env.push_back("SCRIPT_FILENAME=" + _scriptPath);
    env.push_back("PATH_INFO=" + _req.path);
    env.push_back("SERVER_PROTOCOL=" + _req.version);
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("REDIRECT_STATUS=200");

    // forward relevant HTTP headers
    std::string ct = _req.getHeader("Content-Type");
    if (!ct.empty())
        env.push_back("CONTENT_TYPE=" + ct);

    std::string cl = _req.getHeader("Content-Length");
    if (!cl.empty())
        env.push_back("CONTENT_LENGTH=" + cl);

    std::string host = _req.getHeader("Host");
    if (!host.empty())
        env.push_back("SERVER_NAME=" + host);

    // allocate char** array
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

// ─── parseCgiOutput ───────────────────────────────────────
// CGI scripts output: "Header: value\r\n...\r\n\r\nBody"
// At minimum: "Content-Type: text/html\n\nBody"

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
        // no header separator — treat everything as body
        resp.status_code = 200;
        resp.reason_phrase = "OK";
        resp.headers["Content-Type"] = "text/html";
        resp.body = out;
        return resp;
    }

    // parse headers
    std::string headerBlock = out.substr(0, sep);
    resp.body = out.substr(sep + sepLen);

    std::istringstream iss(headerBlock);
    std::string line;
    resp.status_code = 200;
    resp.reason_phrase = "OK";

    while (std::getline(iss, line)) {
        // remove trailing \r
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;
        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        // trim leading space
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

// ─── execute ──────────────────────────────────────────────

HttpResponse CgiHandler::execute() {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        HttpResponse err;
        err.status_code = 500;
        err.reason_phrase = "Internal Server Error";
        err.body = "pipe() failed";
        return err;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        HttpResponse err;
        err.status_code = 500;
        err.reason_phrase = "Internal Server Error";
        err.body = "fork() failed";
        return err;
    }

    if (pid == 0) {
        // ─── child process ───
        close(pipefd[0]);                      // close read-end
        dup2(pipefd[1], STDOUT_FILENO);        // redirect stdout → pipe
        close(pipefd[1]);                      // close original write-end after dup

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

    // ─── parent process ───
    close(pipefd[1]); // close write-end

    // read CGI output with timeout
    std::string output;
    char buffer[4096];
    ssize_t n;
    time_t startTime = time(NULL);

    while (true) {
        // check timeout
        if (difftime(time(NULL), startTime) >= CGI_TIMEOUT) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            close(pipefd[0]);

            HttpResponse err;
            err.status_code = 504;
            err.reason_phrase = "Gateway Timeout";
            err.body = "CGI script timed out";
            return err;
        }

        n = read(pipefd[0], buffer, sizeof(buffer));
        if (n > 0)
            output.append(buffer, n);
        else if (n == 0)
            break; // EOF — child closed its stdout
        else {
            if (errno == EINTR)
                continue;
            break; // read error
        }
    }
    close(pipefd[0]);

    int status;
    waitpid(pid, &status, 0);

    // check if child exited with error
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0 && output.empty()) {
        HttpResponse err;
        err.status_code = 502;
        err.reason_phrase = "Bad Gateway";
        err.body = "CGI script exited with error";
        return err;
    }

    return parseCgiOutput(output);
}