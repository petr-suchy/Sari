#include <iostream>
#include <boost/asio.hpp>
#include "Asio.h"

namespace asio = boost::asio;
namespace Utils = Sari::Utils;
namespace Asio = Sari::Asio;

int main()
{
    try {

        asio::io_context ioContext;

        Utils::Promise p = Utils::Promise(
            ioContext,
            [](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
                resolve(2, 3);
            }
        );

        p.then([&ioContext](int x, int y) {
            int seconds = x + y;
            auto t1 = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(seconds));
            return Asio::AsyncWait(*t1).then([t1, seconds]() {
                std::cout << "done!\n";
                return seconds;
            }).fail([]() {
                std::cerr << "error!\n";
                return 0;
            });
        }).then([](int seconds) {
            return seconds;
        }).then([](int seconds) {
            std::cout << "seconds: " << seconds << "\n";
            return seconds;
        }).fail([](const boost::system::error_code& ec) {
            std::cerr << "error" << ec << "\n";
        }).fail([](const std::exception& e) {
            std::cerr << "error: " << e.what() << '\n';
        }).fail([]() {
            std::cerr << "error!!!\n";
        });

        ioContext.run();

        std::cout << "state: " << static_cast<int>(p.state()) << '\n';
        std::cout << "result: " << p.result().size() << '\n';

    }
    catch (const std::exception& e) {
        std::cerr << "-error: " << e.what() << '\n';
    }
}
