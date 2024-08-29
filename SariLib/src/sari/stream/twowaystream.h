#pragma once

namespace Sari { namespace Stream {

	template<typename ReadableEnd, typename WritableEnd = ReadableEnd>
	class TwoWayStream {
	public:

		using executor_type = typename ReadableEnd::executor_type;

		TwoWayStream(boost::asio::io_context& ioContext) :
			readableEnd_(ioContext),
			writableEnd_(ioContext)
		{}

		TwoWayStream(boost::asio::any_io_executor ioExecutor) :
			readableEnd_(ioExecutor),
			writableEnd_(ioExecutor)
		{}

		boost::asio::any_io_executor get_executor()
		{	return readableEnd_.get_executor();
		};

		WritableEnd& readable_end() { return writableEnd_; }
		ReadableEnd& writable_end() { return readableEnd_; }

		template<typename ConstBufferSequence, typename WriteHandler>
		void async_write_some(
			const ConstBufferSequence& buffers,
			WriteHandler handler
		) {
			writableEnd_.async_write_some(buffers, handler);
		}

		template<typename MutableBufferSequence, typename ReadHandler>
		void async_read_some(
			const MutableBufferSequence& buffers,
			ReadHandler&& handler
		) {
			readableEnd_.async_read_some(buffers, handler);
		}

		void close()
		{
			readableEnd_.close();
			writableEnd_.close();
		}

	private:

		ReadableEnd readableEnd_;
		WritableEnd writableEnd_;

	};

}}
