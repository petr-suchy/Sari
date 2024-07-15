#include <iostream>
#include "sari/utils/promise.h"

namespace asio = boost::asio;
namespace Utils = Sari::Utils;

template<typename Object>
Utils::Promise AsyncWait(Object& object)
{
	return Utils::Promise(
		object.get_executor(),
		[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {
			object.async_wait([=](const boost::system::error_code& ec) {
				if (ec) {
					reject(ec);
				}
				else {
					resolve();
				}
			});
		},
		Utils::Promise::Async
	);
}

int main()
{
    asio::io_context ioContext;

    // Example #1:

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

    // Example #2:

    {

    auto timer = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(3));

    AsyncWait(*timer)
        .then([timer]() {
            std::cout << "Time elapsed!\n";
        })
        .fail([](const boost::system::error_code& ec) {
            std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
        });

    }

    // Example #3:

    {
        int n = 10;

        auto task = [&ioContext, n](int i) {

            if (i < n) {

                auto timer = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(1));

                return AsyncWait(*timer)
                    .then([&ioContext, i, timer]() {
                        std::cout << i << '\n';
                        return Utils::Promise::Resolve(ioContext, true, i + 1);
                    });
            }
            else {
                return Utils::Promise::Resolve(ioContext, false, i);
            }

        };

        Utils::Promise::Repeat(ioContext, task, 0)
            .then([](int x) {
                std::cout << "Done with " << x << ".\n";
            }).fail([](const boost::system::error_code& ec) {
                std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
            });
    }

    ioContext.run();

    return 0;
}
