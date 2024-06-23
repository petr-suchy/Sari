# AnyFunction

`Utils::MakeAnyFunc` is a utility function that can wrap any function into `Utils::AnyFunction` object, enabling type erasure and polymorphic behavior. This is particularly useful when you want to store functions with different types of arguments in the same container.

Example #1:
```cpp
auto add = [](int a, int b) {
    return a + b;
};

Utils::AnyFunction f = Utils::MakeAnyFunc(add);

std::vector<std::any> args{ 2, 3 };
std::any result = f(args);
std::cout << "Result: " << std::any_cast<int>(result) << '\n';
```
Example #2:
```cpp
#include <iostream>
#include "AnyFunction.h"
#include <map>

constexpr const char* ClickEvent = "onClick";
constexpr const char* ChangeEvent = "onChange";

class EventTarget {
public:

    template<typename F>
    void addEventListener(const std::string& eventType, F eventHandler)
    {
        eventHandlers_[eventType] = Utils::MakeAnyFunc(eventHandler);
    }

    template<typename... Args>
    void invokeEvent(const std::string& eventType, Args... args)
    {
        auto it = eventHandlers_.find(eventType);
        
        if (it != eventHandlers_.end()) {
            std::vector<std::any> vargs = {args...};
            Utils::AnyFunction eventHandler = it->second;
            eventHandler(vargs);
        }
    }

private:

    std::map<std::string, Utils::AnyFunction> eventHandlers_;

};

class Button : public EventTarget {
public:

    Button(const std::string labelText) :
        labelText_(labelText)
    {}

    void click()
    {
        invokeEvent(ClickEvent);
    }

    void setLabelText(const std::string str)
    {
        labelText_ = str;
        invokeEvent(ChangeEvent, labelText_);
    }

private:

    std::string labelText_;

};

int main()
{
    try {

        Button button("some label text");

        button.addEventListener(ClickEvent, []() {
            std::cout << "Button clicked!\n";
        });

        button.addEventListener(ChangeEvent, [](const std::string& labelText) {
            std::cout << "Button text changed to '" << labelText << "'\n";
        });

        button.click();
        button.setLabelText("some other text");

    }
    catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
    }

    return 0;
}
```
