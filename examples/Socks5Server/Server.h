#pragma once

#include <functional>
#include <boost/asio.hpp>

class Server {
public:

	using ConnectionHandler = std::function<void(
		const boost::system::error_code&,
		boost::asio::ip::tcp::socket
	)>;

	Server(boost::asio::io_service& ioService, unsigned short port, ConnectionHandler connectionHandler) :
		ioService_(ioService),
		acceptor_(ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
		connectionHandler_(connectionHandler)
	{
		startAccept();
	}

	void startAccept()
	{
		acceptor_.async_accept(
			[this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket peer) {
				this->connectionHandler_(ec, std::move(peer));
				startAccept();
			}
		);
	}

private:

	boost::asio::io_service& ioService_;
	boost::asio::ip::tcp::acceptor acceptor_;
	ConnectionHandler connectionHandler_;

};

