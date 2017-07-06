#ifndef REQUEST_PROCESSOR_H
#define REQUEST_PROCESSOR_H


using boost::asio::buffer;
using boost::asio::ip::tcp;


namespace tarantool {
    namespace client {


class request_processor {
public:
    void process_request(tcp::socket *socket, const std::vector<uint8_t> &request, msgpack::unpacker &response_unpacker,
                         msgpack::object_handle &handler, size_t *call_id,
                         boost::optional<boost::asio::yield_context> yield = {}) {
        socket_ = socket;
        boost::system::error_code write_errc;
        yield ? socket_->async_write_some(buffer(request.data(), request.size()), (*yield)[write_errc]) :
                socket_->write_some(buffer(request.data(), request.size()), write_errc);
        if (write_errc) {
            std::cerr << "Failed to write into tarantool. Error message: \"" << write_errc.message() << "\""
                      << std::endl;
            return;
        }

        size_t greeting_len = 0;
        if ((*call_id)++ == 1) greeting_len = 128;

        const size_t msg_size_part_len = 5;
        boost::system::error_code read_size_errc;
        std::vector<uint8_t> msg_prefix = read_fixed(greeting_len + msg_size_part_len, read_size_errc);

        response_unpacker.reserve_buffer(msg_size_part_len);
        memcpy(response_unpacker.buffer(), msg_prefix.data() + greeting_len, msg_size_part_len);
        response_unpacker.buffer_consumed(msg_size_part_len);

        uint32_t msg_len = 0;
        if (!response_unpacker.next(handler)) {
            std::cerr << "Failed to unpack size." << std::endl;
            return;
        }

        handler.get().convert(msg_len);

        boost::system::error_code read_msg_errc;
        std::vector<uint8_t> msg = read_fixed(msg_len, read_msg_errc);
        response_unpacker.reserve_buffer(msg_len);
        memcpy(response_unpacker.buffer(), msg.data(), msg_len);
        response_unpacker.buffer_consumed(msg_len);
    }

protected:
    std::vector<uint8_t> read_fixed(uint32_t len, boost::system::error_code &errc,
                                    boost::optional<boost::asio::yield_context> yield = {}) {
        std::vector<uint8_t> msg;
        const size_t buf_size = 256;
        while (len > 0) {
            std::vector<uint8_t> buff(len > buf_size ? buf_size : len, 0);
            const size_t bytes_read = yield ?
                                        socket_->async_read_some(buffer(buff.data(), buff.size()), (*yield)[errc])
                                      : socket_->read_some(buffer(buff.data(), buff.size()), errc);
            if (errc) {
                std::cerr << "Failed to read from tarantool. Error message: \"" << errc.message() << "\"" << std::endl;
                return msg;
            }

            msg.resize(msg.size() + bytes_read);
            memcpy(msg.data() + msg.size() - bytes_read, buff.data(), bytes_read);
            len -= bytes_read;
        }

        return msg;
    };

private:
    tcp::socket *socket_;
};

}
}

#endif // REQUEST_PROCESSOR_H
