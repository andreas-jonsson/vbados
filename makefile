# This is an Open Watcom wmake makefile, not GNU make.
# Assuming you have sourced `owsetenv` beforehand.
# 
dosobjs = dostsr.obj dosmain.obj vbox.obj
doscflags = -bt=dos -ms -s -6 -os -w3
# -ms to use small memory model
# -s to disable stack checks, since it inserts calls to the runtime from the TSR part
# -os to optimize for size

w16objs = w16mouse.obj
w16cflags = -bt=windows -bd -mc -zu -s -6 -w3
# -bd to build DLL
# -mc to use compact memory model (far data pointers, since ss != ds)
# -zu for DLL calling convention (ss != ds)
# -s to disable stack checks, since the runtime uses MessageBox() to abort (which we can't call from mouse.drv)

.BEFORE:
	# We need DOS and Windows headers, not host platform's
	set include=$(%watcom)/h/win;$(%watcom)/h

# Main DOS driver file
vbmouse.exe: dosmouse.lnk $(dosobjs) 
	wlink @$[@ name $@ file { $(dosobjs) } 

dostsr.obj: dostsr.c .AUTODEPEND
	wcc -fo=$^@ $(doscflags) -g=RES_GROUP -nd=RES -nt=RES_TEXT -nc=RES_CODE $[@

dosmain.obj: dosmain.c .AUTODEPEND
	wcc -fo=$^@ $(doscflags) $[@

vbox.obj: vbox.c .AUTODEPEND
        wcc -fo=$^@ $(doscflags) $[@

vbmouse.drv: w16mouse.lnk $(w16objs)
	wlink @$[@ name $@ file { $(w16objs) }

w16mouse.obj: w16mouse.c .AUTODEPEND
	wcc -fo=$^@ $(w16cflags) $[@

clean: .SYMBOLIC
	rm -f vbmouse.exe vbmouse.drv vbmouse.flp *.obj *.map

vbmouse.flp:
	mformat -C -f 1440 -v VBMOUSE -i $^@ ::

# Build a floppy image containing the driver
flp: vbmouse.flp vbmouse.exe vbmouse.drv .SYMBOLIC
	mcopy -i vbmouse.flp -o vbmouse.exe vbmouse.drv ::

