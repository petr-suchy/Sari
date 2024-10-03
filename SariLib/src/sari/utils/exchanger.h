#pragma once

#include "sari/utils/promise.h"
#include "sari/utils/dlinked_list.h"

namespace Sari { namespace Utils {

    class Exchanger {
    public:

        class ExchangeHandler {

        public:

            ExchangeHandler(Sari::Utils::AnyFunction resolve, Sari::Utils::AnyFunction reject, const std::vector<std::any>& vargs) :
                resolve_(resolve),
                reject_(reject),
                vargs_(vargs)
            {}

            ~ExchangeHandler()
            {
                if (isPending()) {
                    reject_(make_error_code(boost::system::errc::operation_canceled));
                }
            }

            bool isPending() {
                return isPending_;
            }

            void resolve(const std::vector<std::any>& vargs)
            {
                resolve_(vargs);
                isPending_ = false;
            }

            void reject(const std::vector<std::any>& vargs)
            {
                reject_(vargs);
                isPending_ = false;
            }

            const std::vector<std::any>& vargs() const
            {
                return vargs_;
            }

        private:

            bool isPending_ = true;
            Sari::Utils::AnyFunction resolve_;
            Sari::Utils::AnyFunction reject_;
            std::vector<std::any> vargs_;

        };

        using ExchangeHandlerList = DLinkedList<std::unique_ptr<ExchangeHandler>>;

        class Transaction {
        friend class Exchanger;
        public:

            boost::asio::any_io_executor getExecutor() const { return ioExecutor_; }
            bool isPending() const { return handlerElement_ != nullptr && handlerElement_->isStored(); }
            void cancel() { handlerElement_ = nullptr; }

            Transaction(boost::asio::any_io_executor ioExecutor) :
                ioExecutor_(ioExecutor)
            {}

            Transaction(boost::asio::io_context& ioContext) :
                ioExecutor_(ioContext.get_executor())
            {}

        private:

            boost::asio::any_io_executor ioExecutor_;
            std::unique_ptr<typename ExchangeHandlerList::Element> handlerElement_;

        };

        template<typename... Args>
        Sari::Utils::Promise asyncConsume(Transaction& trans, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };
            return exchange(trans, vargs, consumeHandlers_, produceHandlers_);
        }

        template<typename... Args>
        Sari::Utils::Promise asyncProduce(Transaction& trans, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };
            return exchange(trans, vargs, produceHandlers_, consumeHandlers_);
        }

        Exchanger() :
            consumeHandlers_([](ExchangeHandlerList::Element* e) {
                e->data()->reject(
                    // Cancel any pending request when Exchanger is destroyed.
                    std::vector<std::any>{make_error_code(boost::system::errc::operation_canceled)}
                );
            }),
            produceHandlers_([](ExchangeHandlerList::Element* e) {
                e->data()->reject(
                    // Cancel any pending request when Exchanger is destroyed.
                    std::vector<std::any>{make_error_code(boost::system::errc::operation_canceled)}
                );
            })
        {}

    private:

        ExchangeHandlerList consumeHandlers_;
        ExchangeHandlerList produceHandlers_;

        Sari::Utils::Promise exchange(
            Transaction& trans, const std::vector<std::any>& vargs,
            ExchangeHandlerList& handlers, ExchangeHandlerList& counterpartHandlers
        ) {
		    return Sari::Utils::Promise(
                trans.getExecutor(),
			    [&](Sari::Utils::AnyFunction resolve, Sari::Utils::AnyFunction reject) {

                    typename ExchangeHandlerList::Element* counterpartHandlerElement = counterpartHandlers.first();

                    if (counterpartHandlerElement) {
                        std::unique_ptr<ExchangeHandler> counterpartHandler = std::move(counterpartHandlerElement->data());
                        counterpartHandlers.remove(counterpartHandlerElement);
                        counterpartHandler->resolve(vargs);
                        resolve(counterpartHandler->vargs());
                    }
                    else {
                        auto exchangeHandler = std::make_unique<ExchangeHandler>(resolve, reject, vargs);
                        typename ExchangeHandlerList::Element* newHandlerElement = new typename ExchangeHandlerList::Element(std::move(exchangeHandler));
                        trans.handlerElement_.reset(newHandlerElement);
                        handlers.append(newHandlerElement);
                    }
			    }
		    );
        }

    };

}}