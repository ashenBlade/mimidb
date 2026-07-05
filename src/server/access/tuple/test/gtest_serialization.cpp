#include "gtest/gtest.h"
#include "access/TupleDescriptor.hpp"
#include "access/tuple/TupleSerializer.hpp"
#include "db/builtin/int.hpp"
#include "db/builtin/text.hpp"
#include "db/catalog/TypeId.hpp"
#include "executor/Datum.hpp"
#include "executor/func/FunctionArgs.hpp"
#include "executor/func/MIFunction.hpp"
#include "utils/DatumArray.hpp"

#include <cstdint>
#include <functional>
#include <vector>

using namespace mi::access;
using namespace mi::access::tuple;
using namespace mi::db::builtin;


static AttributeDescriptor attr_int64() {
    return AttributeDescriptor{mi::schema::catalog::TypeId::Int64, sizeof(uint64_t), true};
}

static AttributeDescriptor attr_int32() {
    return AttributeDescriptor{mi::schema::catalog::TypeId::Int32, sizeof(uint32_t), true};
}

static AttributeDescriptor attr_int16() {
    return AttributeDescriptor{mi::schema::catalog::TypeId::Int16, sizeof(uint16_t), true};
}

static AttributeDescriptor attr_str() {
    return AttributeDescriptor{mi::schema::catalog::TypeId::Text, -1, false};
}

struct TestTypeInfo {
    mi::executor::MIFunction Equality;
    std::function<std::string(mi::Datum)> Output;
};

static TestTypeInfo tti_int64() {
    return TestTypeInfo{Int64Eq, Int64Output};
}


static TestTypeInfo tti_int32() {
    return TestTypeInfo{Int32Eq, Int32Output};
}


static TestTypeInfo tti_int16() {
    return TestTypeInfo{Int32Eq, Int32Output};
}

static TestTypeInfo tti_text() {
    return TestTypeInfo{TextEq, TextOutput};
}

template<typename T, typename ...Rest>
inline constexpr bool all_same = (std::is_same_v<T, Rest> && ...);

template<typename ...Args>
static TupleDescriptor make_tupdesc(Args ...args) {
    static_assert(all_same<Args...>, "All arguments must be of the same type");
    auto attrs = std::vector<typename std::common_type<Args...>::type>{static_cast<typename std::common_type<Args...>::type>(args)...};
    return TupleDescriptor{std::move(attrs)};
}

template<>
TupleDescriptor make_tupdesc() {
    return TupleDescriptor{{}};
}

static void test_common(const TupleDescriptor &tupdesc,
                        const std::vector<mi::Datum> &values,
                        const std::vector<bool> &isnull,
                        const std::vector<TestTypeInfo> &tti) {
    auto serializer = TupleSerializer{&tupdesc};

    auto info = serializer.Serialize(values, isnull);
    auto result = serializer.Deserialize(info.Tuple.data(), info.HasNulls, info.Tuple.size());

    auto &resultValues = result.Values;
    auto &resultIsnull = result.IsNull;

    // Common check for sanity
    EXPECT_EQ(resultValues.size(), values.size());
    EXPECT_EQ(resultIsnull.size(), isnull.size());

    for (size_t i = 0; i < values.size(); i++) {
        EXPECT_EQ(resultIsnull[i], isnull[i]);

        if (isnull[i]) {
            continue;
        }

        auto args = mi::executor::FunctionArgs{
            mi::DatumArray{
                {resultValues[i], values[i]},
                {false, false}
            }
        };
        auto &info = tti[i];
        auto cmpresult = info.Equality(args);
        EXPECT_TRUE(cmpresult.value().getScalar<bool>()) << "Value " << info.Output(resultValues[i]) << " must be equal to " << info.Output(values[i]);
    }
}

TEST(TupleSerializer, Empty) {
    test_common(make_tupdesc(), {}, {}, {});
}

