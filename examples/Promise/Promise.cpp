#include <iostream>
#include "sari/utils/promise.h"

namespace asio = boost::asio;
namespace Utils = Sari::Utils;

template<typename Object>
Utils::Promise AsyncWait(Object& object)
{
	return Utils::Promise(
		object.get_executor(),
		[&](Utils::AnyFunction resolve, Utils::AnyFunction reject) {
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
    /*
    // Example #1:

    {

        Utils::Promise promise = Utils::Promise(
            ioContext,
            [](Utils::AnyFunction resolve, Utils::AnyFunction reject) {
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
                [](Utils::AnyFunction resolve, Utils::AnyFunction reject) {
                    resolve(std::string{"Next step success!"});
                }
            );
        }).then([](std::string value) {
            std::cout << value << '\n'; // "Next step success!"
        })
        .fail([](std::string error) {
            std::cerr << error << '\n'; // "Operation failed."
        }).fail([]() {
            std::cerr << "Failed!\n";
        });

    }

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

        // The following code sets up an asynchronous task that runs up to n times.

        auto task = [&ioContext, n](int i) {

            if (i < n) {

                // Each iteration waits for 1 second before printing the current iteration number.
                auto timer = std::make_shared<boost::asio::steady_timer>(ioContext, boost::asio::chrono::seconds(1));

                return AsyncWait(*timer)
                    .then([&ioContext, i, timer]() {
                        std::cout << i << '\n';
                        return Utils::Promise::Resolve(ioContext, true, i + 1);
                    });
            }
            else {
                // End of iteration.
                return Utils::Promise::Resolve(ioContext, false, i);
            }

        };

        // Repeats the task starting with the initial value 0 and continues until the task
        // returns a promise resolved with false.
        Utils::Promise::Repeat(ioContext, task, 0)
            .then([](int x) {
                // After completing n iterations, it prints a completion message.
                std::cout << "Done with " << x << ".\n";
            }).fail([](const boost::system::error_code& ec) {
                std::cerr << ec.category().name() << " error: " << ec.message() << '\n';
            });
    }

    // Example #4:

    {
        Utils::Promise p1 = Utils::Promise::Resolve(ioContext, 10);
        Utils::Promise p2 = Utils::Promise::Resolve(ioContext, 20);

        try {

            Utils::Promise::All(ioContext, { p1, p2 })
                .then([](int x, int y) {
                    std::cout << "Done with x = " << x << ", y = " << y << '\n';
                }).fail([](int e) {
                    std::cerr << "Failed with " << e << ".\n";
                }).fail([]() {
                    std::cerr << "Failed!.";
                });

        }
        catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << '\n';
        }
    }

    // Example #5:

    {
        Utils::Promise p1 = Utils::Promise::Reject(ioContext, 10);
        Utils::Promise p2 = Utils::Promise::Resolve(ioContext, 20);

        try {

            Utils::Promise::Any(ioContext, { p1, p2 })
                .then([](int x) {
                    std::cout << "Done with " << x << ".\n";
                }).fail([](int x, int y) {
                    std::cerr << "Failed with x = " << x << ", y = " << y << '\n';
                }).fail([]() {
                    std::cerr << "Failed!.";
                });

        }
        catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << '\n';
        }
    }

    // Example #6:

    {
        Utils::Promise p1 = Utils::Promise::Reject(ioContext, 10);
        Utils::Promise p2 = Utils::Promise::Resolve(ioContext, 20);

        try {

            Utils::Promise::Race(ioContext, { p1, p2 })
                .then([](int x) {
                    std::cout << "Done with " << x << ".\n";
                }).fail([](int x) {
                    std::cerr << "Failed with " << x << ".\n";
                }).fail([]() {
                    std::cerr << "Failed!.";
                });

        }
        catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << '\n';
        }
    }

    // Example #7:

    {
        Utils::Promise::Try(ioContext, Utils::Promise::Resolve(ioContext, 10, 20))
            .then([](Utils::Promise p) {
                if (p.state() == Utils::Promise::State::Fulfilled) {

                    int x = std::any_cast<int>(p.result()[0]);
                    int y = std::any_cast<int>(p.result()[1]);

                    return Utils::Promise::Resolve(p.getExecutor(), x, y);
                }
                else {
                    return Utils::Promise::Reject(p.getExecutor(), p.result());
                }
            }).then([](int x, int y) {
                std::cout << "x = " << x << ", y = " << y << '\n';
            }).fail([](int ec) {
                std::cerr << "Failed with " << ec << '\n';
            });
    } */

    // Example #8:

    {
        Utils::Promise p1 = Utils::Promise::Resolve(ioContext, 10, std::string{ "Hello!" });
        Utils::Promise p2 = Utils::Promise::Reject(ioContext, 123);

        Utils::Promise p3 = Utils::Promise::AllSettled(ioContext, { p1, p2 })
            .then([](Utils::Promise p1, Utils::Promise p2) {

                std::cout << "Promise 1: state=" << static_cast<int>(p1.state()) << " ";
                std::cout << "result=" << p1.result().size() << "\n";

                if (p1.result(0).type() == typeid(int) && p1.result(1).type() == typeid(std::string)) {

                    int n = p1.result<int>(0);
                    std::string str = p1.result<std::string>(1);

                    std::cout << n << " " << str << "\n";
                }

                std::cout << "Promise 2: state=" << static_cast<int>(p2.state()) << " ";
                std::cout << "result=" << p2.result().size() << "\n";

                if (p2.result(0).type() == typeid(int)) {
                    int ec = p2.result<int>(0);
                    std::cout << ec << '\n';
                }

                return true;
            }).fail([](std::exception e) {
                std::cerr << "error: " << e.what() << '\n';
            }).fail([]() {
                std::cerr << "Failed!\n";
            });

        p3.then([](bool success) {
            std::cout << "success: " << success << '\n';
        });
    }

    ioContext.run();

    return 0;
}
