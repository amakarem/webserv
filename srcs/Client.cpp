/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/07 23:29:32 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int _fd, const ServerConfig &config)
{
    this->fd = _fd;
    this->file = NULL;
    this->headersSent = false;
    this->finished = false;
    this->setlastActivity();
    this->rootDir = config.root;
    this->indexFiles = config.indexFiles;
    this->serverName = config.serverName;
    this->autoindex = config.autoindex;
    this->headersParsed = false;
    this->bodyComplete = false;
    this->contentLength = 0;
    this->PHP = false;
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

bool Client::isKeepAlive() const { return request.isKeepAlive(); }

bool Client::isRequestComplete() const { return request.isRequestComplete(); }

void Client::setFile(std::ifstream *f) { file = f; }
std::ifstream *Client::getFile() const { return file; }

void Client::setHeaderBuffer(const std::string &buf) { headerBuffer = buf; }
std::string &Client::getHeaderBuffer() { return headerBuffer; }

void Client::setHeadersSent(bool val) { headersSent = val; }
bool Client::isHeadersSent() const { return headersSent; }

void Client::setFinished(bool val) { finished = val; }
bool Client::isFinished() const { return finished; }
bool Client::isPHP() const { return PHP; }

void Client::resetRequest() { return request.reset();}

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
    char buffer[10];
    while (true)
    {
        ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) // Client closed connection
            return (1);
        if (bytesRead < 0) // Error
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;  // no data yet, still alive
            return (1); // real error → disconnect
        }
        std::string x(buffer, bytesRead);
        request.append(buffer, bytesRead);
    }
    this->setlastActivity();
    if (!request.isHeadersComplete())
        return (0);

    if (this->fullPath.empty())
        this->fullPath = resolvePath(request.getPath());

    struct stat st;
    if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
    {
        if (this->isPHP())
        {
            std::string phpOut = executePHP(fullPath, request.getBody());
            size_t pos = phpOut.find("\r\n\r\n");
            if (pos != std::string::npos)
            {
                request.setcgiHeaders(phpOut.substr(0, pos));
                this->sendBuffer = phpOut.substr(pos + 4);
            }
            this->setHeaderBuffer(request.buildHttpResponse("", 200, this->sendBuffer.size()));
        }
        else
        {
            this->setFile(new std::ifstream(fullPath.c_str(), std::ios::in | std::ios::binary));
            this->setHeaderBuffer(request.buildHttpResponse("", 200, st.st_size));
        }
    }
    else if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        if (autoindex)
            this->setHeaderBuffer(request.buildHttpResponse(generateDirectoryListing(fullPath), 200));
        else
            this->setHeaderBuffer(request.buildHttpResponse("", 403));
        this->setFinished(true);
    }
    else
    {
        this->setHeaderBuffer(request.buildHttpResponse("", 404));
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
        else if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            return 1;
        }
    }

    const size_t CHUNK_SIZE = 16 * 1024;// 16 KB
    char buf[CHUNK_SIZE];

    if (this->isPHP() && this->sendBuffer.empty())
    {
        // error running PHP
        this->setHeaderBuffer("HTTP/1.1 500 Internal Server Error\r\n\r\n");
        this->setFinished(true);
        return 1;
    }
    else if (this->isPHP() && !this->sendBuffer.empty())
    {
        size_t toSend = CHUNK_SIZE;
        if (this->sendBuffer.size() < CHUNK_SIZE)
            toSend = this->sendBuffer.size();
        ssize_t bytesSent = send(fd, this->sendBuffer.c_str(), toSend, 0);
        if (bytesSent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return 0;
            return 1;
        }
        if (bytesSent > 0)
            this->sendBuffer = this->sendBuffer.substr(bytesSent);
        if (this->sendBuffer.empty())
            this->setFinished(true);
        return 0;
    }
    else if (!this->isPHP() && this->getFile())
    {
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
    if (this->isFinished() && this->getHeaderBuffer().empty() && !this->getFile() && this->sendBuffer.empty())
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
    std::cout << "Client:" << fd << " Request: " << request.getMethod() << fullPath << " From:" << serverName << "\n";
    if (fullPath.size() > 4 && fullPath.substr(fullPath.size() - 4) == ".php")
        this->PHP = true;
    return fullPath;
}

std::string Client::generateDirectoryListing(const std::string &dir)
{
    std::ostringstream oss;
    std::string directory = dir;
    directory = directory.substr(rootDir.length(), directory.length());
    oss << "<html><body><h1>Index of " << directory << "</h1><ul>";
    DIR *dp = opendir(dir.c_str());
    if (dp)
    {
        struct dirent *entry;
        while ((entry = readdir(dp)) != nullptr)
        {
            oss << "<li><a href=\"" << entry->d_name << "\">"
                << entry->d_name << "</a></li>";
        }
        closedir(dp);
    }
    oss << "</ul></body></html>";
    return oss.str();
}

std::string Client::executePHP(const std::string &scriptPath, const std::string &body)
{
    int inPipe[2], outPipe[2];
    if (pipe(inPipe) != 0 || pipe(outPipe) != 0)
        return "";

    pid_t pid = fork();
    if (pid < 0) return "";

    if (pid == 0) { // child
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);

        std::vector<std::string> envStrings;
        envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envStrings.push_back("SCRIPT_FILENAME=" + scriptPath);
        envStrings.push_back("REQUEST_METHOD=" + request.getMethod());
        envStrings.push_back("REDIRECT_STATUS=200");

        if (request.getMethod() == "POST" || request.getMethod() == "PUT") {
            envStrings.push_back("CONTENT_LENGTH=" + std::to_string(body.size()));
            envStrings.push_back("CONTENT_TYPE=" + request.getContentType());
        } else {
            std::string path = request.getPath();
            size_t qpos = path.find('?');
            std::string query = (qpos != std::string::npos) ? path.substr(qpos + 1) : "";
            envStrings.push_back("QUERY_STRING=" + query);
            envStrings.push_back("CONTENT_LENGTH=0");
        }

        std::vector<char*> envp;
        for (auto &s : envStrings) envp.push_back(&s[0]);
        envp.push_back(nullptr);

        char* argv[] = { (char*)"php-cgi", nullptr };
        execve("/usr/bin/php-cgi", argv, envp.data());
        _exit(1);
    }

    // parent
    close(inPipe[0]);  // write only
    close(outPipe[1]); // read only

    // Write body safely
    size_t totalSent = 0;
    while (totalSent < body.size()) {
        ssize_t n = write(outPipe[1], body.c_str() + totalSent, body.size() - totalSent);
        if (n <= 0) break;
        totalSent += n;
    }
    close(inPipe[1]);

    char buffer[4096];
    std::string result;
    ssize_t n;
    while ((n = read(outPipe[0], buffer, sizeof(buffer))) > 0)
        result.append(buffer, n);

    close(outPipe[0]);
    waitpid(pid, nullptr, 0);

    return result;
}
