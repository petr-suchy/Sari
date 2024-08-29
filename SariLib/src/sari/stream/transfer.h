#pragma once

#include <algorithm>
#include <utility>
#include <memory>
#include <functional>
#include <boost/asio/buffer.hpp>

#include "../utils/promise.h"

namespace Sari { namespace Stream { namespace Transfer {

	constexpr std::size_t EndpointBufferSize = 4096;

	class Finalizer {
	public:

		using Handler = std::function<void()>;

		Finalizer(Handler handler) :
			handler_(handler)
		{}

		~Finalizer()
		{
			handler_();
		}

	private:

		Handler handler_;

	};

	// Holds a readable stream.
	template<typename ReadbaleStream>
	struct Source;

	// Holds a writable stream.
	template<typename WritableStream>
	struct Sink;

	// Holds a readable stream.
	template<typename ReadableStream>
	struct Source : std::enable_shared_from_this<Source<ReadableStream>> {

		char readBuff_[EndpointBufferSize] = { 0 };
		std::size_t bytesRead_ = 0;
		ReadableStream& readableStream_;
		std::shared_ptr<Finalizer> finalizer_;

		Source(ReadableStream& readableStream, std::shared_ptr<Finalizer> finalizer) :
			readableStream_(readableStream),
			finalizer_(finalizer)
		{}

		template<typename WritableStream>
		void transfer(std::shared_ptr<Sink<WritableStream>> sink)
		{
			readableStream_.async_read_some(
				boost::asio::buffer(readBuff_),
				[source = this->shared_from_this(), sink](const boost::system::error_code& err, std::size_t bytesRead) {

					source->bytesRead_ = bytesRead;

					if (err) {

						source->close();

						if (sink->bytesToWrite_ == 0) {
							sink->close();
						}
						else {
							sink->closeAfterWrite_ = true;
						}

						return;
					}
				 
					if (sink->bytesToWrite_ == 0) {
						sink->transfer(source);
						source->transfer(sink);
					}

				}
			);
		}

		void close()
		{
			readableStream_.close();
		}

	};

	// Holds a writable stream.
	template<typename WritableStream>
	struct Sink : std::enable_shared_from_this<Sink<WritableStream>> {

		char writeBuff_[EndpointBufferSize] = { 0 };
		std::size_t bytesToWrite_ = 0;
		bool closeAfterWrite_ = false;
		WritableStream& writableStream_;
		std::shared_ptr<Finalizer> finalizer_;

		Sink(WritableStream& writableStream, std::shared_ptr<Finalizer> finalizer) :
			writableStream_(writableStream),
			finalizer_(finalizer)
		{}

		template<typename ReadableStream>
		void transfer(std::shared_ptr<Source<ReadableStream>> source)
		{
			bytesToWrite_ = std::exchange(source->bytesRead_, 0);
			std::copy(source->readBuff_, source->readBuff_ + bytesToWrite_, writeBuff_);

			writableStream_.async_write_some(
				boost::asio::buffer(writeBuff_, bytesToWrite_),
				[sink = this->shared_from_this(), source](const boost::system::error_code& err, std::size_t bytesWritten) {

					sink->bytesToWrite_ = 0;

					if (err) {
						sink->close();
						source->close();
						return;
					}

					if (sink->closeAfterWrite_) {
						sink->close();
						return;
					}

					if (source->bytesRead_ != 0) {
						sink->transfer(source);
						source->transfer(sink);
					}

				}
			);
		}

		void close()
		{
			writableStream_.close();
		}

	};

	// Redirect a readable stream to a writable stream so that all data read from the readable stream
	// will be written to the writable stream.
	template<typename ReadableStream, typename WritableStream>
	static void Redirect(ReadableStream& readableStream, WritableStream& writableStream, std::shared_ptr<Finalizer> finalizer)
	{
		auto source = std::make_shared<Source<ReadableStream>>(readableStream, finalizer);
		auto sink = std::make_shared<Sink<WritableStream>>(writableStream, finalizer);

		source->transfer(sink);
	}

	// Redirect a readable stream to a writable stream so that all data read from the readable stream
	// will be written to the writable stream.
	template<typename ReadableStream, typename WritableStream>
	static void Redirect(ReadableStream& readableStream, WritableStream& writableStream, Finalizer::Handler handler)
	{
		auto finalizer = std::make_shared<Finalizer>(handler);
		Redirect(readableStream, writableStream, finalizer);
	}

	// Redirect a readable stream to a writable stream so that all data read from the readable stream
	// will be written to the writable stream.
	template<typename ReadableStream, typename WritableStream>
	static Utils::Promise Redirect(ReadableStream& readableStream, WritableStream& writableStream)
	{
		return Utils::Promise(
			readableStream.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {

				Finalizer::Handler handler = [=]() {
					resolve();
				};

				Redirect(readableStream, writableStream);
			},
			Utils::Promise::Async
		);
	}

	// Forward the first stream to the second stream so that any data read from the first stream
	// is written to the second stream and vice versa.
	template<typename FirstStream, typename SecondStream>
	static void Forward(FirstStream& firstStream, SecondStream& secondStream, std::shared_ptr<Finalizer> finalizer)
	{
		Redirect(firstStream, secondStream, finalizer);
		Redirect(secondStream, firstStream, finalizer);
	}

	// Forward the first stream to the second stream so that any data read from the first stream
	// is written to the second stream and vice versa.
	template<typename FirstStream, typename SecondStream>
	static void Forward(FirstStream& firstStream, SecondStream& secondStream, Finalizer::Handler handler)
	{
		auto finalizer = std::make_shared<Finalizer>(handler);
		Forward(firstStream, secondStream, finalizer);
	}

	// Forward the first stream to the second stream so that any data read from the first stream
	// is written to the second stream and vice versa.
	template<typename FirstStream, typename SecondStream>
	static Utils::Promise Forward(FirstStream& firstStream, SecondStream& secondStream)
	{
		return Utils::Promise(
			firstStream.get_executor(),
			[&](Utils::VariadicFunction resolve, Utils::VariadicFunction reject) {

				Finalizer::Handler handler = [=]() {
					resolve();
				};

				Forward(firstStream, secondStream, handler);
			},
			Utils::Promise::Async
		);
	}

}}}
