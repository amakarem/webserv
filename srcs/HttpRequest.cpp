/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:40:32 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 21:33:38 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"

std::string HttpRequest::extractPath(const std::string &request)
{
    std::istringstream iss(request);
    std::string method;
    std::string path;
    std::string version;

    if (!(iss >> method >> path >> version))
        return ""; // invalid request

    return path; // only the requested URL path
}

std::string HttpRequest::getMimeType(const std::string &path)
{
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "text/plain"; // default
    std::string ext = path.substr(dot + 1);
    if (ext == "html" || ext == "htm") return "text/html";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    if (ext == "txt") return "text/plain";
    if (ext == "ico") return "image/x-icon";

    return "application/octet-stream"; // fallback for unknown types
}

std::string HttpRequest::buildHttpResponse(const std::string &body, bool ok, size_t fileSize)
{
    std::ostringstream oss;

    if (ok)
    {
        std::string mime = (path.empty() ? "text/html" : getMimeType(path));
        oss << "HTTP/1.1 200 OK\r\n";
        if (fileSize > 0)
            oss << "Content-Length: " << fileSize << "\r\n"; // for large files
        else
            oss << "Content-Length: " << body.length() << "\r\n"; // small body
        oss << "Content-Type: " << mime << "\r\n";
        oss << "Connection: close\r\n";
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
        oss << "Connection: close\r\n";
        oss << "\r\n";
        oss << msg;
    }

    return oss.str();
}