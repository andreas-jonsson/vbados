
This is a DOS mouse driver as a TSR written in C and compilable using OpenWatcom C.
It is not going to stand out in either compatibility, performance, or memory usage;
the main goal is to have a driver that is at least a bit easier to hack on, for experimentation.

Like any other DOS mouse driver, it partially supports the MS Mouse API (int 33h), but has
some additional features:

* Implements the hooks required for DOS boxes inside Windows. 
  Multiple DOS boxes can use this driver simultaneously without conflict,
  and clicks in the DOS window will be passed through to the correct running DOS application.
  [See in action](https://depot.javispedro.com/vbox/vbmouse/vbm_win_adlib.webm).

* Integration with VirtualBox: in many DOS programs, the mouse can be used without requiring capture,
  and will seamlessly integrate with the mouse cursor in the host.
  The mouse cursor will be rendered by the host rather than the guest OS, appearing much more responsive.
  [See in action](https://depot.javispedro.com/vbox/vbmouse/vbm_edit.webm).
  Programs/games that utilize relative mouse motion information will be "jumpy" when this is enabled,
  so this integration can be disabled (either from the VirtualBox menu or by using `vbmouse integ off` after loading
  the driver).

* Integration with VMware/qemu vmmouse: like the above.
  Use `vbmouse integ off` to disable it for software requiring relative mouse motions.
  Host mouse cursor is not implemented and will be always rendered by the guest.

* Wheel and 3 button mouse support, using the API from CuteMouse.
  This is currently limited to the VirtualBox/VMware integration, albeit limited PS/2 wheel support is planned.

* Sending fake keys on wheel movements, i.e. faking wheel scroll support using arrow up/down keys.

* A companion driver for Windows 3.x (_vbmouse.drv_) that uses this driver (via int33h) instead of accessing the mouse directly,
  so that Windows 3.x gains some of the features of this driver (like mouse integration in VirtualBox).
  Wheel is not implemented in Windows 3.x right now.

Note that it does not support serial mice or anything other than PS/2.

# Usage

To install the driver, just run `vbmouse`. 

Run `vbmouse <action>` for specific configuration. Here are the supported actions:

* `install` installs the driver (i.e. the same as if you run `vbmouse`).
  `vbmouse install low` can be used to force installation in conventional memory; by default, it tries to use a DOS UMB block.

* `uninstall` uninstalls the driver. Note that if you have installed some other TSRs after vbmouse, you may not be able to uninstall it.

* `wheel on|off` to enable/disable the wheel support.

* `wheelkey key|off` to set up sending fake keypresses on wheel movement. Only supported `key` right now are `updn` (for the up and down arros) and `pageupdn`.

* `integ on|off` to enable/disable the VirtualBox/VMware cursor integration. 
   Useful for programs that expect relative mouse coordinates.

* `hostcur on|off` to enable/disable the host-rendered mouse cursor.

* `reset` resets the mouse to default settings and re-initializes the hardware. 
   This does not include any of the above settings, but rather the traditional int33 mouse settings (like sensitivity)
   that may be altered by other programs. It is equivalent to int33/ax=0.

# Building

This requires [OpenWatcom 2.0](http://open-watcom.github.io/) to build,
albeit it may work with an older version, and was only tested on a Linux host.

The included makefile is a wmake makefile. To build it just enter the OpenWatcom environment and run `wmake flp`.
This will create a floppy image containing vbmouse.exe plus the Windows 3.x driver (oemsetup.inf and vbmouse.drv).

# Design

This is at its core a driver for a plain PS/2 mouse driver, using the PS/2 BIOS.
If running outside VirtualBox, in fact it will behave like a PS/2 mouse driver.
Therefore most of the complexity is in the standard "DOS mouse driver" parts,
like drawing the cursor.

The .exe file is comprised of two segments, the first one contains the resident part,
while the second one contains the command line interface. This second segment
is dropped once the driver goes resident. 

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

The VirtualBox guest integrations present itself as a PCI device to the guest.
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

