
# �f�t�H���g����

default :
	../z_tools/make.exe img

# �t�@�C�������K��

ipl.bin : ipl.nas Makefile
	../z_tools/nask.exe ipl.nas ipl.bin ipl.lst

honya.img : ipl.bin Makefile
	../z_tools/edimg.exe   imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0   imgout:honya.img

# �R�}���h

asm :
	../z_tools/make.exe -r ipl.bin

img :
	../z_tools/make.exe -r honya.img

run :
	../z_tools/make.exe img
	copy honya.img ..\z_tools\qemu\fdimage0.bin
	../z_tools/make.exe -C ../z_tools/qemu

clean :
	-del ipl.bin
	-del ipl.lst

src_only :
	../z_tools/make.exe clean
	-del honya.img
