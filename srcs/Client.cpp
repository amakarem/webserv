/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/08 01:12:31 by aelaaser         ###   ########.fr       */
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
    this->allowedMethods = config.allowedMethods;
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
bool Client::continueAfterHeader()
{
        if (request.isHeadersComplete())
        {
            if (allowedMethods.size() > 0)
            {
                for (size_t i = 0; i < allowedMethods.size(); ++i)
                {
                    if (allowedMethods[i] == request.getMethod())
                    {
                        std::cout << "Method NOT Allowed\n";
                        return (true);
                    }
                }
                setHeaderBuffer("HTTP/1.1 405 OK\r\n\r\nMethod Not Allowed");
                setFinished(true);
                this->setHeadersSent(true);
                request.setRequestComplete();
                return (false);
            }
        }
        return (true);
}

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
        request.append(buffer, bytesRead);
    }
    this->setlastActivity();
    if (!request.isHeadersComplete() || !continueAfterHeader())
        return (0);

    if (this->fullPath.empty())
        this->fullPath = resolvePath(request.getPath());

    struct stat st;
    if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
    {
        if (this->isPHP())
        {
            // std::string phpOut = executePHP(fullPath, request.getBody());
            std::string phpOut = executePHP(fullPath);
            size_t pos = phpOut.find("\r\n\r\n");
            if (pos != std::string::npos)
            {
                request.setcgiHeaders(phpOut.substr(0, pos));
                this->sendBuffer = phpOut.substr(pos + 4);
            }
            this->setHeaderBuffer(request.buildHttpResponse("", 200, this->sendBuffer.size()));
        }
        else if (request.getMethod() == "DELETE")
        {
            if (std::remove(fullPath.c_str()) == 0)
                setHeaderBuffer("HTTP/1.1 200 OK\r\n\r\nFile deleted");
            else
                setHeaderBuffer("HTTP/1.1 404 Not Found\r\n\r\nFile not found");
            setFinished(true);
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

    std::string safePath = path;
    size_t qpos = path.find('?');
    if (qpos != std::string::npos)
         safePath = path.substr(0, qpos);

    // Always start with /
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

std::string Client::executePHP(const std::string &scriptPath)
{
    int outPipe[2];  // child -> parent
    if (pipe(outPipe) != 0) return "";

    pid_t pid = fork();
    if (pid < 0)
        return "";

    if (pid == 0) // child
    {
        std::string tmpFileName = request.gettmpFileName();
        // Redirect stdin and stdout
        dup2(outPipe[1], STDOUT_FILENO); // send output to parent
        close(outPipe[0]); close(outPipe[1]);

        // Build environment
        std::string path = request.getPath();
        size_t qpos = path.find('?');
        std::string query = (qpos != std::string::npos) ? path.substr(qpos + 1) : "";
        std::string safePath = (qpos != std::string_view::npos) ? path.substr(0, qpos) : path;

        std::vector<std::string> envVec;
        envVec.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envVec.push_back("SCRIPT_FILENAME=" + scriptPath);
        envVec.push_back("SCRIPT_NAME=" + safePath);
        envVec.push_back("REQUEST_URI=" + path);
        envVec.push_back("SERVER_PROTOCOL=HTTP/1.1");
        envVec.push_back("REQUEST_METHOD=" + request.getMethod());
        envVec.push_back("REDIRECT_STATUS=200");

        if (!tmpFileName.empty() && (request.getMethod() == "POST" || request.getMethod() == "PUT"))
        {
            int inputfd = open(tmpFileName.c_str(), O_RDONLY);// read POST body
            if (inputfd < 0) _exit(1);
            lseek(inputfd, 0, SEEK_SET);
            dup2(inputfd, STDIN_FILENO);
            close(inputfd);
            struct stat st;
            if (stat(tmpFileName.c_str(), &st) != 0)
                _exit(1); 
            envVec.push_back("CONTENT_LENGTH=" + std::to_string(st.st_size));
            std::string contentType = request.getContentType();
            if (contentType.empty())
                contentType = "application/x-www-form-urlencoded"; // default
            envVec.push_back("CONTENT_TYPE=" + contentType);
        }
        else
            envVec.push_back("CONTENT_LENGTH=0");
        //query string avilable with all requests
        envVec.push_back("QUERY_STRING=" + query);

        // Convert to char* array
        std::vector<char*> envp;
        for (auto &s : envVec)
            envp.push_back(s.data());
        envp.push_back(nullptr);

        char* argv[] = { (char*)"php-cgi", nullptr };
        execve("/usr/bin/php-cgi", argv, envp.data());
        _exit(1); // exec failed
    }
    
    // parent

    close(outPipe[1]);  // read from child

    // Read PHP output
    char buffer[4096];
    std::string result;
    ssize_t n;
    while ((n = read(outPipe[0], buffer, sizeof(buffer))) > 0)
        result.append(buffer, n);

    close(outPipe[0]);
    waitpid(pid, nullptr, 0);

    return result;
}

