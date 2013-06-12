bits 32

section .data
    TSCEvery1us     dd 0

section .text
    extern StartVirtualization
    extern VmExitHandler

    global _ScaleTSCBasedTimer
    global _CpuSleep
    global _FlushTLB
    global TSCEvery1us

    global _Reboot
    global _BeepOn
    global _BeepOff

    global READ_PORT_UCHAR
    global READ_PORT_USHORT
    global READ_PORT_ULONG
    global WRITE_PORT_UCHAR
    global WRITE_PORT_USHORT
    global WRITE_PORT_ULONG
    global _ReadPRT
    global _WritePRT

    global _VmCallFillScreen
    global _Return1
    global _ReadMSR
    global _WriteMSR
    global  _TSC

    global _CS
    global _DS
    global _ES
    global _FS
    global _GS
    global _SS

    global _CR0
    global _CR2
    global _CR3
    global _CR4

    global _DR0
    global _DR1
    global _DR2
    global _DR3
    global _DR6
    global _DR7

    global _SetCR0
    global _SetCR2
    global _SetCR3
    global _SetCR4

    global _SetDR0
    global _SetDR1
    global _SetDR2
    global _SetDR3
    global _SetDR6
    global _SetDR7

    global _CpuId

    global _Ldtr
    global _TrSelector
    global _Eflags
    global _Invd

    global _GdtBase
    global _IdtBase
    global _GdtLimit
    global _IdtLimit
    global _RegSetIdtr
    global _RegSetGdtr
    global _DisableWP
    global _EnableWP

    global _VmxOn
    global _VmClear
    global _VmPtrLd
    global _ReadVMCS
    global _WriteVMCS
    global _VmLaunch
    global _VmxOff
    global _VmxOff_NoGuest
    global _VmFailValid
    global _VmFailInvalid

    global _StartVirtualization
    global _GuestEntryPoint

    global _StopVirtualization
    global _GuestExit

    global _VmExitHandler

_VmCallFillScreen:
    mov     eax,0deadc0deh
    vmcall
    ret

_Read8254Counter0:
    xor     eax,eax
    out     43h,al
    in      al,40h
    mov     ah,al
    in      al,40h
    xchg    al,ah
    ret

_ScaleTSCBasedTimer:
    push    ebp
    mov     ebp,esp
    sub     esp,8
    pushad
    rdtsc
    mov     [ebp-4],eax     ;StartTSC
    call    _Read8254Counter0
    mov     [ebp-8],eax     ;StartCount
LoopRead:
    call    _Read8254Counter0
    mov     ecx,[ebp-8]
    sub     ecx,eax
    cmp     cx,59659        ;25ms (1193181/(1000/25*2))
    jb      LoopRead
    rdtsc
    sub     eax,[ebp-4]     ;TSC Every 25ms
    xor     edx,edx
    mov     ecx,25000
    div     ecx
    mov     [TSCEvery1us],eax
    popad
    leave
    ret

_CpuSleep:
    push    ebp
    mov     ebp,esp
    sub     esp,10h
    pushad
    rdtsc
    mov     [ebp-4],eax
    mov     [ebp-8],edx
    mov     eax,[ebp+8]     ;Sleep x MicroSecond
    mov     ecx,[TSCEvery1us]
    mul     ecx
    mov     [ebp-0ch],eax
    mov     [ebp-10h],edx
LoopWait:
    rdtsc
    sub     eax,[ebp-4]
    sbb     edx,[ebp-8]
    cmp     eax,[ebp-0ch]
    jb      LoopWait
    popad
    leave
    ret

_FlushTLB:
    mov     eax,cr3
    mov     cr3,eax
    ret

_Reboot:
    mov     edx,64h
    mov     eax,0feh
    out     dx,al
    ret

_BeepOn:
    mov     al,0b6h
    out     43h,al      ; Timer 8253-5 (AT: 8254.2).

    mov     eax,1500h
    out     42h,al

    mov     al,ah
    out     42h,al

    ; speaker ON
    in      al,61h
    or      al,3
    out     61h,al
deadloop:
    jmp     deadloop
    ret

_BeepOff:
    ; speaker OFF
    in      al,61h
    and     al,0fch
    out     61h,al
    ret

READ_PORT_UCHAR:
    xor     eax,eax
    mov     edx,[esp+4]
    in      al,dx
    ret

READ_PORT_USHORT:
    xor     eax,eax
    mov     edx,[esp+4]
    in      ax,dx
    ret

READ_PORT_ULONG:
    mov     edx,[esp+4]
    in      eax,dx
    ret

WRITE_PORT_UCHAR:
    mov     edx,[esp+4]
    mov     eax,[esp+8]
    out     dx,al
    ret

WRITE_PORT_USHORT:
    mov     edx,[esp+4]
    mov     eax,[esp+8]
    out     dx,ax
    ret

