#pragma once

#include "httpCore/HttpRequest.hpp"
#include "httpCore/HttpResponse.hpp"
#include <string>

class RuntimeLocation;

struct MultipartPart {
    std::string filename;
    std::string content;
    bool        valid;
};

class UploadHandler {
    public:
        static HttpResponse handle(const HttpRequest& req, const RuntimeLocation* loc);

    private:
        UploadHandler();

        // multipart/form-data helpers
        static std::string extractBoundary(const std::string& contentType);
        static std::string extractFilenameFromDisposition(const std::string& line);
        static MultipartPart parseMultipartFirstFile(const std::string& body, const std::string& boundary);

        // extension helpers
        static std::string getExtensionFromFilename(const std::string& filename);
        static std::string extensionFromContentType(const std::string& ct);

        // file writing
        static std::string makeUploadFilename(const std::string& ext);
        static bool        writeFileBinary(const std::string& filePath, const std::string& data);
    };
