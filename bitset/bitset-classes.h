#pragma once
#include <cstddef>
#include <cstdint>

using base_word_type = uint64_t;
using word_pointer = base_word_type*;

struct bitset;
template <typename T>
struct base_iterator;

template <typename T>
struct base_view;

namespace bitset_members {
const size_t word_len = sizeof(base_word_type) * 8;
} // namespace bitset_members
