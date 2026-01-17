/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aelaaser <aelaaser@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 18:26:46 by aelaaser          #+#    #+#             */
/*   Updated: 2026/01/17 17:59:07 by aelaaser         ###   ########.fr       */
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
    try
    {
        if (argc == 1)
            Server server;
        else if (argc == 2)
            Server server(argv[1]);
        // while (1)
        //     ; // temporary (select loop later)
    }
    catch (std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return (0);
}
