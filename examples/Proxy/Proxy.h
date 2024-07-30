#pragma once

#include "sari/asio/asio.h"
#include "sari/string/trim.h"
#include "sari/utils/exchanger.h"
#include "Sari/stream/coupler.h"
#include "sari/stream/forward.h"

class Proxy {
public:

    template<typename Socket>
    static Sari::Utils::Promise Start(Socket&& peer, Sari::Utils::Exchanger& exchanger)
    {
        namespace asio = boost::asio;
        namespace Asio = Sari::Asio;
        namespace Utils = Sari::Utils;
        namespace Stream = Sari::Stream;

	    auto sock = std::make_shared<boost::asio::ip::tcp::socket>(std::move(peer));

        return Utils::Promise::Resolve(sock->get_executor())
            .then([sock]() {
                // Try to receive a command in the time limit.
                return Utils::Promise::AllSettled(sock->get_executor(),
                        { Asio::AsyncDeadline(AsyncReadUntilCommand(sock), boost::asio::chrono::seconds(15)) }
                    ).then([sock](Utils::Promise p) {
                        if (p.isFulfilled()) {
                            return Utils::Promise::Resolve(p.getExecutor(), p.result());
                        }
                        else { // Rejected
                            return Asio::AsyncWrite(*sock, "ERR " + PromiseResultToStatusTest(p) + "\r\n")
                                .then([sock, p]() {
                                    sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                                    return Utils::Promise::Reject(p.getExecutor(), p.result());
                                });
                        }
                    });
            }).then([sock, &exchanger](std::string command) {

                using PipeCoupler = Stream::Coupler<boost::asio::readable_pipe, boost::asio::writable_pipe>;

                auto sourcePipe = std::make_shared<PipeCoupler>(sock->get_executor());
                auto sinkPipe = std::make_shared<PipeCoupler>(sock->get_executor());

                boost::asio::connect_pipe(sourcePipe->writable_end(), sinkPipe->readable_end());
                boost::asio::connect_pipe(sinkPipe->writable_end(), sourcePipe->readable_end());

                auto trans = std::make_shared<Utils::Exchanger::Transaction>(sock->get_executor());

                Utils::Promise sinkToOther;

                if (command == "CONNECT") {
                    sinkToOther = exchanger.asyncConsume(*trans, sinkPipe)
                        .then([sinkPipe, command](std::shared_ptr<PipeCoupler> other) {
                            return Asio::AsyncWrite(*other, "ACK\r\n")
                                .then([sinkPipe, other, command]() {
                                    std::cout << command << " request forwarded.\n";
                                    return Stream::Forward(*sinkPipe, *other)
                                        .then([sinkPipe, other]() {});
                                });
                        });
                }
                else if (command == "BIND") {
                    sinkToOther = exchanger.asyncPrtoduce(*trans, sinkPipe)
                        .then([sinkPipe, command](std::shared_ptr<PipeCoupler> other) {
                            std::cout << command << " request forwarded.\n";
                            return Stream::Forward(*sinkPipe, *other)
                                .then([sinkPipe, other]() {});
                        });
                }
                else {
                    return Asio::AsyncWrite(*sock, "ERR Invalid command\r\n")
                        .then([sock]() {
                            sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                            return Utils::Promise::Reject(sock->get_executor(), std::string{"invalid command"});
                        });
                }

                if (trans->isPending()) {
                    std::cout << command << " request is pending...\n";
                }

                auto sockToSource = Stream::Forward(*sock, *sourcePipe)
                    .then([sock, sourcePipe, trans, command]() {

                        bool isPending = trans->isPending();

                        trans->cancel();

                        if (isPending && !trans->isPending()) {
                            std::cout << command << " request canceled.\n";
                        }
                    });

                return Utils::Promise::All(sock->get_executor(), { sockToSource, sinkToOther });
            }).finalize([sock](Utils::Promise p) {
                sock->close();
            });
    }

private:

    static Sari::Utils::Promise AsyncReadUntilCommand(std::shared_ptr<boost::asio::ip::tcp::socket> sock)
    {
        namespace asio = boost::asio;
        namespace Asio = Sari::Asio;
        namespace Utils = Sari::Utils;
        namespace String = Sari::String;

        auto streamBuf = std::make_shared<asio::streambuf>();

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
        return Utils::Promise::Repeat(sock->get_executor(), readUntilCommandTask, streamBuf);
    }

    static std::string PromiseResultToStatusTest(Sari::Utils::Promise p)
    {
        if (p.isPending()) {
            return "Pending";
        }
        else if (p.isFulfilled()) {
            return "Successful";
        }

        // When rejected:

        std::string statusText = "Internal server error";

        if (p.result().empty() || p.result(0).type() != typeid(boost::system::error_code)) {
            return statusText;
        }

        auto ec = p.result<boost::system::error_code>(0);

        if (ec == make_error_code(boost::system::errc::timed_out)) {
            statusText = "Timeout";
        }

        return statusText;
    }

};

