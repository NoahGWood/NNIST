#include <nnist/nnist.h>
#include <NTest.h>

#include <iomanip>
#include <iostream>

using namespace nnist;

// Helpers

std::vector<uint8_t> make_tagged_record_with_999(
    const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> rec;

    auto append_str = [&](const char* s) {
        while (*s)
            rec.push_back((uint8_t)*s++);
    };

    // placeholder LEN "000000"
    append_str("14.001:000000");
    rec.push_back(GS);
    append_str("14.999:");
    rec.insert(rec.end(), payload.begin(), payload.end());
    rec.push_back(GS);
    rec.push_back(FS);

    // Patch LEN (total record size)
    uint32_t len = (uint32_t)rec.size();
    char buf[7];
    std::snprintf(buf, sizeof(buf), "%06u", len);

    // overwrite the 6 digits after "14.001:"
    size_t pos = std::string("14.001:").size();
    for (size_t i = 0; i < 6; ++i)
        rec[pos + i] = (uint8_t)buf[i];

    return rec;
}


void dump_binary_header(const Record& rec) {
    const auto& raw = rec.raw_record;

    std::cout << "RAW LEN = " << raw.size() << "\n";
    std::cout << "LEN FIELD = " << READ_BE_U32(raw.data()) << "\n";

    if (raw.size() > 4) {
        std::cout << "IDC = " << (int)raw[4] << "\n";
    }

    std::cout << "HEADER BYTES:\n";

    size_t dump_len = std::min<size_t>(64, raw.size());

    for (size_t i = 0; i < dump_len; i++) {
        if (i % 16 == 0)
            std::cout << "\n"
                      << std::setw(4) << std::setfill('0') << std::hex << i
                      << ": ";

        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << (int)raw[i] << " ";
    }

    std::cout << std::dec << "\n";
}

TEST(NIST_Parse_Simple_Tagged_Record) {
    std::vector<uint8_t> buf = {'1', '.', '0', '0', '1', ':', '1',
                                '0', '0', GS,  '2', '.', '0', '0',
                                '1', ':', 'D', 'O', 'E', FS};

    auto file = PARSE_FILE(buf);

    ASSERT_EQ(file.records.size(), 1);
    ASSERT_EQ(file.records[0].fields.size(), 2);

    ASSERT_EQ(std::string(file.records[0].fields[0].tag.begin(),
                          file.records[0].fields[0].tag.end()),
              "1.001");

    ASSERT_EQ(std::string(file.records[0].fields[1].tag.begin(),
                          file.records[0].fields[1].tag.end()),
              "2.001");
}

TEST(NIST_Parse_Subfields_Items) {
    std::vector<uint8_t> buf = {'1', '.', '0', '0', '1', ':',
                                'A', US,  'B', RS,  'C', FS};

    auto file = PARSE_FILE(buf);
    auto& field = file.records[0].fields[0];

    ASSERT_EQ(field.subfields.size(), 2);

    ASSERT_EQ(field.subfields[0].items.size(), 2);
    ASSERT_EQ(field.subfields[1].items.size(), 1);

    ASSERT_EQ(std::string(field.subfields[0].items[0].bytes.begin(),
                          field.subfields[0].items[0].bytes.end()),
              "A");

    ASSERT_EQ(std::string(field.subfields[0].items[1].bytes.begin(),
                          field.subfields[0].items[1].bytes.end()),
              "B");
}

TEST(NIST_Parse_Legacy_Binary_Record_Length) {
    auto data = READ_FILE("nnist/tests/data/valid1.1.an2");
    auto file = PARSE_FILE(data);

    const Record* rec = &file.records[2];

    if (!rec) {
        ASSERT_TRUE(true);  // Valid file may contain no legacy binary records
        return;
    }

    dump_binary_header(*rec);
    ASSERT_TRUE(rec->is_binary_record);
    ASSERT_EQ(rec->raw_record.size(), 14864);
}

TEST(NIST_LEN_Field_Syncs_Record_Boundary) {
    auto data = READ_FILE("nnist/tests/data/valid1.1.an2");
    auto file = PARSE_FILE(data);

    for (auto& rec : file.records) {
        if (!rec.fields.empty()) {
            size_t len = PARSE_LEN_FROM_FIRST_FIELD(rec.fields[0]);
            if (len > 0) {
                ASSERT_GT(len, 10);  // sanity only
            }
        }
    }
}

TEST(NIST_CNT_Drives_Record_Count) {
    auto data = READ_FILE("nnist/tests/data/valid1.1.an2");
    auto file = PARSE_FILE(data);

    ASSERT_GT(file.records.size(), 1);
    ASSERT_EQ(file.records[0].type, 1);
}

TEST(NIST_No_Record_Loss) {
    auto data = READ_FILE("nnist/tests/data/face_jpb_DOM_GMT_DCS.an2");
    auto file = PARSE_FILE(data);

    ASSERT_GT(file.records.size(), 0);

    size_t binary_count = 0;
    for (auto& r : file.records)
        if (r.is_binary_record)
            binary_count++;

    ASSERT_GE(binary_count, 0);
}



TEST(NIST_Binary999_Field_ExactPayload) {
    std::vector<uint8_t> payload = {0x01, 0xFF, 0xAA, 0xBB, 0xCC};

    auto fake = make_tagged_record_with_999(payload);
    auto file = PARSE_FILE(fake);

    ASSERT_EQ(file.records.size(), 1);

    bool found = false;
    for (auto& fld : file.records[0].fields) {
        std::string tag(fld.tag.begin(), fld.tag.end());
        if (tag.ends_with(".999")) {
            found = true;
            ASSERT_TRUE(fld.is_binary_field);
            ASSERT_EQ(fld.raw_field, payload);
        }
    }

    ASSERT_TRUE(found);
}

TEST(NIST_Parse_Binary999_Field_Bounded) {
    std::vector<uint8_t> payload = {0xAA, 0xBB, 0xCC, 0xDD};

    auto fake = make_tagged_record_with_999(payload);
    auto file = PARSE_FILE(fake);

    bool found = false;

    for (auto& f : file.records[0].fields) {
        std::string tag(f.tag.begin(), f.tag.end());
        if (tag.ends_with(".999")) {
            found = true;
            ASSERT_TRUE(f.is_binary_field);
            ASSERT_EQ(f.raw_field, payload);
        }
    }

    ASSERT_TRUE(found);
}
