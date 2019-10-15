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
    optional(Type&& actual_value)
    {
        this->value = std::make_unique<Type> (actual_value);
    }

    operator bool()
    {
        return value != nullptr;
    }

    Type get_or(Type default_value)
    {
        if (this->value) {
            return *this->value;
        } else {
            return default_value;
        }
    }
};

}
