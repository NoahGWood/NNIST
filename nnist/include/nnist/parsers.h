#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "nnist/constants.h"
#include "nnist/types.h"

namespace nnist {

    // --------------------------------------------
    // Cursor (Safe Span-Based Reader)
    // --------------------------------------------
    class Cursor {
      public:
        explicit Cursor(std::span<const uint8_t> buffer) noexcept
            : buffer_(buffer) {}

        [[nodiscard]] bool done() const noexcept {
            return position_ >= buffer_.size();
        }

        [[nodiscard]] size_t position() const noexcept { return position_; }

        [[nodiscard]] size_t remaining() const noexcept {
            return buffer_.size() - position_;
        }
        [[nodiscard]] size_t size() const noexcept { return buffer_.size(); }

        void seek(size_t new_position) noexcept { position_ = new_position; }

        void advance(size_t count = 1U) noexcept { position_ += count; }

        [[nodiscard]] uint8_t peek() const noexcept {
            return done() ? 0U : buffer_[position_];
        }

        uint8_t get() noexcept { return done() ? 0U : buffer_[position_++]; }

        [[nodiscard]] uint8_t at(size_t index) const noexcept {
            return buffer_[index];
        }

        [[nodiscard]] std::span<const uint8_t> slice(
            size_t start, size_t end) const noexcept {
            return buffer_.subspan(start, end - start);
        }

      private:
        std::span<const uint8_t> buffer_;
        size_t position_ = 0U;
    };

    // --------------------------------------------
    // Big-endian read helper
    // --------------------------------------------

    inline uint32_t READ_BE_U32(std::span<const uint8_t> bytes) noexcept {
        if (bytes.size() < 4U) {
            return 0U;
        }

        return (uint32_t(bytes[BYTE_INDEX_0]) << SHIFT_24) |
               (uint32_t(bytes[BYTE_INDEX_1]) << SHIFT_16) |
               (uint32_t(bytes[BYTE_INDEX_2]) << SHIFT_8) |
               (uint32_t(bytes[BYTE_INDEX_3]));
    }

    inline uint32_t READ_BE_U32(const uint8_t* pointer) noexcept {
        if (pointer == nullptr) {
            return 0U;
        }

        std::span<const uint8_t> bytes(pointer, 4U);
        return READ_BE_U32(bytes);
    }

    // --------------------------------------------
    // Binary Record Type Check
    // --------------------------------------------

    inline bool IS_LEGACY_BINARY_TYPE(int record_type) noexcept {
        switch (record_type) {
            case RECORD_TYPE_3:
            case RECORD_TYPE_4:
            case RECORD_TYPE_5:
            case RECORD_TYPE_6:
            case RECORD_TYPE_7:
            case RECORD_TYPE_8:
                return true;
            default:
                return false;
        }
    }

    // --------------------------------------------
    // Token Extraction
    // --------------------------------------------
    inline std::vector<uint8_t> TAKE_UNTIL(Cursor& cursor, uint8_t delimiter) {
        const size_t start_position = cursor.position();

        while (!cursor.done() && cursor.peek() != delimiter) {
            cursor.advance();
        }

        auto bytes = cursor.slice(start_position, cursor.position());
        return {bytes.begin(), bytes.end()};
    }

    // --------------------------------------------
    // Item Parsing
    // --------------------------------------------
    inline Item PARSE_ITEM(Cursor& cursor) {
        const size_t start_position = cursor.position();

        while (!cursor.done()) {
            const uint8_t value = cursor.peek();
            if (value == US || value == RS || value == GS || value == FS) {
                break;
            }
            cursor.advance();
        }

        auto bytes = cursor.slice(start_position, cursor.position());
        return {{bytes.begin(), bytes.end()}};
    }

    // --------------------------------------------
    // Subfield Parsing
    // --------------------------------------------
    inline Subfield PARSE_SUBFIELD(Cursor& cursor) {
        Subfield subfield;

        while (!cursor.done()) {
            subfield.items.push_back(PARSE_ITEM(cursor));

            if (cursor.done()) {
                break;
            }

            if (cursor.peek() == US) {
                cursor.advance();
                continue;
            }

            break;
        }

        return subfield;
    }

