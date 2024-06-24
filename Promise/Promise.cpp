#include <iostream>
#include <boost/asio.hpp>
#include "Asio.h"

namespace asio = boost::asio;
namespace Utils = Sari::Utils;

boost::asio::any_io_executor test(asio::io_context& ioContext)
{
    return ioContext.get_executor();
}

int main()
{
    try {

        asio::io_context ioContext;

        Utils::Promise p = Utils::Promise(
            ioContext,
            [](Utils::VariadicFunction resolve, Utils::VariadicFunction reject)
            {
                resolve(2, 3);
            }
        );

        p.then([](int x, int y) {
            return x + y;
        }).then([&ioContext](int sum) {

            std::cout << sum << '\n';

            return Utils::Promise::Resolve(ioContext, sum)
                .then([](int sum) {
                    return sum;
                }).fail([](const std::exception& e) {
                    std::cerr << "error2: " << e.what() << '\n';
                    return 0;
                });

        }).then([](int sum) {
            std::cout << "test: " << sum << "\n";
        }).fail([](int ec) {
            std::cerr << "error: " << ec << '\n';
        }).fail([]() {
            std::cerr << "error!\n";
        }).fail([](const std::exception& e) {
            std::cerr << "error: " << e.what() << '\n';
        });

        ioContext.run();

        std::cout << "state: " << static_cast<int>(p.state()) << '\n';
        std::cout << "result: " << p.result().size() << '\n';

    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }
}
