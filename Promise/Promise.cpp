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
            std::cout << x << ' ' << y << '\n';
            return x + y;
        }).then([&service = p.service()](int sum) {
            std::cout << sum << '\n';
            return Utils::Promise::Resolve(service, 123).then([&service](int x) {
                std::cout << x << '\n';
                return Utils::Promise::Resolve(service, 10, 20);
            });
        });

        service.run();

        std::cout << "state: " << static_cast<int>(p.state()) << '\n';
        std::cout << "result: " << p.result().size() << '\n';

    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }
}