    // --------------------------------------------
    // Binary Field Parsing
    // --------------------------------------------
    inline Field PARSE_BINARY_FIELD(Cursor& cursor, size_t record_end) {
        Field field;
        field.is_binary_field = true;

        const size_t start_position = cursor.position();
        size_t end_position = record_end;

        if (end_position == INVALID_INDEX) {
            while (!cursor.done() && cursor.peek() != FS) {
                cursor.advance();
            }
            end_position = cursor.position();
        } else {
            if (end_position > start_position &&
                cursor.at(end_position - 1U) == FS) {
                --end_position;
            }
            if (end_position > start_position &&
                cursor.at(end_position - 1U) == GS) {
                --end_position;
            }
            cursor.seek(end_position);
        }

        auto bytes = cursor.slice(start_position, end_position);
        field.raw_field.assign(bytes.begin(), bytes.end());

        return field;
    }

    // --------------------------------------------
    // Text Field Parsing
    // --------------------------------------------
    inline Field PARSE_TEXT_FIELD(Cursor& cursor) {
        Field field;

        while (!cursor.done() && cursor.peek() != GS && cursor.peek() != FS) {
            field.subfields.push_back(PARSE_SUBFIELD(cursor));

            if (!cursor.done() && cursor.peek() == RS) {
                cursor.advance();
                continue;
            }

            break;
        }

        return field;
    }

    // --------------------------------------------
    // Field Dispatcher
    // --------------------------------------------
    inline Field PARSE_FIELD(Cursor& cursor, size_t record_end) {
        Field field;
        field.tag = TAKE_UNTIL(cursor, static_cast<uint8_t>(':'));

        if (!cursor.done() && cursor.peek() == static_cast<uint8_t>(':')) {
            cursor.advance();
        }

        const std::string tag_string(field.tag.begin(), field.tag.end());

        if (tag_string.ends_with(".999")) {
            Field parsed = PARSE_BINARY_FIELD(cursor, record_end);
            parsed.tag = std::move(field.tag);  // <-- keep tag
            return parsed;
        }

        Field parsed = PARSE_TEXT_FIELD(cursor);
        parsed.tag = std::move(field.tag);  // <-- keep tag
        return parsed;
    }

    // --------------------------------------------
    // LEN Parsing
    // --------------------------------------------
    inline size_t PARSE_LEN_FROM_FIRST_FIELD(const Field& first_field) {
        if (first_field.subfields.empty() ||
            first_field.subfields[0].items.empty()) {
            return 0U;
        }

        const std::string tag(first_field.tag.begin(), first_field.tag.end());

        if (!tag.ends_with(".001") && !tag.ends_with(".01")) {
            return 0U;
        }

        const auto& bytes = first_field.subfields[0].items[0].bytes;
        const std::string value(bytes.begin(), bytes.end());

        try {
            return std::stoul(value);
        } catch (...) {
            return 0U;
        }
    }

    // --------------------------------------------
    // Tagged Record Parsing
    // --------------------------------------------
    inline Record PARSE_RECORD(Cursor& cursor) {
        Record record;
        const size_t record_start = cursor.position();

        Field first_field = PARSE_FIELD(cursor, INVALID_INDEX);
        record.fields.push_back(first_field);

        if (!cursor.done() && cursor.peek() == GS) {
            cursor.advance();
        }

        size_t record_end = INVALID_INDEX;
        const size_t declared_length = PARSE_LEN_FROM_FIRST_FIELD(first_field);

        if (declared_length > 0U &&
            record_start + declared_length <= cursor.size()) {
            record_end = record_start + declared_length;
        }

        while (!cursor.done() && cursor.peek() != FS) {
            record.fields.push_back(PARSE_FIELD(cursor, record_end));

            if (!cursor.done() && cursor.peek() == GS) {
                cursor.advance();
            } else {
                break;
            }
        }

        return record;
    }

