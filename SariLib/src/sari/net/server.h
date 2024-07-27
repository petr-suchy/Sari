#pragma once

#include <boost/asio.hpp>

namespace Sari { namespace Net {

	class Server {
	public:

		using ConnectionHandler = std::function<void(
			const boost::system::error_code&,
			boost::asio::ip::tcp::socket
		)>;

		Server(boost::asio::io_context& ioContext, unsigned short port, ConnectionHandler connectionHandler) :
			acceptor_(ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
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

		boost::asio::ip::tcp::acceptor acceptor_;
		ConnectionHandler connectionHandler_;

	};

}}

