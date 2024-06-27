#pragma once

#include "Socks5Raw.h"

namespace Socks5 {

	class MethodReply {
	public:

		MethodReply(Method method)
		{
			methReply_.ver = ProtocolVersion;
			methReply_.method = method;
		}

		MethodReply(const RawMethodReply& methReply) :
			methReply_(methReply)
		{}

		const RawMethodReply& getRaw() const
		{
			return methReply_;
		}

		Method getMethod() const
		{
			return methReply_.method;
		}

	private:

		RawMethodReply methReply_;

	};

}