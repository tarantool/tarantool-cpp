#ifndef ASIO_CONNECTOR_H
#define ASIO_CONNECTOR_H

/// C++ Tarantool connector.
/**
 * Uses Boost.Asio library for network and msgpack library for request messages serialization.
 * Can be used in both synchronous and asynchronous manner. Uses Boost.Coroutine for asynchronous calls.
 *
 * Tested with Tarantool 1.6, 1.7
 */

#include <msgpack.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <mutex>
#include <tuple>
#include <stdint.h>
#include "header.h"
#include "request_processor.h"
#include "update_op.h"


using boost::asio::buffer;
using boost::asio::io_service;
using boost::asio::ip::tcp;


namespace tarantool {
namespace client {

class space;

/**
 * Class provides convenient interface for processing requests to Tarantool DB while encapsulating low-level
 * protocol implementation.
 */
class box {
public:
    explicit box(boost::asio::io_service& srvc);
    box(const box&) = delete;
    box& operator=(const box&) = delete;

public:
    /**
     * Wrapper class for accessing a particular space.
     */
    class space {
    public:
        /**
         * Create space instance which still needs space id for valid operating.
         * @param box Pointer to outer class object used as function calls proxy.
         */
        space(box* box)
            : box_(box) {
        }

        space& operator[](uint32_t id) {
            space_id_ = id;
            return *this;
        }

        /**
         * Execute SELECT request for a given space
         * @tparam Args1 Types of Key parts
         * @tparam Args2 Types of returned tuples
         * @param index_id Index number in space
         * @param key Mutltipart key
         * @param offset
         * @param limit Limits number of returned tuples
         * @param tuples Returned tuples
         * @param error_code Tarantool error code
         * Defined here: https://github.com/tarantool/tarantool/blob/1.7/src/box/errcode.h
         * @param yield Yield context used for asynchronous requests
         */
        template <typename... Args1, typename... Args2>
        void select_limit(uint32_t index_id, boost::optional<std::tuple<Args1...>> key, uint64_t offset,
                          boost::optional<uint32_t> limit, std::vector<std::tuple<Args2...>>& tuples,
                          uint32_t& error_code, boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->select_limit(*space_id_, index_id, key, offset, limit, tuples, error_code, yield);
        }

        /**
         * Execute SELECT request for a given space. Expects to receive a single tuple.
         * @tparam Args1 Types of Key parts
         * @tparam Args2 Types of returned tuple
         * @param index_id Index number in space
         * @param key Mutltipart key
         * @param tuple Returned tuple
         * @param error_code Tarantool error code
         * Defined here: https://github.com/tarantool/tarantool/blob/1.7/src/box/errcode.h
         * @param yield Yield context used for asynchronous requests
         */
        template <typename... Args1, typename... Args2>
        void select(uint32_t index_id, const std::tuple<Args1...>& key,
                    boost::optional<std::tuple<Args2...>>& tuple, uint32_t& error_code,
                    boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->select(*space_id_, index_id, key, tuple, error_code, yield);
        }

        /**
         * Execute DELETE request for a given space.
         * @tparam T Primary key type
         * @param index_id Index number in space
         * @param key Primary key
         * @param error_code Tarantool error code
         * @param yield Yield context used for asynchronous requests
         */
        template <typename T>
        void erase(uint32_t index_id, const T& key, uint32_t& error_code,
                   boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->erase(*space_id_, index_id, key, error_code, yield);
        }

        /**
         * Execute INSERT request for a given space.
         * @tparam Args Type of tuple to insert
         * @param tuple Tuple to insert
         * @param error_code Tarantool error code
         * @param yield Yield context used for asynchronous requests
         */
        template <typename... Args>
        void insert(const std::tuple<Args...>& tuple, uint32_t& error_code,
                    boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->insert(*space_id_, tuple, error_code, yield);
        }

        /**
         * Execute REPLACE request for a given space.
         * @tparam Args Type of tuple to insert or replace
         * @param tuple Tuple to insert or replace
         * @param error_code Tarantool error code
         * @param yield Yield context used for asynchronous requests
         */
        template <typename... Args>
        void replace(const std::tuple<Args...>& tuple, uint32_t& error_code,
                     boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->replace(*space_id_, tuple, error_code, yield);
        }

