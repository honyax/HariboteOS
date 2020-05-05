TOOLPATH = ../z_tools/
MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# デフォルト動作

default :
	$(MAKE) img

# ファイル生成規則

ipl.bin : ipl.nas Makefile
	$(NASK) ipl.nas ipl.bin ipl.lst

honyaos.sys : honyaos.nas Makefile
	$(NASK) honyaos.nas honyaos.sys honyaos.lst

honyaos.img : ipl.bin honyaos.sys Makefile
	$(EDIMG)    imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl.bin len:512 from:0 to:0 \
		copy from:honyaos.sys to:@: \
		imgout:honyaos.img

# コマンド

img :
	$(MAKE) honyaos.img

run :
	$(MAKE) img
	$(COPY) honyaos.img ..\z_tools\qemu\fdimage0.bin
	$(MAKE) -C ../z_tools/qemu

clean :
	-$(DEL) ipl.bin
	-$(DEL) ipl.lst
	-$(DEL) honyaos.sys
	-$(DEL) honyaos.lst

src_only :
	$(MAKE) clean
	-$(DEL) honyaos.img
