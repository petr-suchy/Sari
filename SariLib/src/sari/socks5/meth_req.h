#pragma once

#include <stdexcept>
#include <vector>
#include <algorithm>

#include "raw.h"

namespace Sari { namespace Socks5 {

	class MethodRequest {
	public:

		MethodRequest(const std::vector<Method>& methods)
		{
			if (methods.size() > sizeof(RawMethodRequest::methods)) {
				throw std::out_of_range("too many methods");
			}

			methReq_.ver = ProtocolVersion;
			methReq_.nmethods = static_cast<unsigned char>(methods.size());

			std::copy(methods.cbegin(), methods.cend(), methReq_.methods);
		}

		MethodRequest(const RawMethodRequest& methReq) :
			methReq_(methReq)
		{}

		const RawMethodRequest& getRaw() const
		{
			return methReq_;
		}

		const Method* begin() const
		{
			return &methReq_.methods[0];
		}

		const Method* end() const
		{
			return &methReq_.methods[methReq_.nmethods];
		}

		bool contains(Method method) const
		{
			return std::find(begin(), end(), method) != end();
		}

	private:

		RawMethodRequest methReq_;

	};

}}
