#pragma once

#include <typeinfo>
#include <typeindex>
#include <type_traits>
#include <memory>
#include <list>
#include <map>
#include <boost/asio.hpp>

#include "function_signature.h"
#include "any_function.h"

namespace Sari { namespace Utils {

    class Promise {
    public:

        enum class State {
            Pending, Fulfilled, Rejected
        };

        using Executor = std::function<void(VariadicFunction, VariadicFunction)>;

        struct AsyncAttr{};
        static constexpr AsyncAttr Async{};

        using FinalizeHandler = std::function<void(Promise)>;

        // Constructors.

        Promise() :
            impl_(nullptr)
        {}

        Promise(boost::asio::io_context& ioContext, const Executor& executor) :
            impl_(std::make_shared<Impl>(ioContext.get_executor()))
        {
            impl_->launchExecutor(executor);
        }

        Promise(boost::asio::io_context& ioContext, const Executor& executor, AsyncAttr) :
            impl_(std::make_shared<Impl>(ioContext.get_executor()))
        {
            impl_->launchExecutor(executor, Async);
        }
        
        Promise(boost::asio::any_io_executor ioExecutor, const Executor& executor) :
            impl_(std::make_shared<Impl>(ioExecutor))
        {
            impl_->launchExecutor(executor);
        }

        Promise(boost::asio::any_io_executor ioExecutor, const Executor& executor, AsyncAttr) :
            impl_(std::make_shared<Impl>(ioExecutor))
        {
            impl_->launchExecutor(executor, Async);
        }

        // Returns true if the Promise object is null.
        bool isNull() const
        {
            return impl_ == nullptr;
        }

        // Returns the execution context.
        boost::asio::any_io_executor getExecutor() const
        {
            return impl_->ioExecutor_;
        }

        // Returns the current state of the Promise object.
        State state() const
        {
            return impl_->state();
        }

        // Returns true if the Promise is pending.
        bool isPending() const
        {
            return state() == State::Pending;
        }

        // Returns true if the Promise has been fulfilled.
        bool isFulfilled() const
        {
            return state() == State::Fulfilled;
        }

        // Returns true if the Promise has been rejected.
        bool isRejected() const
        {
            return state() == State::Rejected;
        }

        // Returns a constant reference to the vector of results.
        const std::vector<std::any>& result() const
        {
            return impl_->result_;
        }

        // Returns the result at the specified index as std::any type.
        // Throws std::out_of_range if the index is out of bounds.
        std::any result(std::size_t index) const
        {
            if (index >= impl_->result_.size()) {
                throw std::out_of_range("result index is out of range");
            }

            return impl_->result_[index];
        }

        // Returns the result at the specified index, cast to the specified type T.
        // Throws std::out_of_range if the index is out of bounds.
        // Throws std::bad_any_cast if the cast to T is not possible.
        template<typename T>
        T result(std::size_t index) const
        {
            return std::any_cast<T>(result(index));
        }

        // It schedules a function to be called when the promise is fulfilled.
        // It returns Promise object itself, allowing you to chain calls to other promise methods.
        template<typename Handler>
        Promise& then(Handler resolveHandler)
        {
            switch (state()) {
                case State::Fulfilled:
                    throw std::logic_error("attempt to add a resolve handler for fulfilled promise");
                break;
                case State::Rejected:
                    throw std::logic_error("attempt to add a resolve handler for rejected promise");
                break;
            }

            impl_->resolveHandlers_.push_back(
                MakeAnyFunc(resolveHandler)
            );

            return *this;
        }

        // It schedules a function to be called when the promise is rejected.
        // It returns Promise object itself, allowing you to chain calls to other promise methods.
        template<typename Handler>
        Promise& fail(Handler failHandler) 
        {
            using FuncSign = FunctionSignature<decltype(std::function{failHandler})>;

            static_assert(
                std::is_same_v<typename FuncSign::Result, void>,
                "Promise::fail() only accepts functions with no return type"
            );

            switch (state()) {
                case State::Fulfilled:
                    throw std::logic_error("attempt to add a fail handler for fulfilled promise");
                break;
                case State::Rejected:
                    throw std::logic_error("attempt to add a fail handler for rejected promise");
                break;
            }

            impl_->failHandlers_[std::type_index(typeid(FuncSign::Params::Type))] = MakeAnyFunc(failHandler);

            return *this;
        }

