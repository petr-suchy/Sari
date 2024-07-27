#include <iostream>
#include "sari/net/server.h"
#include "Proxy.h"

namespace asio = boost::asio;
namespace Net = Sari::Net;
namespace Asio = Sari::Asio;
namespace Utils = Sari::Utils;
namespace String = Sari::String;

int main(int argc, char* argv[])
{
    boost::asio::io_context ioContext;

    try {

        Net::Server server(
            ioContext, 1234,
            [](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer) {

                if (ec) {
                    std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
                    return;
                }

                StartProxy(std::move(peer))
                    .fail([](const boost::system::error_code ec) {
                        std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
                    }).fail([](const std::exception e) {
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