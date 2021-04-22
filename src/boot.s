.global _start
_start:
    ;@ Clear BSS
    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0
initbss:
    cmp r0, r1
    beq endbss
    str r2, [r0]
    add r0, #4
    b initbss
endbss:

    ;@ Initialize volatile data
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_etext
initdata:
    cmp r0, r1
    beq enddata
    ldr r3, [r2]
    str r3, [r0]
    add r0, #4
    add r2, #4
    b initdata
enddata:

    ;@ Initialize stack
    ;@ldr r0, =_estack
    ;@mov sp, r0

    ;@ Squadala!
    b main

done:
    b done
