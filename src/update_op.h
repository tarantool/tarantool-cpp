#ifndef UPDATE_OP_H
#define UPDATE_OP_H

#include <msgpack.hpp>

namespace tarantool {
namespace client {

/**
 * Class represents UPDATE/UPSERT operation structure according to
 * https://tarantool.org/doc/1.7/dev_guide/internals_index.html#box-protocol-iproto-protocol
 */
template <typename T>
class update_op {
public:
    enum type {
        add_t,
        subtract_t,
        and_t,
        xor_t,
        or_t,
        erase_t,
        insert_t,
        assign_t,
        splice_t,
    };

    update_op(type op_type, int field_number, T value)
    : op_type_(op_type),
      field_number_(field_number),
      value_(value) {
    }

    /**
     * Pack UPDATE/UPSERT operation structure.
     * This method is used by msgpack for serialization.
     */
    template <typename Packer>
    void msgpack_pack(Packer& packer) const {
        static std::unordered_map<int, std::string> op_symbols {
            { type::add_t,      "+" },
            { type::subtract_t, "-" },
            { type::and_t,      "&" },
            { type::xor_t,      "^" },
            { type::or_t,       "|" },
            { type::erase_t,   "#" },
            { type::insert_t,   "!" },
            { type::assign_t,   "=" },
            { type::splice_t,   ":" },
        };

        packer.pack_array(3);
        packer.pack(op_symbols[op_type_]);
        packer.pack(field_number_);
        packer.pack(value_);
    }

private:
    type op_type_;
    int field_number_;
    T value_;
};

}
}

#endif // UPDATE_OP_H
