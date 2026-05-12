project('wasm-sp', 'cpp', default_options : ['cpp_std=c++23'])

fmt = subproject('fmt')
fmt_dep = fmt.get_variable('fmt_dep')

sp_sources = files(
    'src/grammar/instructions.cpp',
    'src/grammar/module.cpp',
    'src/grammar/sections.cpp',
    'src/grammar/types.cpp',
    'src/grammar/values.cpp',
    'src/grammar/dwarf.cpp',
    'src/runtime/instance.cpp',
    'src/runtime/operations.cpp',
    'src/runtime/process_manager.cpp',
    'src/runtime/context_manager.cpp',
    'src/runtime/waiting_room.cpp',
    'src/runtime/function.cpp',
    'src/runtime/vm.cpp',
    'src/runtime/handler.cpp',
    'src/runtime/setjmp.cpp',
    'src/runtime/device_manager.cpp',
    'src/runtime/kernel.cpp',
    'src/runtime/process.cpp',
    'src/runtime/optimization.cpp',
    'src/devices/timer.cpp',
    'src/devices/terminal.cpp',
    'src/hw/mem/mmu.cpp',
    'src/devices/disk.cpp',
    'src/util/error_handling.cpp'
)

memory_type = get_option('memory_type')
trace_level = get_option('trace_level')

add_project_arguments('-DMEMORY_' + memory_type.to_upper(), language : 'cpp')

if trace_level > 0
    add_project_arguments('-DTRACE_LEVEL=' + trace_level.to_string(), language : 'cpp')
endif

sp_lib = static_library(
    'wasm-sp-lib',
    sources: sp_sources,
    dependencies: fmt_dep,
    include_directories: include_directories('src/')
)

sp_dep = declare_dependency(
    link_with: sp_lib,
    dependencies: fmt_dep,
    include_directories: include_directories('src/')
)

sp = executable(
    'wasm-sp',
    sources: 'src/main.cpp',
    dependencies: [
        sp_dep, 
        fmt_dep,
        dependency('cxxopts')
    ]
)

subdir('test')
