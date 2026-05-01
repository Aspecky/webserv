#include "Response/HttpResponse.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <ctime>


// ----------- static helpers -----------

static std::string extractBoundary(const std::string& contentType)
{
    size_t pos = contentType.find("boundary=");
    if (pos == std::string::npos)
        return "";
    std::string b = contentType.substr(pos + 9);
    if (!b.empty() && b[0] == '"')
        b = b.substr(1, b.size() - 2);
    return b;
}

static std::string extractFilename(const std::string& disposition)
{
    size_t pos = disposition.find("filename=\"");
    if (pos == std::string::npos)
        return "";
    pos += 10;
    size_t end = disposition.find('"', pos);
    return (end == std::string::npos) ? "" : disposition.substr(pos, end - pos);
}

static std::string getDisposition(const std::string& partHeaders)
{
    std::string lower = partHeaders;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    size_t pos = lower.find("content-disposition:");
    if (pos == std::string::npos)
        return "";
    size_t start = pos + 20;
    size_t end = lower.find("\r\n", start);
    std::string value = partHeaders.substr(start, (end == std::string::npos ? partHeaders.size() : end) - start);
    size_t s = value.find_first_not_of(" \t");
    return (s == std::string::npos) ? "" : value.substr(s);
}


// ----------- POST handler -----------

void HttpResponse::_handlePost(const HttpRequest& req, const LocationConfig& loc)
{
    std::string contentType = req.header("content-type");
    const std::string& body = req.body();

    // --- multipart/form-data (file uploads) ---
    if (contentType.find("multipart/form-data") != std::string::npos)
    {
        if (loc.upload_store.empty()) {
            _buildError(400);
            return;
        }

        std::string boundary = extractBoundary(contentType);
        if (boundary.empty()) {
            _buildError(400);
            return;
        }

        std::string delimiter = "--" + boundary;
        int filesUploaded = 0;
        size_t pos = 0;

        while (true)
        {
            size_t boundaryPos = body.find(delimiter, pos);
            if (boundaryPos == std::string::npos)
                break;

            pos = boundaryPos + delimiter.size();

            if (body.compare(pos, 2, "--") == 0)
                break;

            if (body.compare(pos, 2, "\r\n") == 0)
                pos += 2;

            size_t headerEnd = body.find("\r\n\r\n", pos);
            if (headerEnd == std::string::npos)
                break;

            std::string partHeaders = body.substr(pos, headerEnd - pos);
            pos = headerEnd + 4;

            size_t bodyEnd = body.find("\r\n" + delimiter, pos);
            if (bodyEnd == std::string::npos)
                break;

            std::string partBody = body.substr(pos, bodyEnd - pos);
            pos = bodyEnd + 2;

            std::string disposition = getDisposition(partHeaders);
            std::string filename = extractFilename(disposition);

            if (filename.empty())
                continue;

            // strip directory traversal
            size_t slash = filename.rfind('/');
            if (slash != std::string::npos)
                filename = filename.substr(slash + 1);
            if (filename.empty())
                filename = "upload_" + _toString((size_t)time(NULL));

            std::string dest = loc.upload_store + "/" + filename;
            std::ofstream out(dest.c_str(), std::ios::binary);
            if (!out.is_open()) {
                _buildError(500);
                return;
            }
            out.write(partBody.c_str(), partBody.size());
            out.close();
            filesUploaded++;
        }

        if (filesUploaded == 0) {
            _buildError(400);
            return;
        }

        _statusCode                  = 201;
        _statusText                  = _statusMsg(_statusCode);
        _body                        = "Files uploaded successfully.";
        _headers["Content-Type"]     = "text/plain";
        _headers["Content-Length"]   = _toString(_body.size());
    }
    // --- application/x-www-form-urlencoded ---
    else if (contentType.find("application/x-www-form-urlencoded") != std::string::npos)
    {
        if (body.empty()) {
            _buildError(400);
            return;
        }

        std::ostringstream html;
        html << "<html><body><h2>Form Received</h2><ul>";

        std::string remaining = body;
        while (!remaining.empty())
        {
            size_t amp = remaining.find('&');
            std::string pair = (amp == std::string::npos) ? remaining : remaining.substr(0, amp);
            remaining = (amp == std::string::npos) ? "" : remaining.substr(amp + 1);

            size_t eq = pair.find('=');
            if (eq != std::string::npos)
            {
                std::string key = pair.substr(0, eq);
                std::string val = pair.substr(eq + 1);
                html << "<li><b>" << key << "</b>: " << val << "</li>";
            }
        }
        html << "</ul></body></html>";

        _body                        = html.str();
        _statusCode                  = 200;
        _statusText                  = _statusMsg(_statusCode);
        _headers["Content-Type"]     = "text/html";
        _headers["Content-Length"]   = _toString(_body.size());
    }
    // --- raw body fallback ---
    else
    {
        if (loc.upload_store.empty()) {
            _buildError(400);
            return;
        }

        std::string dest = loc.upload_store + "/upload_" + _toString((size_t)time(NULL));
        std::ofstream out(dest.c_str(), std::ios::binary);
        if (!out.is_open()) {
            _buildError(500);
            return;
        }
        out.write(body.c_str(), body.size());
        out.close();

        _statusCode                  = 201;
        _statusText                  = _statusMsg(_statusCode);
        _body                        = "Uploaded successfully.";
        _headers["Content-Type"]     = "text/plain";
        _headers["Content-Length"]   = _toString(_body.size());
    }
}
