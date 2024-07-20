#include <iostream>
#include "sari/asio/asio.h"

namespace Utils = Sari::Utils;
namespace Asio = Sari::Asio;

int main()
{
    boost::asio::io_context ioContext;

    {
        auto timer = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(3));

        Asio::Deadline(Sari::Asio::AsyncWait(*timer), boost::asio::chrono::seconds(1))
            .then([timer]() {
                std::cout << "Done!\n";
            }).fail([](const boost::system::error_code& ec) {
                std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
            }).fail([](){
                std::cerr << "An unknown error occurred!\n";
            });
    }

    ioContext.run();

    return 0;
}
