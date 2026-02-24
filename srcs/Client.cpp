/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/13 17:49:20 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

std::string getFirstLine(const std::string &s)
{
    size_t pos = s.find('\n');
    std::string line = (pos == std::string::npos) ? s : s.substr(0, pos);

    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    return line;
}

bool isText(const std::string line)
{
    std::string key = "Content-Type:";
    size_t pos = line.find(key);
    if (pos == std::string::npos)
        return false;
    pos += key.size();
    while (pos < line.size() && line[pos] == ' ')
        pos++;
    return line.compare(pos, 5, "text/") == 0;
}

bool Client::saveUploadedFileBinary(const std::string &uploadFolder)
{
    std::ifstream in(request.gettmpFileName(), std::ios::binary);
    if (!in.is_open())
        return false;

    std::string line;
    std::string filename;
    std::string headers;
    bool typeText = false;

    // Read headers (ASCII) line by line
    while (std::getline(in, line))
    {
        if (line.back() == '\r')
            line.pop_back(); // remove trailing \r
        headers += line + "\n";
        if (line.empty()) // empty line -> end of headers
            break;

        // Extract filename
        size_t fnPos = line.find("filename=\"");
        if (fnPos != std::string::npos)
        {
            fnPos += 10; // skip filename="
            size_t fnEnd = line.find("\"", fnPos);
            if (fnEnd != std::string::npos)
                filename = line.substr(fnPos, fnEnd - fnPos);
        }
        else if (!typeText && isText(line))
        {
            typeText = true;
        }
    }
    line.clear();
    headers = getFirstLine(headers);
    if (filename.empty())
        return false;
    // Ensure upload folder exists
    std::filesystem::create_directories(uploadFolder);
    std::string filePath = uploadFolder + "/" + filename;
    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open())
        return false;

    // Write remaining content (binary-safe)
    char buffer[8192];
    if (!typeText)
    {
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
            out.write(buffer, in.gcount());
    }
    else
    {
        std::string carry = "";
        while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0)
        {
            size_t n = in.gcount();
            std::string data = carry + std::string(buffer, n);
            size_t pos = data.find(headers);
            if (pos != std::string::npos)
            {
                out.write(data.data(), pos); // write only before headers
                break;                       // stop reading further
            }
            out.write(data.data(), data.size());
            // keep tail for next chunk (for partial match)
            if (data.size() >= headers.size())
                carry = data.substr(data.size() - headers.size());
            else
                carry = data;
        }
    }

    out.close();
    setHeaderBuffer(request.buildHttpResponse("<h1>" + filename + " uploaded</h1>", 200, 0));
    return true;
}

