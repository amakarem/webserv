/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:32:26 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/13 17:15:30 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

volatile sig_atomic_t g_running = 1;

void handle_sigint(int)
{
    g_running = 0;
}

static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

static std::string strtoupper(const std::string &s)
{
    std::string upperString;
    upperString.resize(s.size());

    for (size_t i = 0; i < s.size(); i++)
    {
        upperString[i] = std::toupper((unsigned char)s[i]);
    }
    return upperString;
}

Server::Server()
{
    this->listenFd = -1;
    this->epollFd = -1;
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

Server::~Server()
{
    if (listenFd >= 0)
        close(listenFd);
    if (epollFd >= 0)
        close(epollFd);
}

// void Server::setConfigOLD(char const *filename)
// {
//     std::ifstream file(filename);
//     std::string line;

//     if (!file.is_open())
//         throw openFileError();
//     while (std::getline(file, line))
//     {
//         std::string trimmed = trim(line);
//         if (trimmed.empty() || trimmed[0] == '#')
//             continue;
//         ;

//         std::string key, value;
//         size_t sep = trimmed.find(' ');
//         if (sep == std::string::npos)
//         {
//             std::cerr << "Error: Invalid config line: " << trimmed;
//             throw KeyError();
//         }
//         key = trimmed.substr(0, sep);
//         value = trim(trimmed.substr(sep));
//         if (!value.empty() && value[value.size() - 1] == ';')
//             value = value.substr(0, value.size() - 1);
//         if (key == "port")
//             this->port = std::atoi(value.c_str());
//         else if (key == "root")
//             this->rootdir = value;
//         else if (key == "index")
//             this->index = value;
//         else
//         {
//             std::cerr << "Error: Invalid Key " << key;
//             throw KeyError();
//         }
//     }
//     file.close();
// }

void Server::setConfig(const char *filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
        throw openFileError();

    std::vector<ServerConfig> configs;
    ServerConfig current;
    bool inServerBlock = false;
    std::string line;

    while (std::getline(file, line))
    {
        std::string trimmed = trim(line);
        if (trimmed.empty() || trimmed[0] == '#')
            continue;

        if (trimmed == "server {")
        {
            current = ServerConfig(); // start new server block
            current.autoindex = false;
            inServerBlock = true;
            continue;
        }

        if (trimmed == "}")
        {
            if (inServerBlock)
            {
                configs.push_back(current); // save finished server block
                inServerBlock = false;
            }
            continue;
        }

        if (!inServerBlock)
            continue; // ignore lines outside server blocks

        // --- Parse key/value ---
        size_t sep = trimmed.find(' ');
        if (sep == std::string::npos)
        {
            std::cerr << "Invalid config line: " << trimmed << "\n";
            throw KeyError();
        }

        std::string key = trimmed.substr(0, sep);
        std::string value = trim(trimmed.substr(sep));
        if (!value.empty() && value.back() == ';')
            value.pop_back();

        if (key == "listen")
        {
            // format: ip:port or just port
            size_t colon = value.find(':');
            if (colon != std::string::npos)
            {
                current.ip = value.substr(0, colon);
                current.port = std::atoi(value.substr(colon + 1).c_str());
            }
            else
            {
                current.ip = "0.0.0.0"; // default listen all interfaces
                current.port = std::atoi(value.c_str());
            }
        }
        else if (key == "root")
        {
            if (value.rfind("./", 0) == 0) // starts with "./"
                value = std::filesystem::current_path().string() + value.substr(1);
            current.root = value;
        }
        else if (key == "index")
        {
            current.indexFiles.clear();
            std::istringstream iss(value);
            std::string idx;
            while (iss >> idx)
                current.indexFiles.push_back(idx);
        }
        else if (key == "allowedMethods")
        {
            current.allowedMethods.clear();
            std::istringstream iss(value);
            std::string idx;
            while (iss >> idx)
                current.allowedMethods.push_back(strtoupper(idx));
        }
        else if (key == "error_page")
        {
            value = trim(value);
            size_t colon = value.find(' ');
            if (colon != std::string::npos)
            {
                int errorCode = std::atoi(value.substr(0, colon).c_str());
                std::string errorPage = value.substr(colon + 1);
                current.error_pages[errorCode] = errorPage;
            }
        }
        else if (key == "location")
        {
            std::string location, newlocation, code;
            std::istringstream riss(value);
            riss >> location >> newlocation >> code;
            if (code.empty())
                code = "302";
            if (!location.empty() && !newlocation.empty())
                current.redirects[location] = {newlocation, std::atoi(code.c_str())};
        }
        else if (key == "allowupload")
        {
            std::string url, localfolder;
            std::istringstream riss(value);
            riss >> url >> localfolder;
            if (!url.empty() && !localfolder.empty())
                current.allowupload[url] = localfolder;
        }
        else if (key == "server_name")
            current.serverName = value;
        else if (key == "tmpdir")
            current.tmpdir = value;
        else if (key == "autoindex")
        {
            if (value == "on" || value == "1")
                current.autoindex = true;
        }
        else if (key == "php_upload_max_filesize")
            current.php_upload_max_filesize = value;
        else if (key == "php_post_max_size")
            current.php_post_max_size = value;
        else if (key == "php_memory_limit")
            current.php_memory_limit = value;
        else
        {
            std::cout << "WARNING::Invalid config key: " << key << "\n";
            // throw KeyError();
        }
    }

    if (inServerBlock)
        configs.push_back(current);

    file.close();

    // Save to your server object
    this->serverConfigs = configs; // std::vector<ServerConfig> serverConfigs;
}

void Server::validateConfig()
{

    for (std::vector<ServerConfig>::iterator it = serverConfigs.begin();
         it != serverConfigs.end();)
    {
        bool valid = true;
        if (it->port <= 0 || it->port > 65535)
        {
            std::cerr << "Error: port " << it->port << " not valid\n";
            valid = false;
        }
        if (it->root.empty())
        {
            std::cerr << "Error: root directory not set in config";
            valid = false;
        }
        if (it->indexFiles.empty())
        {
            std::cerr << "Error: index not set in config";
            valid = false;
        }

        if (!valid)
        {
            std::cerr << "Removing invalid server config\n\n";
            it = serverConfigs.erase(it); // ⚠️ returns next iterator
        }
        else
        {
            std::cout << "Server config OK\n";
            std::cout << "  Server: " << it->serverName << "\n";
            std::cout << "  Port: " << it->port << "\n";
            std::cout << "  Root: " << it->root << "\n";
            std::cout << "  Index files: ";
            for (size_t i = 0; i < it->indexFiles.size(); ++i)
                std::cout << it->indexFiles[i] << " ";
            std::cout << "\n\n";
            ++it;
        }
    }
    if (serverConfigs.size() == 0)
        throw noValidServer();
}

void Server::startListening()
{
    epollFd = epoll_create1(0);
    if (epollFd < 0)
        throw std::runtime_error("epoll_create failed");

    for (size_t i = 0; i < serverConfigs.size(); ++i)
    {
        int listenFd = socket(AF_INET, SOCK_STREAM, 0);
        if (listenFd < 0)
            throw std::runtime_error("socket() failed");

        int opt = 1;
        if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            throw std::runtime_error("setsockopt() failed");

        // Non-blocking
        if (fcntl(listenFd, F_SETFL, O_NONBLOCK) < 0)
            throw std::runtime_error("fcntl() failed");

        sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(serverConfigs[i].port);
        addr.sin_addr.s_addr = inet_addr(serverConfigs[i].ip.c_str());

        if (bind(listenFd, (sockaddr *)&addr, sizeof(addr)) < 0)
            throw std::runtime_error("bind() failed");

        if (listen(listenFd, 128) < 0)
            throw std::runtime_error("listen() failed");

        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = listenFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &ev) < 0)
            throw std::runtime_error("epoll_ctl ADD listenFd failed");

        listenSockets.push_back(listenFd);
        listenFdConfig[listenFd] = serverConfigs[i];
        std::cout << "Server listening on "
                  << serverConfigs[i].ip << ":"
                  << serverConfigs[i].port << std::endl;
    }
}

