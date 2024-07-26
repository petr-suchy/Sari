#pragma once

#include "sari/utils/promise.h"
#include "sari/utils/dlinked_list.h"

namespace Sari { namespace Utils {

    class Exchanger {
    public:

        struct ExchangeHandler {

            Sari::Utils::VariadicFunction resolve_;
            Sari::Utils::VariadicFunction reject_;
            std::vector<std::any> vargs_;

            ExchangeHandler(Sari::Utils::VariadicFunction resolve, Sari::Utils::VariadicFunction reject, const std::vector<std::any>& vargs) :
                resolve_(resolve),
                reject_(reject),
                vargs_(vargs)
            {}

            ~ExchangeHandler()
            {
                reject_();
            }

        };

        using ExchangeHandlerList = DLinkedList<std::unique_ptr<ExchangeHandler>>;

        class Transaction {
        friend class Exchanger;
        public:

            boost::asio::any_io_executor getExecutor() const { return ioExecutor_; }
            bool isPending() const { return handlerElement_ != nullptr && handlerElement_->isStored(); }
            void cancel() { handlerElement_ = nullptr; }

        private:

            boost::asio::any_io_executor ioExecutor_;
            std::shared_ptr<typename ExchangeHandlerList::Element> handlerElement_;

            Transaction(boost::asio::any_io_executor ioExecutor) :
                ioExecutor_(ioExecutor)
            {}

        };

        template<typename... Args>
        Sari::Utils::Promise asyncConsume(Transaction& trans, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };
            return exchange(trans, vargs, consumeHandlers_, produceHandlers_);
        }

        template<typename... Args>
        Sari::Utils::Promise asyncPrtoduce(Transaction& trans, Args&&... args)
        {
            std::vector<std::any> vargs = { args... };
            return exchange(trans, vargs, produceHandlers_, consumeHandlers_);
        }

        static Transaction CreateTransaction(boost::asio::any_io_executor ioExecutor)
        {
            return Transaction{ ioExecutor };
        }

        static Transaction CreateTransaction(boost::asio::io_context& ioContext)
        {
            return CreateTransaction(ioContext.get_executor());
        }

    private:

        ExchangeHandlerList consumeHandlers_;
        ExchangeHandlerList produceHandlers_;

        Sari::Utils::Promise exchange(
            Transaction& trans, const std::vector<std::any>& vargs,
            ExchangeHandlerList& handlers, ExchangeHandlerList& counterpartHandlers
        ) {
		    return Sari::Utils::Promise(
                trans.getExecutor(),
			    [&](Sari::Utils::VariadicFunction resolve, Sari::Utils::VariadicFunction reject) {

                    typename ExchangeHandlerList::Element* counterpartHandlerElement = counterpartHandlers.first();

                    if (counterpartHandlerElement) {
                        std::unique_ptr<ExchangeHandler> counterpartHandler = std::move(counterpartHandlerElement->data());
                        counterpartHandlers.remove(counterpartHandlerElement);
                        counterpartHandler->resolve_(vargs);
                        resolve(counterpartHandler->vargs_);
                    }
                    else {
                        auto exchangeHandler = std::make_unique<ExchangeHandler>(resolve, reject, vargs);
                        typename ExchangeHandlerList::Element* newHandlerElement = new typename ExchangeHandlerList::Element(std::move(exchangeHandler));
                        trans.handlerElement_ = std::shared_ptr<typename ExchangeHandlerList::Element>(newHandlerElement);
                        handlers.append(newHandlerElement);
                    }
			    }
		    );
        }

    };

}}