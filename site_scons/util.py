import os
import re
from collections import UserList
from SCons.Script import  *

join_path = os.path.join
File_t = SCons.Node.FS.File
Dir_t = SCons.Node.FS.Dir

def is_list(item):
    return isinstance(item, list) or isinstance(item, UserList)

def is_string(item):
    return isinstance(item, str)

def replace_ext(item, ext):
    match = re.sub('.[\w]*?$', ext, item)
    return match if match else item

def get_ext(item):
    match = re.search('(.[\w]*?$)', item)
    return match[1] if match else None

def call_on_each(list_or_item, fn):
    ret_val = []
    if is_list(list_or_item):
        for item in list_or_item:
            ret_val.append(fn(item))
    else:
        ret_val.append(fn(list_or_item))

    return ret_val

def get_srcpath(node):
    if not(isinstance(node, File_t) or isinstance(node, Dir_t)):
        try:
            node = Dir(node)
        except Exception as e:
            raise e
        try:
            node = File(node)
        except Exception as e:
            raise e
    return node.srcnode().path

def Split(arg):
    if is_List(arg) or is_Tuple(arg):
        return arg
    elif is_String(arg):
        return arg.split()
    else:
        return [arg]

#class CLVar(UserList):
#    def __init__(self, seq = []):
#        UserList.__init__(self, Split(seq))
#    def __add__(self, other):
#        return UserList.__add__(self, CLVar(other))
#    def __radd__(self, other):
#        return UserList.__radd__(self, CLVar(other))
#    def __coerce__(self, other):
#        return (self, CLVar(other))
#    def __str__(self):
#        return ' '.join(self.data)

class AppendList(UserList):
    def __init__(self, seq = []):
        UserList.__init__(self, seq)

    def __add__(self, other):
        self_copy = AppendList(self)
        self_copy.append(other)
        return self_copy
    def __radd__(self, other):
        self_copy = AppendList(self)
        self_copy.append(other)
        return self_copy
    def __iadd__(self, other):
        self.append(other)
        return self
    def __coerce__(self, other):
        return (self, self.__class__(other))
    def append(self, other):
        if is_list(other):
            self.extend(other)
        else:
            self.data.append(other)
    def extend(self, other):
        for item in other:
            self.append(item)

class IncludeList(AppendList):
    def __init__(self, seq=[]):
        seq = call_on_each(seq, get_srcpath)
        super(IncludeList, self).__init__(seq)

    def append(self, other):
        if is_list(other):
            self.extend(other)
        else:
            self.data.append(Dir(other))

class SrcList(AppendList):
    def __init__(self, seq=[]):
        seq = call_on_each(seq, get_srcpath)
        super(SrcList, self).__init__(seq)

    def append(self, other):
        if is_list(other):
            self.extend(other)
        else:
            self.data.append(File(other))

def SrcObject(path_or_file):
    file_ = File(path_or_file)
    file_.tagged = True
    file_.env = DefaultEnvironment
    return file_
