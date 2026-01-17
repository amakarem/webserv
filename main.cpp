/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:26:46 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/17 18:17:20 by aelaaser         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        std::cerr << "Usage: ./webserv [config_file]\n";
        return (1);
    }
    Server server;
    try
    {
        if (argc == 1)
            server.setdefaultConf();
        else if (argc == 2)
            server.setConfig(argv[1]);
        server.startListening();
        server.run();
        // while (1)
        //     ; // temporary (select loop later)
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return (0);
}
