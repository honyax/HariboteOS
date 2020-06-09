[BITS 32]
        MOV     AL, 'A'
        CALL    0xbfb
fin:
        HLT
        JMP fin
