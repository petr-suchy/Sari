#include <iostream>
#include "sari/utils/exchanger.h"

namespace asio = boost::asio;

int main()
{
    asio::io_context ioContext;
    Sari::Utils::Exchanger exchanger;

    Sari::Utils::Exchanger::Transaction trans1 = Sari::Utils::Exchanger::CreateTransaction(ioContext);

    int goods = 100;

    Sari::Utils::Promise selling = exchanger.asyncPrtoduce(trans1, goods)
        .then([goods](int price) {
            std::cout << "Alice sold " << goods << " for " << price << "$.\n";
        }).fail([]() {
            std::cerr << "Selling failed!\n";
        });

    int price = 10;

    Sari::Utils::Exchanger::Transaction trans2 = Sari::Utils::Exchanger::CreateTransaction(ioContext);

    Sari::Utils::Promise buying = exchanger.asyncConsume(trans2, price)
        .then([price](int goods) {
            std::cout << "Bob bought " << goods << " for " << price << "$.\n";
        }).fail([]() {
            std::cerr << "Buying failed!\n";
        });

    Sari::Utils::Promise::All(ioContext, { selling, buying })
        .then([]() {
            std::cout << "Business done!\n";
        });

    ioContext.run();

    return 0;
}
