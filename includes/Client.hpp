/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:38:20 by aelaaser          #+#    #+#             */
/*   Updated: 2026/02/08 00:07:37 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <ctime>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "HttpRequest.hpp"
#include "Struct.hpp"

class Client
{
    private:
        int fd;                      // client socket
        std::ifstream* file;          // pointer to open file
        std::string headerBuffer;     // HTTP headers to send
        bool headersSent;             // headers already sent
        bool finished;                // done sending everything
        long lastActivity;
        const ServerConfig& config;
        std::string fullPath;
        std::string recvBuffer;
        std::string body;
        size_t contentLength;
        bool headersParsed;
        bool bodyComplete;
        HttpRequest request;
        bool PHP;
        std::string sendBuffer;

    public:
        Client(int fd, const ServerConfig& config);
        ~Client();
        int getFd() const;

        bool isKeepAlive() const;
        bool isRequestComplete() const;
        
        void setFile(std::ifstream* f);
        std::ifstream* getFile() const;

        void setHeaderBuffer(const std::string& buf);
        std::string& getHeaderBuffer();

        void setlastActivity();
        long getlastActivity() const;
        bool isTimeout() const;
        bool isPHP() const;
        
        void setHeadersSent(bool val);
        bool isHeadersSent() const;

        void setFinished(bool val);
        bool isFinished() const;

        std::string resolvePath(const std::string &path);
        std::string generateDirectoryListing(const std::string &dir);
        bool continueAfterHeader();
        int readRequest();
        int sendResponse();
        // std::string executePHP(const std::string &scriptPath, const std::string &body);
        std::string executePHP(const std::string &scriptPath);
        void resetRequest();
};
#endif