WRITE_PORT_ULONG:
    mov     edx,[esp+4]
    mov     eax,[esp+8]
    out     dx,eax
    ret

_ReadPRT:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     ecx,[ebp+8]
    mov     eax,[ebp+0ch]
    add     eax,10h
    mov     [ecx],eax
    mov     eax,[ecx+10h]
    pop     ecx
    leave
    ret

_WritePRT:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     ecx,[ebp+8]
    mov     eax,[ebp+0ch]
    add     eax,10h
    mov     [ecx],eax
    mov     eax,[ebp+10h]
    mov     [ecx+10h],eax
    pop     ecx
    leave
    ret


_Return1:
    mov     eax,1
    ret

_ReadMSR:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     ecx,[ebp+8]
    rdmsr
    pop     ecx
    leave
    ret

_WriteMSR:
    push    ebp
    mov     ebp,esp
    pushad
    mov     ecx,[ebp+8]
    mov     eax,[ebp+0ch]
    mov     edx,[ebp+10h]
    wrmsr
    popad
    leave
    ret

_TSC:
    rdtsc
    ret

_CS:
    xor     eax,eax
    mov     ax,cs
    ret
_DS:
    xor     eax,eax
    mov     ax,ds
    ret
_ES:
    xor     eax,eax
    mov     ax,es
    ret
_FS:
    xor     eax,eax
    mov     ax,fs
    ret
_GS:
    xor     eax,eax
    mov     ax,gs
    ret
_SS:
    xor     eax,eax
    mov     ax,ss
    ret

_CR0:
    mov     eax,cr0
    ret
_CR2:
    mov     eax,cr2
    ret
_CR3:
    mov     eax,cr3
    ret
_CR4:
    mov     eax,cr4
    ret

_DR0:
    mov     eax,dr0
    ret
_DR1:
    mov     eax,dr1
    ret
_DR2:
    mov     eax,dr2
    ret
_DR3:
    mov     eax,dr3
    ret
_DR6:
    mov     eax,dr6
    ret
_DR7:
    mov     eax,dr7
    ret

_SetCR0:
    mov     eax,[esp+4]
    mov     cr0,eax
    ret
_SetCR2:
    mov     eax,[esp+4]
    mov     cr2,eax
    ret
_SetCR3:
    mov     eax,[esp+4]
    mov     cr3,eax
    ret
_SetCR4:
    mov     eax,[esp+4]
    mov     cr4,eax
    ret

_SetDR0:
    mov     eax,[esp+4]
    mov     dr0,eax
    ret
_SetDR1:
    mov     eax,[esp+4]
    mov     dr1,eax
    ret
_SetDR2:
    mov     eax,[esp+4]
    mov     dr2,eax
    ret
_SetDR3:
    mov     eax,[esp+4]
    mov     dr3,eax
    ret
_SetDR6:
    mov     eax,[esp+4]
    mov     dr6,eax
    ret
_SetDR7:
    mov     eax,[esp+4]
    mov     dr7,eax
    ret

_CpuId:
    push    ebp
    mov     ebp,esp
    pushad
    mov     esi,[ebp+8]
    mov     eax,[esi]
    mov     esi,[ebp+0ch]
    mov     ebx,[esi]
    mov     esi,[ebp+10h]
    mov     ecx,[esi]
    mov     esi,[ebp+14h]
    mov     edx,[esi]
    cpuid
    mov     esi,[ebp+8]
    mov     [esi],eax
    mov     esi,[ebp+0ch]
    mov     [esi],ebx
    mov     esi,[ebp+10h]
    mov     [esi],ecx
    mov     esi,[ebp+14h]
    mov     [esi],edx
    popad
    leave
    ret

_Ldtr:
    sldt    ax
    ret

_TrSelector:
    str     ax
    ret

_Eflags:
    pushfd
    pop     eax
    ret

_Invd:
    invd
    ret

_GdtBase:
    push    ebp
    mov     ebp,esp
    sub     esp,8
    sgdt    [ebp-8]
    mov     eax,[ebp-6]
    leave
    ret

_IdtBase:
    push    ebp
    mov     ebp,esp
    sub     esp,8
    sidt    [ebp-8]
    mov     eax,[ebp-6]
    leave
    ret

_GdtLimit:
    push    ebp
    mov     ebp,esp
    sub     esp,8
    sgdt    [ebp-8]
    mov     ax,[ebp-8]
    leave
    ret

_IdtLimit:
    push    ebp
    mov     ebp,esp
    sub     esp,8
    sidt    [ebp-8]
    mov     ax,[ebp-8]
    leave
    ret

_RegSetIdtr:
    push    ebp
    mov     ebp,esp
    push    dword [ebp+8]
    mov     eax,[ebp+0ch]
    shl     eax,16
    push    eax
    lidt    [esp+2]
    add     esp,8
    leave
    ret

