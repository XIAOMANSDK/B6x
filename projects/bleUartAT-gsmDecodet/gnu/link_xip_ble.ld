OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(Reset_Handler)

#include "blelib.h"
#include "cfg.h"

MEMORY
{
    FLASH (rx) : ORIGIN = 0x18004000, LENGTH = 0x1C000
    RAM   (rw) : ORIGIN = 0x20003000, LENGTH = 0x5000
    EM    (rw) : ORIGIN = 0x20008000, LENGTH = 0x2000
}

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */

SECTIONS
{
    _stext = ADDR(.text);
    _etext = ADDR(.text) + SIZEOF(.text);

    .text :
    {
        KEEP(*(.isr_vector))
        KEEP(*(.text.user_irq_handler))

        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        *(.rodata*)
        *(ri_*func)
        *(le_*func)
        KEEP(*(.eh_frame*))
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    /* To copy multiple ROM to RAM sections,
     * uncomment .copy.table section and,
     * define __STARTUP_COPY_MULTIPLE in startup_ARMCMx.S */
    /*
    .copy.table :
    {
        . = ALIGN(4);
        __copy_table_start__ = .;
        LONG (__etext)
        LONG (__data_start__)
        LONG (__data_end__ - __data_start__)
        LONG (__etext2)
        LONG (__data2_start__)
        LONG (__data2_end__ - __data2_start__)
        __copy_table_end__ = .;
    } > FLASH
    */

    /* To clear multiple BSS sections,
     * uncomment .zero.table section and,
     * define __STARTUP_CLEAR_BSS_MULTIPLE in startup_ARMCMx.S */
    /*
    .zero.table :
    {
        . = ALIGN(4);
        __zero_table_start__ = .;
        LONG (__bss_start__)
        LONG (__bss_end__ - __bss_start__)
        LONG (__bss2_start__)
        LONG (__bss2_end__ - __bss2_start__)
        __zero_table_end__ = .;
    } > FLASH
    */

    __etext = ALIGN (4);

    .vector (NOLOAD) :
    {
        _vector_start  = .;
        /* Vector Table Size */
        . = . + 152;
        _vector_end = .;
    } > RAM

    .data : AT (__etext)
    {
        . = ALIGN(4);
        __data_start__ = .;

        *(ram_func*)
        *(.data*)

        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM

    /*
    .heap (COPY):
    {
        __end__ = .;
        PROVIDE(end = .);
        *(.heap*)
        __HeapLimit = .;
    } > RAM
    */

    /*
    .ble_heap BLE_HEAP_BASE (NOLOAD) :
    {
        __BleHeapBase = .;
        . += BLE_HEAP_SIZE;
        __BleHeapLimit = .;
    } > RAM
    */

    /* .stack_dummy section doesn't contains any symbols. It is only
     * used for linker to calculate size of stack sections, and assign
     * values to stack symbols later */
    /*.stack_dummy (COPY):*/
    .stack_dummy (NOLOAD):
    {
        *(.stack*)
    } > RAM

    /* BLE Exchange Memory Section */
    .ble_exchange (NOLOAD) :
    {
        __ble_exchange_start = .;
        . = ALIGN(4);
        /* Reserve space for BLE Exchange Memory */
         . += BLE_EXCH_SIZE;
         . += BLE_HEAP_SIZE;
        *libble6*.a:*(var*mem*)
        *(user_retention)
        __ble_exchange_end = .;
    } > EM

    /* Ensure BLE Exchange Memory fits within EM region */
    ASSERT(__ble_exchange_end <= (ORIGIN(EM) + LENGTH(EM)),
           "BLE Exchange Memory exceeds EM region size")

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
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    PROVIDE(__stack = __StackTop);

    _sstack = __StackLimit;
    _estack = __StackTop;
    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}