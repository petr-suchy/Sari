#pragma once

#include "Socks5.h"
#include "ForwardStream.h"

template<typename Stream>
Sari::Utils::Promise Socks5Server(Stream&& sock)
{
    using Sari::Utils::Promise;

    auto iSock = std::make_shared<boost::asio::ip::tcp::socket>(std::move(sock));
    auto oSock = std::make_shared<boost::asio::ip::tcp::socket>(iSock->get_executor());
    
    return Socks5::AsyncRecvMethodRequest(*iSock)
        .then([iSock](Socks5::MethodRequest methReq) {

            Socks5::Method method = Socks5::Method::NoAuthRequired;

            if (!methReq.contains(method)) {
                method = Socks5::Method::NoAcceptableMethods;
            }

            Socks5::MethodReply methReply{ method };

            return Socks5::AsyncSendMethodReply(*iSock, methReply)
                .then([iSock, method]() {
                    if (method == Socks5::Method::NoAcceptableMethods) {

                        iSock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                        iSock->close();

                        return Promise::Reject(
                            iSock->get_executor(), make_error_code(Socks5Errc::NoAcceptableMethods)
                        );
                    }
                    return Socks5::AsyncRecvCommandRequest(*iSock);
                });

        }).then([iSock, oSock](Socks5::CommandRequest cmdReq){
                    
            if (cmdReq.getCmd() != Socks5::Command::Connect) {

                Socks5::CommandReply cmdReply{
                    Socks5::Reply::CommandNotSupported, cmdReq.dest().getAddr()
                };
                        
                return Promise::Resolve(iSock->get_executor(), cmdReply);
            }

            Promise promise;

            if (cmdReq.dest().getAddrType() == Socks5::AddressType::DomainName) {

                auto domainResolver = std::make_shared<boost::asio::ip::tcp::resolver>(iSock->get_executor());

                boost::asio::ip::tcp::resolver::query query(
                    cmdReq.dest().getDomainName(), std::to_string(cmdReq.dest().getPort())
                );

                promise = Sari::Asio::AsyncResolve(*domainResolver, query)
                    .then([domainResolver, oSock](boost::asio::ip::tcp::resolver::iterator it) {
                        return Sari::Asio::AsyncConnectEndpoints(*oSock, it);
                    });
            }
            else {
                promise = Sari::Asio::AsyncConnect(
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
            return Socks5::AsyncSendCommandReply(*iSock, cmdReply)
                .then([iSock, oSock, reply = cmdReply.getReply()]() {
                    if (reply != Socks5::Reply::Succeeded) {

                        iSock->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
                        iSock->close();

                        return Promise::Reject(
                            iSock->get_executor(), make_error_code(Socks5Errc::HostUnreachable)
                        );
                    }
                    return Sari::Stream::Forward(*iSock, *oSock)
                        .then([iSock, oSock]() {});
                }); 
        });
}