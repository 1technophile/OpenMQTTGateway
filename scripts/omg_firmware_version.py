import subprocess

Import("env")

def get_firmware_specifier_build_flag():
    #ret = subprocess.run(["git", "describe"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
    ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses any tags
    branch = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], stdout=subprocess.PIPE, text=True)
    build_version = env['PIOENV'] + "-" + ret.stdout.strip() + "[" + branch.stdout.strip() + "]"
    build_flag = "-D OMG_VERSION=\\\"" + build_version + "\\\""
    print ("OpenMQTTGateway Build Version: " + build_version)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag()]
)
