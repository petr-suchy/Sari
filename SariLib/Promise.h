#pragma once

#include <typeinfo>
#include <typeindex>
#include <memory>
#include <list>
#include <map>
#include <boost/asio.hpp>
#include "FunctionSignature.h"
#include "AnyFunction.h"

namespace Sari { namespace Utils {

    class Promise {
    public:

        enum class State {
            Pending, Fulfilled, Rejected
        };

        using Executor = std::function<void(Utils::VariadicFunction, Utils::VariadicFunction)>;

        Promise() :
            impl_(nullptr)
        {}

        Promise(boost::asio::io_service& service, const Executor& executor) :
            impl_(std::make_shared<Impl>(service))
        {
            service.post([executor, impl = impl_]() {

                Utils::VariadicFunction resolve{
                    [impl](const std::vector<std::any>& vargs) {
                        impl->resolve(vargs);
                        return std::any{};
                    }
                };

                Utils::VariadicFunction reject{
                    [impl](const std::vector<std::any>& vargs) {
                        impl->reject(vargs);
                        return std::any{};
                    }
                };

                try {
                    executor(resolve, reject);
                }
                catch (const std::exception& e) {

                    impl->state_ = State::Rejected;

                    if (impl->parent_) {
                        impl->parent_->reject(std::vector<std::any>{e});
                    }
                }
                catch (...) {

                    impl->state_ = State::Rejected;

                    if (impl->parent_) {
                        impl->parent_->reject(std::vector<std::any>{});
                    }
                }
            });
        }

        bool isNull() const
        {
            return impl_ == nullptr;
        }

        boost::asio::io_service& service()
        {
            return impl_->service_;
        }

        State state() const
        {
            return impl_->state_;
        }

        const std::vector<std::any>& result() const
        {
            return impl_->result_;
        }

        template<typename Handler>
        Promise& then(Handler resolveHandler)
        {
            impl_->resolveHandlers_.push_back(
                Utils::MakeAnyFunc(resolveHandler)
            );

            return *this;
        }

        template<typename Handler>
        Promise& fail(Handler failHandler)
        {
            using FuncSign = FunctionSignature<decltype(std::function{failHandler})> ;

            impl_->failHandlers_[std::type_index(typeid(FuncSign::Params::Type))] = Utils::MakeAnyFunc(failHandler);

            return *this;
        }

        template<typename... Args>
        static Promise Resolve(boost::asio::io_service& service, Args... args)
        {
            std::vector<std::any> vargs = { args... };

            return Promise(
                service,
                [vargs](Utils::VariadicFunction resolve, Utils::VariadicFunction reject)
                {
                    resolve(vargs);
                }
            );
        }

        template<typename... Args>
        static Promise Reject(boost::asio::io_service& service, Args... args)
        {
            std::vector<std::any> vargs = { args... };

            return Promise(
                service,
                [vargs](Utils::VariadicFunction resolve, Utils::VariadicFunction reject)
                {
                    reject(vargs);
                }
            );
        }

    private:

        struct Impl : public std::enable_shared_from_this<Impl> {

            boost::asio::io_service& service_;
            std::shared_ptr<Impl> parent_;
            State state_ = State::Pending;
            std::vector<std::any> result_;
            std::list<Utils::AnyFunction> resolveHandlers_;
            std::unordered_map<std::type_index, Utils::AnyFunction> failHandlers_;

            Impl(boost::asio::io_service& service) :
                service_(service)
            {}

            void resolve(const std::vector<std::any>& vargs)
            {
                if (state_ != State::Pending) {
                    return;
                }
            
                // If there is no other resolve handler in the queue, the promise is fulfilled
                // and the arguments are available through public function result().
                if (resolveHandlers_.empty()) {
                    result_ = vargs;
                    state_ = State::Fulfilled;
                    return;
                }

                // Get the next resolve handler from the queue.
                Utils::AnyFunction resolveHandler = resolveHandlers_.front();
                resolveHandlers_.pop_front();

                std::any result = resolveHandler(vargs);

                Promise promise;
            
                if (result.has_value()) {
                    if (result.type() == typeid(Promise)) {
                        promise = std::any_cast<Promise>(result);
                    }
                    else {
                        if (resolveHandlers_.empty()) {
                            result_ = std::vector<std::any>{ result };
                            state_ = State::Fulfilled;
                        }
                        else {
                            promise = Promise::Resolve(service_, result);
                        }
                    }
                }
                else {
                    if (resolveHandlers_.empty()) {
                        state_ = State::Fulfilled;
                    }
                    else {
                        promise = Promise::Resolve(service_);
                    }
                }

                if (!promise.isNull()) {

                    if (promise.state() != State::Pending) {
                        throw std::runtime_error("a pending promise is expected");
                    }

                    promise.impl_->parent_ = shared_from_this();

                    promise.impl_->resolveHandlers_.push_back(
                        [self = shared_from_this()](const std::vector<std::any>& vargs) {
                            self->resolve(vargs);
                            return std::any{};
                        }
                    );
                }

            }

            void reject(const std::vector<std::any>& vargs)
            {
                if (state_ != State::Pending) {
                    return;
                }

                resolveHandlers_.erase(resolveHandlers_.begin(), resolveHandlers_.end());

                bool handlerFound = false;
                std::any result;

                if (vargs.size() > 0) {

                    auto it = failHandlers_.find(std::type_index(vargs[0].type()));
                    
                    if (it != failHandlers_.end()) {
                        handlerFound = true;
                        
                        result = it->second(vargs);
                    }
                    else {

                        auto it2 = failHandlers_.find(std::type_index(typeid(void)));
                        
                        if (it2 != failHandlers_.end()) {
                            handlerFound = true;
                            result = it2->second(std::vector<std::any>{});
                        }
                    }
                }
                else {

                    auto it = failHandlers_.find(std::type_index(typeid(void)));

                    if (it != failHandlers_.end()) {
                        handlerFound = true;
                        result = it->second(std::vector<std::any>{});
                    }
                }

                if (handlerFound) {

                    state_ = State::Fulfilled;

                    if (parent_) {
                        if (result.has_value()) {
                            parent_->resolve(std::vector<std::any>{result});
                        }
                        else {
                            parent_->resolve(std::vector<std::any>{});
                        }
                    }
                    else {
                        if (result.has_value()) {
                            result_ = std::vector<std::any>{result};
                        }
                    }
                }
                else {

                    state_ = State::Rejected;

                    if (parent_) {
                        parent_->reject(vargs);
                    }
                    else {
                        result_ = vargs;
                    }
                }
            }

        };

        std::shared_ptr<Impl> impl_;

    };

}}
