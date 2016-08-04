import os
from SCons.Script import *

#from SCons.Script import GlobalContext, StringArray
GlobalContext = GlobalContext()
Export('GlobalContext')
init_site()

def crosstool_path(tool):
    return join_path(TOOL_DIR, TARGET+tool)

# TODO: Logic for deciding the actual target.
TARGET      = 'i686-pc-chronos-'
BUILD_ARCH  = 'i386'
GlobalContext.add_global(BUILD_ARCH=BUILD_ARCH)

# TODO: Take from the build environment. 
TOOL_DIR 		= os.path.abspath('../tools/bin')

target_sysroot 	= join_path('..', 'sysroot')


# Make the path to the tool os agnostic.

CROSS_CC        = crosstool_path('gcc')
CROSS_LD        = crosstool_path('ld')
CROSS_AS        = crosstool_path('gcc')
CROSS_OBJCOPY   = crosstool_path('objcopy')

HOST_CC         = '/home/s/swilson/Projects/chronos/test.sh'#'gcc'
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
         '-DARCH_' + BUILD_ARCH, 
         '-DARCH_STR=' + BUILD_ARCH,
         ]

ASFLAGS = [
       '-ggdb',
       '-Werror',
       '-Wall',
       '-DARCH_' + BUILD_ARCH,
       '-DARCH_STR=' + BUILD_ARCH
       ]

QEMU = 'qemu-system-' + BUILD_ARCH

#FIXME: Ld Builder should be Program
generic_env = Environment()

# Initialize the Callback list builder.
InitCallbackListBuilder(generic_env)

# Example of how to the the Callback list hack
#
# >>> generic_env.AddCall('ls', '-l', 'kernel')
# >>>
# >>> generic_env.AlwaysBuild('FakeTarget')
# >>> generic_env.RunCalls('FakeTarget', None)


host_env = generic_env.Clone(
        CC=HOST_CC,
        LD=HOST_LD,
        OBJCOPY=HOST_OBJCOPY,
        CCFLAGS=CCFLAGS, 
        ASFLAGS=ASFLAGS,
        variant_dir='build'
        )

cross_env = generic_env.Clone(
        CC=CROSS_CC,
        LD=CROSS_LD,
        OBJCOPY=CROSS_OBJCOPY,
        CCFLAGS=CCFLAGS, 
        ASFLAGS=ASFLAGS
        )


kernel = LinkedBuildObject(cross_env, 'kernel.o')

GLOBALS = {
        'kinclude':kernel.include,
        'ksrc':kernel.sources,
        }
GlobalContext.add_globals(GLOBALS)
GlobalContext.import_globals(globals())

include('kernel')

kernel.env.Append(CPPPATH=kernel.include)
kernel.Build(cross_env)

for file_ in kernel.sources:
    pass
    #print(file_.rfile())
