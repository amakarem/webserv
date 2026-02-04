/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/05 00:44:26 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int _fd, const ServerConfig &config)
{
    this->fd = _fd;
    this->file = NULL;
    this->headersSent = false;
    this->finished = false;
    this->keepAlive = false;
    this->setlastActivity();
    this->rootDir = config.root;
    this->indexFiles = config.indexFiles;
    this->serverName = config.serverName;
    this->autoindex = config.autoindex;
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
}
long Client::getlastActivity() const { return lastActivity; }

bool Client::isTimeout() const
{
    if (time(NULL) - this->getlastActivity() > 5)
        return (true);
    return (false);
}

// return 0 Client is still alive, keep it in epoll, 1 Client must be disconnected

int Client::readRequest()
{
    char buffer[1024];
    int bytesRead = recv(fd, buffer, sizeof(buffer), 0);

    if (bytesRead == 0) // Client closed connection
        return (1);
    if (bytesRead < 0) // Error
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
    std::string fullPath = resolvePath(urlPath);

    struct stat st;
    if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
    {
        this->setFile(new std::ifstream(fullPath.c_str(), std::ios::in | std::ios::binary));
        std::string headers = r.buildHttpResponse("", 200, st.st_size);
        this->setHeaderBuffer(headers);
    }
    else if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        if (autoindex)
            this->setHeaderBuffer(r.buildHttpResponse(generateDirectoryListing(fullPath), 200));
        else
            this->setHeaderBuffer(r.buildHttpResponse("", 403));
        this->setFinished(true);
    }
    else
    {
        this->setHeaderBuffer(r.buildHttpResponse("", 404));
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

        if (this->getFile() && !this->getFile()->eof())
        {
            this->getFile()->read(buf, CHUNK_SIZE);
            std::streamsize bytesRead = this->getFile()->gcount();
            if (bytesRead <= 0)
                return (1);

            ssize_t bytesSent = send(fd, buf, bytesRead, 0);
            if (bytesSent < 0)
            {
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

std::string Client::resolvePath(const std::string &path)
{
    // Prevent empty paths
    if (path.empty())
        return "";
    // Prevent directory traversal
    if (path.find("..") != std::string::npos)
        return "";
    if (path.find("/.") != std::string::npos)
        return "";
    // Always start with /
    std::string safePath = path;
    if (safePath[0] != '/')
        safePath = "/" + safePath;
    std::string fullPath = rootDir + safePath;

    struct stat st;
    if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) // to handle different defualt indexs
    {
        for (size_t i = 0; i < indexFiles.size(); ++i)
        {
            std::string tryPath = fullPath + indexFiles[i];
            if (stat(tryPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
            {
                fullPath = tryPath;
                break;
            }
        }
    }
    std::cout << "Client:" << fd << " Requested Path: " << fullPath << " From:" << serverName << "\n";
    return fullPath;
}

std::string Client::generateDirectoryListing(const std::string &dir)
{
    std::ostringstream oss;
    std::string directory = dir;
    directory = directory.substr(rootDir.length(), directory.length());
    oss << "<html><body><h1>Index of " << directory << "</h1><ul>";
    DIR* dp = opendir(dir.c_str());
    if (dp) {
        struct dirent* entry;
        while ((entry = readdir(dp)) != nullptr) {
            oss << "<li><a href=\"" << entry->d_name << "\">"
                << entry->d_name << "</a></li>";
        }
        closedir(dp);
    }
    oss << "</ul></body></html>";
    return oss.str();
}
