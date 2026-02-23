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

static bool methodFromString (const std::string& s, HttpMethod& out)
{
    if (s == "GET") { out = GET; return true; }
    if (s == "POST") {out = POST; return true; }
    if (s == "DELETE") {out = DELETE; return true; }
    return false;
}

static std::string reasonPhraseForStatus(int code)
{
    if (code == 405) return "Method Not Allowed";
    if (code == 501) return "Not Implemented";
    return "Error";
}

static std::string buildAllowHeader(const std::set<HttpMethod>& allowed)
{
    std::string allow;
    for (std::set<HttpMethod>::const_iterator it = allowed.begin(); it != allowed.end(); ++it)
    {
        if (!allow.empty()) allow += ", ";
        if (*it == GET) allow += "GET";
        else if (*it == POST) allow += "POST";
        else if (*it == DELETE) allow += "DELETE";
    }
    return allow;
}

static bool checkMethod(const HttpRequest& req, const RuntimeLocation* loc, HttpResponse& res, HttpMethod& outMethod)
{
    if (!methodFromString(req.method, outMethod))
    {
        res.status_code = 501;
        res.reason_phrase = reasonPhraseForStatus(501);
        res.body = "Method not implemented\n";
        return false;
    }

    if (loc)
    {
        const std::set<HttpMethod>& allowed = loc->getAllowedMethods();
        if (!allowed.empty() && allowed.find(outMethod) == allowed.end())
        {
            res.status_code = 405;
            res.reason_phrase = reasonPhraseForStatus(405);

            std::string allow = buildAllowHeader(allowed);
            if (!allow.empty())
                res.headers["Allow"] = allow;
            
            res.body = "Method not allowed\n";
            return false;
        }
    }
    return true;
}

HttpResponse RequestHandler::handle(const HttpRequest& req,
                                    const RuntimeServer* server)
{
    HttpResponse res;
    //server is non-null in Connection:
    const RuntimeLocation* loc = matchLocation(req.uri, *server);


    if (!loc)
    {
        res.status_code = 404;
        res.reason_phrase = "Not Found";
        res.body = "No matching location\n";
        return res;
    }

    //redirection handling (return directive)
    if (loc->getHasReturn())
    {
        const ReturnRule& r = loc->getRedirect();

        //if r.target is empty for bad config:
        if (r.target.empty())
        {
            res.status_code = 500;
            res.reason_phrase = "Internal Server Error";
            res.body = "Invalid return directive\n";
            return res;
        }

        res.status_code = r.status_code;

        //reason phrase:
        if (r.status_code == 301)
            res.reason_phrase = "Moved Permanently";
        else if (r.status_code == 302)
            res.reason_phrase = "Found";
        else
            res.reason_phrase = "Redirect";
        
        //location header:
        res.headers["Location"] = r.target;

        //optional body (keeps curl redable:)
        res.body= "Redirection to " + r.target + "\n";
        return res;
    }

    HttpMethod m;

    if (!checkMethod(req, loc, res, m))
        return res;
    
    // TEMP until post/delete:

    if (m == POST || m == DELETE)
    {
        res.status_code = 501;
        res.reason_phrase = reasonPhraseForStatus(501);
        res.body = "Not implemented yet\n";
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