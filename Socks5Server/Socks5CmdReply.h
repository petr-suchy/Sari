#pragma once

#include "Socks5Addr.h"

namespace Socks5 {

	class CommandReply {
	public:

		CommandReply(const RawCommandReply cmdReply) :
			cmdReply_(cmdReply)
		{}

		CommandReply(Reply reply, const RawAddress& addr)
		{
			cmdReply_.ver = ProtocolVersion;
			cmdReply_.rep = reply;
			cmdReply_.rsv = 0;

			bind().setAddr(addr);
		}

		CommandReply(Reply reply, const boost::asio::ip::tcp::endpoint& endpoint)
		{
			cmdReply_.ver = ProtocolVersion;
			cmdReply_.rep = reply;
			cmdReply_.rsv = 0;

			bind().setEndpoint(endpoint);
		}

		const RawCommandReply& getRaw() const
		{
			return cmdReply_;
		}

		Reply getReply() const
		{
			return cmdReply_.rep;
		}

		void setReply(Reply reply)
		{
			cmdReply_.rep = reply;
		}

		Address bind()
		{
			return Address{cmdReply_.bind};
		}

	private:

		RawCommandReply cmdReply_;

	};

}