        /**
         * Execute UPDATE request for a given space.
         * @tparam T Type of primary key
         * @tparam Args Types of update operations
         * @param index_id
         * @param key Primary key
         * @param ops Update operations
         * @param error_code Tarantool error code
         * @param yield Yield context used for asynchronous requests
         */
        template <typename T, typename... Args>
        void update(uint32_t index_id, const T& key, const std::tuple<Args...>& ops,
                    uint32_t& error_code, boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->update(*space_id_, index_id, key, ops, error_code, yield);
        }

        /**
         * Execute UPSERT request for a given space
         * @tparam Args1 Type of tuple to upsert
         * @tparam Args2 Types of update operations
         * @param tuple Tuple to upsert
         * @param ops Update operations
         * @param error_code Tarantool error code
         * @param yield Yield context used for asynchronous requests
         */
        template <typename... Args1, typename... Args2>
        void upsert(const std::tuple<Args1...>& tuple, std::tuple<Args2...> ops,
                    uint32_t& error_code, boost::optional<boost::asio::yield_context> yield = {}) {
            assert(space_id_);
            box_->upsert(*space_id_, tuple, ops, error_code, yield);
        }

    private:
        box* box_;
        boost::optional<uint32_t> space_id_;
    } space;

public:
    void connect(const std::string& host, uint16_t port, uint32_t timeout); // TODO: timeout type?
    void connect_async(const std::string& host, uint16_t port, uint32_t timeout, boost::asio::yield_context yield);
    void disconnect(bool force = false); // TODO: flush all operations

    /**
     * Execute SELECT request.
     * @tparam Args1 Types of Key parts
     * @tparam Args2 Types of returned tuples
     * @param space_id Index of space to execute operation
     * @param index_id Index number in space
     * @param key Mutltipart key
     * @param offset
     * @param limit Limits number of returned tuples
     * @param tuples Returned tuples
     * @param error_code Tarantool error code
     * Defined here: https://github.com/tarantool/tarantool/blob/1.7/src/box/errcode.h
     * @param yield Yield context used for asynchronous requests
     */
    template <typename... Args1, typename... Args2>
    void select_limit(uint32_t space_id, uint32_t index_id, boost::optional<std::tuple<Args1...>> key, uint64_t offset,
                      boost::optional<uint32_t> limit, std::vector<std::tuple<Args2...>>& tuples, uint32_t& error_code,
                      boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::select), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (6 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(6);
        packer.pack(static_cast<uint32_t>(keys::space_id));
        packer.pack(space_id);
        packer.pack(static_cast<uint32_t>(keys::index_id));
        packer.pack(index_id);

        packer.pack(static_cast<uint32_t>(keys::limit));
        if (!limit) {
            limit = std::numeric_limits<uint32_t>::max();
        }

        packer.pack(limit.get());
        packer.pack(static_cast<uint32_t>(keys::offset));
        packer.pack(offset);
        packer.pack(static_cast<uint32_t>(keys::iterator));
        packer.pack(0);

        packer.pack(static_cast<uint32_t>(keys::key));
        if (!key) {
            packer.pack_array(0);
        } else {
            packer.pack(key.get());
        }

        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;

        obj_handler.get().convert(response_header);

        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
            return;
        }

        response_unpacker.next(obj_handler);
        std::map<uint32_t, std::vector<std::tuple<Args2...>>> tmap;

        obj_handler.get().convert(tmap);

        auto it = tmap.find(static_cast<uint32_t>(keys::data));
        if (it != tmap.end()) {
            tuples = it->second;
        }

    }

