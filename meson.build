project(
	'wf-config',
	'cpp',
	version: '0.3',
	license: 'MIT',
	meson_version: '>=0.43.0',
	default_options: [
		'cpp_std=c++14',
		'warning_level=2',
		'werror=false',
	],
)

glm = dependency('glm')
evdev = dependency('libevdev')
wlroots = dependency('wlroots', version: '>=0.8.0')

sources = [ 'src/types.cpp' ]
wfconfig_inc = include_directories('include/wayfire')

lib_wfconfig = library('wf-config',
    sources,
    dependencies: [evdev, wlroots, glm],
    include_directories: wfconfig_inc,
    install: true,
    version: meson.project_version())

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    libraries:    lib_wfconfig,
    subdirs:      'wayfire',
    version:      meson.project_version(),
    filebase:     meson.project_name(),
    name:         meson.project_name(),
    description: 'Dynamic file-based configuration library for Wayfire')

install_headers([], subdir: 'wayfire/config')

wfconfig = declare_dependency(link_with: lib_wfconfig,
    include_directories: wfconfig_inc)

# Unit tests
subdir('test')
