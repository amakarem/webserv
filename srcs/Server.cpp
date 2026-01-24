/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:32:26 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 20:14:06 by aelaaser         ###   ########.fr       */
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
    fd_set readfds;

    while (1)
    {
        FD_ZERO(&readfds);

        // Add listening socket
        FD_SET(listenFd, &readfds);
        int maxFd = listenFd;

        // Add all client sockets
        for (size_t i = 0; i < this->clients.size(); ++i)
        {
            FD_SET(this->clients[i], &readfds);
            if (this->clients[i] > maxFd)
                maxFd = this->clients[i];
        }

        // Wait for activity
        int activity = select(maxFd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0)
        {
            std::cerr << "select() error\n";
            continue;
        }

        // New connection
        if (FD_ISSET(listenFd, &readfds))
        {
            int newClient = accept(listenFd, NULL, NULL);
            if (newClient >= 0)
            {
                this->clients.push_back(newClient);
                std::cout << "New client connected: " << newClient << std::endl;
            }
        }

        // Check existing this->clients
        for (size_t i = 0; i < this->clients.size(); )
        {
            int fd = this->clients[i];
            if (FD_ISSET(fd, &readfds))
            {
                char buffer[4096];
                std::string request;

                while (request.find("\r\n\r\n") == std::string::npos)
                {
                    int r = recv(fd, buffer, sizeof(buffer) - 1, 0);
                    if (r <= 0)
                        break;
                    buffer[r] = '\0';
                    request += buffer;
                }
                
                if (request.empty())
                {
                    // Client disconnected
                    std::cout << "Client disconnected: " << fd << std::endl;
                    close(fd);
                    clients.erase(clients.begin() + i);
                    continue;
                }

                std::string urlPath = extractPath(request);
                std::cout << "\nClient " << fd << " requested: " << urlPath << std::endl;
                std::string fullPath = resolvePath(urlPath);
                std::string body;
                std::string response;

                struct stat st;
                if (!fullPath.empty() && stat(fullPath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode))
                {
                    // Read file
                    std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
                    if (file)
                    {
                        std::ostringstream ss;
                        ss << file.rdbuf();
                        body = ss.str();
                        // Build 200 OK response
                        std::ostringstream oss;
                        oss << "HTTP/1.1 200 OK\r\n"
                            << "Content-Length: " << body.length() << "\r\n"
                            << "Content-Type: text/html\r\n"
                            << "Connection: close\r\n"
                            << "\r\n"
                            << body;
                        response = oss.str();
                    }
                    else
                        response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length:0\r\n\r\n"; // File exists but cannot open (rare)
                }
                else
                {
                    // 404 Not Found
                    std::string msg = "<h1>404 Not Found</h1>";
                    std::ostringstream oss;
                    oss << "HTTP/1.1 404 Not Found\r\n"
                        << "Content-Length: " << msg.length() << "\r\n"
                        << "Content-Type: text/html\r\n"
                        << "Connection: close\r\n"
                        << "\r\n"
                        << msg;
                    response = oss.str();
                }

                // --- Send response ---
                send(fd, response.c_str(), response.length(), 0);

                // --- Close client ---
                close(fd);
                clients.erase(clients.begin() + i);
                continue;
            }
            ++i;
        }
    }
}

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
