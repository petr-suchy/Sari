#pragma once

namespace Socks5 {

	const unsigned char ProtocolVersion = 0x05;

	enum class Method : unsigned char {
		NoAuthRequired = 0x00,
		GSSAPI = 0x01,
		UsernamePassword = 0x02,
		NoAcceptableMethods = 0xFF
	};

	enum class Command : unsigned char {
		Connect = 0x01,
		Bind = 0x02,
		UDP = 0x03
	};

	enum class AddressType : unsigned char {
		IPV4 = 0x01,
		DomainName = 0x03,
		IPV6 = 0x04
	};

	enum class Reply : unsigned char {
		Succeeded = 0x00,
		GeneralServeFailure = 0x01,
		ConnectionNoAllowed = 0x02,
		NetworkUnreachable = 0x03,
		HostUnreachable = 0x04,
		ConnectionRefused = 0x05,
		TTLExpired = 0x06,
		CommandNotSupported = 0x07,
		AddressTypeNotSupported = 0x08
	};

	struct RawMethodRequest {
		unsigned char ver;
		unsigned char nmethods;
		Method methods[255];
	};

	struct RawMethodReply {
		unsigned char ver;
		Method method;
	};

	struct RawAddress {
		AddressType atyp;
		unsigned char addr[256];
		unsigned short port;
	};

	struct RawCommandRequest {
		unsigned char ver;
		Command cmd;
		unsigned char rsv;
		RawAddress dest;
	};

	struct RawCommandReply {
		unsigned char ver;
		Reply rep;
		unsigned char rsv;
		RawAddress bind;
	};

}