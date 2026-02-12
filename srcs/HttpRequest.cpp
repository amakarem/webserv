/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:40:32 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/08 01:04:36 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

HttpRequest::HttpRequest()
{
    this->headersComplete = false;
    this->requestComplete = false;
    this->keepAlive = false;
    this->contentLength = false;
}

std::string cleanString(const std::string &input)
{
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])))
        ++start;

    if (start == input.size())
        return ""; // all whitespace

    size_t end = input.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(input[end])))
        --end;

    return input.substr(start, end - start + 1);
}

void HttpRequest::append(const char *data, size_t len)
{
    raw.append(data, len);
    // Headers not done yet
    if (!headersComplete)
    {
        size_t headerEnd = raw.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return; // wait for more data

        headersComplete = true;

        // Parse headers
        std::istringstream iss(raw.substr(0, headerEnd));
        iss >> method >> path >> version;

        std::string line;
        while (std::getline(iss, line))
        {
            if (line.find("Content-Type:") != std::string::npos)
                contentType = cleanString(line.substr(14));
            if (line.find("Content-Length:") != std::string::npos)
                contentLength = std::atoi(line.c_str() + 15);
            if (line.find("Connection:") != std::string::npos &&
                line.find("keep-alive") != std::string::npos)
                keepAlive = true;
        }
        // HTTP/1.1 default keep-alive
        if (version == "HTTP/1.1" && !keepAlive)
            keepAlive = true;
        if (contentLength > 0)
        {
            char tmpName[] = "./www/tmp/httpbodyXXXXXX"; // XXXXXX will be replaced
            int fd = mkstemp(tmpName);
            if (fd < 0)
                throw std::runtime_error("Cannot create temporary file for HTTP body");

            tmpFileName = tmpName; // store the filename for later use
            std::cout << "tmpFileName:" << tmpFileName << "\n";
            tmpFile.open(tmpFileName, std::ios::out | std::ios::binary);
            if (!tmpFile.is_open())
            {
                close(fd);
                throw std::runtime_error("Cannot open temporary file stream");
            }

            // Write any body bytes already in buffer
            size_t bodyStart = headerEnd + 4;
            if (raw.size() > bodyStart)
            {
                tmpFile.write(raw.data() + bodyStart, raw.size() - bodyStart);
                bodyReceived = raw.size() - bodyStart;
            }
            else
                bodyReceived = 0;
            if (bodyReceived >= contentLength)
            {
                tmpFile.close();
                requestComplete = true;
            }
        }
        else
            requestComplete = true;
        raw.erase(headerEnd + 4);
    }
    else if (!requestComplete)
    {
        if (!tmpFile.is_open())
            throw std::runtime_error("Tmp file not open for body");

        tmpFile.write(data, len);
        bodyReceived += len;

        if (bodyReceived >= contentLength)
        {
            tmpFile.close();
            requestComplete = true;
        }
    }
}

HttpRequest::HttpRequest(const std::string &request)
{
    this->keepAlive = false;
    std::istringstream iss(request);
    iss >> this->method >> this->path >> this->version;

    std::string line;
    while (std::getline(iss, line) && line != "\r")
    {
        if (line.find("Connection:") != std::string::npos)
        {
            if (line.find("Keep-Alive") != std::string::npos)
                this->keepAlive = true;
        }
    }

    // HTTP/1.1 defaults to keep-alive if not specified
    if (version == "HTTP/1.1" && !keepAlive)
        this->keepAlive = true;
    if (this->method.empty() || this->path.empty() || this->version.empty())
    {
        this->method = "";
        this->path = "";
        this->version = "";
    }
}

HttpRequest::~HttpRequest() {};

std::string HttpRequest::getPath() const
{
    return this->path;
}

std::string HttpRequest::getMethod() const
{
    return this->method;
}

std::string HttpRequest::getVersion() const
{
    return this->version;
}

bool HttpRequest::isKeepAlive() const
{
    return this->keepAlive;
}

bool HttpRequest::isHeadersComplete() const
{
    return this->headersComplete;
}

bool HttpRequest::isRequestComplete() const
{
    return this->requestComplete;
}

void HttpRequest::setRequestComplete()
{
    this->requestComplete = true;
}

bool HttpRequest::isPost() const
{
    if (this->method == "POST")
        return (true);
    return (false);
}

size_t HttpRequest::getContentLength() const
{
    return this->contentLength;
}

size_t HttpRequest::getBodyReceived() const
{
    return this->bodyReceived;
}

std::string HttpRequest::getContentType() const
{
    return this->contentType;
}

