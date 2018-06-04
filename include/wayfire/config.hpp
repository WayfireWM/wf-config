#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <memory>

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

using wf_option_callback = std::function<void()>;
struct wf_option_t
{
    friend class wayfire_config_section;
    friend class wayfire_config;
    private:
        union
        {
            int i;
            double d;
            wf_key key;
            wf_button button;
            wf_color color;
        } cached;

        bool is_cached = false;
        int64_t age;
    public:

    wf_option_t(std::string name);
    void set_value(std::string value, int64_t age = -1);

    std::string name, raw_value, default_value;
    std::vector<wf_option_callback*> updated;

    std::string as_string();
    operator std::string();

    int    as_int();
    operator int();

    double as_double();
    operator double();

    wf_key    as_key();
    operator wf_key();

    wf_button as_button();
    operator wf_button();

    wf_color  as_color();
    operator wf_color();

    /* "performance-oriented" variants of the regular as_* functions
     * These cache the result of the parsing operation, but you can't mix
     * casts to different types for the same option. Use ONLY when the
     * option's type will NEVER change */
    int    as_cached_int();
    double as_cached_double();

    wf_key    as_cached_key();
    wf_button as_cached_button();
    wf_color  as_cached_color();
};

using wf_option = std::shared_ptr<wf_option_t>;

wf_option new_static_option(std::string);

class wayfire_config_section
{
    wf_option get_option(std::string name);

    public:
        std::string name;

        std::map<std::string, wf_option> options;
        void update_option(std::string name, std::string value, int64_t age);
        wf_option get_option(std::string name, std::string default_value);
};

class wayfire_config
{
    std::string fname;
    std::map<std::string, wayfire_config_section*> sections;
    int watch_id;

    /* number of reloads since start */
    int64_t reload_age = 0;

    public:

    wayfire_config(std::string file);

    void reload_config();

    wayfire_config_section* get_section (const std::string& name);
    wayfire_config_section* operator [] (const std::string& name);
};

#endif /* end of include guard: CONFIG_HPP */