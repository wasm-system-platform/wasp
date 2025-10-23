project('stubs-sp', 'cpp', default_options : ['cpp_std=c++23'])

fmt = subproject('fmt')
fmt_dep = fmt.get_variable('fmt_dep')

sp_sources = files(
    'src/main.cpp',
    'src/grammar/instructions.cpp',
    'src/grammar/module.cpp',
    'src/grammar/sections.cpp',
    'src/grammar/types.cpp',
    'src/grammar/values.cpp',
    'src/runtime/instance.cpp',
    'src/runtime/operations.cpp',
    'src/runtime/process_manager.cpp',
    'src/runtime/context_manager.cpp',
    'src/runtime/waiting_room.cpp',
    'src/runtime/function.cpp',
    'src/runtime/vm.cpp',
    'src/devices/device.cpp',
    'src/devices/timer.cpp',
    'src/hw/wasm_disk.cpp',
    'src/util/error_handling.cpp'
)

if get_option('enable_trace')
    add_project_arguments('-DENABLE_TRACE', language : 'cpp')
endif

sp = executable(
    'stubs-sp',
    sources: sp_sources,
    dependencies: fmt_dep,
    include_directories: include_directories('src/')
)
