#include <iostream>
#include "sari/asio/asio.h"
#include "sari/stream/twowaystream.h"

namespace asio = boost::asio;
namespace Asio = Sari::Asio;
namespace Utils = Sari::Utils;
namespace Stream = Sari::Stream;

template<typename Stream, typename StreamBuf>
static Utils::Promise AsyncReadLine(std::shared_ptr<Stream> stream, std::shared_ptr<StreamBuf> streamBuf)
{
    return Asio::AsyncReadUntil(*stream, *streamBuf, "\n")
        .then([stream, streamBuf]() {

            // Extract one line from the stream buffer.
            std::istream is(streamBuf.get());
            std::string line;
            std::getline(is, line);

            return Utils::Promise::Resolve(stream->get_executor(), line, streamBuf);
        });
}

template<typename Stream>
static Utils::Promise AsyncEcho(std::shared_ptr<Stream> stream)
{
    auto streamBuf = std::make_shared<asio::streambuf>();

    auto task = [stream, streamBuf]() {
        return AsyncReadLine(stream, streamBuf)
            .then([stream](std::string line) {
                auto resp = std::make_shared<std::string>(line + '\n');
                return Asio::AsyncWriteSome(*stream, asio::buffer(*resp))
                    .then([stream, resp]() {
                        return true;
                    });
            });
    };

    return Utils::Promise::Repeat(stream->get_executor(), task);
}

template<typename Stream>
static Utils::Promise AsyncClient(std::shared_ptr<Stream> stream)
{
    auto streamBuf = std::make_shared<asio::streambuf>();

    return Utils::Promise::Resolve(stream->get_executor())
        .then([stream, streamBuf]() {
            auto msg = std::make_shared<std::string>("Hello!\n");
            return Asio::AsyncWriteSome(*stream, asio::buffer(*msg))
                .then([stream, streamBuf, msg]() {
                    return AsyncReadLine(stream, streamBuf);
                });
        }).then([stream, streamBuf](std::string resp) {

            std::cout << resp << '\n';

            auto msg = std::make_shared<std::string>("Good bye.\n");

            return Asio::AsyncWriteSome(*stream, asio::buffer(*msg))
                .then([stream, streamBuf, msg]() {
                    return AsyncReadLine(stream, streamBuf);
                });
        }).then([stream](std::string resp) {
            std::cout << resp << '\n';
            stream->close();
        });
}

int main()
{
    using PipeCoupler = Stream::TwoWayStream<boost::asio::readable_pipe, boost::asio::writable_pipe>;

    asio::io_context ioContext;

    auto sourcePipe = std::make_shared<PipeCoupler>(ioContext);
    auto sinkPipe = std::make_shared<PipeCoupler>(ioContext);

    boost::asio::connect_pipe(sourcePipe->writable_end(), sinkPipe->readable_end());
    boost::asio::connect_pipe(sinkPipe->writable_end(), sourcePipe->readable_end());

    Utils::Promise echo = AsyncEcho(sourcePipe);
    Utils::Promise client = AsyncClient(sinkPipe);

    Utils::Promise::AllSettled(ioContext, { echo, client })
        .then([]() {
            std::cout << "End\n";
        });

    ioContext.run();

    return 0;
}