        // It schedules a function to be called when the promise is settled (either fulfilled or rejected).
        // It returns Promise object itself, allowing you to chain calls to other promise methods.
        Promise finalize(FinalizeHandler finalizeHandler) const
        {
            switch (state()) {
                case State::Fulfilled:
                    throw std::logic_error("attempt to add a finalize handler for fulfilled promise");
                break;
                case State::Rejected:
                    throw std::logic_error("attempt to add a finalize handler for rejected promise");
                break;
            }

            impl_->finalizeHandlers_.push_back(finalizeHandler);

            return *this;
        }

        // It takes one or more values and returns a promise that will be fulfilled with that values(s).
        template<typename... Args>
        static Promise Resolve(boost::asio::any_io_executor ioExecutor, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };

            return Promise(
                ioExecutor,
                [vargs](VariadicFunction resolve, VariadicFunction reject) {
                    resolve(vargs);
                }
            );
        }

        // It takes one or more values and returns a promise that will be fulfilled with that values(s).
        template<typename... Args>
        static Promise Resolve(boost::asio::io_context& ioContext, Args&&... args)
        {
            return Resolve(ioContext.get_executor(), std::forward<Args>(args)...);
        }

        // It takes one or more values and returns a promise that will be rejected with that values(s).
        template<typename... Args>
        static Promise Reject(boost::asio::any_io_executor ioExecutor, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };

            return Promise(
                ioExecutor,
                [vargs](VariadicFunction resolve, VariadicFunction reject) {
                    reject(vargs);
                }
            );
        }

        // It takes one or more values and returns a promise that will be rejected with that values(s).
        template<typename... Args>
        static Promise Reject(boost::asio::io_context& ioContext, Args&&... args)
        {
            return Reject(ioContext.get_executor(), std::forward<Args>(args)...);
        }

        template<typename F, typename... Args>
        static Promise Repeat(boost::asio::any_io_executor ioExecutor, F task, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };

