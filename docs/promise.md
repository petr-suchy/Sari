# Promise

A Promise is an object that represents the eventual completion (or failure) of an asynchronous operation and its resulting value. Promises provide a way to handle asynchronous operations in a more manageable and readable manner compared to traditional callback-based approaches.

Here's a breakdown of the key concepts and components of a Promise:

## 1. States of a Promise

A Promise can be in one of three states:

- `Promise::State::Pending`: The initial state, neither fulfilled nor rejected.
- `Promise::State::Fulfilled`: The operation completed successfully.
- `Promise::State::Rejected`: The operation failed.

The current state of a promise object is available through its `state()` method.

## 2. Creating a Promise

A Promise is created using the Promise constructor, which takes an execution context and a function called the executor. The executor function has two parameters: `resolve` and `reject`. These are callbacks used to change the state of the Promise.

```cpp
namespace asio = boost::asio;
namespace Utils = Sari::Utils;

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
```
## 3. Consuming a Promise

To handle the results of a Promise, you use the `then` and `fail` methods.

```cpp
promise.then([](std::string value) {
    std::cout << value << '\n'; // "Operation was successful!"
}).fail([](std::string error) {
    std::cerr << error << '\n'; // "Operation failed."
});

ioContext.run();
```

## 4. Chaining Promises

Promises can be chained to handle sequences of asynchronous operations. Each method `then` returns a reference to the Promise object itself, allowing for the chaining of further `then` or `fail` calls.

```cpp
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
    std::cerr << error << '\n';
});

ioContext.run();
```