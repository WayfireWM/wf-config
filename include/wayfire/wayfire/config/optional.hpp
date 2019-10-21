#pragma once
#include <memory>
#include <functional>

namespace wf
{
/**
 * A class to hold a value of the given type or nothing.
 * This is in some aspects similar to the C++17 std::optional, but is nowhere
 * near correct or complete. wf::optional is rather meant to ease returning
 * values from functions which might fail, and only this.
 */
template<class Type> class optional
{
    std::unique_ptr<Type> value;

  public:
    optional() {}
    optional(const Type& actual_value)
    {
        this->value = std::make_unique<Type> (actual_value);
    }

    optional(Type&& actual_value)
    {
        this->value = std::make_unique<Type> (actual_value);
    }

    operator bool() const
    {
        return value != nullptr;
    }

    Type get_unchecked() const
    {
        if (!value)
            throw std::runtime_error("Unchecked access to optional failed!");
        return *value;
    }

    Type get_or(Type default_value) const
    {
        if (this->value) {
            return *this->value;
        } else {
            return default_value;
        }
    }
};

}
