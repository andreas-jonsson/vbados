# This is an Open Watcom wmake makefile, not GNU make.
# Assuming you have sourced `owsetenv` beforehand.

.BEFORE:
	# We need DOS and Windows headers, not host platform's
	set include=$(%watcom)/h/win;$(%watcom)/h

# The main driver file
vbmouse.drv: mousew16.c mousew16.h vbox.c vbox.h vboxdev.h ps2.h pci.h vds.h int2fwin.h
	# -bd to build DLL
	# -mc to use compact memory model (far data pointers, since ss != ds)
	# -zu for DLL calling convention (ss != ds)
	# -zc put constants on the code segment (cs)
	# -s to disable stack checks, since the runtime uses MessageBox() to abort (which we can't call from mouse.drv)
	wcl -6 -mc -bd -zu -zc -s -bt=windows -l=windows_dll @vbmouse.lnk -fe=$^@ mousew16.c vbox.c

clean: .SYMBOLIC
	rm -f vbmouse.drv vbmouse.flp *.o

vbmouse.flp:
	mformat -C -f 1440 -v VBMOUSE -i $^@ ::

# Build a floppy image containing the driver
flp: vbmouse.flp vbmouse.drv oemsetup.inf .SYMBOLIC
	mcopy -i vbmouse.flp -o oemsetup.inf vbmouse.drv ::

