#pragma once

#include "httpCore/HttpRequest.hpp"
#include "httpCore/HttpResponse.hpp"

class RuntimeServer;

class RequestHandler 
{
    public: 
        HttpResponse handle(const HttpRequest& req, const RuntimeLocation* loc );
};
