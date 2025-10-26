wasm_cc = find_program('clang', required: true)

wasm_cflags = [
  '--target=wasm32',
  '-nostdlib',
  '-Wl,--no-entry',
  '-Wl,--export=main',
  '-Wl,--import-memory'
]

ackermann_wasm = custom_target(
    'ackermann_wasm',
    input: 'programs/ackermann.c',
    output: 'ackermann.wasm',
    command: [wasm_cc, wasm_cflags, '-o', '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
)

gtest = subproject('gtest')
gtest_dep = gtest.get_variable('gtest_dep')
gtest_main_dep = gtest.get_variable('gtest_main_dep')
gmock_dep = gtest.get_variable('gmock_dep')

test_exe = executable(
  'wasm-sp-testsuite',
  sources: 'src/test.cpp',
  dependencies: [gtest_dep, gtest_main_dep, sp_dep],
)

test('test test', test_exe)
