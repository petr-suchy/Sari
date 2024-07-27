#pragma once

#include "sari/asio/asio.h"
#include "sari/string/trim.h"

template<typename Socket>
Sari::Utils::Promise StartProxy(Socket&& peer)
{
    namespace asio = boost::asio;
	namespace Asio = Sari::Asio;
	namespace Utils = Sari::Utils;
	namespace String = Sari::String;

	auto sock = std::make_shared<boost::asio::ip::tcp::socket>(std::move(peer));
    auto streamBuf = std::make_shared<asio::streambuf>();

    return Utils::Promise::Resolve(sock->get_executor())
        .then([sock, streamBuf]() {
        
            auto readUntilCommandTask = [sock](std::shared_ptr<asio::streambuf> streamBuf) {
                return Asio::AsyncReadUntil(*sock, *streamBuf, "\r\n")
                    .then([sock, streamBuf]() {

                        // Extract one line from the stream buffer.
                        std::istream is(streamBuf.get());
                        std::string line;
                        std::getline(is, line);

                        std::string command = String::Trim(line).str();

                        if (command.empty()) {
                            // When the line is empty, read again.
                            return Utils::Promise::Resolve(sock->get_executor(), true, streamBuf);
                        }
                        else {
                            // Otherwise resolve with the command string.
                            return Utils::Promise::Resolve(sock->get_executor(), false, command, streamBuf);
                        }
                });
            };

            // Repeat the command reading task until a command is received.
            Utils::Promise readUntilCommand = Utils::Promise::Repeat(
                sock->get_executor(),
                readUntilCommandTask, streamBuf
            );

            // Add the time limit for receiving the command.
            return Asio::AsyncDeadline(readUntilCommand, boost::asio::chrono::seconds(15));
        }).then([sock](std::string command) {

            std::cout << ": \"" << command << "\"\n";
            sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);

        }).finalize([sock](Utils::Promise p) {
            sock->close();
        });
}
