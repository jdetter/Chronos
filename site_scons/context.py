__global_context_instance = None
def GlobalContext(*args, **kwargs):
    _globals = {}

    global __global_context_instance
    if __global_context_instance is not None:
        return __global_context_instance

    class GlobalClosure(object):

        def __init__(self):
            self.add_globals = self.add_global

        def __call__(self):
            return self

        def add_global(self, opt_dict=None, **kwargs):
            if opt_dict is not None:
                if type(opt_dict) is not type({}):
                    Exception('opt_dict must be of type dict')

                for name, arg in opt_dict.items():
                    _globals[name] = arg

            for name, arg in kwargs.items():
                _globals[name] = arg

        @property
        def globals(self):
            return _globals

        @globals.setter
        def globals(self):
            raise AttributeException(str(self) + ' is a protected property!\n'
                    'Set it with .add_global()')

        @staticmethod
        def import_globals(globals_dict):
            for name, arg in _globals.items():
                globals_dict[name] = arg

        @staticmethod
        def import_global(globals_dict, name):
                globals_dict[name] = _globals[name]

    __global_context_instance = GlobalClosure(*args, **kwargs)
    return __global_context_instance

GlobalContext()

def import_globals(globals_dict):
    __check_init_or_raise_exception()
    __global_context_instance.import_globals(globals_dict)

def import_global(globals_dict, name):
    __check_init_or_raise_exception()
    __global_context_instance.import_globals(globals_dict, name)

def add_global(*args, **kwargs):
    __check_init_or_raise_exception()
    __global_context_instance.add_global(*args, **kwargs)

add_globals = add_global

def __check_init_or_raise_exception():
    if __global_context_instance is None:
        raise UninitializedException('The GlobalContext is unitialized.')

class UninitializedException(Exception):
    pass

# Global Context Object is a singleton.
assert(GlobalContext() == GlobalContext())
