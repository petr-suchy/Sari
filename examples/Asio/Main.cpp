#include <iostream>
#include "sari/asio/asio.h"

namespace Utils = Sari::Utils;
namespace Asio = Sari::Asio;

int main()
{
    boost::asio::io_context ioContext;

    {
        Utils::Promise::Resolve(ioContext)
            .then([&]() {

                auto timer = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(1));

                Utils::Promise operationInTime = Sari::Asio::AsyncWait(*timer);

                return Asio::AsyncDeadline(operationInTime, boost::asio::chrono::seconds(2))
                    .then([]() {
                        std::cout << "The first operation is successfully done!\n";
                    }).finalize([timer](Utils::Promise p) {
                        timer->cancel();
                    });
            }).then([&]() {

                auto timer = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(2));

                Utils::Promise lateOperation = Sari::Asio::AsyncWait(*timer);

                return Asio::AsyncDeadline(lateOperation, boost::asio::chrono::seconds(1))
                    .then([]() {
                        std::cout << "The second operation should never be completed. :)\n";
                    }).finalize([timer](Utils::Promise p) {
                        timer->cancel();
                    });
            }).fail([](const boost::system::error_code& ec) {
                std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
            }).fail([](){
                std::cerr << "An unknown error occurred!\n";
            });
    }

    ioContext.run();

    return 0;
}
