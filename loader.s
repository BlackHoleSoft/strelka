;Napalm - access bios from pmode!!!

section .__mbHeader
  align 0x4                         ; the code must be 4 byte aligned
  mboot:

    MAGIC_NUMBER equ 0x1BADB002     ; define the magic number constant
    FLAGS        equ  0      ; multiboot flags
    CHECKSUM     equ -(MAGIC_NUMBER + FLAGS)  ; calculate the checksum
                                  ; (magic number + checksum + flags should equal 0)

    dd MAGIC_NUMBER             ; write the magic number to the machine code,
    dd FLAGS                    ; the flags,
    dd CHECKSUM                 ; and the checksum

    dd mboot
    dd $$
    dd bss
    dd kernel_end
    dd kernel_start

    ;dd 0
    ;dd 0
    ;dd 0
    ;dd 0
    ;dd 0

    ;dd 0
    ;dd 640 ;width
    ;dd 480 ;height
    ;dd 24 ;bpp

    ;resb 1024*8 - 4*3

section .bss
align 4                                     ; align at 4 bytes
bss:

KERNEL_STACK_SIZE equ 4096      ; size of stack in bytes

kernel_stack:                               ; label points to beginning of memory
        resb KERNEL_STACK_SIZE                  ; reserve stack for the kernel


section .text                  ; start of the text (code) section
align 0x4
  kernel_start:

    global loader                   ; the entry symbol for ELF
    global outb
    global inb
    global set_gdt
    global load_idt
    global bios_test
    extern kernel_main
    extern interrupt_handler

; outb - send a byte to an I/O port
; stack: [esp + 8] the data byte
;        [esp + 4] the I/O port
;        [esp    ] return address

;common_interrupt_handler:               ; the common parts of the generic interrupt handler
;    ; save the registers
;    push    eax
;    push    ebx
;    push    ecx
;    push    edx
;    push    ebp

    ; call the C function
;    call    interrupt_handler

    ; restore the registers
;    pop     ebp

;    pop     edx
;    pop     ecx
;    pop     ebx
;    pop     eax

    ; restore the esp
;    add     esp, 8

    ; return to the code that got interrupted
;    iret



gdtr DW 0 ; For limit storage
     DD 0 ; For base storage

set_gdt:
   MOV   EAX, [esp + 4]
   MOV   [gdtr + 2], EAX
   MOV   AX, [ESP + 8]
   MOV   [gdtr], AX
   LGDT  [gdtr]
   JMP   0x08:reload_CS ; 0x08 points at the new code selector
   RET

reload_CS:
   ; Reload data segment registers:
   MOV   AX, 0x10 ; 0x10 points at the new data selector
   MOV   DS, AX
   MOV   ES, AX
   MOV   FS, AX
   MOV   GS, AX
   MOV   SS, AX
   RET

outb:
    mov al, [esp + 8]    ; move the data to be sent into the al register
    mov dx, [esp + 4]    ; move the address of the I/O port into the dx register
    out dx, al           ; send the data to the I/O port
    ret                  ; return to the calling function

inb:
    mov dx, [esp + 4]       ; move the address of the I/O port to the dx register
    xor al, al
    in  al, dx              ; read a byte from the I/O port and store it in the al register
    ret                     ; return the read byte

load_idt:
    mov     eax, [esp+4]    ; load the address of the IDT into register eax
    lidt    [eax]            ; load the IDT
    ret                     ; return to the calling function

bios_test:
    mov ah, 0x0e    ; function number = 0Eh : Display Character
    mov al, '!'     ; AL = code of character to display
    int 0x10        ; call INT 10h, BIOS video service


loader:                         ; the loader label (defined as entry point in linker script)
    mov esp, kernel_stack + KERNEL_STACK_SIZE   ; init stack at the end position esp - stack poiner

    ;no_error_code_interrupt_handler 0       ; create handler for interrupt 0

    ;push ebx
    call kernel_main

kernel_end:

loop:
    jmp loop


;
; Protected Mode BIOS Call Functionailty v2.0 - by Napalm
; -------------------------------------------------------
;
; This is code shows how its POSSIBLE to execute BIOS interrupts
; by switch out to real-mode and then back into protected mode.
;
; If you wish to use all or part of this code you must agree
; to the license at the following URL.
;
; License: http://creativecommons.org/licenses/by-sa/2.0/uk/
;
; Notes: This file is in NASM syntax.
;        Turn off paging before calling these functions.
;        int32() resets all selectors.
;
; C Prototype:
;   void _cdelc int32(unsigned char intnum, regs16_t *regs);
;
; Example of usage:
;   regs.ax = 0x0013;
;   int32(0x10, &regs);
;   memset((char *)0xA0000, 1, (320*200));
;   memset((char *)0xA0000 + (100*320+80), 14, 80);
;   regs.ax = 0x0000;
;   int32(0x16, &regs);
;   regs.ax = 0x0003;
;   int32(0x10, &regs);
;
;
[bits 32]

global int32, _int32

