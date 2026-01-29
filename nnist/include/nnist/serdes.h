#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "nnist/parsers.h"
#include "nnist/types.h"
namespace nnist {
    inline void SERIALIZE_ITEM(std::vector<uint8_t>& out, const Item& item,
                               bool end = false) {
        out.insert(out.end(), item.bytes.begin(), item.bytes.end());
        if (!end) {
            out.push_back(US);
        }
    }

    inline void SERIALIZE_SUBFIELD(std::vector<uint8_t>& out,
                                   const Subfield& subfield, bool end = false) {
        for (size_t i = 0; i < subfield.items.size(); i++) {
            SERIALIZE_ITEM(out, subfield.items[i],
                           i + 1 == subfield.items.size());
        }
        if (!end) {
            out.push_back(RS);
        }
    }

    inline void SERIALIZE_FIELD(std::vector<uint8_t>& out, const Field& field,
                                bool end = false) {
        out.insert(out.end(), field.tag.begin(), field.tag.end());
        out.push_back(':');

        if (!field.is_binary_field) {
            for (size_t i = 0; i < field.subfields.size(); i++) {
                SERIALIZE_SUBFIELD(out, field.subfields[i],
                                   i + 1 == field.subfields.size());
            }
            if (!end) {
                out.push_back(GS);
            }
        } else {
            out.insert(out.end(), field.raw_field.begin(),
                       field.raw_field.end());
        }
    }

    inline void SERIALIZE_RECORD(std::vector<uint8_t>& out, const Record& rec) {
        if (rec.is_binary_record) {
            out.insert(out.end(), rec.raw_record.begin(), rec.raw_record.end());
            // Standard says: no FS after legacy binary records unless required
            // by specific implementation
            return;
        }

        for (size_t i = 0; i < rec.fields.size(); i++) {
            SERIALIZE_FIELD(out, rec.fields[i], i + 1 == rec.fields.size());
        }
        out.push_back(FS);  // Every tagged record ends with FS
    }

    inline void UPDATE_CNT_FIELD(File& file) {
        if (file.records.empty()) {
            return;
        }
        Record& type1 = file.records[0];

        // Find field 1.003 or 1.03
        Field* cnt_field = nullptr;
        for (auto& field : type1.fields) {
            std::string tag(field.tag.begin(), field.tag.end());
            if (tag.ends_with(".003") || tag.ends_with(".03")) {
                cnt_field = &field;
                break;
            }
        }
        if (cnt_field == nullptr) {
            return;
        }

        // Rebuild subfields: first subfield is the record count (total records)
        cnt_field->subfields.clear();

        // Total number of records (including Type-1)
        std::string total_count = std::to_string(file.records.size());
        cnt_field->subfields.push_back(
            {{{{total_count.begin(), total_count.end()}}}});

        // Subsequent subfields: [Type, IDC]
        for (size_t i = 1; i < file.records.size(); ++i) {
            const auto& rec = file.records[i];
            Subfield subfield;
            std::string t_str = std::to_string(rec.type);
            std::string idc_str = std::to_string(rec.idc >= 0 ? rec.idc : 0);
            subfield.items.push_back({{t_str.begin(), t_str.end()}});
            subfield.items.push_back({{idc_str.begin(), idc_str.end()}});
            cnt_field->subfields.push_back(subfield);
        }
    }
    inline void UPDATE_ALL_LENGTHS(File& file) {
        for (auto& rec : file.records) {
            if (rec.is_binary_record) {
                auto size = (uint32_t)rec.raw_record.size();
                rec.raw_record[BYTE_INDEX_0] = (size >> SHIFT_24) & BYTE_MASK;
                rec.raw_record[BYTE_INDEX_1] = (size >> SHIFT_16) & BYTE_MASK;
                rec.raw_record[BYTE_INDEX_2] = (size >> SHIFT_8) & BYTE_MASK;
                rec.raw_record[BYTE_INDEX_3] = size & BYTE_MASK;
            } else {
                // Update tagged record .001
                if (rec.fields.empty()) {
                    continue;
                }
                Field* first_field = rec.fields.data();

                first_field->subfields = {{{{std::vector<uint8_t>(
                    TAGGED_LEN_WIDTH, static_cast<uint8_t>('0'))}}}};

                auto calc = [&]() {
                    std::vector<uint8_t> tmp;
                    for (size_t i = 0; i < rec.fields.size(); i++) {
                        SERIALIZE_FIELD(tmp, rec.fields[i],
                                        i + 1 == rec.fields.size());
                    }
                    tmp.push_back(FS);
                    return tmp.size();
                };

                std::string str = std::to_string(calc());
                first_field->subfields[0].items[0].bytes = {str.begin(),
                                                            str.end()};
                // Double-pass for string length changes
                str = std::to_string(calc());
                first_field->subfields[0].items[0].bytes = {str.begin(),
                                                            str.end()};
            }
        }
    }
    inline void PATCH_RECORD_LENGTHS(Record& rec) {
        if (rec.is_binary_record) {
            return;
        }
        // 1. Find the length field (ending in .001 or .01)
        Field* len_field = nullptr;
        for (auto& field : rec.fields) {
            std::string tag(field.tag.begin(), field.tag.end());
            if (tag.ends_with(".001") || tag.ends_with(".01")) {
                len_field = &field;
                break;
            }
        }
        if (len_field == nullptr) {
            return;
        }
        // 2. Clear current length and put a placeholder
        len_field->subfields = {{{{std::vector<uint8_t>(
            LEN_FIELD_WIDTH, static_cast<uint8_t>('0'))}}}};

        // 3. Simple trick: Serialize to find current size
        std::vector<uint8_t> tmp;
        SERIALIZE_RECORD(tmp, rec);

        // 4. Update the field with the real size
        std::string new_len = std::to_string(tmp.size());
        len_field->subfields[0].items[0].bytes = {new_len.begin(),
                                                  new_len.end()};
    }

    inline std::vector<uint8_t> SERIALIZE_FILE(File& file) {
        UPDATE_ALL_LENGTHS(file);
        std::vector<uint8_t> out;
        for (auto& rec : file.records) {
            // Update record size
            SERIALIZE_RECORD(out, rec);
        }
        return out;
    }

    // ---- File IO ----
    inline std::vector<uint8_t> READ_FILE(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + path.string());
        }
        return {std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()};
    }

    inline void WRITE_FILE(const std::filesystem::path& path,
                           const std::vector<uint8_t>& data) {
        std::ofstream file(path, std::ios::binary);
        const auto byte_count = static_cast<std::streamsize>(data.size());

        file.write(reinterpret_cast<const char*>(data.data()), byte_count);
    }
}  // namespace nnist
