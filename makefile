CC=cl
CFLAGS=
OUTFLAG=/Fe
DLLFLAG=/LD /Zi
RM=del /Q
TESTFLAG=/DTEST

#CC=gcc
#CFLAGS=
#OUTFLAG=-o 
#DLLFLAG=-Wl,--kill-at -shared
#RM=rm -f
#TESTFLAG=-DTEST

all : xpmenu.dll
	@echo done

xpmenu.dll : xpmenu.c
	$(CC) $(DLLFLAG) $(OUTFLAG)$@ $?

test :
	gvim xpmenu.vim
clean :
	$(RM) *.obj *.exe *.dll *.exp *.lib