            return Promise(
                ioExecutor,
                [ioExecutor, task, vargs](VariadicFunction resolve, VariadicFunction reject) {
                    Repeat_(ioExecutor, MakeAnyFunc(task), vargs, resolve, reject);
                },
                Promise::Async
            );
        }

        template<typename F, typename... Args>
        static Promise Repeat(boost::asio::io_context& ioContext, F task, Args&&... args)
        {
            return Repeat(ioContext.get_executor(), task, std::forward<Args>(args)...);
        }

        // It takes a vector of promises as input and returns a single Promise. This returned promise fulfills
        // when all of the input's promises fulfill (including when an empty vector is passed), with fulfillment values.
        // It rejects when any of the input's promises rejects, with this first rejection reason.
        static Promise All(boost::asio::any_io_executor ioExecutor, const std::vector<Promise>& promises)
        {
            if (promises.empty()) {
                return Promise::Resolve(ioExecutor);
            }

            auto group = std::make_shared<Group>(promises.size());

            Promise resultingPromise = Promise(
                ioExecutor,
                [group](VariadicFunction resolve, VariadicFunction reject) {
                    group->resolve = resolve;
                    group->reject = reject;
                },
                Async
            );

            for (auto& promise : promises) {
                promise.finalize([group](Promise promise) {

                    if (promise.state() == State::Fulfilled) {
                        // Append the result of the promise to the result of the group.
                        group->result.insert(
                            group->result.end(),
                            promise.result().begin(), promise.result().end()
                        );

                        if (--group->counter == 0) {
                            // All promises fulfilled.
                            group->resolve(group->result);
                        }
                    }
                    else { // Rejected
                        group->reject(promise.result());
                    }

                });
            }

            return resultingPromise;
        }

        // It takes a vector of promises as input and returns a single Promise. This returned promise fulfills
        // when all of the input's promises fulfill (including when an empty vector is passed), with  fulfillment values.
        // It rejects when any of the input's promises rejects, with this first rejection reason.
        static Promise All(boost::asio::io_context& ioContext, const std::vector<Promise>& promises)
        {
            return All(ioContext.get_executor(), promises);
        }

        // It takes a vector of promises as input and returns a single Promise.
        // This returned promise fulfills when any of the input's promises fulfills, with this first fulfillment
        // value. It rejects when all of the input's promises reject (including when an empty vector is passed),
        // with rejection reasons.
        static Promise Any(boost::asio::any_io_executor ioExecutor, const std::vector<Promise>& promises)
        {
                if (promises.empty()) {
                    return Promise::Reject(ioExecutor);
                }

                auto group = std::make_shared<Group>(promises.size());

                Promise resultingPromise = Promise(
                    ioExecutor,
                    [group](VariadicFunction resolve, VariadicFunction reject) {
                        group->resolve = resolve;
                        group->reject = reject;
                    },
                    Async
                );

                for (auto& promise : promises) {
                    promise.finalize([group](Promise promise) {

                        if (promise.state() == State::Rejected) {
                            // Append the result of the promise to the result of the group.
                            group->result.insert(
                                group->result.end(),
                                promise.result().begin(), promise.result().end()
                            );

                            if (--group->counter == 0) {
                                // All promises rejected.
                                group->reject(group->result);
                            }
                        }
                        else { // Fulfilled
                            group->resolve(promise.result());
                        }

                    });
                }

                return resultingPromise;
        }

        // It takes a vector of promises as input and returns a single Promise.
        // This returned promise fulfills when any of the input's promises fulfills, with this first fulfillment
        // value. It rejects when all of the input's promises reject (including when an empty vector is passed),
        // with rejection reasons.
        static Promise Any(boost::asio::io_context& ioContext, const std::vector<Promise>& promises)
        {
            return Any(ioContext.get_executor(), promises);
        }

        // It takes a vector of promises as input and returns a single Promise. This returned promise settles
        // (is either fulfilled or rejected), with the eventual state of the first promise that settles.
        // The returned promise remains pending forever if the passed vector is empty. 
        static Promise Race(boost::asio::any_io_executor ioExecutor, const std::vector<Promise>& promises)
        {
                auto group = std::make_shared<Group>(promises.size());

                Promise resultingPromise = Promise(
                    ioExecutor,
                    [group](VariadicFunction resolve, VariadicFunction reject) {
                        group->resolve = resolve;
                        group->reject = reject;
                    },
                    Async
                );

                for (auto& promise : promises) {
                    promise.finalize([group](Promise promise) {

                        if (promise.state() == State::Fulfilled) {
                            group->resolve(promise.result());
                        }
                        else { // Rejected
                            group->reject(promise.result());
                        }

                    });
                }

                return resultingPromise;
        }

        // It takes a vector of promises as input and returns a single Promise. This returned promise settles
        // (is either fulfilled or rejected), with the eventual state of the first promise that settles.
        // The returned promise remains pending forever if the passed vector is empty. 
        static Promise Race(boost::asio::io_context& ioContext, const std::vector<Promise>& promises)
        {
            return Race(ioContext.get_executor(), promises);
        }

        // It takes a vector of promises as input and returns a single Promise. This returned promise fulfills
        // when all of the input's promises settle (including when an empty vector is passed).
        static Promise AllSettled(boost::asio::any_io_executor ioExecutor, const std::vector<Promise>& promises)
        {
            if (promises.empty()) {
                return Promise::Resolve(ioExecutor);
            }

            auto group = std::make_shared<Group>(promises.size(), promises.size());

            Promise resultingPromise = Promise(
                ioExecutor,
                [group](VariadicFunction resolve, VariadicFunction reject) {
                    group->resolve = resolve;
                    group->reject = reject;
                },
                Async
            );

            int index = 0;

            for (auto& promise : promises) {
                promise.finalize([index = index++, group](Promise promise) {

                    group->result[index] = promise;

                    if (--group->counter == 0) {
                        // All promises settled.
                        group->resolve(group->result);
                    }
                });
            }

            return resultingPromise;
        }

        // It takes a vector of promises as input and returns a single Promise. This returned promise fulfills
        // when all of the input's promises settle (including when an empty vector is passed).
        static Promise AllSettled(boost::asio::io_context& ioContext, const std::vector<Promise>& promises)
        {
            return AllSettled(ioContext.get_executor(), promises);
        }

    private:

        struct Impl : public std::enable_shared_from_this<Impl> {

            boost::asio::any_io_executor ioExecutor_;
            State state_ = State::Pending;
            std::vector<std::any> result_;
            std::list<AnyFunction> resolveHandlers_;
            std::unordered_map<std::type_index, AnyFunction> failHandlers_;
            std::list<FinalizeHandler> finalizeHandlers_;

            Impl(boost::asio::any_io_executor ioExecutor) :
                ioExecutor_(ioExecutor)
            {}

            State state() const
            {
                return state_;
            }

            State state(State newState, const std::vector<std::any>& result = {})
            {
                if (state_ != State::Pending) {
                    return state_;
                }

                state_ = newState;
                result_ = result;

                for (auto& finalizeHandler : finalizeHandlers_) {
                    finalizeHandler(Promise{shared_from_this()});
                }

                return state_;
            }

            void launchExecutor(const Executor& executor)
            {
                VariadicFunction resolve{
                    [impl = shared_from_this()] (const std::vector<std::any>& vargs) {
                        boost::asio::post(impl->ioExecutor_, [impl, vargs]() {
                            impl->resolve(vargs);
                        });
                        return std::any{};
                    }
                };

                VariadicFunction reject{
                    [impl = shared_from_this()] (const std::vector<std::any>& vargs) {
                        boost::asio::post(impl->ioExecutor_, [impl, vargs]() {
                            impl->reject(vargs);
                        });
                        return std::any{};
                    }
                };

                callExecutor(executor, resolve, reject);
            }

            void launchExecutor(const Executor& executor, AsyncAttr)
            {
                VariadicFunction resolve{
                    [impl = shared_from_this()](const std::vector<std::any>& vargs) {
                        impl->resolve(vargs);
                        return std::any{};
                    }
                };

                VariadicFunction reject{
                    [impl = shared_from_this()](const std::vector<std::any>& vargs) {
                        impl->reject(vargs);
                        return std::any{};
                    }
                };

                callExecutor(executor, resolve, reject);
            }

            void callExecutor(const Executor& executor, const VariadicFunction& resolve, const VariadicFunction& reject)
            {
                try {
                    executor(resolve, reject);
                }
                catch (const std::exception& e) {
                    boost::asio::post(ioExecutor_, [impl = shared_from_this(), e]() {
                        impl->reject(std::vector<std::any>{e});
                    });
                }
                catch (...) {
                    boost::asio::post(ioExecutor_, [impl = shared_from_this()]() {
                        impl->reject();
                    });
                }
            }

            void resolve(const std::vector<std::any>& vargs = std::vector<std::any>{})
            {
                if (state() != State::Pending) {
                    return;
                }
            
                // If there is no other resolve handler in the queue, the promise is fulfilled
                // and the arguments are available through public function result().
                if (resolveHandlers_.empty()) {
                    state(State::Fulfilled, vargs);
                    return;
                }

                // Get the next resolve handler from the queue.
                AnyFunction resolveHandler = resolveHandlers_.front();
                resolveHandlers_.pop_front();

                std::any result;

                try {
                    // Call the resolve handler.
                    result = resolveHandler(vargs);
                }
                catch (const std::exception& e) {
                    reject(std::vector<std::any>{e});
                    return;
                }
                catch (...) {
                    reject();
                    return;
                }

                if (result.type() != typeid(Promise) && resolveHandlers_.empty()) {

                    // When the result of the resolve handler call is not another promise
                    // and there is no other resolve handler in the queue, the promise is fulfilled
                    // with the result.

                    if (result.has_value()) {
                        state(State::Fulfilled, std::vector<std::any>{ result });
                    }
                    else {
                        state(State::Fulfilled);
                    }

                    return;
                }

                // Otherwise, the result will be casted or converted to a promise.

                Promise resultingPromise;

                if (result.type() == typeid(Promise)) {
                    resultingPromise = std::any_cast<Promise>(result);
                }
                else {
                    if (result.has_value()) {
                        resultingPromise = Promise::Resolve(ioExecutor_, result);
                    }
                    else {
                        resultingPromise = Promise::Resolve(ioExecutor_);
                    }
                }

                resultingPromise.finalize([parent = shared_from_this()](Promise resultingPromise) {

                    // After the promise resulting from the resolve handler call is settled,
                    // the "parent" promise is settled accordingly.

                    if (resultingPromise.state() == State::Fulfilled) {
                        parent->resolve(resultingPromise.result());
                    }
                    else { // Rejected
                        parent->reject(resultingPromise.result());
                    }
                });

            }

            void reject(const std::vector<std::any>& vargs = std::vector<std::any>{})
            {
                if (state() != State::Pending) {
                    return;
                }

                resolveHandlers_.erase(resolveHandlers_.begin(), resolveHandlers_.end());

                AnyFunction failHandler;
                bool isCatchAllHandler = false;

                if (!vargs.empty()) {
                    failHandler = findFailHandler(vargs[0].type());
                }
                
                if (!failHandler) {

                    failHandler = findGenericFailHandler();

                    if (!failHandler) {
                        state(State::Rejected, vargs);
                        return;
                    }

                    isCatchAllHandler = true;
                }

                try {

                    if (isCatchAllHandler) {
                        failHandler(std::vector<std::any>{});
                    }
                    else {
                        failHandler(vargs);
                    }

                    state(State::Rejected);
                }
                catch (const std::exception& e) {
                    state(State::Rejected, std::vector<std::any>{ e });
                }
                catch (...) {
                    state(State::Rejected);
                }
            }

            template<typename T>
            AnyFunction findFailHandler(const T& type)
            {
                AnyFunction result;

                auto it = failHandlers_.find(std::type_index(type));

                if (it != failHandlers_.end()) {
                    result = it->second;
                }

                return result;
            }

            AnyFunction findGenericFailHandler()
            {
                AnyFunction result;

                auto it = failHandlers_.find(std::type_index(typeid(void)));

                if (it != failHandlers_.end()) {
                    result = it->second;
                }

                return result;
            }

        };

        std::shared_ptr<Impl> impl_;

        struct Group {

            std::size_t counter;
            std::vector<std::any> result;
            VariadicFunction resolve;
            VariadicFunction reject;

            Group(std::size_t size, std::size_t resultSize = 0) :
                counter(size),
                result(resultSize)
            {}

        };

        Promise(std::shared_ptr<Impl> impl) :
            impl_(impl)
        {}

        static void Repeat_(
            boost::asio::any_io_executor ioExecutor,
            AnyFunction task, const std::vector<std::any>& vargs,
            VariadicFunction resolve, VariadicFunction reject
        )
        {
            std::any result = task(vargs);
            Promise promise;
    
            if (result.has_value()) {
                if (result.type() == typeid(Promise)) {
                    promise = std::any_cast<Promise>(result);
                }
                else {
                    promise = Promise::Resolve(ioExecutor, result);
                }
            }
            else {
                promise = Promise::Resolve(ioExecutor);
            }

            promise.finalize([ioExecutor, task, resolve, reject](Promise promise) {

                if (promise.state() == Promise::State::Rejected) {
                    reject(promise.result());
                    return;
                }
            
                bool cond = false;
                std::vector<std::any> result = promise.result();

                if (result.size() > 0) {

                    if (result[0].type() == typeid(bool)) {
                        cond = std::any_cast<bool>(result[0]);
                        result.erase(result.begin());
                    }
                    else if (result[0].type() == typeid(int)) {
                        cond = std::any_cast<int>(result[0]) != 0;
                        result.erase(result.begin());
                    }

                }

                if (cond) {
                    Repeat_(ioExecutor, task, result, resolve, reject);
                }
                else {
                    resolve(result);
                }

            }); 
        }

    };

}}
