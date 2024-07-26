#include <iostream>
#include "sari/utils/trader.h"

namespace asio = boost::asio;

int main()
{
    asio::io_context ioContext;
    Sari::Utils::Trader trader;

    Sari::Utils::Trader::Transaction trans1 = Sari::Utils::Trader::CreateTransaction(ioContext);

    int goods = 100;

    Sari::Utils::Promise selling = trader.asyncSell(trans1, goods)
        .then([goods](int price) {
            std::cout << "Alice sold " << goods << " for " << price << "$.\n";
        }).fail([]() {
            std::cerr << "Selling failed!\n";
        });

    int price = 10;

    Sari::Utils::Trader::Transaction trans2 = Sari::Utils::Trader::CreateTransaction(ioContext);

    Sari::Utils::Promise buying = trader.asyncBuy(trans2, price)
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
