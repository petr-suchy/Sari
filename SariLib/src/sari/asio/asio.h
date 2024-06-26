#pragma once

#include "../utils/promise.h"

namespace Sari { namespace Asio {

	template<typename Object>
	Utils::Promise AsyncWait(Object& object)
	{
		return Utils::Promise(
			object.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				object.async_wait([=](const boost::system::error_code& ec) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve();
					}
				});
			},
			Utils::Promise::Async
		);
	}

	template<typename Object, typename ConstBufferSequence>
	Utils::Promise AsyncWriteSome(Object& object, const ConstBufferSequence& buffers) {
		return Utils::Promise(
			object.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				object.async_write_some(buffers, [=](const boost::system::error_code& ec, std::size_t bytesTransferred) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve(bytesTransferred);
					}
				});
			},
			Utils::Promise::Async
		);
	}

	template<typename Object, typename ConstBufferSequence>
	Utils::Promise AsyncReadSome(Object& object, const ConstBufferSequence& buffers) {
		return Utils::Promise(
			object.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				object.async_read_some(buffers, [=](const boost::system::error_code& ec, std::size_t bytesTransferred) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve(bytesTransferred);
					}
				});
			},
			Utils::Promise::Async
		);
	}

	template<typename Object, typename ConstBufferSequence>
	Utils::Promise AsyncRead(Object& object, const ConstBufferSequence& buffers) {
		return Utils::Promise(
			object.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				boost::asio::async_read(object, buffers, [=](const boost::system::error_code& ec, std::size_t bytesTransferred) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve(bytesTransferred);
					}
				});
			},
			Utils::Promise::Async
		);
	}

	template<typename Resolver, typename Query>
	Utils::Promise AsyncResolve(Resolver& resolver, Query& query)
	{
		return Utils::Promise(
			resolver.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				resolver.async_resolve(query, [=](const boost::system::error_code& ec, typename Resolver::iterator it) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve(it);
					}
				});
			},
			Utils::Promise::Async
		);
	}

	template<typename Socket>
	Utils::Promise AsyncConnect(Socket& socket, const typename Socket::endpoint_type& endpoint)
	{
		return Utils::Promise(
			socket.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				socket.async_connect(endpoint, [=](const boost::system::error_code& ec) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve();
					}
				});
			},
			Utils::Promise::Async
		);
	}
	
	template<typename Socket, typename Iterator>
	Utils::Promise AsyncConnectEndpoints(Socket& socket, Iterator it)
	{
		return Utils::Promise(
			socket.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				boost::asio::async_connect(socket, it, [=](const boost::system::error_code& ec, Iterator it) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve();
					}
				});
			},
			Utils::Promise::Async
		);
	}

}}