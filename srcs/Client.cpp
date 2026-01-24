/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/24 20:41:35 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/24 20:47:42 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client() : fd(-1), file(NULL), headersSent(false), finished(false) {}
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

void Client::setFd(int clientFd) { fd = clientFd; }
int Client::getFd() const { return fd; }

void Client::setFile(std::ifstream *f) { file = f; }
std::ifstream *Client::getFile() const { return file; }

void Client::setHeaderBuffer(const std::string &buf) { headerBuffer = buf; }
std::string &Client::getHeaderBuffer() { return headerBuffer; }

void Client::setHeadersSent(bool val) { headersSent = val; }
bool Client::isHeadersSent() const { return headersSent; }

void Client::setFinished(bool val) { finished = val; }
bool Client::isFinished() const { return finished; }
