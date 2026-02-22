#include "resolver/RuntimeServer.hpp"
#include "network/RequestHandler.hpp"
#include "httpCore/HttpResponse.hpp"
#include "resolver/RuntimeLocation.hpp"

static bool isPrefixMatch(const std::string& uri, const std::string& locPath)
{
    //exact match:
    if (uri == locPath)
        return true;
    // / matches everything:
    if (locPath == "/")
        return true;
    
    // must start with locPath
    if (uri.size() < locPath.size())
        return false;
    
    if (uri.size() == locPath.size())
        return true;
    return (uri[locPath.size()] == '/');
}

static const RuntimeLocation* matchLocation (const std::string& uri, const RuntimeServer& server)
{
    const std::vector<RuntimeLocation>& locs = server.getLocations();

    size_t i = 0;
    while (i < locs.size())
    {
        const RuntimeLocation& loc = locs[i];
        if (isPrefixMatch(uri, loc.getPath()))
            return &loc; // pointing into vector owned by server
        ++i;
    }
    return NULL;
}

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
    const RuntimeLocation* loc = matchLocation(req.uri, *server);

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

    res.body += "matched=";
    if (loc)
        res.body +=loc->getPath();
    else
        res.body += "(none)";
    res.body += "\n";

    return res;
}