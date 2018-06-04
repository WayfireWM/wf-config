#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <string>
#include <map>

/* designed to be compatible with wlroots' modifiers */
/* TODO: in wayfire, check if these are the same */
enum wf_modifier
{
	WF_MODIFIER_SHIFT = 1,
	WF_MODIFIER_CTRL = 4,
	WF_MODIFIER_ALT = 8,
	WF_MODIFIER_LOGO = 64,
};

struct wf_key
{
    uint32_t mod;
    uint32_t keyval;

    bool valid();
};

struct wf_button
{
    uint32_t mod;
    uint32_t button;

    bool valid();
};

struct wf_color
{
    float r, g, b, a;
};


class wayfire_config_section
{
    public:

    std::string name;
    int refresh_rate;
    std::map<std::string, std::string> options;

    std::string get_string(std::string name, std::string default_value);
    int get_int(std::string name, int default_value);

    /* reads the specified option which is interpreted as duration in milliseconds
     * returns the number of frames this duration is equivalent to */
    int get_duration (std::string name, int default_value);
    double get_double(std::string name, double default_value);

    wf_key    get_key   (std::string name, wf_key    default_value);
    wf_button get_button(std::string name, wf_button default_value);
    wf_color  get_color (std::string name, wf_color  default_value);
};

class wayfire_config
{
    std::vector<wayfire_config_section*> sections;
    int refresh_rate;

    public:
    wayfire_config(std::string file, int refresh_rate = -1);
    wayfire_config_section* get_section(std::string name);
    void set_refresh_rate(int refresh_rate);
};

#endif /* end of include guard: CONFIG_HPP */
