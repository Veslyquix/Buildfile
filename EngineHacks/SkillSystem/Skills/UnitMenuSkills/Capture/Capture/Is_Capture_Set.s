.thumb
.org 0x0

@since I'm not 100% sure which bit is being used for Capture, I'll just call this function to return true if capturing
@r0=char data

@ Vesly made this always return 0 to take over 
@ so gaiden magic target & usability etc. can be used 
mov r0, #0 
b GoBack 


@ldr		r0,[r0,#0xC]
@mov		r1,#0x80
lsl		r1,#0x17
and		r0,r1
cmp		r0,#0x0
beq		GoBack
mov		r0,#0x1
GoBack:
bx		r14
