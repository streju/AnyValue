#pragma once

#include <exception>
#include <functional>
#include <string>
#include <type_traits>
#include <typeinfo>

class AnyValueException : public std::exception
{
public:
    AnyValueException(const std::string &msg) : mMsg(msg) {}
    const char *what() const noexcept override { return mMsg.c_str(); }
private:
    std::string mMsg;
};

class AnyValue
{
public:
    AnyValue() : mData(nullptr) { InitializeTypeFunctions<std::nullptr_t>(); }

    template <typename T,
              typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type, AnyValue>::value>::type>
    explicit AnyValue(T &&value) : mData(new typename std::decay<T>::type{std::forward<T>(value)})
    {
        InitializeTypeFunctions<typename std::decay<T>::type>();
    }

    explicit AnyValue(const char *value) : mData(new std::string{value}) { InitializeTypeFunctions<std::string>(); }
    AnyValue(const AnyValue &other)
        : mData(other.mCopyDataFunc(other.mData)), mGetTypeFunc(other.mGetTypeFunc), mCopyDataFunc(other.mCopyDataFunc),
          mClearFunc(other.mClearFunc), mCompareFunc(other.mCompareFunc)
    {
    }

    AnyValue &operator=(AnyValue const &other)
    {
        mClearFunc(mData);
        mData = other.mCopyDataFunc(other.mData);
        mGetTypeFunc = other.mGetTypeFunc;
        mCopyDataFunc = other.mCopyDataFunc;
        mClearFunc = other.mClearFunc;
        mCompareFunc = other.mCompareFunc;
        return *this;
    }

    template <typename T>
    T Get() const
    {
        AssertType<typename std::decay<T>::type>();
        return *static_cast<T *>(mData);
    }

    template <typename T>
    void Set(const T &val)
    {
        if (mGetTypeFunc() == typeid(typename std::decay<T>::type))
        {
            *static_cast<T *>(mData) = val;
            return;
        }
        mClearFunc(mData);
        mData = new typename std::decay<T>::type(val);
        InitializeTypeFunctions<typename std::decay<T>::type>();
    }

    void Set(const char *val)
    {
        if (mGetTypeFunc() == typeid(std::string))
        {
            *static_cast<std::string *>(mData) = val;
            return;
        }
        mClearFunc(mData);
        mData = new std::string(val);
        InitializeTypeFunctions<std::string>();
    }

    template <typename T>
    void AssertType() const
    {
        if (mGetTypeFunc() != typeid(typename std::decay<T>::type))
        {
            std::string error = std::string("AssertType: type error: Value type(") + mGetTypeFunc().name()
                                + ") asserted(" + typeid(typename std::decay<T>::type).name() + ")";
            throw AnyValueException(error);
        }
    }

    const std::type_info &GetType() const { return mGetTypeFunc(); }

    ~AnyValue() { mClearFunc(mData); }

    bool operator==(AnyValue const &other) const
    {
        if (mGetTypeFunc() != other.mGetTypeFunc())
        {
            return false;
        }
        return mCompareFunc(mData, other.mData);
    }

    bool operator!=(AnyValue const &other) const { return !(*this == other); }
    template <typename T>
    AnyValue &operator=(const T &rhs)
    {
        *this = AnyValue(rhs);
        return *this;
    }

    template <typename T>
    bool operator==(const T &value) const
    {
        return this->operator==(AnyValue(value));
    }

    template <typename T>
    bool operator!=(const T &value) const
    {
        return !(this->operator==<T>(value));
    }

private:
    template <typename T>
    void InitializeTypeFunctions()
    {
        mGetTypeFunc = []() -> std::type_info const & { return typeid(T); };
        mCopyDataFunc = [](void *otherData) -> void * { return new T(*static_cast<T *>(otherData)); };
        mClearFunc = [](void *data) { delete static_cast<T *>(data); };
        mCompareFunc = [](void *data, void *otherData) {
            return *static_cast<T *>(data) == *static_cast<T *>(otherData);
        };
    }

    void *mData;
    std::function<const std::type_info &()> mGetTypeFunc;
    std::function<void *(void *)> mCopyDataFunc;
    std::function<void(void *)> mClearFunc;
    std::function<bool(void *, void *)> mCompareFunc;
};
