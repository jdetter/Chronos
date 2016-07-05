from SCons.Script import *

def crosstool_path(tool):
    return Join_path(TOOL_DIR, TARGET+tool)

# TODO: Move to using a global context object instead of multiple Imports and
# Exports.

#GlobalContext = GlobalContextObject()
#Export(GlobalContext)

# TODO: Logic for deciding the actual target.
TARGET      = 'i686-pc-chronos-'
BUILD_ARCH  = 'i386'

# TODO: Take from the build environment. 
TOOL_DIR = os.path.abspath('../tools/bin')

target_sysroot = Join_path('..', 'sysroot')

subbuilds           = ['kernel']

# Make the path to the tool os agnostic.

CROSS_CC        = crosstool_path('gcc')
CROSS_LD        = crosstool_path('ld')
CROSS_AS        = crosstool_path('gcc')
CROSS_OBJCOPY   = crosstool_path('objcopy')

HOST_CC         = 'gcc'
HOST_LD         = 'ld'
HOST_AS         = 'gcc'
HOST_OBJCOPY    = 'objcopy'

# TODO: Debug mode, both by config or cli flag.

CCFLAGS = [
         '-ggdb', 
         '-Werror', 
         '-Wall', 
         '-gdwarf-2', 
         '-fno-common',
         '-fno-builtin', 
         '-fno-stack-protector',
         '-DARCH_'+BUILD_ARCH, 
         '-DARCH_STR='+BUILD_ARCH,
         ]

ASFLAGS = [
       '-ggdb',
       '-Werror',
       '-Wall',
       '-DARCH_'+BUILD_ARCH,
       '-DARCH_STR='+BUILD_ARCH
       ]

QEMU = 'qemu-system-'+BUILD_ARCH

generic_env = Environment(
        BUILDERS = {'Objcopy': OBJCPY_BUILDER, 'Ld': LD_BUILDER}
        )

InitCallbackListBuilder(generic_env)

# Example of how to the the Callback list hack
#generic_env.AddCall('ls', '-l', 'kernel')
#
#generic_env.AlwaysBuild('FakeTarget')
#generic_env.RunCalls('FakeTarget', None)


host_env = generic_env.Clone(
        CC=HOST_CC,
        LD=HOST_LD,
        OBJCOPY=HOST_OBJCOPY,
        CCFLAGS=CCFLAGS, 
        ASFLAGS=ASFLAGS
        )

cross_env = generic_env.Clone(
        CC=CROSS_CC,
        LD=CROSS_LD,
        OBJCOPY=CROSS_OBJCOPY,
        CCFLAGS=CCFLAGS, 
        ASFLAGS=ASFLAGS
        )

#GlobalContext.Environments = [host_env, cross_env]
Export('cross_env', 'host_env', 'BUILD_ARCH') 

# Run all other builds in a build dir..
for subbuild in subbuilds:
    REAL_CURDIR = lambda : subbuild
    SConscript(Join_path(subbuild,'SConscript'), exports='REAL_CURDIR', variant_dir='build', duplicate=0)
    #SConscript(Join_path(subbuild,'SConscript'), exports='REAL_CURDIR')