    /**
     * Execute SELECT request. Expects to receive a single tuple.
     * @tparam Args1 Types of Key parts
     * @tparam Args2 Types of returned tuple
     * @param space_id Index of space to execute operation
     * @param index_id Index number in space
     * @param key Mutltipart key
     * @param tuple Returned tuple
     * @param error_code Tarantool error code
     * Defined here: https://github.com/tarantool/tarantool/blob/1.7/src/box/errcode.h
     * @param yield Yield context used for asynchronous requests
     */
    template <typename... Args1, typename... Args2>
    void select(uint32_t space_id, uint32_t index_id, const std::tuple<Args1...>& key,
                boost::optional<std::tuple<Args2...>>& tuple, uint32_t& error_code,
                boost::optional<boost::asio::yield_context> yield = {}) {
        std::vector<std::tuple<Args2...>> tuples;
        boost::optional<uint32_t> limit(1);
        boost::optional<std::tuple<Args1...>> k(key);
        select_limit(space_id, index_id, k, 0, limit, tuples, error_code, yield);
        if (tuples.empty()) {
            tuple = boost::none;
        } else {
            tuple = tuples[0];

            if (tuples.size() > 1) {
                std::cerr << "More than one tuple selected." << std::endl;
            }
        }
    }

    /**
     * Execute DELETE request.
     * @tparam T Primary key type
     * @param space_id Index of space to execute operation
     * @param index_id Index number in space
     * @param key Primary key
     * @param error_code Tarantool error code
     * @param yield Yield context used for asynchronous requests
     */
    template <typename T>
    void erase(uint32_t space_id, uint32_t index_id, const T& key, uint32_t& error_code,
               boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::erase), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (3 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(3);
        packer.pack(static_cast<uint32_t>(keys::space_id));
        packer.pack(space_id);
        packer.pack(static_cast<uint32_t>(keys::index_id));
        packer.pack(index_id);
        packer.pack(static_cast<uint32_t>(keys::key));
        packer.pack_array(1);
        packer.pack(key);
        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;
        obj_handler.get().convert(response_header);
        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
        }
    }

