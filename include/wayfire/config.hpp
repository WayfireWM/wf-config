#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <vector>
#include <string>
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

/* Represents a keybinding */
struct wf_key
{
    uint32_t mod;
    uint32_t keyval;

    bool valid() const;
    bool matches(const wf_key& other) const;
};

/* Represents a buttonbinding */
struct wf_button
{
    uint32_t mod;
    uint32_t button;

    bool valid() const;
    bool matches(const wf_button& other) const;
};

enum wf_gesture_type
{
    GESTURE_NONE,
    GESTURE_SWIPE,
    GESTURE_EDGE_SWIPE,
    GESTURE_PINCH
};

#define GESTURE_DIRECTION_LEFT (1 << 0)
#define GESTURE_DIRECTION_RIGHT (1 << 1)
#define GESTURE_DIRECTION_UP (1 << 2)
#define GESTURE_DIRECTION_DOWN (1 << 3)
#define GESTURE_DIRECTION_IN (1 << 4)
#define GESTURE_DIRECTION_OUT (1 << 5)

/* Represents a gesture binding */
struct wf_touch_gesture
{
    wf_gesture_type type;
    uint32_t direction;
    int finger_count;

    bool valid() const;
    bool matches(const wf_touch_gesture& other) const;
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
            wf_touch_gesture gesture;
            wf_color color;
        } cached;

        /* Because libevdev treats buttons and keys the same, we can use
         * the wf_key structure for buttons as well */
        std::vector<wf_key> activator_keys;
        std::vector<wf_touch_gesture> activator_gestures;
        void reinitialize_activators();

        bool is_cached = false;
        bool is_from_file = false;
        int64_t age;
    public:

    wf_option_t(std::string name);

    void set_value(int value,         int64_t age = -1);
    void set_value(double value,      int64_t age = -1);
    void set_value(std::string value, int64_t age = -1);

    void set_value(const wf_key& key,       int64_t age = -1);
    void set_value(const wf_color& color,   int64_t age = -1);
    void set_value(const wf_button& button, int64_t age = -1);
    void set_value(const wf_touch_gesture& gesture, int64_t age = -1);

    std::string name, raw_value, default_value;

    std::vector<wf_option_callback*> updated;
    void add_updated_handler(wf_option_callback* callback);
    void rem_updated_handler(wf_option_callback* callback);

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

    wf_touch_gesture as_gesture();
    operator wf_touch_gesture();

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
    wf_touch_gesture as_cached_gesture();

    /* Activator functions. Again, as for other cached properties, use only as
     * an activator */
    bool matches_key(const wf_key& key);
    bool matches_button(const wf_button& button);
    bool matches_gesture(const wf_touch_gesture& gesture);
};

using wf_option = std::shared_ptr<wf_option_t>;

wf_option new_static_option(std::string);

class wayfire_config_section
{
    wf_option get_option(std::string name);

    public:
        std::string name;

        std::vector<wf_option> options;
        void update_option(std::string name, std::string value, int64_t age);
        wf_option get_option(std::string name, std::string default_value);
};

class wayfire_config
{
    std::string fname;
    int watch_id;

    /* number of reloads since start */
    int64_t reload_age = 0;

    public:

    std::vector<wayfire_config_section*> sections;
    wayfire_config(std::string file);

    void reload_config();
    void save_config();
    void save_config(std::string file);

    wayfire_config_section* get_section (const std::string& name);
    wayfire_config_section* operator [] (const std::string& name);
};

#endif /* end of include guard: CONFIG_HPP */
