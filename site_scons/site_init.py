from context import *
from wrappers import *
from objects import *
from util import *
from collections import UserList
_build_targets = None

def init_site():
    global _build_targets
    _build_targets = []

def include(include_path):
    return call_on_each(include_path, lambda include_path: SConscript(join_path(include_path, 'SConscript')))

#class BaseTarget(object):
#    def __init__(self, name, env=None):
#        if _build_targets is None:
#            raise UninitilizedException('_build_targets hasn\'t been initilized, did you call init_site()?')
#        _build_targets.append(self)
#        self.name               = name
#        self.sources            = SrcArray()
#        self.include            = SubdirArray()
#        self.subbuilds          = SubbuildArray()
#        self.env                = DefaultEnvironment
#        self.target_location    = self.name
#
#    def collect_subbuilds(self):
#        num_subbuilds = len(self.subbuilds)
#        # num_subbuilds = 0
#        # Run all other builds recursively through the list.
#        while num_subbuilds != 0:
#            subbuild = self.subbuilds.pop(len(self.subbuilds) - 1)
#            print('Build: ' + str(subbuild))
#
#            variant_dir = join_path('build', subbuild)
#            variant_dir = variant_dir.rstrip('SConscript')
#
#            SConscript(subbuild, variant_dir=variant_dir, duplicate=0)
#            num_subbuilds = len(self.subbuilds)
#
#class ObjectTarget(BaseTarget):
#    def __init__(self, name, *args, **kwargs):
#        super(ObjectTarget, self).__init__(name, *args, **kwargs)
#        self.output = SrcObject(self.target_location)
#
#    def link_all_srcs(self):
#        self.env.Object(target=self.target_location, sources=self.sources)


################################################################################
## HAXs
##

def _call_with_self(self, function):
    """
    Attach self to the given function as the first argument to that function.
    """
    return lambda *args, **kwargs: function(self, *args, **kwargs)

###############################
# CallList hack.
#
# These functions are part of a hack to add external shell commands to the
# dependency checking of scons. Since scons can't run more than one command on
# a target, which we often need to do, and the only way to add a these would be
# to define a whole 'nother function and then set that as the builder action.
# This gets to be a hassle to have to do for two or three shell commands every
# time. So here's the solution...
#
# `env.AddCall('shellcmd', 'shell', 'args')`
#
# will add a shell command to a list to be ran with the registered builder
#
# `env.RunCalls(target, sources)`
#
# which allows scons to still determine the correct dependancies.
#
# It's recommended to create a `env.Clone()` for every CallList you build
# otherwise you may end up mixing with another CallList.
#
def _AddCallToEnvCallbackList(self, command, *args, **kwargs):
    args = join_single_list(list(args), kwargs.values())

    command = join_single_list(command, args)

    self.__CallList.append(command)

    # TODO: For debugging purposes, remove
    return command

def _RunCallbackListBuilderAction(target, source, env):

    for command in env.__CallList:
        subprocess.check_call(command)

def _ResetCallbackList(self):
    self.__CallList = []

def InitCallbackListBuilder(env):
    """Initialize the 'CallList Hack' within the given environment."""

    RunCalls = Builder(action=_RunCallbackListBuilderAction,
            suffix='',
            src_suffix=''
            )

    env['BUILDERS']['RunCalls'] = RunCalls
    env.__CallList = []
    env.AddCall = _call_with_self(env, _AddCallToEnvCallbackList)

    env.RemoveCalls = _ResetCallbackList
#
# CallList hack
###############################

class UninitilizedException(Exception):
    pass
