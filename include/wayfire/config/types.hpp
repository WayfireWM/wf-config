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

}
