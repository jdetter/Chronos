import os
import re

Dir_t = type(Dir('#'))

def Join_path(*subpath):
    """
    Return the joined path of all strings given as args.

    :param *subpath:
        A variable number of arguments to combine paths.
    """
    return os.path.join(*subpath)

def Build_include_paths(REAL_CURDIR, include_paths):
    return [Get_real_path(REAL_CURDIR, path) for path in include_paths]

def Get_real_path(REAL_CURDIR, path):
    return '#' + Join_path(REAL_CURDIR(), path)

def Path(file_):
    if file_ is str:
        file_ = File(file_)
    return Relpath(file_)

def Get_subscripts(subdirs):
    """
    Return a list of all SConscripts in the given directories.

    :param subdirs:
        An iterable containing dirs to combine with 'SConscript'
    """
    return [Join_path(dir_, 'SConscript') for dir_ in subdirs]

def Relpath(file_):
    return CURDIR().rel_path(file_)

def Glob_recursive(base_dir):
    Get_subdirs(base_dir)

def Get_dirs(base_dir):
    """
    Return and generate a list of Node.FS.Dirs in the given directory.

    :param dir_:
        The directory to Glob and list contained dirs of.
    """
    dirs = set((file_.abspath for file_ in Glob(Relpath(base_dir)+'/*') if type(file_) is Dir_t))

    link_dirs = set(filter(os.path.islink, dirs))

    dirs -= link_dirs
    dirs -= {CURDIR().abspath, os.path.dirname(CURDIR().abspath)}

    dirs = [Dir(dir_) for dir_ in dirs]
    return dirs

def Get_subdirs(base_dir):
    """
    Return a list of all subdirectories (any level of depth) ignoring symlinks.

    :param dir_:
        The directory to list subdirs of.
    """
    dirs = []
    for dir_ in Get_dirs(base_dir):
        dirs.append(dir_)
        dirs.extend(Get_subdirs(dir_))
    return dirs

def Get_sources_in_dir(dir_, suffixes='.c'):
    """
    Return a list of files in a dir_ with given suffix(es).

    :param dir_:
        The directory to Glob suffixes for.

    :param suffixes:
        A suffix or list of suffixes to Glob the dir_ for.
    """

    dir_ = _make_file(dir_)

    # Accept a single suffix string as well as a list of them.
    if type(suffixes) is str:
        suffixes = [suffixes]

    # Add the * Glob to each suffix
    suffixes = ['*'+suffix for suffix in suffixes]

    sources = []
    for suffix in suffixes:
        pattern_sources = Glob(Join_path(Relpath(dir_), suffix))
        if pattern_sources:
            sources.extend(pattern_sources)
    return sources

def CURDIR():
    """Return the current directory."""
    return Dir('.')

def Get_sources_recursive(base_dir, suffixes='.c', blacklist=[]):
    """
    Recursively search for all sources with the given suffix(es) in the
    base_dir and contained folders. If folder paths match a regex in the
    blacklist, don't look within them for sources.

    Return a list of all sources that fit this criteria.

    :param base_dir:
        The directory to start the recursive search in.

    :param suffixes:
        The list of, or single suffix to search recursively for.

    :param blacklist:
        A list of regexes to avoid looking in for objects.
    """

    blacklist = [re.compile(entry) for entry in blacklist]

    def in_blacklist(file_):
        if file_ is None:
            return True

        for entry in blacklist:
            if entry.search(Relpath(file_)):
                return True
        return False

    dirs_to_build = [dir_ for dir_ in Get_dirs(base_dir) if not in_blacklist(dir_)]

    # Add all the subdirs of subdirs not in the blacklist
    inner_subdirs = []
    for dir_ in dirs_to_build:
        subdirs = Get_subdirs(dir_)
        for subdir in subdirs:
            if not in_blacklist(subdir):
                inner_subdirs.append(subdir)

    dirs_to_build = inner_subdirs + dirs_to_build + [base_dir]

    sources = []
    for dir_ in dirs_to_build:
        dir_sources = Get_sources_in_dir(dir_, suffixes)
        if dir_sources is not None:
            sources.extend(dir_sources)

    return sources

def _make_file(file_):
    if type(file_) is str:
        file_ = File(file_)
    return file_

def objcopy_generator(source, target, env, for_signature):
    return '$OBJCOPY $OBJCOPYFLAGS %s -o %s'%(source[0], target[0])

OBJCPY_BUILDER = Builder(
        generator=objcopy_generator,
        suffix='',
        src_suffix='.o')

LD_BUILDER = Builder(
        action='$LD $LDFLAGS $SOURCES -o $TARGET',
        suffix='',
        src_suffix='.o')
