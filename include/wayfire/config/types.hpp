#pragma once
#include <glm/vec4.hpp>
#include <string>

namespace wf
{

/**
 * Represents a color in RGBA format.
 */
struct color_t
{
  public:
    /** Initialize a black transparent color (default) */
    color_t();

    /**
     * Initialize a new color value with the given values
     * Values will be clamped to the [0, 1] range.
     */
    color_t(double r, double g, double b, double a);

    /**
     * Initialize a new color value with the given values.
     * Values will be clamped to the [0, 1] range.
     */
    explicit color_t(const glm::vec4& value);

    /**
     * Initialize a new color value from the given hex string, format is either
     * #RRGGBBAA or #RGBA.
     *
     * Invalid input is considered as the default color.
     */
    explicit color_t(const std::string& value);

    /** Red channel value */
    double r;
    /** Green channel value */
    double g;
    /** Blue channel value */
    double b;
    /** Alpha channel value */
    double a;

    /** Check whether the given hex string is a valid color */
    static bool is_valid(const std::string& value);
};

/**
 * A list of valid modifiers.
 * The enumerations values are the same as the ones in wlroots.
 */
enum keyboard_modifier_t
{
    /* Shift modifier, <shift> */
	KEYBOARD_MODIFIER_SHIFT = 1,
    /* Control modifier, <ctrl> */
	KEYBOARD_MODIFIER_CTRL = 4,
    /* Alt modifier, <alt> */
	KEYBOARD_MODIFIER_ALT = 8,
    /* Windows/Mac logo modifier, <super> */
	KEYBOARD_MODIFIER_LOGO = 64,
};

/**
 * Represents a single keyboard shortcut.
 */
struct keybinding_t
{
  public:
    /**
     * Construct a new keybinding with the given modifier and key.
     */
    keybinding_t(uint32_t modifier, uint32_t keyval);

    /**
     * Construct a new keybinding from the given string description.
     * Format is <modifier1> .. <modifierN> KEY_<keyname>, where whitespace
     * characters between the different modifiers and KEY_* are ignored.
     *
     * For a list of available modifieres, see @keyboard_modifier_t.
     *
     * The KEY_<keyname> is derived from evdev, and possible names are
     * enumerated in linux/input-event-codes.h
     *
     * For example, "<super> <alt> KEY_E" represents pressing the Logo, Alt and
     * E keys together.
     *
     * Invalid values result in mod and keyval being set to 0.
     */
    keybinding_t(const std::string& description);

    /** Check the given string is a valid keyboard shortcut description */
    static bool is_valid(const std::string& value);

    /* Check whether two keybindings refer to the same shortcut */
    bool operator == (const keybinding_t& other) const;

    /** @return The modifiers of the keybinding */
    uint32_t get_modifiers() const;
    /** @return The key of the keybinding */
    uint32_t get_key() const;

  private:
    /** The modifier mask of this keybinding */
    uint32_t mod;
    /** The key of this keybinding */
    uint32_t keyval;
};

/**
 * Represents a single button shortcut (pressing a mouse button while holding
 * modifiers).
 */
struct buttonbinding_t
{
  public:
    /**
     * Construct a new buttonbinding with the given modifier and button.
     */
    buttonbinding_t(uint32_t modifier, uint32_t button);

    /**
     * Construct a new buttonbinding from the given description.
     * The format is the same as a keybinding, however instead of KEY_* values,
     * the buttons are prefixed with BTN_*
     *
     * Invalid descriptions result in mod = button = 0
     */
    buttonbinding_t(const std::string& description);

    /** Check the given string is a valid button shortcut description */
    static bool is_valid(const std::string& value);

    /* Check whether two keybindings refer to the same shortcut */
    bool operator == (const buttonbinding_t& other) const;

    /** @return The modifiers of the buttonbinding */
    uint32_t get_modifiers() const;
    /** @return The button of the buttonbinding */
    uint32_t get_button() const;

  private:
    /** The modifier mask of this keybinding */
    uint32_t mod;
    /** The key of this keybinding */
    uint32_t button;
};


}
