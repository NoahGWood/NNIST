#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace nnist {
    enum DELIM : uint8_t {
        FS = 0x1C,  // File Separator (Ends a Record)
        GS = 0x1D,  // Group Separator (Ends a Field)
        RS = 0x1E,  // Record Separator (Ends a Subfield)
        US = 0x1F   // Unit Separator (Ends an Item)
    };

    struct Item {
        std::vector<uint8_t> bytes;
    };
    struct Subfield {
        std::vector<Item> items;
    };
    struct Field {
        bool is_binary_field = false;
        std::vector<uint8_t> raw_field;
        std::vector<uint8_t> tag;
        std::vector<Subfield> subfields;
    };
    struct Record {
        int type = -1;
        int idc = -1;
        bool is_binary_record =
            false;  // legacy/binary record (Type 3-6, 8, etc.)
        std::vector<uint8_t>
            raw_record;  // EXACT bytes of the record when binary
        std::vector<Field> fields;
    };
    struct File {
        std::vector<Record> records;
    };
}  // namespace nnist
