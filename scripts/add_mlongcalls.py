# Apply -mlongcalls to the whole build (project sources + lib_deps) on Xtensa
# ESP32 targets. The precompiled esp32-arduino-libs only inject -mlongcalls into
# the framework compile, not into lib_deps (NimBLE, ArduinoJson) or project
# sources. For large images (OMG is ~90% of a 2 MB partition on IDF 5.5.4 libs)
# the resulting fixed-range call8 relocations overflow and the link fails with
#   "dangerous relocation: call8: call target out of range".
# RISC-V targets (esp32c*/h*/p4) have no call8 and reject -mlongcalls, so this is
# restricted to the Xtensa MCUs. Guarded so nothing is duplicated.
Import("env")

mcu = env.BoardConfig().get("build.mcu", "")
if mcu in ("esp32", "esp32s2", "esp32s3"):
    if "-mlongcalls" not in env.get("CCFLAGS", []):
        env.Append(CCFLAGS=["-mlongcalls"])
    if "-mlongcalls" not in env.get("ASFLAGS", []):
        env.Append(ASFLAGS=["-mlongcalls"])
