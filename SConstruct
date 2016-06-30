import os
from SCons.Script import *

def crosstool_path(tool):
    return Join_path(tool_dir, target+tool)

def objcopy_generator(source, target, env, for_signature):
    return '$OBJCOPY $OBJCOPYFLAGS %s -o %s'%(source[0], target[0])

#def ld_generator(source, target, env, for_signature):
#    return '$LD $LDFLAGS %s -o %s'%(source[0], target[0])


objcopy_builder = Builder(
        generator=objcopy_generator,
        suffix='',
        src_suffix='.o')
ld_builder = Builder(
        action='$LD $LDFLAGS $SOURCES -o $TARGET',
        suffix='',
        src_suffix='.o')

# TODO: Logic for deciding the actual target.
target      = 'i686-pc-chronos-'
build_arch  = 'i386'

# TODO: Take from the build environment. 
tool_dir = os.path.abspath('../tools/bin')

target_sysroot = Join_path('..', 'sysroot')

subbuilds           = ['kernel']
subbuild_scripts    = [str(os.path.join(subbuild, 'SConscript')) for subbuild in subbuilds]

# Make the path to the tool os agnostic.

cross_cc        = crosstool_path('gcc')
cross_ld        = crosstool_path('ld')
cross_as        = crosstool_path('gcc')
cross_objcopy   = crosstool_path('objcopy')

host_cc         = 'gcc'
host_ld         = 'ld'
host_as         = 'gcc'
host_objcopy    = 'objcopy'

# TODO: Debug mode, both by config or cli flag.

ccflags = [
         '-ggdb', 
         '-Werror', 
         '-Wall', 
         '-gdwarf-2', 
         '-fno-common',
         '-fno-builtin', 
         '-fno-stack-protector',
         '-DARCH_'+build_arch, 
         '-DARCH_STR='+build_arch,
         ]

asflags = [
       '-ggdb',
       '-Werror',
       '-Wall',
       '-DARCH_'+build_arch,
       '-DARCH_STR='+build_arch
       ]

qemu = 'qemu-system-'+build_arch

generic_env = Environment(BUILDERS = {'Objcopy': objcopy_builder, 'Ld':
    ld_builder})

# TODO: Separate scripts for other target stuff like making filesystem etc.

cross_env = generic_env.Clone(
        CC=cross_cc,
        LD=cross_ld, # TODO: Might not be right, may need a builder
        CCFLAGS=ccflags, 
        ASFLAGS=asflags)
#cross_objcopy #TODO: Need to create a builder

host_env = generic_env.Clone()

Export('cross_env', 'host_env', 'build_arch') 
# Run all other builds.
SConscript(subbuild_scripts)
