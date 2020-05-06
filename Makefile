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

ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

honyaos.sys : honyaos.nas Makefile
	$(NASK) honyaos.nas honyaos.sys honyaos.lst

honyaos.img : ipl10.bin honyaos.sys Makefile
	$(EDIMG)    imgin:../z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
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
	-$(DEL) ipl10.bin
	-$(DEL) ipl10.lst
	-$(DEL) honyaos.sys
	-$(DEL) honyaos.lst

src_only :
	$(MAKE) clean
	-$(DEL) honyaos.img
