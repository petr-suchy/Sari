#include <iostream>
#include "sari/net/server.h"
#include "sari/asio/asio.h"

namespace Net = Sari::Net;
namespace Asio = Sari::Asio;
namespace Utils = Sari::Utils;

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

                std::cout << "Connection established!\n";

                peer.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                peer.close();
            }
        );

        ioContext.run();
    }
    catch (std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }

    return 0;
}