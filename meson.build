project(
	'wf-config',
	'cpp',
	version: '0.0',
	license: 'MIT',
	meson_version: '>=0.43.0',
	default_options: [
		'cpp_std=c++11',
		'warning_level=2',
		'werror=false',
	],
)

evdev = dependency('libevdev')
wlroots = dependency('wlroots')

sources = [
    'src/config.cpp',
    'src/parse.cpp',
    'src/animation.cpp'
    ]

lib_wfconfig = library('wf-config',
    sources,
    dependencies: [evdev, wlroots],
    install: true)

pkgconfig = import('pkgconfig')
pkgconfig.generate(
    libraries:    lib_wfconfig,
    version:      meson.project_version(),
    filebase:     meson.project_name(),
    name:         meson.project_name(),
    description: 'Dynamic file-based configuration library for Wayfire')

install_headers(['src/config.hpp', 'src/animation.hpp'], subdir: 'wayfire')
