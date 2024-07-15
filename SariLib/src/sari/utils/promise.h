#pragma once

#include <typeinfo>
#include <typeindex>
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

        bool isNull() const
        {
            return impl_ == nullptr;
        }

        boost::asio::any_io_executor getExecutor()
        {
            return impl_->ioExecutor_;
        }

        State state() const
        {
            return impl_->state();
        }

        const std::vector<std::any>& result() const
        {
            return impl_->result_;
        }

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

        template<typename Handler>
        Promise& fail(Handler failHandler)
        {
            using FuncSign = FunctionSignature<decltype(std::function{failHandler})> ;

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

        Promise finalize(FinalizeHandler finalizeHandler)
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

        template<typename... Args>
        static Promise Resolve(boost::asio::io_context& ioContext, Args&&... args)
        {
            return Resolve(ioContext.get_executor(), std::forward<Args>(args)...);
        }

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

    private:

        struct Impl : public std::enable_shared_from_this<Impl> {

            boost::asio::any_io_executor ioExecutor_;
            std::shared_ptr<Impl> parent_;
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

                Promise promise;
            
                if (result.has_value()) {
                    if (result.type() == typeid(Promise)) {
                        promise = std::any_cast<Promise>(result);
                    }
                    else {
                        if (resolveHandlers_.empty()) {
                            state(State::Fulfilled, std::vector<std::any>{ result });
                        }
                        else {
                            promise = Promise::Resolve(ioExecutor_, result);
                        }
                    }
                }
                else {
                    if (resolveHandlers_.empty()) {
                        state(State::Fulfilled);
                    }
                    else {
                        promise = Promise::Resolve(ioExecutor_);
                    }
                }

                if (!promise.isNull()) {

                    if (promise.state() != State::Pending) {
                        throw std::runtime_error("a pending promise is expected");
                    }

                    promise.impl_->parent_ = shared_from_this();

                    promise.impl_->finalizeHandlers_.push_back(
                        [parent = promise.impl_->parent_](Promise promise) {
                            if (promise.state() == State::Fulfilled) {
                                parent->resolve(promise.result());
                            }
                        }
                    );
                }

            }

            void reject(const std::vector<std::any>& vargs = std::vector<std::any>{})
            {
                if (state() != State::Pending) {
                    return;
                }

                resolveHandlers_.erase(resolveHandlers_.begin(), resolveHandlers_.end());

                AnyFunction failHandler;
                bool isCatchAllHandler = false;
                
                if (vargs.size() > 0) {

                    auto it = failHandlers_.find(std::type_index(vargs[0].type()));
                    
                    if (it != failHandlers_.end()) {
                        failHandler = it->second;
                    }
                    else {

                        auto it2 = failHandlers_.find(std::type_index(typeid(void)));
                        
                        if (it2 != failHandlers_.end()) {
                            failHandler = it2->second;
                            isCatchAllHandler = true;
                        }
                    }
                }
                else {

                    auto it = failHandlers_.find(std::type_index(typeid(void)));

                    if (it != failHandlers_.end()) {
                        failHandler = it->second;
                        isCatchAllHandler = true;
                    }
                }

                if (failHandler) {
                    try {

                        std::any result;

                        if (isCatchAllHandler) {
                            result = failHandler(std::vector<std::any>{});
                        }
                        else {
                            result = failHandler(vargs);
                        }

                        if (result.has_value()) {
                            state(State::Fulfilled, std::vector<std::any>{ result });
                        }
                        else {
                            state(State::Fulfilled);
                        }

                        if (parent_) {
                            if (result.has_value()) {
                                parent_->resolve(std::vector<std::any>{result});
                            }
                            else {
                                parent_->resolve();
                            }
                        }
                    }
                    catch (const std::exception& e) {

                        state(State::Rejected, std::vector<std::any>{ e });

                        if (parent_) {
                            parent_->reject(std::vector<std::any>{e});
                        }
                    }
                    catch (...) {

                        state(State::Rejected);

                        if (parent_) {
                            parent_->reject();
                        }
                    }

                }
                else {

                    state(State::Rejected, vargs);

                    if (parent_) {
                        parent_->reject(vargs);
                    }
                }
            }

        };

        std::shared_ptr<Impl> impl_;

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
