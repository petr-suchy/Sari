// Socks5Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Server.h"
#include "Socks5.h"
#include "Socks5Server.h"

namespace Asio = Sari::Asio;
namespace Utils = Sari::Utils;
namespace Stream = Sari::Stream;

int main(int argc, char* argv[])
{
    using Sari::Utils::Promise;
 
    boost::asio::io_context ioContext;

    try {

        Server server(
            ioContext, 1234,
            [](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer) {

                if (ec) {
                    std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
                    return;
                }

                Socks5Server(std::move(peer))
                    .fail([](const boost::system::error_code ec) {
                        std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
                    }).fail([](const std::exception& e) {
                        std::cerr << "error: " << e.what() << '\n';
                    }).fail([]() {
                        std::cerr << "an unknown error occurred\n";
                    });

            }
        );

        ioContext.run();
    }
    catch (std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }

    return 0;
}