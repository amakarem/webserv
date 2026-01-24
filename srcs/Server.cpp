/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:32:26 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 22:04:53 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "HttpRequest.hpp"

static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

Server::Server() {
    this->port = 8080;
}

void Server::setdefaultConf()
{
    char const *filename = "config/webserv.conf";

    std::cout << "Loading default config\n";
    try
    {
        setConfig(filename);
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}

Server::Server(char const *filename)
{
    std::cout << "Loading config file:" << filename << "\n";
    try
    {
        setConfig(filename);
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}

Server::Server(Server const &src)
{
    if (this != &src)
        *this = src;
}

Server &Server::operator=(Server const &src)
{
    if (this == &src)
        return (*this);
    return (*this);
}

Server::~Server() {}

void Server::setConfig(char const *filename)
{
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open())
        throw openFileError();
    while (std::getline(file, line))
    {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
            continue;;

        std::string key, value;
        size_t sep = trimmed.find(' ');
        if (sep == std::string::npos)
        {
            std::cerr << "Error: Invalid config line: " << trimmed;
            throw KeyError();
        }
        key = trimmed.substr(0, sep);
        value = trim(trimmed.substr(sep));
        if (!value.empty() && value[value.size() - 1] == ';')
            value = value.substr(0, value.size() - 1);
        if (key == "port")
            this->port = std::atoi(value.c_str());
        else if (key == "root")
            this->rootdir = value;
        else if (key == "index")
            this->index = value;
        else
        {
            std::cerr << "Error: Invalid Key " << key;
            throw KeyError();
        }
    }
    file.close();
}

void Server::validateConfig()
{

    if (this->port <= 0 || this->port > 65535)
    {
        std::cerr << "Error: port " << this->port << " not valid\n";
        throw invalidPort();
    }
    if (this->rootdir.empty())
    {
        std::cerr << "Error: root directory not set in config";
        throw KeyError();
    }
    if (this->index.empty())
    {
        std::cerr << "Error: index not set in config";
        throw KeyError();
    }
    std::cout << "Port:" << this->port;
    std::cout << "\nRoot directory:" << this->rootdir;
    std::cout << "\nindex file:" << this->index << "\n";
}

void Server::startListening()
{
    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    
    if (listenFd < 0)
        throw std::runtime_error("socket() failed");
    if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt() failed");
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;     // 0.0.0.0
    addr.sin_port = htons(port);           // IMPORTANT

    if (bind(listenFd, (sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");
    if (listen(listenFd, 128) < 0)
        throw std::runtime_error("listen() failed");
    std::cout << "Server listening on port " << port << std::endl;
}

void Server::run()
{
    fd_set readfds, writefds;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        // --- Listening socket ---
        FD_SET(listenFd, &readfds);
        int maxFd = listenFd;

        // --- Add clients ---
        for (size_t i = 0; i < clients.size(); ++i)
        {
            int fd = clients[i]->getFd();
            if (fd >= 0)
            {
                FD_SET(fd, &readfds);
                FD_SET(fd, &writefds);
                if (fd > maxFd)
                    maxFd = fd;
            }
        }

        // Wait for activity
        int activity = select(maxFd + 1, &readfds, &writefds, NULL, NULL);
        if (activity < 0)
        {
            std::cerr << "select() error\n";
            continue;
        }

        // --- Accept new client ---
        if (FD_ISSET(listenFd, &readfds))
        {
            int newFd = accept(listenFd, NULL, NULL);
            if (newFd >= 0)
            {
                Client* c = new Client();
                c->setFd(newFd);
                c->setHeadersSent(false);
                c->setFinished(false);
                c->setFile(NULL);
                clients.push_back(c);
                std::cout << "New client connected: " << newFd << std::endl;
            }
        }

        // --- Handle existing clients ---
        for (size_t i = 0; i < clients.size(); )
        {
            Client* c = clients[i];
            int fd = c->getFd();

            // Skip invalid FDs
            if (fd < 0)
            {
                clients.erase(clients.begin() + i);
                delete c;
                continue;
            }

            // 1️⃣ Read request
            if (!c->isHeadersSent() && FD_ISSET(fd, &readfds))
            {
                char buffer[1024];
                HttpRequest r;
                int bytesRead = recv(fd, buffer, sizeof(buffer)-1, 0);

                if (bytesRead <= 0)
                {
                    disconnectClient(i);
                    continue;
                }

                std::string request(buffer, bytesRead);
                std::string urlPath = r.extractPath(request);
                std::string fullPath = resolvePath(urlPath);

                struct stat st;
                if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
                {
                    c->setFile(new std::ifstream(fullPath.c_str(), std::ios::in | std::ios::binary));
                    std::string headers = r.buildHttpResponse("", true, st.st_size);
                    c->setHeaderBuffer(headers);
                }
                else
                {
                    // 404
                    std::string headers = r.buildHttpResponse("<h1>404 Not Found</h1>", false);
                    c->setHeaderBuffer(headers);
                    c->setFinished(true);
                }

                c->setHeadersSent(true);
            }

            // 2️⃣ Send headers
            if (!c->getHeaderBuffer().empty() && FD_ISSET(fd, &writefds))
            {
                int n = send(fd, c->getHeaderBuffer().c_str(), c->getHeaderBuffer().length(), 0);
                if (n > 0)
                    c->setHeaderBuffer(c->getHeaderBuffer().substr(n));
                else {
                    disconnectClient(i);
                    continue;
                }
            }

            // 3️⃣ Send file in chunks
            if (c->getFile() && FD_ISSET(fd, &writefds))
            {
                const size_t CHUNK_SIZE = 1024;
                char buf[CHUNK_SIZE];
                c->getFile()->read(buf, CHUNK_SIZE);
                std::streamsize n = c->getFile()->gcount();
                if (n > 0)
                    send(fd, buf, n, 0);
                else {
                    disconnectClient(i);
                    continue;
                }

                if (c->getFile()->eof())
                {
                    c->getFile()->close();
                    delete c->getFile();
                    c->setFile(NULL);
                    c->setFinished(true);
                }
            }

            // 4️⃣ Close finished clients
            if (c->isFinished() && c->getHeaderBuffer().empty() && !c->getFile())
            {
                disconnectClient(i);
                continue;
            }

            ++i;
        }
    }
}

// void Server::run() //old serve the whole file once
// {
//     fd_set readfds;

//     while (1)
//     {
//         FD_ZERO(&readfds);

//         // Add listening socket
//         FD_SET(listenFd, &readfds);
//         int maxFd = listenFd;

//         // Add all client sockets
//         for (size_t i = 0; i < this->clients.size(); ++i)
//         {
//             FD_SET(this->clients[i], &readfds);
//             if (this->clients[i] > maxFd)
//                 maxFd = this->clients[i];
//         }

//         // Wait for activity
//         int activity = select(maxFd + 1, &readfds, NULL, NULL, NULL);
//         if (activity < 0)
//         {
//             std::cerr << "select() error\n";
//             continue;
//         }

//         // New connection
//         if (FD_ISSET(listenFd, &readfds))
//         {
//             int newClient = accept(listenFd, NULL, NULL);
//             if (newClient >= 0)
//             {
//                 this->clients.push_back(newClient);
//                 std::cout << "New client connected: " << newClient << std::endl;
//             }
//         }

//         // Check existing this->clients
//         for (size_t i = 0; i < this->clients.size(); )
//         {
//             int fd = this->clients[i];
//             if (FD_ISSET(fd, &readfds))
//             {
//                 char buffer[4096];
//                 std::string request;

//                 while (request.find("\r\n\r\n") == std::string::npos)
//                 {
//                     int r = recv(fd, buffer, sizeof(buffer) - 1, 0);
//                     if (r <= 0)
//                         break;
//                     buffer[r] = '\0';
//                     request += buffer;
//                 }
                
//                 if (request.empty())
//                 {
//                     // Client disconnected
//                     std::cout << "Client disconnected: " << fd << std::endl;
//                     close(fd);
//                     clients.erase(clients.begin() + i);
//                     continue;
//                 }
//                 HttpRequest r;
//                 std::string urlPath = r.extractPath(request);
//                 std::cout << "\nClient " << fd << " requested: " << urlPath << std::endl;
//                 std::string fullPath = resolvePath(urlPath);
//                 std::string body;
//                 std::string response;

//                 struct stat st;
//                 if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
//                 {
//                     // Read file
//                     std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
//                     if (file)
//                     {
//                         std::ostringstream ss;
//                         ss << file.rdbuf();
//                         body = ss.str();
//                         // Build 200 OK response
//                         std::ostringstream oss;
//                         oss << "HTTP/1.1 200 OK\r\n"
//                             << "Content-Length: " << body.length() << "\r\n"
//                             << "Content-Type: text/html\r\n"
//                             << "Connection: close\r\n"
//                             << "\r\n"
//                             << body;
//                         response = oss.str();
//                     }
//                     else
//                         response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length:0\r\n\r\n"; // File exists but cannot open (rare)
//                 }
//                 else
//                 {
//                     // 404 Not Found
//                     std::string msg = "<h1>404 Not Found</h1>";
//                     std::ostringstream oss;
//                     oss << "HTTP/1.1 404 Not Found\r\n"
//                         << "Content-Length: " << msg.length() << "\r\n"
//                         << "Content-Type: text/html\r\n"
//                         << "Connection: close\r\n"
//                         << "\r\n"
//                         << msg;
//                     response = oss.str();
//                 }

//                 // --- Send response ---
//                 send(fd, response.c_str(), response.length(), 0);

//                 // --- Close client ---
//                 close(fd);
//                 clients.erase(clients.begin() + i);
//                 continue;
//             }
//             ++i;
//         }
//     }
// }

std::string Server::resolvePath(const std::string &path)
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

void Server::disconnectClient(size_t index)
{
    if (index >= clients.size())
        return;

    Client* c = clients[index];
    int fd = c->getFd();

    std::cout << "Client "<< fd << " disconnected: " << fd << std::endl;

    // Close file if open
    if (c->getFile())
    {
        c->getFile()->close();
        delete c->getFile();
        c->setFile(NULL);
    }

    // Close socket
    if (fd >= 0)
        close(fd);

    // Delete client object
    delete c;

    // Remove from vector
    clients.erase(clients.begin() + index);
}


const char *Server::openFileError::what() const throw()
{
    return "Error: Can not read the file\n";
}

const char *Server::invalidPort::what() const throw()
{
    return "Error: Invalid port number in config\n Allawed range 0 to 65534";
}

const char *Server::KeyError::what() const throw()
{
    return "\n";
}
