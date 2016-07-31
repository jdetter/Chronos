from collections import UserList
from SCons.Script import  *
import os

join_path = os.path.join
File_t = SCons.Node.FS.File

def Split(arg):
    if is_List(arg) or is_Tuple(arg):
        return arg
    elif is_String(arg):
        return arg.split()
    else:
        return [arg]

class CLVar(UserList):
    def __init__(self, seq = []):
        UserList.__init__(self, Split(seq))
    def __add__(self, other):
        return UserList.__add__(self, CLVar(other))
    def __radd__(self, other):
        return UserList.__radd__(self, CLVar(other))
    def __coerce__(self, other):
        return (self, CLVar(other))
    def __str__(self):
        return ' '.join(self.data)
