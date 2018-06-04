libwfconfig = static_library('libwfconfig',
                             'config.cpp',
                             dependencies: [libevdev, wlroots],
                             include_directories: [wayfire_conf_inc])
wfconfig = declare_dependency(link_with: libwfconfig)