std::string HttpRequest::gettmpFileName() {
    return this->tmpFileName;
}

std::string HttpRequest::getMimeType()
{
    size_t dot = this->SCRIPT_NAME.rfind('.');
    if (dot == std::string::npos)
        return "text/html"; // default
    std::string ext = this->SCRIPT_NAME.substr(dot + 1);
    if (ext == "html" || ext == "htm" || ext == "php")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "json")
        return "application/json";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "txt")
        return "text/plain";
    if (ext == "ico")
        return "image/x-icon";

    return "application/octet-stream"; // fallback for unknown types
}

void HttpRequest::reset()
{
    if (!tmpFileName.empty()) {
        tmpFile.close();              // ensure file is closed
        unlink(tmpFileName.c_str());  // delete from disk
        tmpFileName.clear();
    }
    raw.clear();
    headersComplete = false;
    requestComplete = false;
    keepAlive = false;
    contentLength = 0;
    method.clear();
    path.clear();
    version.clear();
    cgiHeaders.clear();
    contentType.clear();
}

void HttpRequest::setcgiHeaders(std::string _cgiHeaders)
{
    this->cgiHeaders = _cgiHeaders;
}
std::string HttpRequest::getcgiHeaders() { return this->cgiHeaders; };


std::string	HttpRequest::getHttpCodeMsg(int httpCode)
{
	switch (httpCode)
	{
		case 200:
			return ("OK");
		case 201:
			return ("Created");
		case 204:
			return ("No Content");//Delete on a resource is successful
		case 301:
			return ("Moved Permanently");
        case 302:
            return ("Found");
		case 400:
			return ("Bad Request");
		case 403:
			return ("Forbidden");
		case 404:
			return ("Not Found");
		case 405:
			return ("Method Not Allowed");
		case 409:
			return ("Conflict");
		case 413:
			return ("Content Too Large");
		case 414:
			return ("URI Too Long");
		case 500:
			return ("Internal Server Error");
		default :
			return ("Not Implemented");
	}
}


std::string HttpRequest::buildHttpResponse(const std::string &body, int httpCode, size_t fileSize)
{
    std::ostringstream oss;
    std::string connectionHeader = "close";
    std::string newbody = body;
    if (this->keepAlive && httpCode == 200)
        connectionHeader = "Keep-Alive: timeout=5";
    if (httpCode != 200 && fileSize == 0)
        newbody = "<h1>" + getHttpCodeMsg(httpCode) + "</h1>";
    // if (httpCode == 200)
    // {
        std::string mime = (this->path.empty() ? "text/html" : getMimeType());
        oss << "HTTP/1.1 " << httpCode << " " << getHttpCodeMsg(httpCode) << "\r\n";
        if (fileSize > 0)
            oss << "Content-Length: " << fileSize << "\r\n"; // for large files
        else
            oss << "Content-Length: " << newbody.length() << "\r\n"; // small body
        if (!getcgiHeaders().empty())
        {
            std::istringstream hs(cgiHeaders);
            std::string line;
            while (std::getline(hs, line))
            {
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();
                oss << line + "\r\n";
            }
        }
        else
            oss << "Content-Type: " << mime << "\r\n";
        oss << "Connection: " << connectionHeader << "\r\n";
        oss << "\r\n";

        if (newbody.length() > 0)
            oss << newbody; // append body only if small message
    // }
    // else if (httpCode == 403)
    // {
    //     std::string msg = "<h1>403 Forbidden</h1>";
    //     oss << "HTTP/1.1 403 Forbidden\r\n";
    //     oss << "Content-Length: " << msg.length() << "\r\n";
    //     oss << "Content-Type: text/html\r\n";
    //     oss << "Connection: " << connectionHeader << "\r\n";
    //     oss << "\r\n";
    //     oss << msg;
    // }
    // else
    // {
    //     std::string msg = "<h1>404 Not Found</h1>";
    //     oss << "HTTP/1.1 404 Not Found\r\n";
    //     oss << "Content-Length: " << msg.length() << "\r\n";
    //     oss << "Content-Type: text/html\r\n";
    //     oss << "Connection: " << connectionHeader << "\r\n";
    //     oss << "\r\n";
    //     oss << msg;
    // }

    return oss.str();
}

std::string HttpRequest::getBody()
{
    if (method != "POST" && method != "PUT")
        return "";

    if (tmpFileName.empty())
        return "";

    // Read body from disk (for PHP CGI)
    std::ifstream f(tmpFileName, std::ios::binary);
    if (!f.is_open())
        return "";

    std::string body((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    return body;
}
