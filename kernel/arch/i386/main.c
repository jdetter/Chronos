

void arch_init(void (*main_func)(void))
{
        /* Initilize the primary tty */
        cprintf_init();
        /* Lets make sure that tty0 gets configured properly. */
        setup_kvm();
        cprintf("Welcome to the Chronos kernel!\n");

        /* Install global descriptor table */
        cprintf("Loading Global Descriptor table...\t\t\t\t\t");
        vm_seg_init();
        cprintf("[ OK ]\n");

        /* Get interrupt descriptor table up in case of a crash. */
        /* Install interrupt descriptor table */
        cprintf("Installing Interrupt Descriptor table...\t\t\t\t");
        trap_init();
        cprintf("[ OK ]\n");

        /* WARNING: we don't have a proper stack right now. */
        /* Get vm up */
        cprintf("Initilizing Virtual Memory...\t\t\t\t\t\t");
        vm_init();
        cprintf("[ OK ]\n");

        /* Setup proper stack */
        pstack_t new_stack = KVM_KSTACK_E;
        k_stack = new_stack;

        cprintf("Switching over to kernel stack...\t\t\t\t\t");
        vm_set_stack(PGROUNDUP(new_stack), (uint)main_stack);
}
