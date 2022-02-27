This is a mouse driver for Windows 3.x with VirtualBox mouse integration support.

I have tested it with Windows 3.0 in real and 386 enhanced modes, as well as Windows 3.11 in 386 enhanced mode.

# Install

Just copy vbmouse.drv to your WINDOWS\SYSTEM directory and edit WINDOWS\SYSTEM.INI 's mouse.drv line to point to it, e.g.

    [boot]
    mouse.drv = vbmouse.drv

For a "proper" installation, you may create a floppy image containing oemsetup.inf and vbmouse.drv
and point the Windows Setup program to it when it asks for a 3rd party mouse driver disk.

# Building

This requires [http://open-watcom.github.io/](OpenWatcom 2.0), albeit it may work with an older version,
and was only tested on a Linux host.

The included makefile is a wmake makefile. To build it just enter the OpenWatcom environment and run `wmake vbmouse.drv`.
`wmake flp` may be used to build a floppy image containg oemsetup.inf and vbmouse.drv for easier installation.

# Design

This is at its core a driver for a plain PS/2 mouse driver, using the PS/2 BIOS.
If running outside VirtualBox, in fact it will behave like a PS/2 mouse driver.
However, it removes a lot of checks for older platforms and is mostly written in C rather than assembly,
so hopefully it is easier to understand than the Windows sample drivers.

The VirtualBox guest integrations present itself as a PCI device to the guest.
Thanks to the BIOS, the device is already pre-configured.
The driver uses the real-mode PCI BIOS to request the current configuration of this PCI device.
To communicate with the host, the guest must send the PCI device the (physical) address
of a buffer containing commands to be sent to the host.
The host will write back the response in the same buffer.
Further details are available in [https://wiki.osdev.org/VirtualBox_Guest_Additions](OSDev).
The only challenge here is getting the physical address from inside 386 protected mode Windows,
but turns out this can be done with the `GetSelectorBase()` WINAPI.

When VirtualBox is told that the guest wants absolute mouse information, VirtualBox will stop sending
relative mouse information via the PS/2 mouse. However, the PS/2 controller will still send interrupts
whenever mouse motion happens, and it will still report mouse button presses. In fact, the only way
to obtain mouse button presses is still through the PS/2 controller.
Thus, the only difference between this driver and a standard PS/2 mouse driver is that,
when an interrupt from the mouse comes in, we won't report the relative mouse motion to Windows.
Rather, we call into VirtualBox (right from the PS/2 BIOS interrupt handler)
to obtain the absolution mouse position, and report that to Windows.

### Known issues

Unfortunately, when a MS-DOS old application is foregrounded full-screen, this mouse driver will remain active,
and therefore VirtualBox will not send relative motion data via the PS/2 protocol.
This means full-screen MS-DOS programs will see a PS/2 mouse and will receive button presses from it, but will not
receive mouse motion. The driver would need to deactivative itself in response to a full-screen MS-DOS VM, perhaps
by hooking interrupt 2Fh function 4002h (Notify Foreground Switch) or the like.

