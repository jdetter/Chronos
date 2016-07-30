import os
from context import *

class _StringArray(list):
    def __init__(self, string_callback):
        super(_StringArray, self).__init__()
        self.string_callback = string_callback

    def __iadd__(self, item):
        self.append(item)
        return self

    def append(self, item):
        if isinstance(item, str):
            item = self.string_callback(item)
            super(_StringArray, self).append(item)
        elif isinstance(item, list):
            for list_item in item:
                self.append(list_item)
        else:
            raise Exception("Unable to append anything but strings or lists.")

SubbuildArray = lambda : _StringArray(lambda string
        : File(join_path(string,'SConscript')).srcnode().path)
SubdirArray = lambda : _StringArray(lambda string: Dir(string).srcnode().path)
join_path = os.path.join

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
