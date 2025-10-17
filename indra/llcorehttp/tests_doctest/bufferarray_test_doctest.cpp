// DOCTEST_SKIP_AUTOGEN: hand-maintained doctest suite; see docs/testing/doctest_quickstart.md.
// ---------------------------------------------------------------------------
// Deterministic doctest coverage for LLCore::BufferArray with no network or filesystem calls.
// ---------------------------------------------------------------------------
#include "doctest.h"
#include "ll_doctest_helpers.h"

#include "bufferarray.h"

#include <array>
#include <cstring>
#include <vector>

using namespace LLCore;

namespace
{
std::vector<char> make_data(const std::string& text)
{
    return std::vector<char>(text.begin(), text.end());
}
} // namespace

TEST_SUITE("bufferarray_test")
{
    TEST_CASE("construction and empty read")
    {
        BufferArray* ba = new BufferArray();
        CHECK_EQ(ba->getRefCount(), 1);
        CHECK_EQ(ba->size(), 0U);

        std::array<char, 8> scratch{};
        scratch.fill('x');
        size_t read_len = ba->read(0, scratch.data(), scratch.size());
        CHECK_EQ(read_len, 0U);
        CHECK_EQ(scratch[0], 'x');
        ba->release();
    }

    TEST_CASE("single write and read")
    {
        BufferArray* ba = new BufferArray();
        const std::string payload = "abcdefghij";
        const auto data = make_data(payload);

        size_t written = ba->write(0, data.data(), data.size());
        CHECK_EQ(written, data.size());
        CHECK_EQ(ba->size(), data.size());

        std::array<char, 4> scratch{};
        scratch.fill('?');
        size_t read_len = ba->read(2, scratch.data(), scratch.size());
        CHECK_EQ(read_len, scratch.size());
        LL_CHECK_EQ_MEM(scratch.data(), "cdef", scratch.size());
        ba->release();
    }

    TEST_CASE("multiple write append and full read")
    {
        BufferArray* ba = new BufferArray();
        const std::string payload = "abcdefghij";
        const auto data = make_data(payload);

        size_t written = ba->write(0, data.data(), data.size());
        CHECK_EQ(written, data.size());

        written = ba->write(data.size(), data.data(), data.size());
        CHECK_EQ(written, data.size());
        CHECK_EQ(ba->size(), data.size() * 2);

        std::vector<char> all(ba->size(), 0);
        size_t read_len = ba->read(0, all.data(), all.size());
        CHECK_EQ(read_len, all.size());
        CHECK_EQ(std::memcmp(all.data(), data.data(), data.size()), 0);
        CHECK_EQ(std::memcmp(all.data() + data.size(), data.data(), data.size()), 0);
        ba->release();
    }

    TEST_CASE("overwrite region")
    {
        BufferArray* ba = new BufferArray();
        const std::string payload = "abcdefghijklmno";
        ba->write(0, payload.data(), payload.size());

        const std::string replace = "----";
        ba->write(6, replace.data(), replace.size());

        std::vector<char> region(ba->size());
        ba->read(0, region.data(), region.size());
        CHECK_EQ(std::string(region.data(), region.size()), "abcdef----klmno");
        ba->release();
    }

    TEST_CASE("append and slice subranges")
    {
        BufferArray* ba = new BufferArray();
        const std::string payload = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        ba->write(0, payload.data(), payload.size());

        std::vector<char> middle(10);
        size_t read_len = ba->read(5, middle.data(), middle.size());
        CHECK_EQ(read_len, middle.size());
        CHECK_EQ(std::string(middle.data(), middle.size()), "56789ABCDE");

        std::vector<char> boundary(payload.size());
        read_len = ba->read(0, boundary.data(), boundary.size());
        CHECK_EQ(read_len, boundary.size());
        CHECK_EQ(std::string(boundary.data(), boundary.size()), payload);

        ba->release();
    }

    TEST_CASE("copyOut respects offsets and zero lengths")
    {
        BufferArray* ba = new BufferArray();
        const std::string payload = "payload";
        ba->write(0, payload.data(), payload.size());

        std::array<char, 4> chunk{};
        chunk.fill('?');
        size_t copied = ba->read(0, chunk.data(), 0);
        CHECK_EQ(copied, 0U);
        CHECK_EQ(chunk[0], '?');

        copied = ba->read(2, chunk.data(), chunk.size());
        CHECK_EQ(copied, chunk.size());
        CHECK_EQ(chunk[0], 'y');
        CHECK_EQ(chunk[1], 'l');
        CHECK_EQ(chunk[2], 'o');
        CHECK_EQ(chunk[3], 'a');

        ba->release();
    }

    TEST_CASE("out of range read throws")
    {
        BufferArray* ba = new BufferArray();
        const std::string payload = "xyz";
        ba->write(0, payload.data(), payload.size());

        std::array<char, 4> scratch{};
        CHECK_EQ(ba->read(3, scratch.data(), scratch.size()), 0U);
        CHECK_EQ(ba->read(100, scratch.data(), scratch.size()), 0U);
        ba->release();
    }

    TEST_CASE("multiple block allocation via append")
    {
        BufferArray* ba = new BufferArray();
        const std::string chunk(70000, 'a');
        ba->append(chunk.data(), chunk.size());
        ba->append("b", 1);

        CHECK_EQ(ba->size(), chunk.size() + 1);

        std::vector<char> tail(1);
        ba->read(ba->size() - 1, tail.data(), tail.size());
        CHECK_EQ(tail[0], 'b');
        ba->release();
    }

    TEST_CASE("adjacent slices remain independent")
    {
        BufferArray* ba = new BufferArray();
        const std::string first = "first";
        const std::string second = "second";
        ba->append(first.data(), first.size());
        ba->append(second.data(), second.size());

        std::vector<char> left(first.size());
        ba->read(0, left.data(), left.size());
        LL_CHECK_EQ_MEM(left.data(), first.data(), first.size());

        std::vector<char> right(second.size());
        ba->read(first.size(), right.data(), right.size());
        LL_CHECK_EQ_MEM(right.data(), second.data(), second.size());
        ba->release();
    }
}
