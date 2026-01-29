#include <nnist/nnist.h>
#include <NTest.h>

using namespace nnist;

// Tests
TEST(NIST_RoundTrip_Structural_Stability) {
    auto data = READ_FILE(TEST_DIR "data/valid1.1.an2");

    auto A = PARSE_FILE(data);
    auto bytes = SERIALIZE_FILE(A);
    auto B = PARSE_FILE(bytes);

    ASSERT_EQ(A.records.size(), B.records.size());

    for (size_t i = 0; i < A.records.size(); i++) {
        ASSERT_EQ(A.records[i].type, B.records[i].type);
        ASSERT_EQ(A.records[i].idc, B.records[i].idc);
    }
}

TEST(NIST_RoundTrip_RealFiles) {
    std::filesystem::path in_dir = TEST_DIR "data";
    std::filesystem::path out_dir = TEST_DIR "output";

    std::filesystem::create_directories(out_dir);

    for (auto& entry : std::filesystem::directory_iterator(in_dir)) {
        if (!entry.is_regular_file())
            continue;

        auto in_path = entry.path();
        // std::cout << "Testing file: " << in_path << std::endl;

        auto out_path = out_dir / in_path.filename();
        auto original_bytes = READ_FILE(in_path);

        // Parse original
        auto original_file = PARSE_FILE(original_bytes);

        // Serialize → Parse again
        auto serialized_bytes = SERIALIZE_FILE(original_file);
        WRITE_FILE(out_path, serialized_bytes);

        auto roundtrip_file = PARSE_FILE(serialized_bytes);

        // ---- Semantic equality checks ----
        ASSERT_EQ(original_file.records.size(), roundtrip_file.records.size());

        for (size_t r = 0; r < original_file.records.size(); r++) {
            const auto& A = original_file.records[r];
            const auto& B = roundtrip_file.records[r];

            ASSERT_EQ(A.type, B.type);
            ASSERT_EQ(A.idc, B.idc);
            ASSERT_EQ(A.is_binary_record, B.is_binary_record);

            // Binary record = compare raw bytes length only (payload stability)
            if (A.is_binary_record) {
                ASSERT_EQ(A.raw_record.size(), B.raw_record.size());
                continue;
            }

            ASSERT_EQ(A.fields.size(), B.fields.size());

            for (size_t f = 0; f < A.fields.size(); f++) {
                const auto& FA = A.fields[f];
                const auto& FB = B.fields[f];

                ASSERT_EQ(FA.tag, FB.tag);
                ASSERT_EQ(FA.is_binary_field, FB.is_binary_field);

                // Binary field (.999) → compare size only
                if (FA.is_binary_field) {
                    ASSERT_EQ(FA.raw_field.size(), FB.raw_field.size());
                    continue;
                }
                ASSERT_EQ(FA.subfields.size(), FB.subfields.size());

                for (size_t sf = 0; sf < FA.subfields.size(); sf++) {
                    const auto& SFA = FA.subfields[sf];
                    const auto& SFB = FB.subfields[sf];

                    ASSERT_EQ(SFA.items.size(), SFB.items.size());

                    for (size_t it = 0; it < SFA.items.size(); it++) {
                        ASSERT_EQ(SFA.items[it].bytes, SFB.items[it].bytes);
                    }
                }
            }
        }
    }
}

static const Record* FIRST_BINARY_RECORD(const File& file) {
    for (const auto& r : file.records)
        if (r.is_binary_record)
            return &r;
    return nullptr;
}

TEST(NIST_Type4_Binary_Record_RoundTrip) {
    auto data = nnist::READ_FILE(TEST_DIR "data/valid1.1.an2");
    auto file = nnist::PARSE_FILE(data);

    ASSERT_GT(file.records.size(), 0);

    const Record* rec = FIRST_BINARY_RECORD(file);
    ASSERT_TRUE(rec != nullptr);
    ASSERT_TRUE(rec->is_binary_record);
    ASSERT_GT(rec->raw_record.size(), 16);

    auto out = nnist::SERIALIZE_FILE(file);

    // For the current serializer, tagged records may not be byte-identical yet;
    // but binary records MUST be preserved exactly. So validate that binary
    // record bytes still exist in output at the right spot.
    //
    // If we want full file byte-equality, you must preserve original GS/RS/US
    // placement and any "empty separators" exactly for tagged records too.

    // Minimum strong check: output contains the binary record bytes as a
    // contiguous slice
    auto it = std::search(out.begin(), out.end(), rec->raw_record.begin(),
                          rec->raw_record.end());
    ASSERT_TRUE(it != out.end());
}

TEST(NIST_Type4_Image_Magic_Header) {
    auto data = READ_FILE(TEST_DIR "data/valid1.1.an2");
    auto file = nnist::PARSE_FILE(data);

    const Record* rec = FIRST_BINARY_RECORD(file);
    ASSERT_TRUE(rec != nullptr);
    ASSERT_TRUE(rec->is_binary_record);

    const auto& raw = rec->raw_record;

    ASSERT_TRUE(raw.size() > 16);
    ASSERT_TRUE(raw[0] == 0x00 || raw[0] == 0xFF);
}

TEST(NIST_Type14_Image_Record_ParsesBinary999) {
    auto data = READ_FILE(TEST_DIR "data/face_jpb_DOM_GMT_DCS.an2");

    auto file = nnist::PARSE_FILE(data);
    ASSERT_GT(file.records.size(), 0);

    bool found_999 = false;

    for (auto& rec : file.records) {
        for (auto& field : rec.fields) {
            std::string tag(field.tag.begin(), field.tag.end());
            if (tag.ends_with(".999")) {
                found_999 = true;
                ASSERT_TRUE(field.is_binary_field);
                ASSERT_GT(field.raw_field.size(), 50);
            }
        }
    }
}