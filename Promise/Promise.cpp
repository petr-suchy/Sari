#include <iostream>
#include <boost/asio.hpp>
#include "AnyFunction.h"
#include "Promise.h"

namespace asio = boost::asio;
namespace Utils = Sari::Utils;

int main()
{
    try {

        asio::io_service service;

        Utils::Promise p = Utils::Promise(
            service,
            [](Utils::VariadicFunction resolve, Utils::VariadicFunction reject)
            {
                resolve(2, 3);
            }
        );

        p.then([](int x, int y) {
            return x + y;
        }).then([&service = p.service()](int sum) {

            std::cout << sum << '\n';

            return Utils::Promise::Resolve(service, sum)
                .then([&service](int sum) {
                    return sum;
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

        service.run();

        std::cout << "state: " << static_cast<int>(p.state()) << '\n';
        std::cout << "result: " << p.result().size() << '\n';

    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }
}
