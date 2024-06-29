#pragma once

#include <boost/system/error_code.hpp>

enum class Socks5Errc {
    Success = 0,
    InvalidProtocolVersion = 1,
    InvalidAddressType = 2,
    NoAcceptableMethods = 3,
    CommandNotSupported = 4,
    HostUnreachable = 5
};

namespace boost {
    namespace system {
        // Tell the C++ 11 STL metaprogramming that enum ConversionErrc
        // is registered with the standard error code system
        template <>
        struct is_error_code_enum<Socks5Errc> : std::true_type {
        };
    }
}

namespace Sari { namespace Socks5 {

    // Define a custom error code category derived from boost::system::error_category
    class Socks5Errc_category : public boost::system::error_category {
    public:

        // Return a short descriptive name for the category
        virtual const char* name() const noexcept override final { return "socks5"; }

        // Return what each enum means in text
        virtual std::string message(int c) const override final
        {
            switch (static_cast<Socks5Errc>(c))
            {
                case Socks5Errc::Success:
                    return "successful";
                case Socks5Errc::InvalidProtocolVersion:
                    return "invalid socks protocol version";
                case Socks5Errc::InvalidAddressType:
                    return "invalid address type";
                case Socks5Errc::NoAcceptableMethods:
                    return "no acceptable methods";
                case Socks5Errc::CommandNotSupported:
                    return "command is not supported";
                case Socks5Errc::HostUnreachable:
                    return "host is unreachable";
                default:
                    return "unknown";
            }
        }

    };

}}


extern inline const Sari::Socks5::Socks5Errc_category& Socks5Errc_category()
{
    static Sari::Socks5::Socks5Errc_category c;
    return c;
}

// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.

inline boost::system::error_code make_error_code(Socks5Errc e)
{
    return { static_cast<int>(e), Socks5Errc_category() };
}