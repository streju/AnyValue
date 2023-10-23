#include <gtest/gtest.h>

#include "AnyValue.h"
#include <array>
#include <map>
#include <set>
#include <string>
#include <typeinfo>
#include <vector>

using namespace testing;
using uint_16 = unsigned short;
using uint_8 = unsigned char;

namespace tests
{

namespace
{

template <typename TestDataType>
struct AnyValueShould : public Test
{
};

template <typename T>
struct TestDataBase
{
    using Type = T;
    const std::type_info &type{typeid(Type)};
};

template <typename T, T... values>
struct TestData : TestDataBase<T>
{
    std::vector<typename TestDataBase<T>::Type> mVals{values...};
};

struct TestDataStr : TestDataBase<std::string>
{
    std::vector<typename TestDataBase<std::string>::Type> mVals{"dummy-1", "dummy-2", "A", "BCD"};
};

struct TestDataConstCharPtr
{
    using Type = const char *;
    const std::type_info &type{typeid(std::string)};
    std::vector<Type> mVals{"dummy-1", "dummy-2", "A", "BCD"};
};

struct DummyComparable
{
    bool operator==(const DummyComparable &) const { return true; }

    bool operator!=(const DummyComparable &) const { return false; }
};

} // namespace

using types =
    testing::Types<TestData<int, 1, 6, -8, 923>, TestData<bool, true, false>, TestDataStr, TestDataConstCharPtr>;

TYPED_TEST_CASE(AnyValueShould, types);

TYPED_TEST(AnyValueShould, Construct)
{
    TypeParam params;
    using T = typename TypeParam::Type;

    for (const T &val : params.mVals)
    {
        AnyValue value(val);
        EXPECT_TRUE(value.GetType() == params.type);
    }
}

TYPED_TEST(AnyValueShould, Compare)
{
    TypeParam params;
    using T = typename TypeParam::Type;

    for (const T &val1 : params.mVals)
    {
        AnyValue value1(val1);
        for (const T &val2 : params.mVals)
        {
            AnyValue value2(val2);
            if (val1 != val2)
            {
                EXPECT_NE(value1, value2);
                EXPECT_NE(value1, val2);
            }
            else
            {
                EXPECT_EQ(value1, value2);
                EXPECT_EQ(value1, val2);
            }
        }
    }
}

TYPED_TEST(AnyValueShould, Cast)
{
    TypeParam params;
    using T = typename std::conditional<std::is_same<const char *, typename TypeParam::Type>::value,
                                        std::string,
                                        typename TypeParam::Type>::type;

    for (const typename TypeParam::Type &val : params.mVals)
    {
        AnyValue value(val);
        EXPECT_EQ(value.Get<T>(), val);
        EXPECT_EQ(value, value.Get<T>());
    }
}

TEST(AnyValueShould, ThrowWhenAccessingWrongValueType)
{
    AnyValue value{uint_8(3)};
    EXPECT_THROW(value.Get<bool>(), AnyValueException);
    EXPECT_THROW(value.Get<std::string>(), AnyValueException);
    EXPECT_THROW(value.Get<const char *>(), AnyValueException);
    EXPECT_THROW(value.Get<int>(), AnyValueException);
    EXPECT_THROW(value.Get<unsigned>(), AnyValueException);
    EXPECT_THROW(value.Get<float>(), AnyValueException);
    EXPECT_THROW(value.Get<uint_16>(), AnyValueException);
    EXPECT_NO_THROW(value.Get<uint_8>());
}

TEST(AnyValueShould, setDifferentTypes)
{
    AnyValue any{"some c_str"};

    EXPECT_THROW(any.Get<const char *>(), AnyValueException);
    EXPECT_NO_THROW(any.AssertType<std::string>());
    EXPECT_EQ(any.GetType(), typeid(std::string));
    EXPECT_EQ(any.Get<std::string>(), "some c_str");

    EXPECT_NO_THROW(any.Set<bool>(true));

    EXPECT_THROW(any.Get<std::string>(), AnyValueException);
    EXPECT_NO_THROW(any.AssertType<bool>());
    EXPECT_EQ(any.GetType(), typeid(bool));
    EXPECT_EQ(any.Get<bool>(), true);

    EXPECT_NO_THROW(any.Set<std::set<AnyValue>>({}));
    EXPECT_THROW(any.Get<bool>(), AnyValueException);
    EXPECT_NO_THROW(any.AssertType<std::set<AnyValue>>());
    EXPECT_EQ(any.GetType(), typeid(std::set<AnyValue>));
    EXPECT_TRUE(any.Get<std::set<AnyValue>>().empty());

    any = '1';
    EXPECT_THROW(any.Get<std::set<AnyValue>>(), AnyValueException);
    EXPECT_NO_THROW(any.AssertType<char>());
    EXPECT_EQ(any.GetType(), typeid(char));
    EXPECT_EQ(any.Get<char>(), '1');

    AnyValue val;
    any = val;

    EXPECT_THROW(any.Get<char>(), AnyValueException);
    EXPECT_NO_THROW(any.AssertType<nullptr_t>());
    EXPECT_EQ(any.GetType(), typeid(nullptr_t));
    EXPECT_EQ(any.Get<nullptr_t>(), nullptr);
}

TEST(AnyValueShould, testCustomStructs)
{
    struct Dummy_1 : DummyComparable
    {
        float f{23.3};
        std::string s;
    };
    struct Dummy_2 : DummyComparable
    {
        uint_16 u{22};
    };

    AnyValue val;

    EXPECT_NO_THROW(val.AssertType<nullptr_t>());
    EXPECT_EQ(val.GetType(), typeid(nullptr_t));
    EXPECT_EQ(val.Get<nullptr_t>(), nullptr);

    std::vector<AnyValue> vec;
    vec.emplace_back(Dummy_1());
    vec.emplace_back(Dummy_2());
    val = vec;

    EXPECT_EQ(val.Get<std::vector<AnyValue>>().size(), size_t(2));
    EXPECT_EQ(val.Get<std::vector<AnyValue>>()[0].Get<Dummy_1>().f, float(23.3));
    EXPECT_EQ(val.Get<std::vector<AnyValue>>()[1].Get<Dummy_2>().u, uint_16(22));
}

TEST(AnyValueShould, storeDifferentStlContainers)
{
    std::array<int, 3> arr = {2, 33, 41};
    AnyValue any{arr};

    const auto &arrToCheck = any.Get<std::array<int, 3>>();
    EXPECT_EQ(arrToCheck[0], 2);
    EXPECT_EQ(arrToCheck[1], 33);
    EXPECT_EQ(arrToCheck[2], 41);

    std::map<std::string, double> map;
    map.emplace(std::make_pair("dummy_1", 8.34));
    map.emplace(std::make_pair("dummy_2", 4.38));
    map.emplace(std::make_pair("dummy_3", 8.43));

    any.Set<std::map<std::string, double>>(map);

    const auto &mapToCheck = any.Get<std::map<std::string, double>>();
    EXPECT_EQ(mapToCheck.at("dummy_1"), 8.34);
    EXPECT_EQ(mapToCheck.at("dummy_2"), 4.38);
    EXPECT_EQ(mapToCheck.at("dummy_3"), 8.43);

    std::vector<AnyValue> vec;
    vec.push_back(AnyValue(44));
    vec.push_back(AnyValue("c_str"));
    vec.push_back(AnyValue(true));
    vec.push_back(AnyValue("std string"));
    vec.push_back(AnyValue(22.37));
    vec.push_back(AnyValue(DummyComparable()));

    any = vec;

    const auto &vecToCheck = any.Get<std::vector<AnyValue>>();
    EXPECT_EQ(vecToCheck[0].Get<int>(), 44);
    EXPECT_EQ(vecToCheck[1].Get<std::string>(), "c_str");
    EXPECT_EQ(vecToCheck[2].Get<bool>(), true);
    EXPECT_EQ(vecToCheck[3].Get<std::string>(), "std string");
    EXPECT_EQ(vecToCheck[4].Get<double>(), 22.37);
}

} // namespace tests