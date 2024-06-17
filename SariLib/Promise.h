#pragma once

#include <memory>
#include <list>
#include <boost/asio.hpp>
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
            Utils::VariadicFunction resolve{[impl = impl_](const std::vector<std::any>& vargs) {
                impl->resolve(vargs);
                return std::any{};
            }};

            Utils::VariadicFunction reject{[impl = impl_](const std::vector<std::any>& vargs) {
                impl->reject(vargs);
                return std::any{};
            }};

            service.post([executor, resolve, reject]() {
                executor(resolve, reject);
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

        template<typename F>
        Promise& then(F resolveHandler)
        {
            impl_->resolveHandlers_.push_back(
                Utils::MakeAnyFunc(resolveHandler)
            );

            return *this;
        }

        static Promise Resolve(boost::asio::io_service& service, const std::vector<std::any>& vargs)
        {
            return Promise(
                service,
                [vargs](Utils::VariadicFunction resolve, Utils::VariadicFunction reject)
                {
                    resolve(vargs);
                }
            );
        }

        template<typename... Args>
        static Promise Resolve(boost::asio::io_service& service, Args... args)
        {
            std::vector<std::any> vargs = { args... };
            return Resolve(service, vargs);
        }

        static Promise Reject(boost::asio::io_service& service, const std::vector<std::any>& vargs)
        {
            return Promise(
                service,
                [vargs](Utils::VariadicFunction resolve, Utils::VariadicFunction reject)
                {
                    reject(vargs);
                }
            );
        }

        template<typename... Args>
        static Promise Reject(boost::asio::io_service& service, Args... args)
        {
            std::vector<std::any> vargs = { args... };
            return Reject(service, vargs);
        }

    private:

        struct Impl : public std::enable_shared_from_this<Impl> {

            boost::asio::io_service& service_;
            State state_ = State::Pending;
            std::vector<std::any> result_;
            std::list<Utils::AnyFunction> resolveHandlers_;

            Impl(boost::asio::io_service& service) :
                service_(service)
            {}

            void resolve(const std::vector<std::any>& vargs)
            {
                if (state_ != State::Pending) {
                    return;
                }
            
                if (resolveHandlers_.empty()) {
                    result_ = vargs;
                    state_ = State::Fulfilled;
                    return;
                }

                Utils::AnyFunction resolveHandler = resolveHandlers_.front();
                resolveHandlers_.pop_front();
                std::any result = resolveHandler(vargs);
            
                Promise promise;
            
                if (result.has_value()) {
                    if (result.type() == typeid(Promise)) {
                        promise = std::any_cast<Promise>(result);
                        switch (promise.state()) {
                            case State::Fulfilled:
                                promise = Promise::Resolve(service_, promise.result());
                            break;
                            case State::Rejected:
                                promise = Promise::Reject(service_, promise.result());
                            break;
                        }
                    }
                    else {
                        std::vector<std::any> vargs2 = { result };
                        if (resolveHandlers_.empty()) {
                            result_ = vargs2;
                            state_ = State::Fulfilled;
                        }
                        else {
                            promise = Promise::Resolve(service_, vargs2);
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

            }

        };

        std::shared_ptr<Impl> impl_;

    };

}}