Client::Client(int fd, const ServerConfig &config) : fd(fd), config(config)
{
    this->file = NULL;
    this->headersSent = false;
    this->finished = false;
    this->setlastActivity();
    this->headersParsed = false;
    this->bodyComplete = false;
    this->contentLength = 0;
    this->PHP = false;
    this->Py = false;
    this->query_string = "";
    this->request.setTmpDir(config.tmpdir);
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
bool Client::isPy() const { return Py; }
bool Client::isCGI() const { return isPHP() || isPy(); }

void Client::resetRequest() { return request.reset(); }

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

bool Client::stopHere()
{
    this->setHeadersSent(true);
    request.setRequestComplete();
    return (false);
}
// return 0 Client is still alive, keep it in epoll, 1 Client must be disconnected
bool Client::continueAfterHeader()
{
    if (request.isHeadersComplete()) // handle redirect
    {
        std::string path = request.getPath();
        size_t qpos = path.find('?');
        this->query_string = (qpos != std::string::npos) ? path.substr(qpos + 1) : "";
        this->script_name = (qpos != std::string_view::npos) ? path.substr(0, qpos) : path;
        // --- prefix matching for redirects ---
        const Redirect *matched_redirect = nullptr;
        std::string matched_prefix;

        for (const auto &pair : config.redirects)
        {
            const std::string &prefix = pair.first;
            // Does script_name start with prefix?
            if (this->script_name.compare(0, prefix.size(), prefix) == 0)
            {
                // Optional: longest prefix wins
                if (!matched_redirect || prefix.size() > matched_prefix.size())
                {
                    matched_redirect = &pair.second;
                    matched_prefix = prefix;
                }
            }
        }
        if (matched_redirect)
        {
            // Build final URL
            std::string suffix = this->script_name.substr(matched_prefix.size());
            std::string url = matched_redirect->new_url + suffix;
            if (!this->query_string.empty())
                url += "?" + this->query_string;
            // Send redirect
            setHeaderBuffer("HTTP/1.1 " + std::to_string(matched_redirect->code) + " " + request.getHttpCodeMsg(matched_redirect->code) + "\r\nLocation: " + url + "\r\n");
            setFinished(true);
            return this->stopHere();
        }

        this->fullPath = resolvePath(request.getPath()); // auto redirect to correct folder path
        if (this->fullPath.empty())
        {
            setFinished(true);
            return this->stopHere();
        }
        if (config.allowedMethods.size() > 0) // validate allwed method
        {
            for (size_t i = 0; i < config.allowedMethods.size(); ++i)
            {
                if (config.allowedMethods[i] == request.getMethod())
                    return (true);
            }
            this->generateErrorPage(405);
            return this->stopHere();
        }
    }
    return (true);
}

int Client::readRequest()
{
    char buffer[4096];
    while (true)
    {
        ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
        if (bytesRead == 0) // Client closed connection
            return (1);
        if (bytesRead < 0) // Error
        {
            // if (errno == EAGAIN || errno == EWOULDBLOCK)
            break; // no data yet, still alive
            // return (1); // real error → disconnect
        }
        if (!request.append(buffer, bytesRead))
            return (1);
    }
    this->setlastActivity();
    if (!request.isHeadersComplete() || !continueAfterHeader())
        return (0);

    if (this->fullPath.empty())
        this->fullPath = resolvePath(request.getPath());

    if (!request.isRequestComplete())
        return (0);
    if (request.getMethod() == "POST" && config.allowupload.find(this->script_name) != config.allowupload.end())
    {
        std::string folder = config.allowupload.at(this->script_name);
        if (!saveUploadedFileBinary(folder))
            this->generateErrorPage(500);
        this->setHeadersSent(true);
        return (0);
    }
    struct stat st;
    if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
    {
        if (this->isCGI())
        {
            std::string cgiOut = executeCGI(fullPath);
            size_t pos = cgiOut.find("\r\n\r\n");
            if (this->isPHP() && pos != std::string::npos)
            {
                request.setcgiHeaders(cgiOut.substr(0, pos));
                this->sendBuffer = cgiOut.substr(pos + 4);
            }
            else if (this->isPy())
            {
                if (pos == std::string::npos)
                    pos = cgiOut.find("\n\n");
                request.setcgiHeaders(cgiOut.substr(0, pos));
                this->sendBuffer = cgiOut.substr(pos + (cgiOut[pos] == '\r' ? 4 : 2));
            }
            this->setHeaderBuffer(request.buildHttpResponse("", 200, this->sendBuffer.size()));
        }
        else if (request.getMethod() == "DELETE")
        {
            if (std::remove(fullPath.c_str()) == 0)
            {
                setHeaderBuffer("HTTP/1.1 204 No Content\r\n\r\nDelete on a resource is successful");
                setFinished(true);
            }
            else
                this->generateErrorPage(404);
        }
        else
        {
            this->setFile(new std::ifstream(fullPath.c_str(), std::ios::in | std::ios::binary));
            this->setHeaderBuffer(request.buildHttpResponse("", 200, st.st_size));
        }
    }
    else if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        if (config.autoindex)
            this->setHeaderBuffer(request.buildHttpResponse(generateDirectoryListing(fullPath), 200));
        else
            this->generateErrorPage(403);
    }
    else
        this->generateErrorPage(404);
    this->setHeadersSent(true);
    return (0);
}

