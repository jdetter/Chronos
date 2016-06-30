import os


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

    # TODO: Use a regex.
    dirs_to_build = [dir_ for dir_ in Get_dirs(start_dir) if dir_ and (dir_ not in blacklist)]

    # Add all the subdirs of subdirs.
    inner_subdirs = []
    for dir_ in dirs_to_build:
        subdirs = Get_subdirs(dir_)
        # TODO: Also check the blacklist doesn't match a dir.
        if subdirs is not None:
            inner_subdirs.extend(subdirs)

    dirs_to_build = inner_subdirs + dirs_to_build + [dir_]

    sources = []
    for dir_ in dirs_to_build:
        dir_sources = Get_sources_in_dir(dir_, suffixes)
        if dir_sources is not None:
            sources.extend(dir_sources)

    return sources
