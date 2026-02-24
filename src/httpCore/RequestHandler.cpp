#include "resolver/RuntimeServer.hpp"
#include "httpCore/RequestHandler.hpp"
#include "httpCore/HttpResponse.hpp"
#include "resolver/RuntimeLocation.hpp"
//includes for file existence/type and reading
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>



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
    const RuntimeLocation* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < locs.size(); i++)
    {
        const RuntimeLocation& loc = locs[i];
        const std::string& p = loc.getPath();

        if (isPrefixMatch(uri, p) && p.size() >= bestLen)
        {
            best = &loc;
            bestLen = p.size();
        }
    }
    return best;
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
static bool isDirectory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return false;
    return S_ISDIR(st.st_mode);
}


static std::string joinPath(const std::string& a, const std::string& b)
{
    if (a.empty()) return b;
    if (b.empty()) return a;
    if (a[a.size() - 1] == '/' && b[0] == '/')
        return a + b.substr(1);
    if (a[a.size() - 1] != '/' && b[0] != '/')
        return a + "/" + b;
    return a + b;
}

static std::string pickRoot(const RuntimeServer* server, const RuntimeLocation* loc)
{
    if (loc && !loc->getRoot().empty())
        return loc->getRoot();
    return server->getRoot();
}

static std::string stripLocationPrefix(const std::string& uri, const std::string& locPath)
{
    //uri starts with locPath, matchLocation() guarantee
    if (locPath == "/")
        return uri; // keep as is
    if (uri.size() == locPath.size())
        return "/"; //exact match
    return uri.substr(locPath.size()); // starts with /
}

static bool hasTrailingSlash(const std::string& s)
{
    return (!s.empty() && s[s.size() - 1] == '/');
}

static std::string resolvePath(const HttpRequest& req,
                               const RuntimeServer* server,
                               const RuntimeLocation* loc)
{
    std::string root = pickRoot(server, loc);
    //1- get uri path (basic)
    std::string uri = req.uri;
    std::string::size_type q = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);

    //2- take suffix after location prefix
    std::string suffix = stripLocationPrefix(uri, loc->getPath()); //begins with /

    //3- filesystem path = root + suffix
    std::string full = joinPath(root, suffix);

    //4- if uri ends with / or points to a directory -> append index[0] if exists
    const std::vector<std::string>& idx = loc->getIndex(); 
    if ((hasTrailingSlash(uri) || isDirectory(full)) && !idx.empty())
        full = joinPath(full, idx[0]);

    return full;
}

static bool endsWith(const std::string&s, const std::string& suf)
{
    if (s.size() < suf.size()) return false;
    return s.compare(s.size() - suf.size(), suf.size(), suf) == 0;
}

static std::string guessContentType(const std::string& path)
{
    if (endsWith(path, ".html") || endsWith(path, ".htm")) return "text/html";
    if (endsWith(path, ".css")) return "text/css";
    if (endsWith(path, ".js")) return "application/javascript";
    if (endsWith(path, ".png")) return "image/png";
    if (endsWith(path, ".jpg") || endsWith(path, ".jpeg")) return "image/jpeg";
    if (endsWith(path, ".gif")) return "image/gif";
    return "text/plain";
}

static HttpResponse serveFileGET (const std::string& path)
{
    HttpResponse res;

    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in.is_open())
    {
        if (errno == ENOENT)
        {
            res.status_code = 404;
            res.reason_phrase = "Not Found";
            res.body = "File not found\n";
        }
        else if (errno == EACCES)
        {
            res.status_code = 403;
            res.reason_phrase = "Forbidden";
            res.body = "Forbidden\n";
        }
        else
        {
            res.status_code = 500;
            res.reason_phrase = "Internal Server Error";
            res.body = "Internal Server Error\n";
        }
        res.headers["Content-Type"] = guessContentType(path);
        return res;
    }
    std::ostringstream ss;
    ss << in.rdbuf();

    res.status_code = 200;
    res.reason_phrase = "OK";
    res.body = ss.str();
    res.headers["Content-Type"] = guessContentType(path);
    return res;
}

HttpResponse RequestHandler::handle(const HttpRequest& req,
                                    const RuntimeServer* server)
{
    HttpResponse res;
    //server is non-null in Connection:

    //1- choose the best location from config
    const RuntimeLocation* loc = matchLocation(req.uri, *server);


    if (!loc)
    {
        res.status_code = 404;
        res.reason_phrase = "Not Found";
        res.body = "No matching location\n";
        return res;
    }

    //redirection handling (return directive)
    //2- redirect check:
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
        if (r.status_code == 301) res.reason_phrase = "Moved Permanently";
        else if (r.status_code == 302) res.reason_phrase = "Found";
        else res.reason_phrase = "Redirect";
        
        //location header:
        res.headers["Location"] = r.target;

        //optional body (keeps curl redable:)
        res.body= "Redirection to " + r.target + "\n";
        return res;
    }
    //3- method check 405/501
    HttpMethod m;

    if (!checkMethod(req, loc, res, m))
        return res;

    // 4- resolve filesystem path
    if (m == GET)
    {
        std::string path = resolvePath(req, server, loc);
        //5- serve file (404/403/200)
        return serveFileGET(path);
    }
    
    // TEMP until post/delete:

    if (m == POST || m == DELETE)
    {
        res.status_code = 501;
        res.reason_phrase = reasonPhraseForStatus(501);
        res.body = "Not implemented yet\n";
        return res;
    }

    //sfety fallback for compiler:
    res.status_code = 501;
    res.reason_phrase = reasonPhraseForStatus(501);
    res.body = "not implemented test for compiler\n"; //CHANGE LATER
    return res;
}