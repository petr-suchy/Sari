#pragma once

#include <memory>
#include <functional>
#include "Promise.h"

namespace Sari { namespace Stream {

	class ForwardFinalizer {
	public:

		using Handler = std::function<void()>;
		using SharedPtr = std::shared_ptr<ForwardFinalizer>;

		ForwardFinalizer(Handler handler) :
			handler_(handler)
		{}

		~ForwardFinalizer()
		{
			handler_();
		}

	private:

		Handler handler_;

	};

	class StreamForwarderBase : public std::enable_shared_from_this<StreamForwarderBase> {
	public:

		virtual void write(std::shared_ptr<StreamForwarderBase>) = 0;
		virtual void read(std::shared_ptr<StreamForwarderBase>) = 0;
		virtual void close() = 0;

		char sendBuff_[4096] = { 0 };
		std::size_t bytesToSend_ = 0;

		char recvBuff_[4096] = { 0 };
		std::size_t bytesRecv_ = 0;

		bool closeAfterSent_ = false;

	};

	template<typename Stream>
	class StreamForwarder : public StreamForwarderBase {
	public:

		StreamForwarder(Stream& stream, ForwardFinalizer::SharedPtr finalizer) :
			StreamForwarderBase(),
			stream_(stream),
			finalizer_(finalizer)
		{}
	
		virtual void write(std::shared_ptr<StreamForwarderBase> second)
		{
			bytesToSend_ = std::exchange(second->bytesRecv_, 0);
			std::copy(second->recvBuff_, second->recvBuff_ + bytesToSend_, sendBuff_);
		
			stream_.async_write_some(
				boost::asio::buffer(sendBuff_, bytesToSend_),
				[first = this->shared_from_this(), second](const boost::system::error_code& err, std::size_t bytesTransferred) {
			
					first->bytesToSend_ = 0;

					if (err) {
						first->close();
						second->close();
						return;
					}

					if (first->closeAfterSent_) {
						first->close();
						return;
					}

					if (second->bytesRecv_ != 0) {
						first->write(second);
						second->read(first);
					}

				}
			);
		
		}

		virtual void read(std::shared_ptr<StreamForwarderBase> second)
		{
			stream_.async_read_some(
				boost::asio::buffer(recvBuff_),
				[first = this->shared_from_this(), second](const boost::system::error_code& err, std::size_t bytesTransferred) {

					first->bytesRecv_ = bytesTransferred;

					if (err) {

						first->close();

						if (second->bytesToSend_ == 0) {
							second->close();
						}
						else {
							second->closeAfterSent_ = true;
						}

						return;
					}
				 
					if (second->bytesToSend_ == 0) {
						second->write(first);
						first->read(second);
					}

				}
			);
		}

		virtual void close()
		{
			stream_.close();
		}

	private:;

		Stream& stream_;
		ForwardFinalizer::SharedPtr finalizer_;

	};

	template<typename FirstStream, typename SecondStream>
	void Forwar(FirstStream& firstStream, SecondStream& secondStream, ForwardFinalizer::Handler handler)
	{
		auto finalizer = std::make_shared<ForwardFinalizer>(handler);
		auto first = std::make_shared<StreamForwarder<FirstStream>>(firstStream, finalizer);
		auto second = std::make_shared<StreamForwarder<SecondStream>>(secondStream, finalizer);

		first->read(second);
		second->read(first);
	}

	template<typename FirstStream, typename SecondStream>
	Utils::Promise Forward(FirstStream& firstStream, SecondStream& secondStream)
	{
		return Utils::Promise(
			firstStream.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {

				ForwardFinalizer::Handler handler = [=]() {
					resolve();
				};

				auto finalizer = std::make_shared<ForwardFinalizer>(handler);
				auto first = std::make_shared<StreamForwarder<FirstStream>>(firstStream, finalizer);
				auto second = std::make_shared<StreamForwarder<SecondStream>>(secondStream, finalizer);

				first->read(second);
				second->read(first);
			},
			Utils::Promise::Async
		);
	}
}}
