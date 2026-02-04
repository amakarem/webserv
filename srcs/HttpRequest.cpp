/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:40:32 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 23:13:22 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

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

std::string HttpRequest::getMimeType()
{
    size_t dot = this->path.rfind('.');
    if (dot == std::string::npos)
        return "text/html"; // default
    std::string ext = this->path.substr(dot + 1);
    if (ext == "html" || ext == "htm")
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

std::string HttpRequest::buildHttpResponse(const std::string &body, bool ok, size_t fileSize)
{
    std::ostringstream oss;
    std::string connectionHeader = "close";
    if (this->keepAlive)
        connectionHeader = "Keep-Alive: timeout=5";
    if (ok)
    {
        std::string mime = (this->path.empty() ? "text/html" : getMimeType());
        oss << "HTTP/1.1 200 OK\r\n";
        if (fileSize > 0)
            oss << "Content-Length: " << fileSize << "\r\n"; // for large files
        else
            oss << "Content-Length: " << body.length() << "\r\n"; // small body
        oss << "Content-Type: " << mime << "\r\n";
        oss << "Connection: " << connectionHeader << "\r\n";
        oss << "\r\n";

        if (body.length() > 0)
            oss << body; // append body only if small message
    }
    else
    {
        std::string msg = "<h1>404 Not Found</h1>";
        oss << "HTTP/1.1 404 Not Found\r\n";
        oss << "Content-Length: " << msg.length() << "\r\n";
        oss << "Content-Type: text/html\r\n";
        oss << "Connection: " << connectionHeader << "\r\n";
        oss << "\r\n";
        oss << msg;
    }

    return oss.str();
}
