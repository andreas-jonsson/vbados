# This is an Open Watcom wmake makefile, not GNU make.
# Assuming you have sourced `owsetenv` beforehand.

# Object files for vbmouse
mousedosobjs = mousetsr.obj mousmain.obj vbox.obj
mousew16objs = mousew16.obj

# Object files for vbsf
sfdosobjs = sftsr.obj sfmain.obj vbox.obj

doscflags = -bt=dos -ms -6 -osi -w3 -wcd=202
# -ms to use small memory model (though sometimes ss != ds...)
# -osi to optimize for size, put intrinsics inline (to avoid runtime calls)
# -w3 enables warnings
# -wcd=202 disables the unreferenced function warning (e.g., for inline functions in headers)
dostsrcflags = -zu -s -g=RES_GROUP -nd=RES -nt=RES_TEXT -nc=RES_CODE
# -s to disable stack checks, since it inserts calls to the runtime from the TSR part
# -zu since ss != ds on the TSR

w16cflags = -bt=windows -bd -mc -zu -s -6 -oi -w3 -wcd=202
# -bd to build DLL
# -mc to use compact memory model (far data pointers, ss != ds in a DLL)
# -zu for DLL calling convention (ss != ds)
# -s to disable stack checks, since the runtime uses MessageBox() to abort (which we can't call from mouse.drv)

.BEFORE:
	# We need DOS and Windows headers, not host platform's
	set include=$(%watcom)/h/win;$(%watcom)/h

all: vbmouse.exe vbmouse.drv vbsf.exe .SYMBOLIC

# DOS mouse driver
vbmouse.exe: vbmouse.lnk $(mousedosobjs) 
	*wlink @$[@ name $@ file { $(mousedosobjs) }

mousetsr.obj: mousetsr.c .AUTODEPEND
	*wcc -fo=$^@ $(doscflags) $(dostsrcflags) $[@

mousmain.obj: mousmain.c .AUTODEPEND
	*wcc -fo=$^@ $(doscflags) $[@

vbox.obj: vbox.c .AUTODEPEND
	*wcc -fo=$^@ $(doscflags) $[@

# Windows 3.x mouse driver
vbmouse.drv: mousew16.lnk $(mousew16objs)
	*wlink @$[@ name $@ file { $(mousew16objs) }

mousew16.obj: mousew16.c .AUTODEPEND
	*wcc -fo=$^@ $(w16cflags) $[@

# DOS shared folders
vbsf.exe: vbsf.lnk $(sfdosobjs)
	*wlink @$[@ name $@ file { $(sfdosobjs) }

sfmain.obj: sfmain.c .AUTODEPEND
	*wcc -fo=$^@ $(doscflags) $[@

sftsr.obj: sftsr.c .AUTODEPEND
	*wcc -fo=$^@ $(doscflags) $(dostsrcflags) $[@

clean: .SYMBOLIC
	rm -f vbmouse.exe vbmouse.drv vbsf.exe vbados.flp *.obj *.map

vbados.flp:
	mformat -C -f 1440 -v VBADOS -i $^@ ::

# Build a floppy image containing the driver
flp: vbados.flp vbmouse.exe vbmouse.drv oemsetup.inf vbsf.exe .SYMBOLIC
	mcopy -i vbados.flp -o vbmouse.exe vbmouse.drv oemsetup.inf vbsf.exe ::

