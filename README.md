
VBADOS is a set of Terminate-and-Stay-Resident (TSR) utilities to be used
inside MS-DOS virtual machines (and Windows 3.x), and help to provide
closer integration between the guest and the host.

There are two utilities right now:

* VBMOUSE.EXE is a DOS mouse driver for VirtualBox/VMware emulated mouse,
  but also real PS/2 mice. It allows for seamless host/guest mouse integration,
  and scroll wheel support.

* VBSF.EXE is a Shared Folders driver for VirtualBox (not VMware yet). It allows
  to map directories from the host OS into drive letters in the MS-DOS guest.

Note: this project has absolutely no official relation with
either [VirtualBox](https://www.virtualbox.org/) or Visual Basic.
The VB stands for "Very Basic" :)

# Downloads

TBD

# Documentation

[TOC]

## VBMOUSE.EXE - DOS mouse driver

VBMOUSE.EXE is a DOS mouse driver as a TSR written in C and compilable using OpenWatcom C.
It is primarily designed to work with the VirtualBox emulated mouse, but also supports 
real PS/2 mice as well as the VMware virtual mouse.
Being in C, it is not going to stand out in either compatibility, performance,
or memory usage; but hoping it is at least a bit easier to hack on, for experimentation.

I tried to keep it working on real hardware as an exercise.
It requires at a very minimum a 386, but probably needs
something a bit beefier to be useful, since the rendering routines are going
to be much slower than an assembly-optimized driver.
Note also that it does not support serial mice or anything other than PS/2.
For older PCs, serial mice, or, to be honest, for real hardware overall, 
[CuteMouse](http://cutemouse.sourceforge.net/) is still the best choice and
hard to beat.

Like any other DOS mouse driver, it partially supports the MS Mouse API (int 33h), and has
the following additional features:

* **Integration with VirtualBox**: in many DOS programs, the mouse can be used 
  without requiring capture, and will seamlessly integrate with the mouse cursor
  position in the host.  
  You can notice that in (compatible) graphical applications the DOS cursor will appear to be
  drawn outside the VM area when near a border:  
  [![Adlib composer mouse cursor partially outside VM area](https://depot.javispedro.com/vbox/vbados/vbm_adlibcomposer_overlap.png)](https://depot.javispedro.com/vbox/vbados/vbm_adlibcomposer.webm)  
  This is because the cursor is actually rendered by the host rather than the guest OS,
  thus feeling much more responsive.  
  [See it on MS-DOS Edit / QBasic](https://depot.javispedro.com/vbox/vbados/vbm_edit.webm).  
  [See it on Adlib Composer](https://depot.javispedro.com/vbox/vbados/bm_adlibcomposer.webm).  

* Note that many MS-DOS programs do not rely on the mouse driver to render the
  cursor, or even just read the "relative" mouse motion from the mouse itself and
  compute the mouse position by themselves.  
  In these cases, the mouse will be "jumpy" or even stuck when the VirtualBox integration
  is enabled, so this integration can be disabled (either from the VirtualBox
  menu or by using `vbmouse integ off` after loading the driver).  
  ![Mouse Integration option under VirtualBox](https://depot.javispedro.com/vbox/vbados/vbox_integ_option.png)  

* **Integration with VMware/qemu/vmmouse**: the same binary is compatible with
  both virtualizers.  
  However, the host cursor rendering is not implemented; the cursor will always
  be rendered by the guest 
  (this is because it's more complicated: the driver would need to implemented
  some VMSVGA support for it to happen).  
  Like with the above bullet point, if you find non-compatible software,
  you can either use VMware's [game mode](https://kb.vmware.com/s/article/1033416),
  or run `vbmouse integ off` to disable the integration.  
  [See MS-DOS Edit / QBasic under VMware](https://depot.javispedro.com/vbox/vbados/vbm_vmware_edit.webm).  
  
* **Windows 3.x enhanced mode support**:
  This driver has the hooks required for DOS boxes inside Windows.  
  Multiple DOS boxes can use this driver simultaneously without conflict,
  and clicks in the DOS window will be passed through to the correct running DOS application.  
  [See Adlib Composer inside a Windows 3.11 enhanced mode DOS box](https://depot.javispedro.com/vbox/vbados/vbm_win_adlibcomposer.webm).

* **Scroll wheel and 3 button mouse support**, using the API from CuteMouse.  
  This is currently limited to the VirtualBox/VMware integration, albeit limited PS/2 wheel support is planned.
  Note *wheel support is broken when running under 386-enhanced mode Windows*, 
  since it will not let PS/2 wheel data reach the DOS driver.

* Sending fake keys on wheel movements, i.e. faking wheel scroll support using arrow up/down keys.
  You can scroll with the mouse wheel inside MS-DOS Edit!  
  This is not enabled by default, see `wheelkey` below.

* The current version uses about 10KiB of memory (when logging is disabled),
  and will autoload itself into HMA if available.  
  ![VBMouse Memory Usage](https://depot.javispedro.com/vbox/vbados/vbmouse_mem.png).  

* A companion driver for Windows 3.x (_VBMOUSE.DRV_) that uses this driver 
  (via int33h) instead of accessing the mouse directly,
  so that Windows 3.x gains some of the features of this driver
  (like mouse integration in VirtualBox).  
  There is some preliminary mouse wheel support based on the ideas from
  [vmwmouse](https://github.com/NattyNarwhal/vmwmouse/issues/5),
  but it only works under real-mode Windows. 
  As of right now wheel support is still broken under 386-enhanced mode Windows.

### Usage

To install the driver, just run `vbmouse`. 

Run `vbmouse <action>` for specific configuration. Here are the supported actions:

* `install` installs the driver (i.e. the same as if you run `vbmouse`).
  `vbmouse install low` can be used to force installation in conventional memory;
  by default, it tries to use a DOS UMB block.

* `uninstall` uninstalls the driver. Note that if you have installed some other TSRs
  after vbmouse, you may not be able to uninstall it.

* `wheel on|off` to enable/disable the wheel support.

* `wheelkey key|off` to set up sending fake key presses on wheel movement. 
  Only supported `key` right now are `updn` (for the up and down arros) and `pageupdn`.

* `integ on|off` to enable/disable the VirtualBox/VMware cursor integration. 
   Useful for programs that expect relative mouse coordinates.

* `hostcur on|off` to enable/disable the host-rendered mouse cursor.

* `reset` resets the mouse to default settings and re-initializes the hardware. 
   This does not include any of the above settings, but rather the traditional int33 mouse settings (like sensitivity)
   that may be altered by other programs. It is equivalent to int33/ax=0.

### Windows 3.x driver

A very simple Windows 3.x mouse driver (called _VBMOUSE.DRV_) is also included,
which _requires VBMOUSE.EXE to be loaded before starting Windows_ ,
and uses its functionality to provide similar host/guest mouse integration
inside Windows 3.x itself (i.e. not just DOS boxes).

To install this driver, and assuming you have for example already inserted
the VBADOS.flp floppy (or copied the files in some other way), 
use the Windows Setup program (accessible either via SETUP.EXE on an installed
Windows or via the corresponding icon in Program Manager).

Go to Options → Change system configuration
→ Mouse → Select "Other mouse (requires disk)"
→ Search in "A:" (or the path with VBADOS files)
→ "VBMouse int33 absolute mouse driver".

![Windows 3.x setup](https://depot.javispedro.com/vbox/vbados/win_setup.png)

If the "Mouse" option becomes empty after doing this, just select 
"VBMouse int33 absolute mouse driver" which will now appear in the same list
(instead of "Other mouse (requires disk)").

If your primary goal is Windows 3.x and you do not want to be forced to use
VBMOUSE.EXE as DOS mouse driver, you may be interested in 
"standalone" Windows 3.x mouse drivers. 
E.g, for either [VirtualBox](https://git.javispedro.com/cgit/vbmouse.git/)
or [VMware](https://github.com/NattyNarwhal/vmwmouse).

You can also use VBMOUSE.DRV with any other int33 mouse driver, however
to use absolute positioning (e.g. under DOSBox-X or DOSemu), the int33
driver needs to support my [absolute int33 API extension bit](https://git.javispedro.com/cgit/vbados.git/about/#absolute-mouse-api).
Otherwise the driver will only pass relative coordinates (i.e. without
SF_ABSOLUTE bit).

## VBSF.EXE - Shared folders

VBSF.EXE allows you to mount VirtualBox shared folders as drive letters.
It is an "MS-DOS network redirector", so the new letters behave as network drives,
not real drives.

Most redirector functionality is supported, including write support, except
changing file attributes (like setting a file to read-only...). The drives
can also be accessed from within Windows 3.x .

It uses around 10KiB of memory, and auto-installs to an UMB if available.
This is still much less memory than a SMB client and network stack!

### Usage

First, you need to configure some shared folders in the Virtual Machine settings
from VirtualBox (right click on the shared folders icon, or just open VM Settings).

![VirtualBox shared folders configuring an automount folder at V:](https://depot.javispedro.com/vbox/vbados/vbox_sf_config.png)

In the add share dialog (seen above):

* Folder Path is the actual host directory you want to mount in the guest.

* Folder Name is just a given name for this shared folder, can be anything you want.  
  When using VBSF, this will become the drive label, so ensure it fits in 8+3
  characters.
  
* Mount point is the drive letter VBSF is going to use for this folder.

* Use "Automount" if you want VBSF to automatically mount this folder
  once the driver is loaded; otherwise, you will need to use `vbsf mount`.

Second, remember to add [LASTDRIVE=Z to your CONFIG.SYS](https://en.wikipedia.org/wiki/CONFIG.SYS#LASTDRIVE)! 

Third, to install the driver, just run `vbsf`. 
The driver will automatically mount all the directories marked as "Automount".

![VBSF mounting V:](https://depot.javispedro.com/vbox/vbados/vbsf_src_c.png)

The driver supports the following actions, too:

* `install` installs the driver (i.e. the same as if you run `vbsf`).
  `vbsf install low` can be used to force installation in conventional memory;
  by default, it tries to use a DOS UMB block.

* `uninstall` uninstalls the driver.

* `list` shows currently mounted drives as well as all available shared folders.

* `mount FOLDER X:` can be used to mount a non-automatic shared folder at a specific drive,
  or to mount a specific shared folder on multiple drives.

* `unmount X:` unmounts a specific drive.

* `rescan` unmounts all shared folders, gets the new list of shared folders
  from VirtualBox and performs automounts again. You _must_ run this command
  if you change the shared folder definitions while the driver is running,
  otherwise you are likely to get mysterious failures.


### File names and timezones

Note that there is NO Long File Name support. This means that all the files
in the shared directory must be 8.3 characters long or shorter. Files with
long files will just not appear in the directory listings and therefore
cause misterious failures (they will be skipped when copying directories,
and DOS will not be able to delete directories containing such files).

Also, there is absolutely NO support for mapping filenames to a specific codepage.
Please limit yourself to plain ASCII filenames or you will quickly see gibberish in DOS.
No spaces, accents or tildes!

To see proper modification dates & times in directory listings in DOS,
you need to set the TZ (timezone) environment variable _before_ loading VBSF.EXE. 
Please see the [OpenWatcom documentation for the syntax of the TZ variable](https://open-watcom.github.io/open-watcom-v2-wikidocs/clib.html#The_TZ_Environment_Variable).

For example, if your local timezone is 8 hours earlier than UTC (e.g. PST), run
`set TZ=PST8`.

# Building the source

This requires [OpenWatcom 2.0](http://open-watcom.github.io/) to build,
and I have only tried with the latest (March 2022) snapshot, 
albeit it may likely work with older versions, starting from 1.9.

I have tested building it on a Linux host as well as building on MS-DOS itself
(with the source code being on a VBSF shared folder :) ).

The included makefile is a `wmake` makefile. 
To build it just enter the OpenWatcom environment and run `wmake flp`.
This will create a floppy image containing vbmouse.exe,
vbsf.exe plus the Windows 3.x driver (oemsetup.inf and vbmouse.drv).

# Design

The two TSRs have a resident part (which stays in memory) and a transient part (which is only used to handle
command line arguments, load the resident part, and configure it; but otherwise doesn't stay in memory). 
The resident part is entirely in one segment (`RESGROUP`), including all the data
it will need. All other segments will be unloaded once the driver is installed
(including the C runtime!). 

VBMOUSE is the only "native" free DOS mouse driver written in C I'm aware of.
There is already a very good free DOS mouse driver written in assembler,
[CuteMouse](http://cutemouse.sourceforge.net/). There are also DOS mouse drivers
written in C inside [DOSBox](https://www.dosbox.com/) and [DOSEMU](http://www.dosemu.org/),
but these drivers are native code running inside the emulator itself; they are
not designed to be run inside the emulated machine. 

VBSF is a "MS-DOS network redirector" of which there are not many free
implementations at all. The only one I'm aware of the _Phantom_ sample code, from the
[Undocumented DOS](https://openlibrary.org/books/OL1419113M/Undocumented_DOS) book,
and [EtherDFS](http://etherdfs.sourceforge.net/), which is also in Watcom C.
EtherDFS leaves most of the actual logic to the server code (running on the host), 
so the DOS part is much more efficient.

### Source code organization

You will notice that most auxiliary functions are in .h files.
This is due to laziness on my part.
While most of the time these auxiliary functions are only used by one
tool and one segment (i.e. the resident part uses it but not the transient part),
sometimes I need the same function in different segments/binaries. The only way to do
that is to build the multiple file several times with different settings. To avoid
that and the resulting makefile complications, I just put all auxiliary functions
directly in the header files.

* [mousmain.c](../tree/mousemain.c) is the transient part of the mouse driver,
  while [mousetsr.c](../tree/mousetsr.c) is the resident part.  
  For example here is the [entry point for int33](https://git.javispedro.com/cgit/vbados.git/tree/mousetsr.c?id=8aea756f5094de4b357c125b75973d82328e0c31#n1055).
  A single function, `handle_mouse_event`, takes the mouse events from
  all the different sources (PS/2, VirtualBox, Windows386, etc.), and posts
  it to the user program as appropiate.  
  Most of the complexity is the standard "DOS mouse driver" tasks,
  like drawing the cursor in all the standard CGA/EGA/VGA modes (ouf!).  
  For reference you can also see [CuteMouse](http://cutemouse.sourceforge.net/).
  
* [sfmain.c](../tree/sfmain.c) and [sftsr.c](../tree/sftsr.c) are the
  transient/resident part (respectively) of the shared folders TSR.
  This includes the entry point for int 2Fh/ah=11h, a.k.a. the "network redirector" interface.  
  While the interface is undocumented, it is similar to a VFS layer,
  implementing the usual file system calls (open, close, readdir, mkdir, etc.).
  It uses lots of MS-DOS internals,
  see [Undocumented DOS](https://openlibrary.org/books/OL1419113M/Undocumented_DOS) for reference.

* [dlog.h](../tree/dlog.h), a poor man's printf-replacement used by the resident
  parts of both drivers. Only for debugging, not used in release builds.
  It can target either a raw IO port (useful for many virtualizers
  and emulators which implement a log out port), or a real serial port.

* [vbox.h](../tree/vbox.h), [vbox.c](../tree/vbox.c) implement initialization
  of the [VirtualBox guest-host interface](#virtualbox-communication), 
  including PCI BIOS access and Virtual DMA.

* [vboxdev.h](../tree/vboxdev.h) contains all the defines/struct from upstream
  VirtualBox that are used in this driver (this file is a poor mix and mash
  of VirtualBox additions header files and thus is the only file of this repo
  under the MIT license).

* [vboxhgcm.h](../tree/vboxhgcm.h), [vboxshfl.h](../tree/vboxshfl.h) contains
  auxiliary functions for the HGCM (Host-Guest Communication) protocol and 
  the shared folders protocol (both are only used by VBSF). 

* [vmware.h](../tree/vmware.h) some defines of the VMware protocol (popularly
  called the _backdoor_).
  See [VMware tools](https://wiki.osdev.org/VMware_tools) on OSDev for a reference.

* [dostsr.h](../tree/dostsr.h), helper functions for loading the resident part
  into an UMB.

* [int10vga.h](../tree/int10vga.h) functions for setting/querying video modes using
  int 10h and generally configuring and getting the state of the VGA.
  Used when rendering the mouse cursor in the guest.
  See [VGA hardware](https://wiki.osdev.org/VGA_Hardware).

* [int15ps2.h](../tree/int15ps2.h) wrappers for the PS/2 BIOS pointing device
  services (int 15h), used for reading the PS/2 mouse.

* [int16kbd.h](../tree/int16kbd.h) wrappers for BIOS keyboard services,
  currently only used to insert fake keypress on wheel movement.
  
* [int1Apci.h](../tree/int1Apci.h) wrappers for the real-mode PCI BIOS services,
  used to locate the VirtualBox guest PCI device.
  
* [int21dos.h](../tree/int21dos.h) wrappers for some TSR-necessary DOS services,
  but also contains structs and definitions for many DOS internal data structures.
  These mostly come from the [Undocumented DOS](https://openlibrary.org/books/OL1419113M/Undocumented_DOS) book.
  Sadly, it is really necessary to mingle with DOS data structures
  when writing a network redirector; there is no clearly defined public API.
  
* [int2fwin.h](../tree/int2fwin.h) wrappers and defines for the Windows 386-Enhanced
  mode interrupts and callbacks. The mouse driver must listen to these callbacks
  in order to work properly under 386-enhanced mode Windows.
  Documentation for those is available on the Windows 3.1 DDKs.

* [int33.h](../tree/int33.h) wrappers and defines for the int33 mouse API.

* [int4Bvds.h](../tree/int4Bvds.h) wrappers for the
  [Virtual DMA services](https://en.wikipedia.org/wiki/Virtual_DMA_Services),
  used for VirtualBox communication under protected mode.
  
* [unixtime.h](../tree/unixtime.h) a (probably wrong) function to convert 
  UNIX into DOS times, since we can't use the C runtime from the TSR.

### int33 extensions

#### Wheel mouse API

This driver supports the
[CuteMouse int33 wheel API 1.0](https://github.com/FDOS/mouse/blob/master/wheelapi.txt).

If the `wheelkey` feature is in use, and to avoid sending wheel events twice (once
as a fake key press, once as a wheel API event), this API will not be enabled until the first
int33 ax=11h call (check wheel support), after which wheel events will only be sent through
the wheel API until the next driver reset.

#### Absolute mouse API

There is a very simple extension of the int33 protocol to let clients know whether
the incoming coordinates come from an absolute device (like the VirtualBox integration)
or from a relative device (like a real PS/2 mouse or when the integration is disabled):

> When the int33 user interrupt routine is called, bit 8 of CX indicates that the
> x, y coordinates passed in CX, DX come from an absolute pointing device
> (and therefore that the mickey counts in SI, DI may be zero or virtualized).

Note that the range of coordinates is still defined as in a traditional int33 driver,
i.e. the size of the screen unless a larger range is defined via int33 ax=7/8.

The rationale for this extension is that for many types of simulated mouse it
is possible to switch between absolute and relative position. E.g. on
VirtualBox you can disable the integration, on VMware you can enable "game mode",
on DOSBox you can lock/unlock the mouse, etc. 
This information is not forwarded to the users of the int33 interface;
this extension fixes that.

The included Win3.x driver uses this API to decide whether to forward the absolute
OR relative coordinates to Windows, so that one can use the same driver for both types
of mouse input without loss of functionality in either case.

### VirtualBox communication

The VirtualBox guest integration presents itself as a PCI device to the guest.
Thanks to the BIOS, the device is already pre-configured.
The driver uses the real-mode PCI BIOS to request the current configuration of this PCI device.
To communicate with the host, the guest must send the PCI device the (physical) address
of a buffer containing commands to be sent to the host.
The host will write back the response in the same buffer.
Further details are available in [OSDev](https://wiki.osdev.org/VirtualBox_Guest_Additions).

The only challenge here is getting the physical address (what the VirtualBox PCI device expects)
corresponding to a logical address (segment:offset).
While running under real mode DOS, the segment can be converted to a physical address without difficulty.
However, when using DOS extenders, EMM386, Windows in 386 mode, etc. DOS is actually run
in virtual 8086 mode, and the logical address may not correspond to a physical address.
However, most DOS extenders still map conventional memory 1:1, and for those who don't,
the driver uses the [Virtual DMA services](https://en.wikipedia.org/wiki/Virtual_DMA_Services)
to obtain the physical address.

When VirtualBox is told that the guest wants absolute mouse information, VirtualBox will stop sending
mouse motion information via the PS/2 mouse. However, the PS/2 controller will still send interrupts
whenever mouse motion happens, and it will still report mouse button presses. In fact, the only way
to obtain mouse button presses (and wheel movement) is still through the PS/2 controller.

### Future work

* Get the scroll wheel to work under 386-enhanced Windows, but this requires
  looking into VKD.386 (or a replacement of it).

* VMware shared folders support is interesting, but there is very little 
  documentation that I can find, no sample code, and no open source implementation.  
  Also, unlike VBMOUSE, where most of the code  is common to all virtualizers,
  it will probably make more sense to make a separate "VMwareSF" TSR since most of
  the VBSF code would be not be useful.