    // --------------------------------------------
    // Binary Record Parsing (LEN-Governed)
    // --------------------------------------------
    inline Record PARSE_BINARY_RECORD_BY_LEN(Cursor& cursor, int record_type) {
        if (cursor.remaining() < 4U) {
            throw std::runtime_error("Binary record EOF");
        }

        const uint32_t declared_length = READ_BE_U32(
            cursor.slice(cursor.position(), cursor.position() + 4U));

        if (declared_length < MIN_BINARY_RECORD_SIZE ||
            declared_length > cursor.remaining()) {
            throw std::runtime_error("Invalid binary LEN");
        }

        Record record;
        record.type = record_type;
        record.is_binary_record = true;

        const size_t start_position = cursor.position();
        cursor.advance(declared_length);

        auto bytes =
            cursor.slice(start_position, start_position + declared_length);
        record.raw_record.assign(bytes.begin(), bytes.end());

        record.idc = record.raw_record.size() > 4U ? record.raw_record[4U] : -1;

        return record;
    }

    // ---- Parse CNT (Type-1 1.003) ----
    inline std::vector<std::pair<int, int>> PARSE_CNT_PLAN(

        const Record& type1) {
        std::vector<std::pair<int, int>> plan;

        const Field* cnt = nullptr;
        for (const auto& field : type1.fields) {
            std::string tag(field.tag.begin(), field.tag.end());
            // Use ends_with or specific matches to be safe
            if (tag == "1.003" || tag == "1.03") {
                cnt = &field;
                break;
            }
        }

        if (cnt == nullptr) {
            return plan;
        }

        for (size_t i = 1; i < cnt->subfields.size(); i++) {
            const auto& subfield = cnt->subfields[i];
            if (subfield.items.size() < 2) {
                continue;
            }
            int tag = std::stoi(std::string(subfield.items[0].bytes.begin(),
                                            subfield.items[0].bytes.end()));
            int idc = std::stoi(std::string(subfield.items[1].bytes.begin(),
                                            subfield.items[1].bytes.end()));

            plan.emplace_back(tag, idc);
        }

        return plan;
    }

    // --------------------------------------------
    // File Parsing Helpers
    // --------------------------------------------
    inline void SKIP_SEPARATORS(Cursor& cursor) {
        while (!cursor.done() && (cursor.peek() == FS || cursor.peek() == GS)) {
            cursor.advance();
        }
    }

    inline void SYNC_TO_LEN_IF_PRESENT(Cursor& cursor, size_t record_start,
                                       const Record& record) {
        if (record.fields.empty()) {
            return;
        }
        const size_t declared_length =
            PARSE_LEN_FROM_FIRST_FIELD(record.fields.front());
        if (declared_length > 0U &&
            (record_start + declared_length) <= cursor.size()) {
            cursor.seek(record_start + declared_length);
        }
    }

    // --------------------------------------------
    // File Parsing (CNT Authoritative)
    // --------------------------------------------
    inline File PARSE_FILE(const std::vector<uint8_t>& buffer) {
        Cursor cursor{std::span<const uint8_t>(buffer)};
        File file;

        // Type-1
        const size_t type1_start = cursor.position();
        Record type_one = PARSE_RECORD(cursor);
        type_one.type = 1;
        type_one.idc = -1;
        SYNC_TO_LEN_IF_PRESENT(cursor, type1_start, type_one);
        SKIP_SEPARATORS(cursor);

        file.records.push_back(type_one);

        auto plan = PARSE_CNT_PLAN(type_one);

        for (auto& [record_type, idc] : plan) {
            if (cursor.remaining() < 4U) {
                break;
            }

            const size_t rec_start = cursor.position();
            Record record;

            if (IS_LEGACY_BINARY_TYPE(record_type)) {
                record = PARSE_BINARY_RECORD_BY_LEN(cursor, record_type);
            } else {
                record = PARSE_RECORD(cursor);
                SYNC_TO_LEN_IF_PRESENT(cursor, rec_start, record);
            }

            SKIP_SEPARATORS(cursor);

            record.type = record_type;
            record.idc = idc;
            file.records.push_back(std::move(record));
        }

        return file;
    }

}  // namespace nnist
