#include "resolver/RuntimeServer.hpp"
#include "httpCore/RequestHandler.hpp"
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

    // uri must be longer than locPath for prefix match
    if (uri.size() < locPath.size())
        return false;

    // must start with locPath
    if (uri.compare(0, locPath.size(), locPath) != 0)
        return false;
    
    //if same size, exact match would have returned true already
    //so here uri is longer -> enfore boundary:
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
    //server is non-null in Connection:
    const RuntimeLocation* loc = matchLocation(req.uri, *server);

    //redirection handling (return directive)
    if (loc && loc->getHasReturn())
    {
        const ReturnRule& r = loc->getRedirect();

        res.status_code = r.status_code;

        //reason phrase:
        if (r.status_code == 301)
            res.reason_phrase = "Moved Permanently";
        else if (r.status_code == 302)
            res.reason_phrase = "Found";
        else
            res.reason_phrase = "redirect";
        
        //location header:
        res.headers["Location"] = r.target;

        //optional body (keeps curl redable:)
        res.body= "Redirection to " + r.target + "\n";
        return res;
    }
    //tem debug response (until: method check + resolvePath + file serving are implemented)
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