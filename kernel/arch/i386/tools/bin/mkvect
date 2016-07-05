#!/usr/bin/python
from __future__ import print_function

EXCEPTION_CASES = [
            8,      # Double fault
            10,     # Invalid TSS
            11,     # Segment not present
            12,     # Stack segment fault
            13,     # general protection
            14,     # Page fault
            17,     # Alignment check
        ]


def main():

    print(".globl tp_mktf")

    for x in range(256):
        print(".globl handle_int_%d" % x)
        print("handle_int_%d:" % x)
        if x not in EXCEPTION_CASES:
            print("\tpushl $0")
        print("\tpushl $%d" % x)
        print("\tjmp tp_mktf")

    print(".data")
    print(".globl trap_handlers")
    print("trap_handlers:")

    for x in range(256):
        print("\t.long handle_int_%d" % x)

if __name__ == '__main__':
    main()
