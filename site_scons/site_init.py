import os
import re


def Join_path(*subpath):
    """
    Return the joined path of all strings given as args.

    :param *subpath:
        A variable number of arguments to combine paths.
    """
    return os.path.join(*subpath)

def Get_subscripts(subdirs):
    """
    Return a list of all SConscripts in the given directories.

    :param subdirs:
        An iterable containing dirs to combine with 'SConscript'
    """
    return [Join_path(dir_, 'SConscript') for dir_ in subdirs]

def Get_dirs(dir_):
    """
    Return a list of directories in the given directory.

    :param dir_:
        The directory to list contained dirs of.
    """
    files = os.listdir(dir_)
    return filter(os.path.isdir, files)

def Get_subdirs(dir_):
    """
    Return a list of all subdirectories (any level of depth).

    :param dir_:
        The directory to list subdirs of.
    """
    return (tup[0] for tup in os.walk(dir_))

def Get_sources_in_dir(dir_, suffixes='.c'):
    """
    Return a list of files in a dir_ with given suffix(es).

    :param dir_:
        The directory to Glob suffixes for.

    :param suffixes:
        A suffix or list of suffixes to Glob the dir_ for.
    """

    # Accept a single suffix string as well as a list of them.
    if type(suffixes) is str:
        suffixes = [suffixes]

    # Add the * Glob to each suffix
    suffixes = ['*'+suffix for suffix in suffixes]

    sources = []
    for suffix in suffixes:
        pattern_sources = Glob(os.path.join(dir_, suffix))
        if pattern_sources:
            sources.extend(pattern_sources)
    return sources

def CURDIR():
    """Return the current directory."""
    return Dir('.').abspath

def Get_sources_recursive(start_dir, suffixes='.c', blacklist=[]):
    """
    Recursively search for all sources with the given suffix(es) in the
    start_dir and contained folders. If folder paths match a regex in the
    blacklist, don't look within them for sources.

    Return a list of all sources that fit this criteria.

    :param start_dir:
        The directory to start the recursive search in.

    :param suffixes:
        The list of, or single suffix to search recursively for.

    :param blacklist:
        A list of regexes to avoid looking in for objects.
    """

    blacklist = [re.compile(entry) for entry in blacklist]

    def not_in_blacklist(val):
        if val is None:
            return False

        for entry in blacklist:
            if entry.search(val):
                return False
        return True

    dirs_to_build = [dir_ for dir_ in Get_dirs(start_dir) if not_in_blacklist(dir_)]

    # Add all the subdirs of subdirs not in the blacklist
    inner_subdirs = []
    for dir_ in dirs_to_build:
        subdirs = Get_subdirs(dir_)
        for subdir in subdirs:
            if not_in_blacklist(subdir):
                inner_subdirs.append(subdir)

    dirs_to_build = inner_subdirs + dirs_to_build + [start_dir]

    sources = []
    for dir_ in dirs_to_build:
        dir_sources = Get_sources_in_dir(dir_, suffixes)
        if dir_sources is not None:
            sources.extend(dir_sources)

    return sources

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
