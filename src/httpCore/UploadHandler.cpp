#include "httpCore/UploadHandler.hpp"
#include "resolver/RuntimeLocation.hpp"
#include "httpCore/ErrorHandler.hpp"
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <cctype>

static std::string itos_ulong(unsigned long v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
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

static bool hasTrailingSlash(const std::string& s) {
    return (!s.empty() && s[s.size() - 1] == '/');
}

static bool isDir(const std::string& p)
{
    struct stat st;
    if (stat(p.c_str(), &st) != 0) return false;
    return S_ISDIR(st.st_mode);
}

static bool canWriteDir(const std::string& p)
{
    return (access(p.c_str(), W_OK) == 0);
}

std::string UploadHandler::extractBoundary(const std::string& contentType)
{
    std::string key = "boundary=";
    size_t pos = contentType.find(key);
    if (pos == std::string::npos) return "";
    std::string boundary = contentType.substr(pos + key.size());
    if (!boundary.empty() && boundary[0] == '"')
    {
        size_t end = boundary.find('"', 1);
        if (end != std::string::npos)
            boundary = boundary.substr(1, end - 1);
    }
    size_t end = boundary.find_first_of(" \t;");
    if (end != std::string::npos)
        boundary = boundary.substr(0, end);
    return boundary;
}

std::string UploadHandler::extractFilenameFromDisposition(const std::string& line)
{
    std::string key = "filename=\"";
    size_t pos = line.find(key);
    if (pos == std::string::npos) return "";
    pos += key.size();
    size_t end = line.find('"', pos);
    if (end == std::string::npos) return "";
    std::string name = line.substr(pos, end - pos);
    // strip path components to avoid traversal
    size_t slash = name.rfind('/');
    size_t bslash = name.rfind('\\');
    if (slash != std::string::npos) name = name.substr(slash + 1);
    if (bslash != std::string::npos && bslash >= slash) name = name.substr(bslash + 1);
    return name;
}

MultipartPart UploadHandler::parseMultipartFirstFile(const std::string& body, const std::string& boundary)
{
    MultipartPart result;
    result.valid = false;

    std::string delim = "--" + boundary;
    size_t pos = body.find(delim);
    if (pos == std::string::npos) return result;
    pos += delim.size();
    if (pos < body.size() && body[pos] == '\r') ++pos;
    if (pos < body.size() && body[pos] == '\n') ++pos;

    size_t headerEnd = body.find("\r\n\r\n", pos);
    if (headerEnd == std::string::npos) return result;

    std::string headers = body.substr(pos, headerEnd - pos);
    // extract filename from Content-Disposition line
    size_t ls = 0;
    while (ls < headers.size())
    {
        size_t le = headers.find("\r\n", ls);
        if (le == std::string::npos) le = headers.size();
        std::string line = headers.substr(ls, le - ls);
        if (line.find("Content-Disposition") != std::string::npos)
            result.filename = extractFilenameFromDisposition(line);
        ls = le + 2;
    }

    size_t contentStart = headerEnd + 4;
    std::string endDelim = "\r\n" + delim;
    size_t contentEnd = body.find(endDelim, contentStart);
    if (contentEnd == std::string::npos) return result;

    result.content = body.substr(contentStart, contentEnd - contentStart);
    result.valid   = true;
    return result;
}

std::string UploadHandler::getExtensionFromFilename(const std::string& filename)
{
    size_t dot = filename.rfind('.');
    if (dot == std::string::npos || dot == 0) return "";
    std::string ext = filename.substr(dot);
    // only allow alphanum + dot
    for (size_t i = 1; i < ext.size(); ++i)
    {
        char c = ext[i];
        if (!std::isalnum(static_cast<unsigned char>(c)))
            return "";
    }
    return ext;
}

std::string UploadHandler::extensionFromContentType(const std::string& ct)
{
    if (ct.find("image/jpeg") != std::string::npos) return ".jpg";
    if (ct.find("image/png")  != std::string::npos) return ".png";
    if (ct.find("image/gif")  != std::string::npos) return ".gif";
    if (ct.find("text/plain") != std::string::npos) return ".txt";
    if (ct.find("text/html")  != std::string::npos) return ".html";
    if (ct.find("application/pdf")  != std::string::npos) return ".pdf";
    if (ct.find("application/json") != std::string::npos) return ".json";
    if (ct.find("application/octet-stream") != std::string::npos) return ".bin";
    return ".bin";
}

std::string UploadHandler::makeUploadFilename(const std::string& ext)
{
    unsigned long t = static_cast<unsigned long>(std::time(NULL));
    unsigned long pid = static_cast<unsigned long>(getpid());
    static unsigned long counter = 0;
    return "upload_" + itos_ulong(t) + "_" + itos_ulong(pid) + "_" + itos_ulong(counter++) + ext;
}

bool UploadHandler::writeFileBinary(const std::string& filePath, const std::string& data)
{
    int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return false;

    const char* buf = data.c_str();
    size_t total = data.size();
    size_t off = 0;
    while (off < total)
    {
        ssize_t n = write(fd, buf + off, total - off);
        if (n <= 0)
        {
            close(fd);
            return false;
        }
        off += static_cast<size_t>(n);
    }
    close(fd);
    return true;
}

HttpResponse UploadHandler::handle(const HttpRequest& req, const RuntimeLocation* loc)
{
    // 1- 413 check
    size_t maxBody = loc->getClientMaxBodySize();
    if (req.body.size() > maxBody)
        return ErrorHandler::build(413, loc);

    // 2- upload enabled?
    if (!loc->getHasUpload())
        return ErrorHandler::build(405, loc);

    // 3- validate upload_store
    const std::string& store = loc->getUploadStore();
    if (store.empty())
        return ErrorHandler::build(500, "upload_store not configured\n", loc);

    if (!isDir(store))
        return ErrorHandler::build(500, "upload_store is not a directory\n", loc);

    if (!canWriteDir(store))
        return ErrorHandler::build(403, "upload_store not writable\n", loc);

    // 4- determine actual file content and extension
    std::string fileContent = req.body;
    std::string ext = ".bin";
    std::string contentType = req.getHeader("Content-Type");

    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        std::string boundary = extractBoundary(contentType);
        if (!boundary.empty())
        {
            MultipartPart part = parseMultipartFirstFile(req.body, boundary);
            if (part.valid)
            {
                fileContent = part.content;
                if (!part.filename.empty())
                {
                    std::string fext = getExtensionFromFilename(part.filename);
                    if (!fext.empty())
                        ext = fext;
                }
            }
        }
    }
    else if (!contentType.empty())
    {
        ext = extensionFromContentType(contentType);
    }

    // 5- write file
    std::string filename = makeUploadFilename(ext);
    std::string fullPath = joinPath(store, filename);

    if (!writeFileBinary(fullPath, fileContent))
        return ErrorHandler::build(500, "failed to write upload file\n", loc);

    // 6- return 201 created
    HttpResponse res;
    res.status_code = 201;
    res.reason_phrase = "Created";
    res.headers["Content-Type"] = "text/plain";
    std::string base = req.path;
    if (!hasTrailingSlash(base)) base += "/";
    res.headers["Location"] = base + filename;
    res.body = "Created\n";
    return res;
}