TEST(TupleSerializer, Int32) {
    auto desc = make_tupdesc(attr_int32());
    test_common(desc, {mi::Datum{1}}, {false}, {tti_int32()});
}

TEST(TupleSerializer, Int32Int16) {
    auto desc = make_tupdesc(attr_int32(), attr_int16());
    auto values = std::vector<mi::Datum>{mi::Datum{INT32_MAX - 1}, mi::Datum{INT16_MAX - 1}};
    auto isnull = std::vector<bool>{false, false};
    auto eq = std::vector{tti_int32(), tti_int16()};
    test_common(desc, values, isnull, eq);
}

TEST(TupleSerializer, Int32Int16Int64) {
    auto desc = make_tupdesc(attr_int32(), attr_int16(), attr_int64());
    auto values = std::vector<mi::Datum>{mi::Datum{INT32_MAX - 1}, mi::Datum{INT16_MAX - 1}, mi::Datum{INT64_MAX - 1}};
    auto isnull = std::vector<bool>{false, false, false};
    auto eq = std::vector{tti_int32(), tti_int16(), tti_int64()};
    test_common(desc, values, isnull, eq);
}

TEST(TupleSerializer, Text) {
    auto desc = make_tupdesc(attr_str());
    auto text = mi::db::builtin::Text::FromCString("hello");
    test_common(desc, {mi::Datum{text.get()}}, {false}, {tti_text()});
}

TEST(TupleSerializer, TextText) {
    auto desc = make_tupdesc(attr_str(), attr_str());
    auto t1 = mi::db::builtin::Text::FromCString("hello");
    auto t2 = mi::db::builtin::Text::FromCString("world");
    test_common(desc, {mi::Datum{t1.get()}, mi::Datum{t2.get()}}, {false, false}, {tti_text(), tti_text()});
}

TEST(TupleSerializer, Int32NULL) {
    auto desc = make_tupdesc(attr_int32());
    test_common(desc, {mi::Datum{0}}, {true}, {tti_int32()});
}

TEST(TupleSerializer, Int32Int16NULL) {
    auto desc = make_tupdesc(attr_int32(), attr_int16());
    test_common(desc, {mi::Datum{1}, mi::Datum{0}}, {false, true}, {tti_int32(), tti_int16()});
}

TEST(TupleSerializer, Int32NULLInt16) {
    auto desc = make_tupdesc(attr_int32(), attr_int16());
    test_common(desc, {mi::Datum{0}, mi::Datum{1}}, {true, false}, {tti_int32(), tti_int16()});
}

TEST(TupleSerializer, Int32NULLInt16NULL) {
    auto desc = make_tupdesc(attr_int32(), attr_int16());
    test_common(desc, {mi::Datum{0}, mi::Datum{0}}, {true, true}, {tti_int32(), tti_int16()});
}

TEST(TestSerializer, Int32Text) {
    auto desc = make_tupdesc(attr_int32(), attr_str());
    auto text = mi::db::builtin::Text::FromCString("hello");
    auto i = mi::Datum{2};
    test_common(desc, {i, mi::Datum{text.get()}}, {false, false}, {tti_int32(), tti_text()});
}

TEST(TestSerializer, TextInt32) {
    
    auto desc = make_tupdesc(attr_str(), attr_int32());
    auto text = mi::db::builtin::Text::FromCString("hello");
    auto i = mi::Datum{2};
    test_common(desc, {mi::Datum{text.get()}, i}, {false, false}, {tti_text(), tti_int32()});
}

TEST(TestSerializer, Int32TextInt64) {
    auto desc = make_tupdesc(attr_int32(), attr_str(), attr_int64());
    auto text = mi::db::builtin::Text::FromCString("hello");
    auto i32 = mi::Datum{2};
    auto i64 = mi::Datum{64};
    test_common(desc, {i32, mi::Datum{text.get()}, i64}, {false, false, false}, {tti_int32(), tti_text(), tti_int64()});
}
