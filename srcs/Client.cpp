/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/04 19:22:08 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int _fd)
{
    this->fd = _fd;
    this->file = NULL;
    this->headersSent = false;
    this->finished = false;
    this->keepAlive = false;
    this->setlastActivity();
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

int Client::getFd() const { return fd; }

void Client::setkeepAlive(bool val) { keepAlive = val; }
bool Client::isKeepAlive() const { return keepAlive; }

void Client::setFile(std::ifstream *f) { file = f; }
std::ifstream *Client::getFile() const { return file; }

void Client::setHeaderBuffer(const std::string &buf) { headerBuffer = buf; }
std::string &Client::getHeaderBuffer() { return headerBuffer; }

void Client::setHeadersSent(bool val) { headersSent = val; }
bool Client::isHeadersSent() const { return headersSent; }

void Client::setFinished(bool val) { finished = val; }
bool Client::isFinished() const { return finished; }

void Client::setlastActivity()
{
    this->lastActivity = time(NULL);
    std::cout << "lastActivity:" << this->getlastActivity();
}
long Client::getlastActivity() const { return lastActivity; }

bool Client::isTimeout() const
{
    std::cout << "timeout:" << time(NULL) - this->getlastActivity();
    if (time(NULL) - this->getlastActivity() > 5)
        return (true);
    return (false);
}

// return 0 Client is still alive, keep it in epoll, 1 Client must be disconnected

int Client::readRequest(const std::string &rootDir, const std::string &index)
{
    char buffer[1024];
    int bytesRead = recv(fd, buffer, sizeof(buffer), 0);
    
    if (bytesRead == 0)// Client closed connection
        return (1);
    if (bytesRead < 0)// Error
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return (0); // no data yet, still alive
        return (1);     // real error → disconnect
    }

    std::string request(buffer, bytesRead);
    HttpRequest r(request);

    this->setkeepAlive(r.isKeepAlive());
    this->setlastActivity();

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
        this->setHeaderBuffer(r.buildHttpResponse("<h1>404 Not Found</h1>", false));
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
            return (1);
    }

    // Send file in chunks
    if (this->getFile())
    {
        const size_t CHUNK_SIZE = 1024;
        char buf[CHUNK_SIZE];

        std::cout << "Send function called\n";
        while (this->getFile() && !this->getFile()->eof())
        {
            this->getFile()->read(buf, CHUNK_SIZE);
            std::streamsize bytesRead = this->getFile()->gcount();
            std::cout << "Read:" << bytesRead << "\n";
            if (bytesRead <= 0)
                return (1);

            ssize_t bytesSent = send(fd, buf, bytesRead, 0);
            std::cout << "Sent:" << bytesSent << "\n";
            if (bytesSent < 0)
            {
                std::cout << "here we go 152\n";
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    return (0);
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
        return (1);
    return (0);
}


int Client::sendResponseIncremental(size_t maxBytes)
{
    int fd = getFd();

    // 1️⃣ Send pending bytes in sendBuffer
    while (!sendBuffer.empty() && maxBytes > 0)
    {
        ssize_t sent = send(fd, sendBuffer.c_str(), std::min(sendBuffer.size(), maxBytes), 0);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0; // try again later
            return 1;     // error -> disconnect
        }
        sendBuffer.erase(0, sent);
        maxBytes -= sent;
    }

    // 2️⃣ If buffer still has bytes, stop and return
    if (!sendBuffer.empty())
        return 0;

    // 3️⃣ Load next chunk from file
    if (file && !file->eof() && maxBytes > 0)
    {
        char buf[1024]; // chunk size per loop
        file->read(buf, std::min(sizeof(buf), maxBytes));
        std::streamsize bytesRead = file->gcount();
        if (bytesRead > 0)
        {
            sendBuffer.assign(buf, bytesRead);
            return 0; // more to send next loop
        }
    }

    // 4️⃣ Close file if finished
    if (file && file->eof() && sendBuffer.empty())
    {
        file->close();
        delete file;
        file = nullptr;
        finished = true;
    }

    // 5️⃣ If fully finished, return 1 to disconnect
    if (finished && sendBuffer.empty() && !file)
        return 1;

    return 0;
}



std::string Client::resolvePath(std::string rootdir, std::string index, const std::string &path)
{
    std::vector<std::string> defaultIndexes = {"index.html", "index.htm", "default.html", "default.htm"};
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

    struct stat st;
    if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))//to handle different defualt indexs
    {
        std::string tryPath = fullPath + index;
        for (size_t i = 0; i < defaultIndexes.size(); ++i)
        {
            std::string tryPath = fullPath + "/" + defaultIndexes[i];
            if (stat(tryPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
            {
                fullPath = tryPath;
                break;
            }
        }
    }
    return fullPath;
}
