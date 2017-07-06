#include "tarantool_connector.h"
#include <boost/bind.hpp>
#include <memory>
#include <string>


using boost::asio::ip::address;


namespace tarantool {
namespace client {

box::box(io_service& service)
    : space(this),
      status_ok(0x00),
      call_id_(1),
      socket_(service) {
}

void box::connect_async(const std::string& host, uint16_t port, uint32_t timeout, boost::asio::yield_context yield) {
    tcp::endpoint ep(address::from_string(host), port);
    boost::system::error_code ec;
    socket_.async_connect(ep, yield[ec]);
    if (ec) {
        std::cerr << "Failed to connect. Error message: " << ec.message() << ".\"" << std::endl;
    }

    socket_.set_option(tcp::socket::reuse_address(true));
}

void box::connect(const std::string& host, uint16_t port, uint32_t timeout) {
    tcp::endpoint ep(address::from_string(host), port);
    boost::system::error_code ec;
    socket_.connect(ep, ec);
    if (ec) {
        std::cerr << "Failed to connect. Error message: " << ec.message() << ".\"" << std::endl;
    }

    socket_.set_option(tcp::socket::reuse_address(true));
}

void box::disconnect(bool force) {
    boost::system::error_code errc;
    socket_.shutdown(tcp::socket::shutdown_both, errc);
    socket_.close();
}

void box::dump_error_msg(msgpack::unpacker& unpacker, msgpack::object_handle& handler) {
    if (!unpacker.next(handler)) {
        std::cerr << "Failed to dump error msg." << std::endl;
        return;
    }

    std::map<uint32_t, std::string> body;
    handler.get().convert(body);
    std::cerr << "Error msg: \"" << body[static_cast<uint32_t>(keys::error)] << "\"" << std::endl;
}

void box::build_request(std::vector<uint8_t>& buffer, const msgpack::sbuffer& msg) {
    msgpack::sbuffer size_buffer;
    msgpack::packer<msgpack::sbuffer> size_packer(&size_buffer);
    size_packer.pack_uint32(msg.size());

    buffer.resize(size_buffer.size() + msg.size());
    memcpy(buffer.data(), size_buffer.data(), size_buffer.size());
    memcpy(buffer.data() + size_buffer.size(), msg.data(), msg.size());
}

}
}
