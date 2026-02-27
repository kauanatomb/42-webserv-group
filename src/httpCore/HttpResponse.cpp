#include "httpCore/HttpResponse.hpp"
#include <sstream> //std::ostringstream

HttpResponse::HttpResponse()
    : status_code(200), reason_phrase("OK") {}

static std::string itos(size_t n)
{
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string HttpResponse::serialize() const
{
    std::ostringstream out;

    out << "HTTP/1.1 " << status_code << " " << reason_phrase << "\r\n";
    //copy heasders but ensure required ones exist
    std::map<std::string, std::string> h = headers;

    if (h.find("Content-Length") == h.end())
        h["Content-Length"] = itos(body.size());

    if (h.find ("Connection") == h.end())
        h["Connection"] = "close";

    if (h.find("Content-Type") == h.end())
        h["Content-Type"] = "text/plain";
    
    for (std::map<std::string, std::string>::const_iterator it = h.begin(); it != h.end(); ++it)
        out << it->first << ": " << it->second << "\r\n";

    out << "\r\n";
    out << body;

    return out.str();
}
