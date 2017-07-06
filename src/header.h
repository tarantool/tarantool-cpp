#ifndef HEADER_H
#define HEADER_H

#include <msgpack.hpp>

namespace tarantool {
namespace client {

/**
 * Class represents unified header of Tarantool request/response message.
 */
class header {
public:
    header() {
    }

    header(uint32_t op_code, uint32_t sync, uint32_t schema_id)
      : code(op_code),
        sync(sync),
        schema_id(schema_id) {
    }

    /**
     * Pack unified header according to
     * https://tarantool.org/doc/dev_guide/internals_index.html#unified-packet-structure.
     * This method is used by msgpack for serialization.
     */
    template <typename Packer>
    void msgpack_pack(Packer& packer) const {
        packer.pack_map(3);

        packer.pack(static_cast<uint32_t>(keys::code));
        packer.pack(code);

        packer.pack(static_cast<uint32_t>(keys::sync));
        packer.pack(sync);

        packer.pack(static_cast<uint32_t>(keys::schema_id));
        packer.pack(schema_id);
    }

    /**
     * Unpack unified header according to
     * https://tarantool.org/doc/dev_guide/internals_index.html#response-packet-structure.
     * This method is used by msgpack for deserialization.
     */
    void msgpack_unpack(msgpack::object o) {
        assert(o.type == msgpack::type::MAP);
        std::map<uint32_t, uint32_t> hmap;
        o.convert(hmap);
        code = hmap[static_cast<uint32_t>(keys::code)];
        sync = hmap[static_cast<uint32_t>(keys::sync)];
        schema_id = hmap[static_cast<uint32_t>(keys::schema_id)];
    }

public:
    uint32_t code;  /// Code of operation in request header / return code in response header
    uint32_t sync;
    uint32_t schema_id;

private:
    enum class keys {
        code = 0x00,
        sync = 0x01,
        schema_id = 0x05,
    };
};

}
}

#endif // HEADER_H
