#!/usr/bin/python
import sys, os


TEXT_MAX = (512 * 128)
DATA_MAX = (512 * 4)
RODATA_MAX = (512 * 4)
BSS_MAX = (512 * 4)
BASENAME ='./boot-stage2'
FILES = {'.text':TEXT_MAX,
        '.data':DATA_MAX,
        '.bss':BSS_MAX,
        'rodata':RODATA_MAX}

FATAL_ERROR_STRING = "FATAL ERROR: Boot stage 2"

def main():

    for file_ext,max_size in FILES:
        file_size = os.stat(BASENAME.join(file_ext)).st_size
        if file_size is None:
            print(FATAL_ERROR_STRING)
            print('Unable to stat' + file_ext + ' file.')
            return 1

        if  file_size > max_size:
            print(FATAL_ERROR_STRING)
            print(file_ext + ' section too large: ' + file_size)
            return 1

    print('Boot stage 2 has been verified.')
    return 0

if __name__ == 'main':
    main()
