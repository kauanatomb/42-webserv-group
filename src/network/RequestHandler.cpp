#include "resolver/RuntimeServer.hpp"
#include "network/RequestHandler.hpp"
#include "httpCore/HttpResponse.hpp"

HttpResponse RequestHandler::handle(const HttpRequest& req,
                                    const RuntimeServer* server)
{
    HttpResponse res;

    if (!server)
    {
        res.status_code = 500;
        res.reason_phrase = "Internal Server Error";
        res.body = "Handler received null server pointer\n";
        return res;
    }

    res.status_code = 200;
    res.reason_phrase = "OK testing";

    res.body = "handler alive\n";
    res.body += "method=";
    res.body += req.method;
    res.body += "\n";

    res.body += "uri=";
    res.body += req.uri; 
    res.body += "\n";
    
    res.body += "root=";
    res.body += server->getRoot();
    res.body += "\n";

    return res;
}