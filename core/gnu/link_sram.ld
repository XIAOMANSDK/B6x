OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

MEMORY
{
    STACK (rw) : ORIGIN = 0x20003000, LENGTH = 0x600
    RAM  (rwx) : ORIGIN = 0x20003600, LENGTH = 0x4A00
}

SECTIONS
{
    _stext = ADDR(.text);
    _etext = ADDR(.text) + SIZEOF(.text);

    .text :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.text.user_irq_handler))

        *(.text*)

        *(.rodata*)
        *(.data*)

        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    .stack_dummy (COPY):
    {
        *(.stack*)
    } > STACK

    /* Discard all sections that are not explicitly mentioned in the linker
     * script */
    /*
    /DISCARD/ :
    {
        *(.debug)
        *(.debug*)
        *(.ARM.attributes)
        *(.comment)
        *(.stab)
        *(.stabstr)
    }
    */

    /* Set stack top to end of RAM, and stack limit move down by
     * size of stack_dummy section */
    __StackTop = ORIGIN(STACK) + LENGTH(STACK);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);
    /* 定义堆栈区 */
    _sstack = __StackLimit;
    _estack = __StackTop;
    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