    /**
     * Execute INSERT request.
     * @tparam Args Type of tuple to insert
     * @param space_id Index of space to execute operation
     * @param tuple Tuple to insert
     * @param error_code Tarantool error code
     * @param yield Yield context used for asynchronous requests
     */
    template <typename... Args>
    void insert(uint32_t space_id, const std::tuple<Args...>& tuple, uint32_t& error_code,
                boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::insert), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (2 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(2);
        packer.pack(static_cast<uint32_t>(keys::space_id));
        packer.pack(space_id);
        packer.pack(static_cast<uint32_t>(keys::tuple));
        packer.pack(tuple);
        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;
        obj_handler.get().convert(response_header);
        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
        }
    }

    /**
     * Execute REPLACE request.
     * @tparam Args Type of tuple to insert or replace
     * @param space_id Index of space to execute operation
     * @param tuple Tuple to insert or replace
     * @param error_code Tarantool error code
     * @param yield Yield context used for asynchronous requests
     */
    template <typename... Args>
    void replace(uint32_t space_id, const std::tuple<Args...>& tuple, uint32_t& error_code,
                 boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::replace), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (2 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(2);
        packer.pack(static_cast<uint32_t>(keys::space_id));
        packer.pack(space_id);
        packer.pack(static_cast<uint32_t>(keys::tuple));
        packer.pack(tuple);
        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;
        obj_handler.get().convert(response_header);
        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
        }
    }

    /**
     * Execute UPDATE request.
     * @tparam T Type of primary key
     * @tparam Args Types of update operations
     * @param space_id Index of space to execute operation
     * @param index_id
     * @param key Primary key
     * @param ops Update operations
     * @param error_code Tarantool error code
     * @param yield Yield context used for asynchronous requests
     */
    template <typename T, typename... Args>
    void update(uint32_t space_id, uint32_t index_id, const T& key, const std::tuple<Args...>& ops,
                uint32_t& error_code, boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::update), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (4 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(4);
        packer.pack(static_cast<uint32_t>(keys::space_id));
        packer.pack(space_id);
        packer.pack(static_cast<uint32_t>(keys::index_id));
        packer.pack(index_id);
        packer.pack(static_cast<uint32_t>(keys::key));
        packer.pack_array(1);
        packer.pack(key);
        packer.pack(static_cast<uint32_t>(keys::tuple));
        packer.pack(ops);
        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;
        obj_handler.get().convert(response_header);
        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
        }
    }

    /**
     * Execute UPSERT request.
     * @tparam Args1 Type of tuple to upsert
     * @tparam Args2 Types of update operations
     * @param space_id Index of space to execute operation
     * @param tuple Tuple to upsert
     * @param ops Update operations
     * @param error_code Tarantool error code
     * @param yield Yield context used for asynchronous requests
     */
    template <typename... Args1, typename... Args2>
    void upsert(uint32_t space_id, const std::tuple<Args1...>& tuple, std::tuple<Args2...> ops,
                uint32_t& error_code, boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::upsert), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (3 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(3);
        packer.pack(static_cast<uint32_t>(keys::space_id));
        packer.pack(space_id);
        packer.pack(static_cast<uint32_t>(keys::tuple));
        packer.pack(tuple);
        packer.pack(static_cast<uint32_t>(keys::ops));
        packer.pack(ops);
        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;
        obj_handler.get().convert(response_header);
        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
        }
    }

    /**
     * Execute CALL request.
     * @tparam Args1 Types of arguments passed to procedure
     * @tparam Args2 Type of tuples returned by executed procedure
     * @param procedure_name Name of procedure to execute
     * @param args Arguments passed to procedure
     * @param error_code Tarantool error code
     * @param returned_tuples Tuples returned by executed procedure
     * @param yield Yield context used for asynchronous requests
     */
    template <typename... Args1, typename... Args2>
    void call(const std::string& procedure_name, const std::tuple<Args1...>& args, uint32_t& error_code,
              std::vector<std::tuple<Args2...>>* returned_tuples = nullptr,
              boost::optional<boost::asio::yield_context> yield = {}) {
        std::lock_guard<std::mutex> guard(mutex_);

        /// Pack request
        msgpack::sbuffer msg;
        msgpack::packer<msgpack::sbuffer> packer(&msg);

        /// Pack unified header
        header request_header(static_cast<uint32_t>(op_code::call), 0x01, 0x00);
        packer.pack(request_header);

        /// Pack body (3 pairs)
        /// https://tarantool.org/doc/dev_guide/internals_index.html#requests
        packer.pack_map(2);
        packer.pack(static_cast<uint32_t>(keys::function_name));
        packer.pack(procedure_name);
        packer.pack(static_cast<uint32_t>(keys::tuple));
        packer.pack(args);
        std::vector<uint8_t> request;
        build_request(request, msg);

        /// Send request
        msgpack::unpacker response_unpacker;
        msgpack::object_handle obj_handler;
        requestor_.process_request(&socket_, request, response_unpacker, obj_handler, &call_id_, yield);

        /// Unpack response
        response_unpacker.next(obj_handler);
        header response_header;
        obj_handler.get().convert(response_header);
        error_code = response_header.code;
        if (error_code != status_ok) {
            dump_error_msg(response_unpacker, obj_handler);
        }

        if (returned_tuples) {
            response_unpacker.next(obj_handler);
            std::map<uint32_t, std::vector<std::tuple<Args2...>>> tmap;
            obj_handler.get().convert(tmap);
            auto it = tmap.find(static_cast<uint32_t>(keys::data));
            if (it != tmap.end()) {
                *returned_tuples = it->second;
            }
        }
    }

private:
    enum class op_code {
        select = 0x01,
        insert = 0x02,
        replace = 0x03,
        update = 0x04,
        erase = 0x05,
        upsert = 0x09,
        call = 0x06,
    };

    enum class keys {
        space_id = 0x10,
        index_id = 0x11,
        limit = 0x12,
        offset = 0x13,
        iterator = 0x14,
        key = 0x20,
        tuple = 0x21,
        function_name = 0x22,
        ops = 0x28,
        data = 0x30,
        error = 0x31,
    };

    void build_request(std::vector<uint8_t>& buffer, const msgpack::sbuffer& msg);
    void dump_error_msg(msgpack::unpacker& unpacker, msgpack::object_handle& handler);

private:
    const uint32_t status_ok;

private:
    size_t call_id_;  // Index of Tarantool request in current session
    std::mutex mutex_;
    request_processor requestor_;

public:
    tcp::socket socket_;
};

}
}

#endif // ASIO_CONNECTOR_H
