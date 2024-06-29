#pragma once

#include "addr.h"

namespace Sari { namespace Socks5 {

	class CommandRequest {
	public:

		CommandRequest(const RawCommandRequest cmdReq) :
			cmdReq_(cmdReq)
		{}

		CommandRequest(Command cmd, const RawAddress& addr)
		{
			cmdReq_.ver = ProtocolVersion;
			cmdReq_.cmd = cmd;
			cmdReq_.rsv = 0;

			dest().setAddr(addr);
		}

		CommandRequest(Command cmd, const boost::asio::ip::tcp::endpoint& endpoint)
		{
			cmdReq_.ver = ProtocolVersion;
			cmdReq_.cmd = cmd;
			cmdReq_.rsv = 0;

			dest().setEndpoint(endpoint);
		}

		CommandRequest(Command cmd, const std::string& domainName, unsigned short port)
		{
			cmdReq_.ver = ProtocolVersion;
			cmdReq_.cmd = cmd;
			cmdReq_.rsv = 0;

			dest().setDomainName(domainName);
			dest().setPort(port);
		}

		const RawCommandRequest& getRaw() const
		{
			return cmdReq_;
		}

		Command getCmd() const
		{
			return cmdReq_.cmd;
		}

		void setCmd(Command cmd)
		{
			cmdReq_.cmd = cmd;
		}

		Address dest()
		{
			return Address{cmdReq_.dest};
		}

	private:

		RawCommandRequest cmdReq_;

	};

}}

