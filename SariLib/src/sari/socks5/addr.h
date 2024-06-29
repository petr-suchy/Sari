#pragma once

#include <string>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/endian/conversion.hpp>

#include "raw.h"

namespace Socks5 {

	class Address {
	public:

		Address(RawAddress& addr) :
			addr_(addr)
		{}

		const RawAddress& getAddr() const
		{
			return addr_;
		}

		void setAddr(const RawAddress& addr)
		{
			addr_ = addr;
		}

		AddressType getAddrType()
		{
			return addr_.atyp;
		}

		unsigned short getPort()
		{
			return boost::endian::big_to_native(addr_.port);
		}

		void setPort(unsigned short port)
		{
			addr_.port = boost::endian::native_to_big(port);
		}

		boost::asio::ip::tcp::endpoint getEndpoint()
		{
			switch (addr_.atyp) {
				case AddressType::IPV4: {

					boost::asio::ip::address_v4 addrV4(
						{ addr_.addr[0], addr_.addr[1], addr_.addr[2], addr_.addr[3] }
					);

					return boost::asio::ip::tcp::endpoint{addrV4, getPort()};
				} case AddressType::IPV6: {

					boost::asio::ip::address_v6 addrV6(
						{
							addr_.addr[0], addr_.addr[1], addr_.addr[2], addr_.addr[3],
							addr_.addr[4], addr_.addr[5], addr_.addr[6], addr_.addr[7],
							addr_.addr[8], addr_.addr[9], addr_.addr[10], addr_.addr[11],
							addr_.addr[12], addr_.addr[13], addr_.addr[14], addr_.addr[15]
						}
					);

					return boost::asio::ip::tcp::endpoint{addrV6, getPort()};
				} default: {
					throw std::logic_error("cannot convert the given address type to an endpoint");
				}
			}
		}

		void setEndpoint(const boost::asio::ip::tcp::endpoint& endpoint)
		{
			boost::asio::ip::address addr = endpoint.address();

			if (addr.is_v4()) {
				addr_.atyp = Socks5::AddressType::IPV4;
				std::array<unsigned char, 4> addrV4 = addr.to_v4().to_bytes();
				std::copy(addrV4.begin(), addrV4.end(), addr_.addr);
			}
			else if (addr.is_v6()) {
				addr_.atyp = Socks5::AddressType::IPV6;
				std::array<unsigned char, 16> addrV6 = addr.to_v6().to_bytes();
				std::copy(addrV6.begin(), addrV6.end(), addr_.addr);
			}

			setPort(endpoint.port());
		}

		std::string getDomainName()
		{
			if (addr_.atyp != AddressType::DomainName) {
				throw std::logic_error("address type is not a domain name");
			}

			return std::string{
				reinterpret_cast<char*>(&addr_.addr[1]),
				addr_.addr[0]
			};
		}

		void setDomainName(const std::string domainName)
		{
			if (domainName.length() > 255) {
				throw std::logic_error("domain name is too long");
			}

			addr_.atyp = Socks5::AddressType::DomainName;
			addr_.addr[0] = static_cast<unsigned char>(domainName.length());
			std::copy(domainName.cbegin(), domainName.cend(), &addr_.addr[1]);
		}

	private:

		RawAddress& addr_;

	};

}

