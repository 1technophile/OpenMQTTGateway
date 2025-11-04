Import("env")

build_flags = env['BUILD_FLAGS']
mcu = env.get("BOARD_MCU").lower()

# General options that are passed to the C++ compiler
env.Append(CXXFLAGS=["-Wno-volatile"])

# General options that are passed to the C compiler (C only; not C++).
env.Append(CFLAGS=["-Wno-implicit-function-declaration"])
