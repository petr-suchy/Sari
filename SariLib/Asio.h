#pragma once

#include "Promise.h"

namespace Sari { namespace Asio {

	template<typename Object>
	Utils::Promise AsyncWait(Object& object)
	{
		return Utils::Promise(
			object.get_executor(),
			[&object](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
				object.async_wait([=](const boost::system::error_code& ec) {
					if (ec) {
						reject(ec);
					}
					else {
						resolve();
					}
				});
			}
		);
	}

}}