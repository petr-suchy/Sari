#include <iostream>
#include <boost/process.hpp>
#include "sari/net/server.h"
#include "sari/stream/twowaystream.h"
#include "sari/stream/transfer.h"

namespace asio = boost::asio;
namespace Net = Sari::Net;
namespace Stream = Sari::Stream;
namespace Utils = Sari::Utils;

template<typename TragetStream>
Utils::Promise StartShell(boost::asio::io_context& ioContext, TragetStream targetStream)
{
    auto target = std::make_shared<asio::ip::tcp::socket>(std::move(targetStream));
    auto pipe = std::make_shared<Stream::TwoWayStream<boost::process::async_pipe>>(ioContext);

    return Utils::Promise::Resolve(ioContext)
        .then([target, pipe]() {

            auto child = std::make_shared<boost::process::child>(
                boost::process::search_path("cmd"),
                boost::process::std_out > pipe->writable_end(),
                boost::process::std_err > pipe->writable_end(),
                boost::process::std_in < pipe->readable_end()
            );
            
            return Stream::Transfer::Forward(*target, *pipe)
                .then([target, pipe, child]() {});
        });
}

int main(int argc, char* argv[])
{
    boost::asio::io_context ioContext;
    
    try {

        Net::Server server(
            ioContext, 1234,
            [&ioContext](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer) {

                if (ec) {
                    std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
                    return;
                }

                StartShell(ioContext, std::move(peer))
                    .then([]() {
                        std::cout << "Connection end.\n";
                    })
                    .fail([](const boost::system::error_code ec) {
                        std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
                    }).fail([](const std::exception e) {
                        std::cerr << "error: " << e.what() << '\n';
                    }).fail([](std::string what) {
                        std::cerr << "error: " << what << '\n';
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