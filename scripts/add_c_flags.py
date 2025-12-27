# Adds compiler flags to suppress warnings during PlatformIO build
# Used by: PlatformIO environments (esp32dev-pilight*, esp32-m5stick-c*)
Import("env")


# General options that are passed to the C++ compiler
env.Append(CXXFLAGS=["-Wno-volatile"])

# General options that are passed to the C compiler (C only; not C++).
env.Append(CFLAGS=["-Wno-implicit-function-declaration"])
