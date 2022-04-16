
VBADOS is a set of Terminate-and-Stay-Resident (TSR) utilities to be used
inside MS-DOS virtual machines.

There are two utilities right now:

* VBMOUSE.EXE is a DOS mouse driver for VirtualBox/VMware emulated mouse,
  but also real PS/2 mice.

Note: this project has absolutely no official relation with
either [VirtualBox](https://www.virtualbox.org/) or Visual Basic.
The VB stands for "Very Basic" :)

# Downloads

TBD

# Documentation

[TOC]

## VBMOUSE - DOS mouse driver

VBMOUSE.EXE is a DOS mouse driver as a TSR written in C and compilable using OpenWatcom C.
It is primarily designed to work with the VirtualBox emulated mouse, but also supports 
real PS/2 mice as well as the VMware virtual one.
Being in C, it is not going to stand out in either compatibility, performance,
or memory usage; but hoping it is at least a bit easier to hack on, for experimentation.

Like any other DOS mouse driver, it partially supports the MS Mouse API (int 33h), and has
the following additional features:

* *Integration with VirtualBox*: in many DOS programs, the mouse can be used 
  without requiring capture, and will seamlessly integrate with the mouse cursor
  position in the host.  
  You can notice that in (compatible) graphical applications the DOS cursor will appear to be
  drawn outside the VM area when near a border:  
  [![Adlib composer mouse cursor partially outside VM area](https://depot.javispedro.com/vbox/vbmouse/vbm_adlibcomposer_overlap.png)](https://depot.javispedro.com/vbox/vbmouse/vbm_adlibcomposer.webm)  
  This is because the cursor is actually rendered by the host rather than the guest OS,
  thus feeling much more responsive.  
  [See it on MS-DOS Edit / QBasic](https://depot.javispedro.com/vbox/vbmouse/vbm_edit.webm).  
  [See it on Adlib Composer](https://depot.javispedro.com/vbox/vbmouse/vbm_adlibcomposer.webm).  

* Note that many MS-DOS programs do not rely on the mouse driver to render the
  cursor, or even just read the "relative" mouse motion from the mouse itself and
  compute the mouse position by themselves.  
  In these cases, the mouse will be "jumpy" or even stuck when the VirtualBox integration
  is enabled, so this integration can be disabled (either from the VirtualBox
  menu or by using `vbmouse integ off` after loading the driver).  
  ![Mouse Integration option under VirtualBox](https://depot.javispedro.com/vbox/vbmouse/vbox_integ_option.png)  

* *Integration with VMware/qemu/vmmouse*: the same binary is compatible with
  both virtualizers.  
  However, the host cursor rendering is not implemented; the cursor will always
  be rendered by the guest 
  (this is because it's more complicated: the driver would need to implemented
  some VMSVGA support for it to happen).  
  Like with the above bullet point, if you find non-compatible software,
  you can either use VMware's [game mode](https://kb.vmware.com/s/article/1033416),
  or run `vbmouse integ off` to disable the integration.  
  [See MS-DOS Edit / QBasic under VMware](https://depot.javispedro.com/vbox/vbmouse/vbm_vmware_edit.webm).  
  
* *Windows 3.x enhanced mode support*:
  This driver has the hooks required for DOS boxes inside Windows. 
  Multiple DOS boxes can use this driver simultaneously without conflict,
  and clicks in the DOS window will be passed through to the correct running DOS application.
  See: [Adlib Composer inside a Windows 3.11 enhanced mode DOS box](https://depot.javispedro.com/vbox/vbmouse/vbm_win_adlibcomposer.webm).

* *Wheel and 3 button mouse support*, using the API from CuteMouse.  
  This is currently limited to the VirtualBox/VMware integration, albeit limited PS/2 wheel support is planned.

* Sending fake keys on wheel movements, i.e. faking wheel scroll support using arrow up/down keys.
  You can scroll with the mouse wheel inside MS-DOS Edit!

* The current version uses about 10KiB of memory (when logging is disabled),
  and will autoload itself into HMA if available.  
  ![VBMouse Memory Usage](https://depot.javispedro.com/vbox/vbmouse/vbmouse_mem.png).  

* A companion driver for Windows 3.x (_VBMOUSE.DRV_) that uses this driver 
  (via int33h) instead of accessing the mouse directly,
  so that Windows 3.x gains some of the features of this driver
  (like mouse integration in VirtualBox).  
  This driver has some preeliminary mouse wheel support based on the ideas from
  [vmwmouse](https://github.com/NattyNarwhal/vmwmouse/issues/5).  

Note that it does not support serial mice or anything other than PS/2.

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

A very simple Windows 3.x mouse driver (called VBMOUSE.DRV) is also included,
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

![Windows 3.x setup](https://depot.javispedro.com/vbox/vbmouse/win_setup.png)

If the "Mouse" option becomes empty after doing this, just select 
"VBMouse int33 absolute mouse driver" which will now appear in the same list
(instead of "Other mouse (requires disk)").

If your primary goal is Windows 3.x and you do not want to be forced to use
VBMOUSE.EXE as DOS mouse driver, you may be interested in 
"standalone" Windows 3.x mouse drivers. 
E.g, for either [VirtualBox](https://git.javispedro.com/cgit/vbmouse.git/)
or [VMware](https://github.com/NattyNarwhal/vmwmouse).

## VBSF - Shared folders

TBD

# Building

This requires [OpenWatcom 2.0](http://open-watcom.github.io/) to build,
albeit it may work with an older version, and was only tested on a Linux host.

The included makefile is a wmake makefile. To build it just enter the OpenWatcom environment and run `wmake flp`.
This will create a floppy image containing vbmouse.exe plus the Windows 3.x driver (oemsetup.inf and vbmouse.drv).

# Design

The two TSRs have a resident part (which stays in memory) and a transient part (which is only used to handle
command line arguments, load the resident part, and configure it; but otherwise doesn't stay in memory). 
The resident part is entirely in one segment (`RESGROUP`), including all the data
it will need. All other segments will be dropped once the driver is installed
(including the C runtime!). 

You will notice that most auxiliary functions are in .h files.
This is due to laziness on my part.
While most of the time these auxiliary functions are only used by one
tool and one segment (i.e. the resident part uses it but not the transient part),
sometimes I need the same function different segments/binaries. The only way to do
is to build the multiple file several times with different settings. To avoid
that and the resulting makefile complications, I just put all auxiliary functions
directly in the header files.

### Source code organization

* [mousmain.c](../tree/mousemain.c) is the transient part of the mouse driver,
  while [mousetsr.c](../tree/mousetsr.c) is the resident part.
  For example here is the [entry point for int33](https://git.javispedro.com/cgit/vbados.git/tree/mousetsr.c?id=8aea756f5094de4b357c125b75973d82328e0c31#n1055).
  A single function, `handle_mouse_event`, takes the mouse events from
  all the different sources (PS/2, VirtualBox, Windows386, etc.), and posts
  it to the user program as appropiate.  
  Most of the complexity is the standard "DOS mouse driver" tasks,
  like drawing the cursor in all the standard CGA/EGA/VGA modes (ouf!).  
  
* [sfmain.c](../tree/sfmain.c) and [sftsr.c](../tree/sftsr.c) are the
  transient/resident part (respectively) of the shared folders TSR.
  This is basically a MS-DOS "network redirector" whose interface is totally
  undocumented save for [a few books](http://link.archive.org/portal/Undocumented-DOS--a-programmers-guide-to/JS0Fn59H2zU/).

* [dlog.h](../tree/dlog.h), a poor man's printf-replacement used by the resident
  parts of both drivers. Only for debugging, not used in release builds.
  It can target either a raw IO port (useful for many virtualizers
  and emulators which implement a log out port), or a real serial port.

* [vbox.h](../tree/vbox.h), [vbox.c](../tree/vbox.c) implement initialization
  of the [VirtualBox guest-host interface](#virtualbox-communication), 
  including PCI BIOS access and Virtual DMA.
  [vboxdev.h](../tree/vboxdev.h) contains all the defines/struct from upstream
  VirtualBox that are used in this driver (this file is a poor mix and mash
  of VirtualBox additions header files and thus is the only file of this repo
  under the MIT license).
  [vboxhgcm.h](../tree/vboxhgcm.h), [vboxshfl.h](../tree/vboxshfl.h) contains
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
  These mostly come from the [Undocumented DOS](http://link.archive.org/portal/Undocumented-DOS--a-programmers-guide-to/JS0Fn59H2zU/) book.
  Sadly, it is really necessary to mingle with DOS data structures
  when writing a network redirector; there is no clearly defined public API.
  
* [int2fwin.h](../tree/int2fwin.h) wrappers and defines for the Windows 386-Enhanced
  mode interrupts and callbacks. The mouse driver must listen to these callbacks
  in order to work properly under 386-enhanced mode Windows.
  Documentation for those is available on the Windows 3.1 DDKs.

* [int33.h](../tree/int33.h) wrappers and defines for the int33 mouse API.

* [int4Bvds.h](../tree/int4Bvds.h) wrappers for the
  [Virtual DMA services](https://en.wikipedia.org/wiki/Virtual_DMA_Services).
  
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
or from a relative device (like a real PS/2 mouse or when the integration is disabled).

> When the int33 user interrupt routine is called, bit 8 of CX indicates that the
> x, y coordinates passed in CX, DX come from an absolute pointing device
> (and therefore that the mickey counts in SI, DI may be zero or virtualized).

Note that the range of coordinates is still defined as in a traditional int33 driver,
i.e. the size of the screen unless a larger range is defined via int33 ax=7/8.

The included Win3.x driver uses this API to decide whether to forward the absolute
OR relative coordinates to Windows, so that one can use the same driver for both types
without loss of functionality.

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