struc regs16_t
   .di   resw 1
   .si   resw 1
   .bp   resw 1
   .sp resw 1
   .bx   resw 1
   .dx   resw 1
   .cx   resw 1
   .ax   resw 1
   .gs   resw 1
   .fs   resw 1
   .es   resw 1
   .ds   resw 1
   .ef resw 1
endstruc

%define INT32_BASE                             0x7C00
%define REBASE(x)                              (((x) - reloc) + INT32_BASE)
%define GDTENTRY(x)                            ((x) << 3)
%define CODE32                                 GDTENTRY(1)   ; 0x08
%define DATA32                                 GDTENTRY(2)   ; 0x10
%define CODE16                                 GDTENTRY(3)   ; 0x18
%define DATA16                                 GDTENTRY(4)   ; 0x20
%define STACK16                                (INT32_BASE - regs16_t_size)


section .text
   int32: use32                               ; by Napalm
   _int32:
      cli                                    ; disable interrupts
      pusha                                  ; save register state to 32bit stack
      mov  esi, reloc                        ; set source to code below
      mov  edi, INT32_BASE                   ; set destination to new base address
      mov  ecx, (int32_end - reloc)          ; set copy size to our codes size
      cld                                    ; clear direction flag (so we copy forward)
      rep  movsb                             ; do the actual copy (relocate code to low 16bit space)
      jmp INT32_BASE                         ; jump to new code location
   reloc: use32                               ; by Napalm
      mov  [REBASE(stack32_ptr)], esp        ; save 32bit stack pointer
      sidt [REBASE(idt32_ptr)]               ; save 32bit idt pointer
      sgdt [REBASE(gdt32_ptr)]               ; save 32bit gdt pointer
      lgdt [REBASE(gdt16_ptr)]               ; load 16bit gdt pointer
      lea  esi, [esp+0x24]                   ; set position of intnum on 32bit stack
      lodsd                                  ; read intnum into eax
      mov  [REBASE(ib)], al                  ; set intrrupt immediate byte from our arguments
      mov  esi, [esi]                        ; read regs pointer in esi as source
      mov  edi, STACK16                      ; set destination to 16bit stack
      mov  ecx, regs16_t_size                ; set copy size to our struct size
      mov  esp, edi                          ; save destination to as 16bit stack offset
      rep  movsb                             ; do the actual copy (32bit stack to 16bit stack)
      jmp  word CODE16:REBASE(p_mode16)      ; switch to 16bit selector (16bit protected mode)
   p_mode16: use16
      mov  ax, DATA16                        ; get our 16bit data selector
      mov  ds, ax                            ; set ds to 16bit selector
      mov  es, ax                            ; set es to 16bit selector
      mov  fs, ax                            ; set fs to 16bit selector
      mov  gs, ax                            ; set gs to 16bit selector
      mov  ss, ax                            ; set ss to 16bit selector
      mov  eax, cr0                          ; get cr0 so we can modify it
      and  al,  ~0x01                        ; mask off PE bit to turn off protected mode
      mov  cr0, eax                          ; set cr0 to result
      jmp  word 0x0000:REBASE(r_mode16)      ; finally set cs:ip to enter real-mode
   r_mode16: use16
      xor  ax, ax                            ; set ax to zero
      mov  ds, ax                            ; set ds so we can access idt16
      mov  ss, ax                            ; set ss so they the stack is valid
      lidt [REBASE(idt16_ptr)]               ; load 16bit idt
      mov  bx, 0x0870                        ; master 8 and slave 112
      call resetpic                          ; set pic's the to real-mode settings
      popa                                   ; load general purpose registers from 16bit stack
      pop  gs                                ; load gs from 16bit stack
      pop  fs                                ; load fs from 16bit stack
      pop  es                                ; load es from 16bit stack
      pop  ds                                ; load ds from 16bit stack
      sti                                    ; enable interrupts
      db 0xCD                                ; opcode of INT instruction with immediate byte
   ib: db 0x00
      cli                                    ; disable interrupts
      xor  sp, sp                            ; zero sp so we can reuse it
      mov  ss, sp                            ; set ss so the stack is valid
      mov  sp, INT32_BASE                    ; set correct stack position so we can copy back
      pushf                                  ; save eflags to 16bit stack
      push ds                                ; save ds to 16bit stack
      push es                                ; save es to 16bit stack
      push fs                                ; save fs to 16bit stack
      push gs                                ; save gs to 16bit stack
      pusha                                  ; save general purpose registers to 16bit stack
      mov  bx, 0x2028                        ; master 32 and slave 40
      call resetpic                          ; restore the pic's to protected mode settings
      mov  eax, cr0                          ; get cr0 so we can modify it
      inc  eax                               ; set PE bit to turn on protected mode
      mov  cr0, eax                          ; set cr0 to result
      jmp  dword CODE32:REBASE(p_mode32)     ; switch to 32bit selector (32bit protected mode)
   p_mode32: use32
      mov  ax, 0x10                        ; get our 32bit data selector
      mov  ds, ax                            ; reset ds selector
      mov  es, ax                            ; reset es selector
      mov  fs, ax                            ; reset fs selector
      mov  gs, ax                            ; reset gs selector
      mov  ss, ax                            ; reset ss selector
      lgdt [REBASE(gdt32_ptr)]               ; restore 32bit gdt pointer
      lidt [REBASE(idt32_ptr)]               ; restore 32bit idt pointer
      mov  esp, [REBASE(stack32_ptr)]        ; restore 32bit stack pointer
      mov  esi, STACK16                      ; set copy source to 16bit stack
      lea  edi, [esp+0x28]                   ; set position of regs pointer on 32bit stack
      mov  edi, [edi]                        ; use regs pointer in edi as copy destination
      mov  ecx, regs16_t_size                ; set copy size to our struct size
      cld                                    ; clear direction flag (so we copy forward)
      rep  movsb                             ; do the actual copy (16bit stack to 32bit stack)
      popa                                   ; restore registers
      ;sti                                    ; enable interrupts
      ;ret                                    ; return to caller

      JMP   0x08:reload_CS
      ret


   resetpic:                                  ; reset's 8259 master and slave pic vectors
      push ax                                ; expects bh = master vector, bl = slave vector
      mov  al, 0x11                          ; 0x11 = ICW1_INIT | ICW1_ICW4
      out  0x20, al                          ; send ICW1 to master pic
      out  0xA0, al                          ; send ICW1 to slave pic
      mov  al, bh                            ; get master pic vector param
      out  0x21, al                          ; send ICW2 aka vector to master pic
      mov  al, bl                            ; get slave pic vector param
      out  0xA1, al                          ; send ICW2 aka vector to slave pic
      mov  al, 0x04                          ; 0x04 = set slave to IRQ2
      out  0x21, al                          ; send ICW3 to master pic
      shr  al, 1                             ; 0x02 = tell slave its on IRQ2 of master
      out  0xA1, al                          ; send ICW3 to slave pic
      shr  al, 1                             ; 0x01 = ICW4_8086
      out  0x21, al                          ; send ICW4 to master pic
      out  0xA1, al                          ; send ICW4 to slave pic
      pop  ax                                ; restore ax from stack
      ret                                    ; return to caller

   stack32_ptr:                               ; address in 32bit stack after we
      dd 0x00000000                          ;   save all general purpose registers

   idt32_ptr:                                 ; IDT table pointer for 32bit access
      dw 0x0000                              ; table limit (size)
      dd 0x00000000                          ; table base address

   gdt32_ptr:                                 ; GDT table pointer for 32bit access
      dw 0x0018                              ; table limit (size)
      dd 0x00130000                          ; table base address

   idt16_ptr:                                 ; IDT table pointer for 16bit access
      dw 0x03FF                              ; table limit (size)
      dd 0x00000000                          ; table base address

   gdt16_base:                                ; GDT descriptor table
      .null:                                 ; 0x00 - null segment descriptor
         dd 0x00000000                      ; must be left zero'd
         dd 0x00000000                      ; must be left zero'd

      .code32:                               ; 0x01 - 32bit code segment descriptor 0xFFFFFFFF
         dw 0xFFFF                          ; limit  0:15
         dw 0x0000                          ; base   0:15
         db 0x00                            ; base  16:23
         db 0x9A                            ; present, iopl/0, code, execute/read
         db 0xCF                            ; 4Kbyte granularity, 32bit selector; limit 16:19
         db 0x00                            ; base  24:31

      .data32:                               ; 0x02 - 32bit data segment descriptor 0xFFFFFFFF
         dw 0xFFFF                          ; limit  0:15
         dw 0x0000                          ; base   0:15
         db 0x00                            ; base  16:23
         db 0x92                            ; present, iopl/0, data, read/write
         db 0xCF                            ; 4Kbyte granularity, 32bit selector; limit 16:19
         db 0x00                            ; base  24:31

      .code16:                               ; 0x03 - 16bit code segment descriptor 0x000FFFFF
         dw 0xFFFF                          ; limit  0:15
         dw 0x0000                          ; base   0:15
         db 0x00                            ; base  16:23
         db 0x9A                            ; present, iopl/0, code, execute/read
         db 0x0F                            ; 1Byte granularity, 16bit selector; limit 16:19
         db 0x00                            ; base  24:31

      .data16:                               ; 0x04 - 16bit data segment descriptor 0x000FFFFF
         dw 0xFFFF                          ; limit  0:15
         dw 0x0000                          ; base   0:15
         db 0x00                            ; base  16:23
         db 0x92                            ; present, iopl/0, data, read/write
         db 0x0F                            ; 1Byte granularity, 16bit selector; limit 16:19
         db 0x00                            ; base  24:31

   gdt16_ptr:                                 ; GDT table pointer for 16bit access
      dw gdt16_ptr - gdt16_base - 1          ; table limit (size)
      dd gdt16_base                          ; table base address

   int32_end:                                 ; end marker (so we can copy the code)