_RegSetGdtr:
    push    ebp
    mov     ebp,esp
    push    dword [ebp+8]
    mov     eax,[ebp+0ch]
    shl     eax,16
    push    eax
    lgdt    [esp+2]
    add     esp,8
    leave
    ret

_DisableWP:
    mov     eax,cr0
    and     eax,0fffeffffh
    mov     cr0,eax
    ret

_EnableWP:
    mov     eax,cr0
    or      eax,10000h
    mov     cr0,eax
    ret

_VmxOn:
    push    ebp
    mov     ebp,esp
    push    dword [ebp+0ch]
    push    dword [ebp+8]
    vmxon   qword [esp]
    add     esp,8
    pushfd
    pop     eax
    leave
    ret

_VmClear:
    push    ebp
    mov     ebp,esp
    push    dword [ebp+0ch]
    push    dword [ebp+8]
    vmclear qword [esp]
    add     esp,8
    pushfd
    pop     eax
    leave
    ret

_VmPtrLd:
    push    ebp
    mov     ebp,esp
    push    dword [ebp+0ch]
    push    dword [ebp+8]
    vmptrld qword [esp]
    add     esp,8
    pushfd
    pop     eax
    leave
    ret

_ReadVMCS:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     eax,[ebp+8]
    vmread  ecx,eax
    mov     eax,ecx
    pop     ecx
    leave
    ret

_WriteVMCS:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     eax,[ebp+8]
    mov     ecx,[ebp+0ch]
    vmwrite eax,ecx
    pop     ecx
    leave
    ret

_VmFailInvalid:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     eax,[ebp+8]
    xor     ecx,ecx
    bt      eax,0    ;RFLAGS.CF
    adc     cl,cl
    mov     eax,ecx
    pop     ecx
    leave
    ret

_VmFailValid:
    push    ebp
    mov     ebp,esp
    push    ecx
    mov     eax,[ebp+8]
    xor     ecx,ecx
    bt      eax,6    ;RFLAGS.ZF
    adc     cl,cl
    mov     eax,ecx
    pop     ecx
    leave
    ret

_VmLaunch:
    vmlaunch
    pushfd
    pop     eax
    ret

_VmxOff:
    push    ebp
    mov     ebp,esp

    push    00006818h    ;GUEST_IDTR_BASE
    call    _ReadVMCS
    add     esp,4
    push    eax
    push    00004812h    ;GUEST_IDTR_LIMIT
    call    _ReadVMCS
    add     esp,4
    shl     eax,16
    push    eax
    lidt    [esp+2]
    add     esp,8

    push    00006816h    ;GUEST_GDTR_BASE
    call    _ReadVMCS
    add     esp,4
    push    eax
    push    00004810h    ;GUEST_GDTR_LIMIT
    call    _ReadVMCS
    add     esp,4
    shl     eax,16
    push    eax
    lgdt    [esp+2]
    add     esp,8

    push    00006800h    ;GUEST_CR0
    call    _ReadVMCS
    add     esp,4
    mov     cr0,eax

    push    00006802h    ;GUEST_CR3
    call    _ReadVMCS
    add     esp,4
    mov     cr3,eax

    push    00006804h    ;GUEST_CR4
    call    _ReadVMCS
    add     esp,4
    mov     cr4,eax

    mov     eax,[ebp+0ch]
    mov     ecx,[ebp+8]
    vmxoff
    leave
    mov     esp,ecx
    push    eax
    mov     eax,cr4
    and     eax,0ffffdfffh
    mov     cr4,eax

    xor     eax,eax
    mov     dr0,eax
    mov     dr1,eax
    mov     dr2,eax
    mov     dr3,eax
    mov     eax,400h
    mov     dr7,eax

    pop     eax
    jmp     eax

_VmxOff_NoGuest:
    vmxoff
    ret

_StartVirtualization:
    pushad
    pushfd
    cli
    push    esp
    call    StartVirtualization
    add     esp,4

_GuestEntryPoint:
    mov     [esp+20h],eax
    sti
    popfd
    popad
    ret

_StopVirtualization:
    push    ebp
    mov     ebp,esp
    pushad
    pushfd
    push    ebp
    mov     eax,[ebp+8]
    mov     edx,[ebp+0ch]
    vmcall

_GuestExit:
    pop     ebp
    popfd
    popad
    leave
    ret

_VmExitHandler:
    cli
    push    edi
    push    esi
    push    ebp
    push    esp    ;invalid esp
    push    edx
    push    ecx
    push    ebx
    push    eax

    lea     eax,[esp+20h]    ;PGUEST_CPU
    push    eax
    call    VmExitHandler
    add     esp,4

    pop     eax
    pop     ebx
    pop     ecx
    pop     edx
    pop     ebp    ;pop esp
    pop     ebp
    pop     esi
    pop     edi
    vmresume
