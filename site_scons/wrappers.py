from SCons.Script import *
from util import File_t as _File_t
from util import join_path

DefaultEnvironment = Environment()

class _TypeArray(list):
    def __init__(self, array_type, type_callback):
        self.array_type = array_type
        super(_TypeArray, self).__init__()
        self.type_callback = type_callback

    def __iadd__(self, item):
        self.append(item)
        return self

    def append(self, item):
        if isinstance(item, self.array_type):
            item = self.type_callback(item)
            super(_TypeArray, self).append(item)
        elif isinstance(item, list):
            for list_item in item:
                self.append(list_item)
        else:
            raise Exception("Tried to add item of type {} to {} _TypeArray".format(type(item), self.array_type))

    def extend(self, list_):
        for item in list_:
            self.append(item)

class _StringArray(_TypeArray):
    def __init__(self, string_callback):
        super(_StringArray, self).__init__(str, string_callback)

class _FileArray(_TypeArray):
    def __init__(self, file_callback):
        super(_FileArray, self).__init__(_File_t, file_callback)

class _SrcObjectArray(_TypeArray):
    def __init__(self, src_callback):
        #super(_SrcObjectArray, self).__init__(SrcObject, src_callback)
        super(_SrcObjectArray, self).__init__(_File_t, src_callback)

class SubbuildArray(_StringArray):
    """ _StringArray that will make each given string or list of strings a SConscript
    appended to the given string path.
    """
    def __init__(self):
        def append_path_sconscript(string):
            return File(join_path(string,'SConscript')).srcnode().path
        super(SubbuildArray, self).__init__(append_path_sconscript)

class SubdirArray (_StringArray):
    """ _StringArray that will make each given string or list of strings a Dir()
    in the source tree.
    """
    def __init__(self):
        def get_srcpath (string):
            return Dir(string).srcnode().path
        super(SubdirArray, self).__init__(get_srcpath)

def is_src_object(arg):
    return isinstance(arg, _File_t) and hasattr(arg, 'tagged')
