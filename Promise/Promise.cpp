#include <iostream>
#include "Promise.h"

namespace asio = boost::asio;
namespace Utils = Sari::Utils;

int main()
{
    asio::io_context ioContext;

    Utils::Promise promise = Utils::Promise(
        ioContext,
        [](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
            // Asynchronous operation
            bool success = true;

            if (success) {
                resolve(std::string{"Operation was successful!"});
            }
            else {
                reject(std::string{"Operation failed."});
            }
        }
    );

    promise.then([&](std::string value) {
        std::cout << value << '\n'; // "Operation was successful!"
        return Utils::Promise(
            ioContext,
            [](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
                resolve(std::string{"Next step success!"});
            }
        );
    }).then([](std::string value) {
        std::cout << value << '\n'; // "Next step success!"
    })
    .fail([](std::string error) {
        std::cerr << error << '\n'; // "Operation failed."
    });

    ioContext.run();

    return 0;
}
