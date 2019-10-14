#pragma once
#include <glm/vec4.hpp>

namespace wf
{

/**
 * Represents a color in RGBA format.
 */
struct color_t
{
  public:
    /** Initialize a black transparent color */
    color_t();
    /** Initialize a new color value with the given values */
    color_t(double r, double g, double b, double a);
    /** Initialize a new color value with the given values */
    color_t(const glm::vec4& value) explicit;
    /**
     * Initialize a new color value from the given hex string, format is either
     * #RRGGBBAA or #RGBA
     */
    color_t(const std::string& value) explicit;

    /** Red channel value */
    double r;
    /** Green channel value */
    double g;
    /** Blue channel value */
    double b;
    /** Alpha channel value */
    double a;
};

}
