#include <config/types.hpp>

/* ----------------------------- wf::color_t -------------------------------- */
wf::color_t::color_t()
    : color_t(0.0, 0.0, 0.0, 0.0) {}

wf::color_t::color_t(double r, double g, double b, double a)
{
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
}

wf::color_t::color_t(const glm::vec4& value)
    : color_t(value.r, value.g, value.b, value.a) {}

static double hex_to_double(std::string value)
{
    char *dummy;
    return std::strtol(value.c_str(), &dummy, 16);
}

wf::color_t::color_t(const std::string& value)
{
    if (!is_valid(value))
    {
        this->r = this->g = this->b = this->a = 0;
        return;
    }

    /* #RRGGBBAA case */
    if (value.size() == 9)
    {
        this->r = hex_to_double(value.substr(1, 2)) / 255.0;
        this->g = hex_to_double(value.substr(3, 2)) / 255.0;
        this->b = hex_to_double(value.substr(5, 2)) / 255.0;
        this->a = hex_to_double(value.substr(7, 2)) / 255.0;
    } else {
        assert(value.size() == 5);
        this->r = hex_to_double(value.substr(1, 1)) / 15.0;
        this->g = hex_to_double(value.substr(2, 1)) / 15.0;
        this->b = hex_to_double(value.substr(3, 1)) / 15.0;
        this->a = hex_to_double(value.substr(4, 1)) / 15.0;
    }
}

bool wf::color_t::is_valid(const std::string& value)
{
    /* Either #RGBA or #RRGGBBAA */
    if (value.size() != 5 && value.size() != 9)
        return false;

    if (value[0] != '#')
        return false;

    const std::string permissible = "0123456789ABCDEF";
    if (value.find_first_not_of(permissible, 1) != std::string::npos)
        return false;

    return true;
}
