#include <iostream>
#include "sari/utils/exchanger.h"

namespace asio = boost::asio;

int main()
{
    asio::io_context ioContext;
    Sari::Utils::Exchanger exchanger;

    Sari::Utils::Exchanger::Transaction trans1 = Sari::Utils::Exchanger::Transaction(ioContext.get_executor());

    int goods = 100;

    Sari::Utils::Promise selling = exchanger.asyncProduce(trans1, goods)
        .then([goods](int price) {
            std::cout << "Alice sold " << goods << " for " << price << "$.\n";
        }).fail([](boost::system::error_code ec) {
            if (ec == make_error_code(boost::system::errc::operation_canceled)) {
                std::cerr << "Selling canceled!\n";
            }
            else {
                std::cerr << "Selling failed!\n";
            }
        });

        

    int price = 10;

    Sari::Utils::Exchanger::Transaction trans2 = Sari::Utils::Exchanger::Transaction(ioContext.get_executor());

    Sari::Utils::Promise buying = exchanger.asyncConsume(trans2, price)
        .then([price](int goods) {
            std::cout << "Bob bought " << goods << " for " << price << "$.\n";
        }).fail([](boost::system::error_code ec) {
            if (ec == make_error_code(boost::system::errc::operation_canceled)) {
                std::cerr << "Buying canceled!\n";
            }
            else {
                std::cerr << "Buying failed!\n";
            }
        });

    Sari::Utils::Promise::All(ioContext, { selling, buying })
        .then([]() {
            std::cout << "Business done!\n";
        });

    ioContext.run();

    return 0;
}
