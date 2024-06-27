// Socks5Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Server.h"
#include "Socks5.h"
#include "ForwardStream.h"

namespace Asio = Sari::Asio;
namespace Utils = Sari::Utils;
namespace Stream = Sari::Stream;

int main(int argc, char* argv[])
{
    using Sari::Utils::Promise;
 
    boost::asio::io_context ioContext;
    boost::asio::ip::tcp::resolver resolver(ioContext);

    try {

        Promise p = Promise(
            ioContext, [](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
                reject(make_error_code(Socks5Errc::InvalidAddressType));
            }
        );

        Server server(
            ioContext, 1234,
            [&ioContext, &resolver](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer) {

                if (ec) {
                    std::cerr << "error: " << ec << '\n';
                    return;
                }

                auto iSock = std::make_shared<boost::asio::ip::tcp::socket>(std::move(peer));
                auto oSock = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);

                Socks5::AsyncRecvMethodRequest(*iSock).then([iSock](Socks5::MethodRequest methReq) {

                    Socks5::Method method = Socks5::Method::NoAuthRequired;

                    if (!methReq.contains(method)) {
                        method = Socks5::Method::NoAcceptableMethods;
                    }

                    Socks5::MethodReply methReply{ method };

                    return Socks5::AsyncSendMethodReply(*iSock, methReply).then([iSock, method]() {
                        if (method == Socks5::Method::NoAcceptableMethods) {
                            iSock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                            iSock->close();
                            return Promise::Reject(
                                iSock->get_executor(), make_error_code(Socks5Errc::NoAcceptableMethods)
                            );
                        }
                        return Socks5::AsyncRecvCommandRequest(*iSock);
                    });
                }).then([&resolver, iSock, oSock](Socks5::CommandRequest cmdReq){
                    
                    if (cmdReq.getCmd() != Socks5::Command::Connect) {

                        Socks5::CommandReply cmdReply{
                            Socks5::Reply::CommandNotSupported, cmdReq.dest().getAddr()
                        };
                        
                        return Promise::Resolve(iSock->get_executor(), cmdReply);
                    }

                    Promise promise;

                    if (cmdReq.dest().getAddrType() == Socks5::AddressType::DomainName) {

                        boost::asio::ip::tcp::resolver::query query(
                            cmdReq.dest().getDomainName(), std::to_string(cmdReq.dest().getPort())
                        );

                        promise = Asio::AsyncResolve(resolver, query).then([oSock](boost::asio::ip::tcp::resolver::iterator it) {
                            return Asio::AsyncConnectEndpoints(*oSock, it);
                        });
                    }
                    else {
                        promise = Asio::AsyncConnect(
                            *oSock, cmdReq.dest().getEndpoint()
                        );
                    }

                    promise.then([oSock]() {
                        return Socks5::CommandReply{
                            Socks5::Reply::Succeeded, oSock->remote_endpoint()
                        };
                    }).fail([cmdReq]() mutable {
                        return Socks5::CommandReply{
                            Socks5::Reply::HostUnreachable, cmdReq.dest().getAddr()
                        };
                    });

                    return promise;
                }).then([iSock, oSock](Socks5::CommandReply cmdReply) {
                    return Socks5::AsyncSendCommandReply(*iSock, cmdReply).then([iSock, oSock, reply = cmdReply.getReply()]() {
                        if (reply != Socks5::Reply::Succeeded) {
                            iSock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                            iSock->close();
                            return Promise::Reject(
                                iSock->get_executor(), make_error_code(Socks5Errc::HostUnreachable)
                            );
                        }
                        return Stream::Forward(*iSock, *oSock).then([iSock, oSock]() {});
                    }); 
                }).fail([](const boost::system::error_code ec) {
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