/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:32:26 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/17 18:00:46 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

Server::Server()
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
            return;

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
    validateConfig();
}

void Server::validateConfig()
{
    if (this->port <= 0 || this->port > 65535)
        throw invalidPort();
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
    startListening();
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
    run();
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
                int bytesRead = recv(fd, buffer, sizeof(buffer) - 1, 0);

                if (bytesRead <= 0)
                {
                    // Client disconnected
                    std::cout << "Client disconnected: " << fd << std::endl;
                    close(fd);
                    this->clients.erase(this->clients.begin() + i);
                    continue; // skip increment
                }
                else
                {
                    buffer[bytesRead] = '\0';
                    std::cout << "Received from client " << fd << ":\n" << buffer << std::endl;

                    // Minimal fixed response
                    const char *response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 13\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "Hello, world! my server is working";
                    send(fd, response, strlen(response), 0);
                }
            }
            ++i;
        }
    }
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
