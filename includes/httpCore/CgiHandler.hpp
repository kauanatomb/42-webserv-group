#pragma once

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../resolver/RuntimeLocation.hpp"

class CgiHandler {
    public:
        CgiHandler(const HttpRequest& req, const RuntimeLocation* loc);

        HttpResponse execute();

    private:
        const HttpRequest& req_;
        const RuntimeLocation* loc_;
        std::string scriptPath_;
        std::string cgiBinary_;
        
        // helpers
        void setupEnvironment();
        std::string readStdin();          // POST
        HttpResponse parseCgiOutput();    // convert stdout script to HttpResponse
};