#pragma once

#include <boost/array.hpp>

#include "errc.h"
#include "meth_req.h"
#include "meth_reply.h"
#include "cmd_req.h"
#include "cmd_reply.h"
#include "../asio/asio.h"

namespace Sari { namespace Socks5 {

	template<typename Stream>
	Sari::Utils::Promise AsyncSendMethodRequest(Stream& stream, const MethodRequest& methReq)
	{
		using Sari::Asio::AsyncWriteSome;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawMethodRequest>(methReq.getRaw());

		boost::array<boost::asio::const_buffer, 3> buffers;

		buffers[0] = buffer(&raw->ver, sizeof(raw->ver));
		buffers[1] = buffer(&raw->nmethods, sizeof(raw->nmethods));
		buffers[2] = buffer(raw->methods, raw->nmethods);

		return AsyncWriteSome(
			stream, buffers
		).then([raw]() {
			return MethodRequest{*raw};
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncRecvMethodRequest(Stream& stream)
	{
		using Sari::Asio::AsyncRead;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawMethodRequest>();

		return AsyncRead(
			stream, buffer(&raw->ver, sizeof(raw->ver))
		).then([&stream, raw]() {
			if (raw->ver != ProtocolVersion) {
				return Sari::Utils::Promise::Reject(
					stream.get_executor(), make_error_code(Socks5Errc::InvalidProtocolVersion)
				);
			}
			return AsyncRead(stream, buffer(&raw->nmethods, sizeof(raw->nmethods)));
		}).then([&stream, raw]() {
			return AsyncRead(stream, buffer(raw->methods, raw->nmethods));
		}).then([raw]() {
			return MethodRequest{*raw};
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncSendMethodReply(Stream& stream, const MethodReply& methReply)
	{
		using Sari::Asio::AsyncWriteSome;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawMethodReply>(methReply.getRaw());

		boost::array<boost::asio::const_buffer, 2> buffers;

		buffers[0] = buffer(&raw->ver, sizeof(raw->ver));
		buffers[1] = buffer(&raw->method, sizeof(raw->method));

		return AsyncWriteSome(
			stream, buffers
		).then([raw]() {
			return MethodReply{*raw};
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncRecvMethodReply(Stream& stream)
	{
		using Sari::Asio::AsyncRead;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawMethodReply>();

		return AsyncRead(
			stream, buffer(&raw->ver, sizeof(raw->ver))
		).then([&stream, raw]() {
			if (raw->ver != ProtocolVersion) {
				return Sari::Utils::Promise::Reject(
					stream.get_executor(), make_error_code(Socks5Errc::InvalidProtocolVersion)
				);
			}
			return AsyncRead(stream, buffer(&raw->method, sizeof(raw->method)));
		}).then([raw]() {
			return MethodReply{*raw};
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncSendCommandRequest(Stream& stream, const CommandRequest& cmdReq)
	{
		using Sari::Asio::AsyncWriteSome;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawCommandRequest>(cmdReq.getRaw());

		boost::array<boost::asio::const_buffer, 6> buffers;

		buffers[0] = buffer(&raw->ver, sizeof(raw->ver));
		buffers[1] = buffer(&raw->cmd, sizeof(raw->cmd));
		buffers[2] = buffer(&raw->rsv, sizeof(raw->rsv));
		buffers[3] = buffer(&raw->dest.atyp, sizeof(raw->dest.atyp));

		switch (raw->dest.atyp) {
			case AddressType::IPV4:
				buffers[4] = buffer(raw->dest.addr, 4);
			break;
			case AddressType::DomainName:
				buffers[4] = buffer(raw->dest.addr, raw->dest.addr[0] + 1);
			break;
			case AddressType::IPV6:
				buffers[4] = buffer(raw->dest.addr, 6);
			break;
			default:
				return Sari::Utils::Promise::Reject(
					stream.get_executor(), make_error_code(Socks5Errc::InvalidAddressType)
				);
		}

		buffers[5] = buffer(&raw->dest.port, sizeof(raw->dest.port));

		return AsyncWriteSome(
			stream, buffers
		).then([raw]() {
			return CommandRequest{*raw};
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncRecvCommandRequest(Stream& stream)
	{
		using Sari::Asio::AsyncRead;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawCommandRequest>();

		return AsyncRead(
			stream, buffer(&raw->ver, sizeof(raw->ver))
		).then([&stream, raw]() {
			if (raw->ver != ProtocolVersion) {
				return Sari::Utils::Promise::Reject(
					stream.get_executor(), make_error_code(Socks5Errc::InvalidProtocolVersion)
				);
			}
			return AsyncRead(stream, buffer(&raw->cmd, sizeof(raw->cmd)));
		}).then([&stream, raw]() {
			return AsyncRead(stream, buffer(&raw->rsv, sizeof(raw->rsv)));
		}).then([&stream, raw]() {
			return AsyncRead(stream, buffer(&raw->dest.atyp, sizeof(raw->dest.atyp)));
		}).then([&stream, raw]() {
			switch (raw->dest.atyp) {
				case AddressType::IPV4:
					return AsyncRead(stream, buffer(raw->dest.addr, 4));
				case AddressType::DomainName:
					return AsyncRead(
						stream, buffer(&raw->dest.addr[0], 1) // length
					).then([&stream, raw]() {
						return AsyncRead(stream, buffer(&raw->dest.addr[1], raw->dest.addr[0]));
					});
				case AddressType::IPV6:
					return AsyncRead(stream, buffer(raw->dest.addr, 16));
				default:
					return Sari::Utils::Promise::Reject(
						stream.get_executor(), make_error_code(Socks5Errc::InvalidAddressType)
					);
			}
		}).then([&stream, raw]() {
			return AsyncRead(
				stream, buffer(&raw->dest.port, sizeof(raw->dest.port))
			).then([raw]() {
				return CommandRequest{*raw};
			});
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncSendCommandReply(Stream& stream, const CommandReply& cmdReply)
	{
		using Sari::Asio::AsyncWriteSome;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawCommandReply>(cmdReply.getRaw());

		boost::array<boost::asio::const_buffer, 6> buffers;

		buffers[0] = buffer(&raw->ver, sizeof(raw->ver));
		buffers[1] = buffer(&raw->rep, sizeof(raw->rep));
		buffers[2] = buffer(&raw->rsv, sizeof(raw->rsv));
		buffers[3] = buffer(&raw->bind.atyp, sizeof(raw->bind.atyp));

		switch (raw->bind.atyp) {
			case AddressType::IPV4:
				buffers[4] = buffer(raw->bind.addr, 4);
			break;
			case AddressType::DomainName:
				buffers[4] = buffer(raw->bind.addr, raw->bind.addr[0] + 1);
			break;
			case AddressType::IPV6:
				buffers[4] = buffer(raw->bind.addr, 6);
			break;
			default:
				return Sari::Utils::Promise::Reject(
					stream.get_executor(), make_error_code(Socks5Errc::InvalidAddressType)
				);
		}

		buffers[5] = buffer(&raw->bind.port, sizeof(raw->bind.port));

		return AsyncWriteSome(
			stream, buffers
		).then([raw]() {
			return CommandReply{*raw};
		});
	}

	template<typename Stream>
	Sari::Utils::Promise AsyncRecvCommandReply(Stream& stream)
	{
		using Sari::Asio::AsyncRead;
		using boost::asio::buffer;

		auto raw = std::make_shared<RawCommandReply>();

		return AsyncRead(
			stream, buffer(&raw->ver, sizeof(raw->ver))
		).then([&stream, raw]() {
			if (raw->ver != ProtocolVersion) {
				return Sari::Utils::Promise::Reject(
					stream.get_executor(), make_error_code(Socks5Errc::InvalidProtocolVersion)
				);
			}
			return AsyncRead(stream, buffer(&raw->rep, sizeof(raw->rep)));
		}).then([&stream, raw]() {
			return AsyncRead(stream, buffer(&raw->rsv, sizeof(raw->rsv)));
		}).then([&stream, raw]() {
			return AsyncRead(stream, buffer(&raw->bind.atyp, sizeof(raw->bind.atyp)));
		}).then([&stream, raw]() {
			switch (raw->bind.atyp) {
				case AddressType::IPV4:
					return AsyncRead(stream, buffer(raw->bind.addr, 4));
				case AddressType::DomainName:
					return AsyncRead(
						stream, buffer(&raw->bind.addr[0], 1) // length
					).then([&stream, raw]() {
						return AsyncRead(stream, buffer(&raw->bind.addr[1], raw->bind.addr[0]));
					});
				case AddressType::IPV6:
					return AsyncRead(stream, buffer(raw->bind.addr, 16));
				default:
					return Sari::Utils::Promise::Reject(
						stream.get_executor(), make_error_code(Socks5Errc::InvalidAddressType)
					);
			}
		}).then([&stream, raw]() {
			return AsyncRead(
				stream, buffer(&raw->bind.port, sizeof(raw->bind.port))
			).then([raw]() {
				return CommandReply{*raw};
			});
		});
	}

}}
