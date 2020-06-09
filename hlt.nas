[BITS 32]
        MOV     AL, 'A'
        CALL    2*8:0xbfb
fin:
        HLT
        JMP fin
