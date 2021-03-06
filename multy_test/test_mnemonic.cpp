/* Copyright 2018 by Multy.io
 * Licensed under Multy.io license
 *
 * See LICENSE for details
 */

#include "multy_core/mnemonic.h"

#include "multy_core/src/hash.h"

#include "multy_test/bip39_test_cases.h"
#include "multy_test/utility.h"
#include "multy_test/value_printers.h"

#include "gtest/gtest.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <string.h>

namespace
{
using namespace multy_core::internal;
using namespace test_utility;
typedef std::vector<unsigned char> bytes;

struct MnemonicTestCase
{
    const bytes entropy;
    const std::string expected_mnemonic;
    const bytes expected_seed;

    explicit MnemonicTestCase(
            const char* entropy,
            const char* expected_mnemonic,
            const char* expected_seed)
        : entropy(from_hex(entropy)),
          expected_mnemonic(expected_mnemonic),
          expected_seed(from_hex(expected_seed))
    {
    }

    explicit MnemonicTestCase(const BIP39TestCase& bip39_test_case)
        : MnemonicTestCase(
                  bip39_test_case.entropy,
                  bip39_test_case.mnemonic,
                  bip39_test_case.seed)
    {
    }
};

class MnemonicTestValidCasesP : public ::testing::TestWithParam<BIP39TestCase>
{
};

} // namespace

INSTANTIATE_TEST_CASE_P(
        BIP39,
        MnemonicTestValidCasesP,
        ::testing::ValuesIn(BIP39_DEFAULT_TEST_CASES));

TEST_P(MnemonicTestValidCasesP, Test)
{
    const BIP39TestCase& param = GetParam();
    const bytes entropy(from_hex(param.entropy));
    const std::string expected_mnemonic(param.mnemonic);
    const bytes expected_seed(from_hex(param.seed));

    ConstCharPtr mnemonic_str;
    ErrorPtr error;

    ASSERT_EQ(nullptr, mnemonic_str);
    ASSERT_EQ(nullptr, error);

    auto fill_entropy = [](void* data, ::size_t size, void* dest) -> ::size_t {
        const bytes* entropy = (const bytes*)(data);
        const size_t result_size = std::min(size, entropy->size());
        memcpy(dest, entropy->data(), result_size);
        return result_size;
    };
    auto entropy_source = EntropySource{(void*)&entropy, fill_entropy};
    HANDLE_ERROR(make_mnemonic(entropy_source, reset_sp(mnemonic_str)));
    EXPECT_NE(nullptr, mnemonic_str);

    EXPECT_STREQ(expected_mnemonic.c_str(), mnemonic_str.get());

    BinaryDataPtr seed;
    HANDLE_ERROR(make_seed(expected_mnemonic.c_str(), "TREZOR", reset_sp(seed)));
    ASSERT_NE(nullptr, seed);
    EXPECT_EQ(as_binary_data(expected_seed), *seed);

    ConstCharPtr dictionary;
    HANDLE_ERROR(mnemonic_get_dictionary(reset_sp(dictionary)));
    ASSERT_NE(nullptr, dictionary);
    ASSERT_NE(0, strlen(dictionary.get()));
}

GTEST_TEST(MnemonicTest, empty_null_password)
{
    ConstCharPtr mnemonic_str;

    HANDLE_ERROR(
            make_mnemonic(make_dummy_entropy_source(), reset_sp(mnemonic_str)));
    ASSERT_NE(nullptr, mnemonic_str);

    BinaryDataPtr empty_pass_seed;
    HANDLE_ERROR(make_seed(mnemonic_str.get(), "", reset_sp(empty_pass_seed)));
    ASSERT_NE(nullptr, empty_pass_seed);

    BinaryDataPtr null_pass_seed;
    HANDLE_ERROR(
            make_seed(mnemonic_str.get(), nullptr, reset_sp(null_pass_seed)));
    ASSERT_NE(nullptr, null_pass_seed);

    ASSERT_EQ(*null_pass_seed, *empty_pass_seed);
}

GTEST_TEST(MnemonicTest, mnemonic_get_dictionary)
{
    ConstCharPtr dictionary;
    HANDLE_ERROR(mnemonic_get_dictionary(reset_sp(dictionary)));

    auto dictionary_hash = do_hash<SHA3, 256>(dictionary.get());
    const bytes expected_hash = from_hex(
            "fca3543969cb6a75a90f898669c89a5ec85215a09d97bcad71ab6e7fd5d560b4");
    ASSERT_EQ(as_binary_data(expected_hash), as_binary_data(dictionary_hash));
}

GTEST_TEST(MnemonicTestInvalidArgs, make_mnemonic)
{
    ConstCharPtr mnemonic_str;

    EXPECT_ERROR(
            make_mnemonic(
                    EntropySource{nullptr, nullptr}, reset_sp(mnemonic_str)));
    EXPECT_EQ(nullptr, mnemonic_str);

    EXPECT_ERROR(make_mnemonic(make_dummy_entropy_source(), nullptr));
}

GTEST_TEST(MnemonicTestInvalidArgs, make_seed)
{
    BinaryDataPtr binary_data;

    EXPECT_ERROR(make_seed(nullptr, "pass", reset_sp(binary_data)));
    EXPECT_EQ(nullptr, binary_data);

    EXPECT_ERROR(make_seed("mnemonic", nullptr, reset_sp(binary_data)));
    EXPECT_EQ(nullptr, binary_data);

    EXPECT_ERROR(make_seed("mnemonic", "pass", nullptr));

    // Invalid mnemonic value
    EXPECT_ERROR(make_seed("mnemonic", "pass", reset_sp(binary_data)));
    EXPECT_EQ(nullptr, binary_data);
}

GTEST_TEST(MnemonicTestInvalidArgs, seed_to_string)
{
    unsigned char data_vals[] = {1U, 2U, 3U, 4U};
    const BinaryData data{data_vals, 3};
    const BinaryData null_data{nullptr, 0};
    const BinaryData zero_len_data{nullptr, 0};

    ConstCharPtr seed_str;

    EXPECT_ERROR(seed_to_string(nullptr, reset_sp(seed_str)));
    EXPECT_EQ(nullptr, seed_str);

    EXPECT_ERROR(seed_to_string(&data, nullptr));

    EXPECT_ERROR(seed_to_string(&null_data, reset_sp(seed_str)));
    EXPECT_EQ(nullptr, seed_str);

    EXPECT_ERROR(seed_to_string(&zero_len_data, reset_sp(seed_str)));
    EXPECT_EQ(nullptr, seed_str);
}

GTEST_TEST(MnemonicTestInvalidArgs, mnemonic_get_dictionary)
{
    EXPECT_ERROR(mnemonic_get_dictionary(nullptr));
}
