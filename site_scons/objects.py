from SCons.Script import *
from util import *

class _BaseTargetObject(object):

    def __init__(self, target, env):
        """The callback called to build us."""
        self._BuildSources = SrcList()

        """What we need build before us."""
        self._BuildDependancies = []# A list of _BaseTargetObject

        """What we will use as a target to self.Build(target)"""
        self._TargetOutput  = target
        #self._BuildOutput

        """Modify anything we need to know to build."""
        self.env = env

    def Build(self, *args, **kwargs):
        """
        Will initiate a call to build each source, which will do
        the same to their respective source(s).
        """
        return self._BuildRecipe(*args, **kwargs)

    def AddSource(self, source):
        """Convinience method to add an item used within the actual self.Build()"""

    def AddDepends(self, dep):
        """
        Add a _BaseTargetObject to a list of Targets which we'll Call Build of
        when we are told to build.
        """
        pass

# A property that makes an object which you


class BasicObjectTarget(_BaseTargetObject):
    """
    Build a normal .o file using the given .c file. Automatically use the
    .env of the Target initializing us. The caller should supply its
    environment.

    What if we need special flags for this object?..
    """
    def __init__(self, target, env=None):
        self.sources = None
        self.include = IncludeList()
        super(BasicObjectTarget, self).__init__(target, env)

        self._BuildDependancies = []

        self.sources = self._BuildSources

        print(target)
        #TODO: Change to a detect
        source = replace_ext(target, '.c')

        self.sources += source

    def _BuildRecipe(self, env=None):
        """
        Override _BaseTargetObject.Build with a simple call to Object using
        the given env.
        """
        build_env = self.env if self.env else env

        return env.Object(self._BuildSources)

class ToolTarget(_BaseTargetObject):
    """
    Build a tool Program which can be used in a recipe of another
    _BaseTargetObject.
    """
    def __init__(self, host_env, target, sources):
        super(ToolTarget, self).__init__(env)
        self.env            = host_env
        self._BuildSources  = sources
        self._TargetOutput  = target


class LinkedBuildObject(_BaseTargetObject):
    """
    Build an object that might have complex dependancies, but in the end is
    linked together using env.LD
    """
    def __init__(self, target, env=None):
        self.include = IncludeList()
        self.sources = None
        super(LinkedBuildObject, self).__init__(env, target)
        self.sources = self._BuildSources

    def _BuildRecipe(self, env=None):
        sources = []

        build_env = self.env if self.env else env

        for item in self._BuildSources:
            if not isinstance(item, _BaseTargetObject):
                # Assume that it is a basic object file
                item = BasicObjectTarget(get_srcpath(item))

            sources.append(item.Build(build_env)) # Hand our environment as default
        print(sources)
        return build_env.Program(self._TargetOutput, sources)


# To create an .img target, use a Genric Image target and set _BuildRecipie to
# self.env.Objcopy(sources=self._BuildSources)

#############################
###      Self Testing     ###
#############################

#kernel_obj = LinkedBuildObject(target='kernel.o')
#kernel_obj.sources += 'kernel/devman.c'
