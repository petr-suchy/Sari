#include <iostream>
#include "sari/string/split.h"
#include "sari/string/trim.h"

using namespace Sari;

int main()
{
    const std::string s = "   Cooke:         foo=bar&cow=dick     ";

    auto parts = String::Split(s, ':');

    auto it = parts.begin();

    std::cout << "name: " << String::Trim(it.curr()).str() << '\n';

    auto parts2 = String::Split(String::Trim(it.rest()), '&');

    for (const auto& part: parts2) {
        std::cout << "'" << part << "'\n";
    }

    return 0;
}
