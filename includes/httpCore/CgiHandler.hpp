#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "resolver/RuntimeLocation.hpp"
#include "network/Connection.hpp"
#include <vector>
#include <string>
#include <sys/types.h>

class CgiHandler {
    public:
        CgiHandler(const HttpRequest& req, const std::string& scriptFsPath, const RuntimeLocation* loc);
        static bool matchCgiExtension(const std::string& fsPath, const RuntimeLocation* loc);
        HttpResponse execute();

        CgiState launch();
        static HttpResponse parseCgiOutput(const std::string& out);

    private:
        const HttpRequest& _req;
        const RuntimeLocation* _loc;
        std::string _scriptPath;
        std::string _cgiBinary;
        
        // helpers
        char** buildEnv();
        void freeEnv(char** envp);
        int validateCgiPreconditions();
        void executeChild(int outfd[2], int infd[2]);
};