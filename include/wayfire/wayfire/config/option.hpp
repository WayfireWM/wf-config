#pragma once
#include <string>
#include <functional>
#include <limits>

#include <experimental/optional>
#include <memory>

namespace wf
{
namespace config
{
/**
 * A base class for all option types.
 */
class option_base_t
{
  public:
    virtual ~option_base_t();
    option_base_t(const option_base_t& other) = delete;
    option_base_t& operator = (const option_base_t& other) = delete;

    /** @return The name of the option */
    std::string get_name() const;

    /**
     * Set the option value from the given string.
     * If the value is invalid depending on option type, the value will be reset
     * to the default value.
     */
    virtual void set_value(const std::string&) {};

    /** Reset the option to its default value.  */
    virtual void reset_to_default() {};

    /**
     * A function to be executed when the option value changes.
     */
    using updated_callback_t = std::function<void()>;

    /**
     * Register a new callback to execute when the option value changes.
     */
    void add_updated_handler(updated_callback_t *callback);

    /**
     * Unregister a callback to execute when the option value changes.
     * If the same callback has been registered multiple times, this unregister
     * all registered instances.
     */
    void rem_updated_handler(updated_callback_t *callback);

  protected:
    /** Construct a new option with the given name. */
    option_base_t(const std::string& name);

    /** Notify all watchers */
    void notify_updated() const;

    struct impl;
    std::unique_ptr<impl> priv;
};

/**
 * A base class for options which can have minimum and maximum.
 * By default, no bounding checks are enabled.
 */
template<class Type, bool enable_bounds>
class bounded_option_base_t
{
  protected:
    Type closest_valid_value(const Type& value) const
    {
        return value;
    }
};

/**
 * Specialization for option types which do support bounded values.
 */
template<class Type>
class bounded_option_base_t<Type, true>
{
  public:
    /** @return The minimal permissible value for this option, if it is set. */
    std::experimental::optional<Type> get_minimum() const
    {
        return minimum;
    }

    /** @return The maximal permissible value for this option, if it is set. */
    std::experimental::optional<Type> get_maximum() const
    {
        return maximum;
    }

  protected:
    std::experimental::optional<Type> minimum;
    std::experimental::optional<Type> maximum;

    /**
     * @return The closest possible value
     */
    Type closest_valid_value(const Type& value) const
    {
        auto real_minimum = minimum.value_or(
            std::numeric_limits<typename Type::wrapped_type_t>::lowest());
        auto real_maximum = maximum.value_or(
            std::numeric_limits<typename Type::wrapped_type_t>::max());

        if (value < real_minimum)
            return real_minimum;

        if (value > real_maximum)
            return real_maximum;

        return value;
    }
};

namespace detail
{
/**
 * Helper class to decide whether the option type supports bounds checking
 */
template<class T, class T2 = void> struct is_wrapped_comparable_type :
    std::false_type {};

template<class T>
struct is_wrapped_comparable_type<T, std::enable_if_t<std::is_class<T>::value>>
{
  private:
    struct fallback_primitive_wrapper { using wrapped_type_t = void; };
    struct primitive_wrapper_tester :
        public T, public fallback_primitive_wrapper {};

    /* This compiles only if T doesn't have wrapped_type_t, otherwise
     * wrapped_type_t becomes ambiguous */
    template<class U> static constexpr bool is_wrapped_comparable(
        typename U::wrapped_type_t *) { return false; }
    /* Otherwise, we have a single wrapped_type_t, just check whether it is
     * arithmetic */
    template<class U> static constexpr bool is_wrapped_comparable(U*)
    { return std::is_arithmetic<typename T::wrapped_type_t>::value; }

  public:
    enum { value = is_wrapped_comparable<primitive_wrapper_tester>(nullptr) };
};

template<class Type, class Result> using boundable_type_only =
std::enable_if_t<is_wrapped_comparable_type<Type>::value, Result>;
}

/**
 * Represents an option of the given type.
 */
template<class Type>
class option_t : public option_base_t,
    public bounded_option_base_t<Type,
        detail::is_wrapped_comparable_type<Type>::value>
{
  public:
    /**
     * Create a new option with the given name and default value.
     */
    option_t(const std::string& name, Type def_value)
        : option_base_t(name), default_value(def_value), value(default_value)
    { }

    /**
     * Set the value of the option from the given string.
     * The value will be auto-clamped to the defined bounds, if they exist.
     * If the value actually changes, the updated handlers will be called.
     */
    virtual void set_value(const std::string& new_value_str)
    {
        auto new_value = Type::from_string(new_value_str);
        set_value(new_value.value_or(default_value));
    }

    /**
     * Reset the option to its default value.
     */
    virtual void reset_to_default()
    {
        set_value(default_value);
    }

    /**
     * Set the value of the option.
     * The value will be auto-clamped to the defined bounds, if they exist.
     * If the value actually changes, the updated handlers will be called.
     */
     void set_value(const Type& new_value)
     {
         auto real_value = this->closest_valid_value(new_value);
         if (!(this->value == real_value))
         {
             this->value = real_value;
             this->notify_updated();
         }
     }

     Type get_value() const
     {
         return value;
     }

  public:
     /**
      * Set the minimum permissible value for arithmetic type options.
      * An attempt to set the value to a value below the minimum will set the
      * value of the option to the minimum.
      */
     template<class U = void>
         detail::boundable_type_only<Type, U> set_minimum(Type min)
     {
         this->minimum = {min};
         this->value = this->closest_valid_value(this->value);
     }

     /**
      * Set the maximum permissible value for arithmetic type options.
      * An attempt to set the value to a value above the maximum will set the
      * value of the option to the maximum.
      */
     template<class U = void>
         detail::boundable_type_only<Type, U> set_maximum(Type max)
     {
         this->maximum = {max};
         this->value = this->closest_valid_value(this->value);
     }

  protected:
     const Type default_value; /* default value */
     Type value; /* current value */
};

}
}
