#include <iostream>
#include <memory>
#include "sari/utils/dlinked_list.h"

struct Point {
    int x;
    int y;
};

int main()
{
    std::unique_ptr<Sari::Utils::DLinkedList<Point>::Element> elm;

    {
        Sari::Utils::DLinkedList<Point> dlist;

        elm = std::make_unique<Sari::Utils::DLinkedList<Point>::Element>(Point{ 10,20 });

        dlist.append(elm.get());

    }

    return 0;
}
