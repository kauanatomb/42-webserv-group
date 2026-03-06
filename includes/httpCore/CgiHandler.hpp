#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "resolver/RuntimeLocation.hpp"
#include <vector>
#include <string>

class CgiHandler {
    public:
        CgiHandler(const HttpRequest& req, std::string fsPath, const RuntimeLocation* loc);
        static bool matchCgiExtension(std::string fsPath, const RuntimeLocation* loc);
        HttpResponse execute();

    private:
        static const int CGI_TIMEOUT = 5; // seconds

        const HttpRequest& _req;
        const RuntimeLocation* _loc;
        std::string _scriptPath;
        std::string _cgiBinary;
        
        // helpers
        char** buildEnv();
        void freeEnv(char** envp);
        HttpResponse parseCgiOutput(const std::string& out);
};