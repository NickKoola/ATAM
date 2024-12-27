#include <asm/desc.h>

void my_store_idt(struct desc_ptr *idtr) {
// <STUDENT FILL> - HINT: USE INLINE ASSEMBLY
asm volatile("sidt %0\n" 
                : "=m" (*idtr)   /* output */
                :               /* input */ 
                :               /* clobbered register */
                );
// </STUDENT FILL>
}

void my_load_idt(struct desc_ptr *idtr) {
// <STUDENT FILL> - HINT: USE INLINE ASSEMBLY
asm volatile("lidt %0\n" 
                :               /* output */
                : "m" (*idtr)   /* input */ 
                :               /* clobbered register */
                );
// <STUDENT FILL>
}

void my_set_gate_offset(gate_desc *gate, unsigned long addr) {
// <STUDENT FILL> - HINT: NO NEED FOR INLINE ASSEMBLY
    gate->offset_low = addr & 0xffff;
    gate->offset_middle = (addr >> 16) & 0xffff;
    gate->offset_high = (addr >> 32) & 0xffffffff;
// </STUDENT FILL>
}

unsigned long my_get_gate_offset(gate_desc *gate) {
// <STUDENT FILL> - HINT: NO NEED FOR INLINE ASSEMBLY
    unsigned long addr = gate->offset_high;
    unsigned long low = gate->offset_low;
    unsigned long middle = gate->offset_middle;
    addr = addr << 16;
    addr += middle;
    addr = addr << 16;
    addr += low;
    return addr;

// </STUDENT FILL>
}