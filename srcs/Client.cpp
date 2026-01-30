/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/30 18:55:17 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int _fd)
{
    this->fd = _fd;
    this->file = NULL;
    this->headersSent = false;
    this->finished = false;
}
Client::~Client()
{
    if (file)
    {
        if (file->is_open())
            file->close();
        delete file;
    }
    if (fd >= 0)
        close(fd);
}

void Client::setFd(int clientFd) { fd = clientFd; }
int Client::getFd() const { return fd; }

void Client::setFile(std::ifstream *f) { file = f; }
std::ifstream *Client::getFile() const { return file; }

void Client::setHeaderBuffer(const std::string &buf) { headerBuffer = buf; }
std::string &Client::getHeaderBuffer() { return headerBuffer; }

void Client::setHeadersSent(bool val) { headersSent = val; }
bool Client::isHeadersSent() const { return headersSent; }

void Client::setFinished(bool val) { finished = val; }
bool Client::isFinished() const { return finished; }

// return 0 continue, 1 break, 2 disconnectClient and break

int Client::readRequest(const std::string &rootDir, const std::string &index)
{
    char buffer[1024];
    int bytesRead = recv(fd, buffer, sizeof(buffer), 0);
    if (bytesRead == 0)
        return (1);
    else if (bytesRead < 0)
        return (2);

    std::string request(buffer, bytesRead);
    HttpRequest r(request);
    std::string urlPath = r.getPath();
    std::string fullPath = resolvePath(rootDir, index, urlPath);

    struct stat st;
    if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
    {
        this->setFile(new std::ifstream(fullPath.c_str(), std::ios::in | std::ios::binary));
        std::string headers = r.buildHttpResponse("", true, st.st_size);
        this->setHeaderBuffer(headers);
    }
    else
    {
        std::string headers = r.buildHttpResponse("<h1>404 Not Found</h1>", false);
        this->setHeaderBuffer(headers);
        this->setFinished(true);
    }
    this->setHeadersSent(true);
    return (0);
}

int Client::sendResponse()
{
    int fd = this->getFd();
    // Send headers
    if (!this->getHeaderBuffer().empty())
    {
        int n = send(fd, this->getHeaderBuffer().c_str(), this->getHeaderBuffer().length(), 0);
        if (n > 0)
            this->setHeaderBuffer(this->getHeaderBuffer().substr(n));
        else
            return (2);
    }

    // Send file in chunks
    if (this->getFile())
    {
        const size_t CHUNK_SIZE = 1024;
        char buf[CHUNK_SIZE];

        while (this->getFile() && !this->getFile()->eof())
        {
            this->getFile()->read(buf, CHUNK_SIZE);
            std::streamsize bytesRead = this->getFile()->gcount();
            if (bytesRead <= 0)
                return (1);

            ssize_t bytesSent = send(fd, buf, bytesRead, 0);
            if (bytesSent < 0)
            {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    return (2);
                else
                    return (1);
            }
            if (bytesSent < bytesRead)
                return (1);
        }

        if (this->getFile() && this->getFile()->eof())
        {
            this->getFile()->close();
            delete this->getFile();
            this->setFile(NULL);
            this->setFinished(true);
        }
    }

    // If finished, disconnect
    if (this->isFinished() && this->getHeaderBuffer().empty() && !this->getFile())
        return (2);
    return (0);
}

std::string Client::resolvePath(std::string rootdir, std::string index, const std::string &path)
{
    // Prevent empty paths
    if (path.empty())
        return "";
    // Prevent directory traversal
    if (path.find("..") != std::string::npos)
        return "";
    // Always start with /
    std::string safePath = path;
    if (safePath[0] != '/')
        safePath = "/" + safePath;
    // Map "/" to index
    if (safePath == "/")
        safePath = "/" + index;
    // Combine with root directory
    std::string fullPath = rootdir + safePath;
    return fullPath;
}