int Client::sendResponse()
{
    int fd = this->getFd();
    // Send headers
    if (!this->getHeaderBuffer().empty())
    {
        ssize_t n = send(fd, this->getHeaderBuffer().c_str(), this->getHeaderBuffer().length(), MSG_NOSIGNAL);
        if (n < 0)
        {
            // if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
            // return 1;
        }
        else if (n == 0)
            return 0;
        this->setHeaderBuffer(this->getHeaderBuffer().substr(n));
        this->setlastActivity();
    }

    const size_t CHUNK_SIZE = 16 * 1024; // 16 KB
    char buf[CHUNK_SIZE];

    if (this->isCGI() && this->sendBuffer.empty())
    {
        // error running PHP
        this->generateErrorPage(500);
        return 1;
    }
    else if (this->isCGI() && !this->sendBuffer.empty())
    {
        size_t toSend = CHUNK_SIZE;
        if (this->sendBuffer.size() < CHUNK_SIZE)
            toSend = this->sendBuffer.size();
        ssize_t bytesSent = send(fd, this->sendBuffer.c_str(), toSend, MSG_NOSIGNAL);
        if (bytesSent < 0)
        {
            // if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
            // return 1;
        }
        if (bytesSent > 0)
        {
            this->sendBuffer = this->sendBuffer.substr(bytesSent);
            this->setlastActivity();
        }
        if (this->sendBuffer.empty())
            this->setFinished(true);
        return 0;
    }
    else if (!this->isCGI() && this->getFile())
    {
        if (this->getFile() && !this->getFile()->eof())
        {
            this->getFile()->read(buf, CHUNK_SIZE);
            std::streamsize bytesRead = this->getFile()->gcount();
            if (bytesRead <= 0)
                return (1);
            ssize_t bytesSent = send(fd, buf, bytesRead, MSG_NOSIGNAL);
            if (bytesSent < 0)
            {
                // if (errno == EAGAIN || errno == EWOULDBLOCK)
                return (0);
                // return (1);
            }
            if (bytesSent < bytesRead)
                return (1);
            this->setlastActivity();
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
    std::cout << "Client:" << fd << " Request: " << request.getMethod() << path << " From:" << config.serverName << " - ";
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
    std::string fullPath = config.root + safePath;

    struct stat st;
    if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) // to handle different defualt indexs
    {
        if (fullPath.back() != '/')
        {
            safePath = safePath + "/";
            if (!this->query_string.empty())
                safePath = safePath + "?" + this->query_string;
            setHeaderBuffer("HTTP/1.1 301 Moved Permanently\r\nLocation:" + safePath + "\r\n");
            return "";
        }
        for (size_t i = 0; i < config.indexFiles.size(); ++i)
        {
            std::string tryPath = fullPath + config.indexFiles[i];
            if (stat(tryPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
            {
                fullPath = tryPath;
                break;
            }
        }
    }
    std::cout << fullPath << "\n";
    if (fullPath.size() > 4 && fullPath.substr(fullPath.size() - 4) == ".php")
        this->PHP = true;
    else if (fullPath.size() > 3 && fullPath.substr(fullPath.size() - 3) == ".py")
        this->Py = true;
    return fullPath;
}

void Client::generateErrorPage(int errorCode)
{
    auto it = config.error_pages.find(errorCode);
    if (it != config.error_pages.end())
    {
        struct stat st;
        if (!it->second.empty() && stat(it->second.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
        {
            this->setFile(new std::ifstream(it->second.c_str(), std::ios::in | std::ios::binary));
            this->setHeaderBuffer(request.buildHttpResponse("", errorCode, st.st_size));
            return;
        }
        std::cout << it->second << "\n";
    }
    this->setHeaderBuffer(request.buildHttpResponse("", errorCode, 0));
    this->setFinished(true);
    return;
}

std::string Client::generateDirectoryListing(const std::string &dir)
{
    std::ostringstream oss;
    std::string directory = dir;
    directory = directory.substr(config.root.length(), directory.length());
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

std::string Client::executeCGI(const std::string &scriptPath)
{
    int outPipe[2]; // child -> parent
    if (pipe(outPipe) != 0)
        return "";

    pid_t pid = fork();
    if (pid < 0)
        return "";

    if (pid == 0) // child
    {
        std::string tmpFileName = request.gettmpFileName();
        // Redirect stdin and stdout
        dup2(outPipe[1], STDOUT_FILENO); // send output to parent
        close(outPipe[0]);
        close(outPipe[1]);

        // Build environment

        std::vector<std::string> envVec;
        envVec.push_back("GATEWAY_INTERFACE=CGI/1.1");
        envVec.push_back("SCRIPT_FILENAME=" + scriptPath);
        envVec.push_back("SCRIPT_NAME=" + this->script_name);
        envVec.push_back("REQUEST_URI=" + request.getPath());
        envVec.push_back("SERVER_PROTOCOL=HTTP/1.1");
        envVec.push_back("REQUEST_METHOD=" + request.getMethod());
        envVec.push_back("REDIRECT_STATUS=200");
        if (!request.getrawCookieHeader().empty())
            envVec.push_back("HTTP_COOKIE=" + request.getrawCookieHeader());

        if (!tmpFileName.empty() && (request.getMethod() == "POST" || request.getMethod() == "PUT"))
        {
            int inputfd = open(tmpFileName.c_str(), O_RDONLY); // read POST body
            if (inputfd < 0)
                _exit(1);
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
        // query string avilable with all requests
        envVec.push_back("QUERY_STRING=" + this->query_string);

        // Convert to char* array
        std::vector<char *> envp;
        for (auto &s : envVec)
            envp.push_back(s.data());
        envp.push_back(nullptr);

        if (this->isPHP())
        {
            std::string upload = "upload_max_filesize=" + config.php_upload_max_filesize;
            std::string post = "post_max_size=" + config.php_post_max_size;
            std::string memory = "memory_limit=" + config.php_memory_limit;
            char *argv[] = {
                (char *)"php-cgi",
                (char *)"-d", (char *)upload.c_str(),
                (char *)"-d", (char *)post.c_str(),
                (char *)"-d", (char *)memory.c_str(),
                nullptr};

            execve("/usr/bin/php-cgi", argv, envp.data());
        }
        else if (this->isPy())
        {
            char *argv[] = {
                (char *)scriptPath.c_str(),
                nullptr};

            execve(scriptPath.c_str(), argv, envp.data());
        }
        _exit(1); // exec failed
    }

    // parent

    close(outPipe[1]); // read from child

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
