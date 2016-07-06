from SCons.Script import *

EXCEPTION_CASES = [
            8,      # Double fault
            10,     # Invalid TSS
            11,     # Segment not present
            12,     # Stack segment fault
            13,     # general protection
            14,     # Page fault
            17,     # Alignment check
        ]

def generate_idt(source, target, env):

    filename = Path(target[0])

    file_ = open(filename, 'w')

    file_.write(".globl tp_mktf\n")

    for x in range(256):
        file_.write(".globl handle_int_%d\n" % x)
        file_.write("handle_int_%d:\n" % x)
        if x not in EXCEPTION_CASES:
            file_.write("\tpushl $0\n")
        file_.write("\tpushl $%d\n" % x)
        file_.write("\tjmp tp_mktf\n")

    file_.write(".data\n")
    file_.write(".globl trap_handlers\n")
    file_.write("trap_handlers:\n")

    for x in range(256):
        file_.write("\t.long handle_int_%d\n" % x)