// void Server::startListening()
// {
//     listenFd = socket(AF_INET, SOCK_STREAM, 0);
//     int opt = 1;
//     sockaddr_in addr;
//     std::memset(&addr, 0, sizeof(addr));

//     if (listenFd < 0)
//         throw std::runtime_error("socket() failed");
//     if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
//         throw std::runtime_error("setsockopt() failed");
//     addr.sin_family = AF_INET;
//     addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
//     addr.sin_port = htons(port);       // IMPORTANT

//     if (bind(listenFd, (sockaddr *)&addr, sizeof(addr)) < 0)
//         throw std::runtime_error("bind() failed");
//     if (listen(listenFd, 128) < 0)
//         throw std::runtime_error("listen() failed");
//     epollFd = epoll_create(1);
//     if (epollFd < 0)
//         throw std::runtime_error("epoll_create failed");
//     epoll_event ev;
//     ev.events = EPOLLIN;
//     ev.data.fd = listenFd;
//     if (epoll_ctl(epollFd, EPOLL_CTL_ADD, listenFd, &ev) < 0)
//         throw std::runtime_error("epoll_ctl ADD listenFd failed");
//     std::cout << "Server listening on port " << port << std::endl;
// }

void Server::run()
{
    while (g_running)
    {
        // Wait for activity
        epoll_event events[64];
        int nfds = epoll_wait(epollFd, events, 64, -1);
        if (nfds < 0)
        {
            if (!g_running)
                continue;
            std::cerr << "epoll_wait() error\n";
            break;
        }

        // --- Handle clients ---
        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            // --- New connection ---
            auto it = std::find(listenSockets.begin(), listenSockets.end(), fd);
            if (it != listenSockets.end())
            {
                while (true)
                {
                    int newFd = accept(fd, NULL, NULL);
                    if (newFd < 0)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // all connections accepted
                        else
                        {
                            std::cerr << "accept() error\n";
                            break;
                        }
                    }

                    fcntl(newFd, F_SETFL, O_NONBLOCK);
                    Client *c = new Client(newFd, listenFdConfig[fd]);

                    // Add to epoll
                    epoll_event ev;
                    ev.events = EPOLLIN;
                    ev.data.fd = newFd;
                    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &ev) < 0)
                        std::cerr << "epoll_ctl ADD client failed\n";

                    clients.push_back(c);
                    std::cout << "New client connected: " << newFd << std::endl;
                }
                continue; // go to next event
            }

            // --- Existing client ---
            Client *c = nullptr;
            for (auto cl : clients)
            {
                if (cl->getFd() == fd)
                {
                    c = cl;
                    break;
                }
            }
            if (!c)
                continue;
            if (c->isTimeout())
            {
                disconnectClient(c);
                continue;
            }

            // --- Read request ---
            if ((events[i].events & EPOLLIN))
            {
                if (c->readRequest())
                {
                    disconnectClient(c);
                    continue;
                }
                // Enable writing events
                if (c->isRequestComplete())
                {
                    struct epoll_event ev;
                    ev.events = EPOLLOUT;
                    ev.data.fd = fd;
                    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
                }
            }

            // --- Send headers and file ---
            if ((events[i].events & EPOLLOUT))
            {
                // std::cout << "SENDING \n";
                if (c->sendResponse())
                {
                    disconnectClient(c);
                    continue;
                }
            }
        }
    }
    std::cout << "\nShutdown: Starting\n";
    for (auto cl : clients)
        disconnectClient(cl);
    clients.clear();
    close(epollFd);
    close(listenFd);
    std::cout << "Shutdown: Finished\n";
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

void Server::disconnectClient(Client *c)
{
    if (!c)
        return;

    int fd = c->getFd();

    // Remove from epoll
    if (fd >= 0)
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);

    // Close socket
    if (fd >= 0)
        close(fd);

    // Close file if open
    if (c->getFile())
    {
        if (c->getFile()->is_open())
            c->getFile()->close();
        delete c->getFile();
        c->setFile(nullptr);
    }
    c->resetRequest();

    // Remove from clients vector
    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        if (*it == c)
        {
            clients.erase(it);
            break;
        }
    }

    delete c;
    std::cout << "Client " << fd << " disconnected: " << fd << std::endl;
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

const char *Server::noValidServer::what() const throw()
{
    return "Error: No Valid Server in configuration file\n";
}
