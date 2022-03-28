/*
 * Definitions in this file come from VirtualBox.
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef VBOXDEV_H
#define VBOXDEV_H

#include <stdint.h>

/* Basic defines required for interoperability with VirtualBox's VMM device */

#define AssertCompileSize(type, size) /**/
#define RT_BIT(bit)                             ( 1U << (bit) )

#define VMMDEV_VERSION                      0x00010004UL

/** Version of VMMDevRequestHeader structure. */
#define VMMDEV_REQUEST_HEADER_VERSION (0x10001)

/** PC port for debug output */
#define RTLOG_DEBUG_PORT 0x504

/** Port for generic request interface (relative offset). */
#define VMMDEV_PORT_OFF_REQUEST                             0
/** Port for requests that can be handled w/o going to ring-3 (relative offset).
 * This works like VMMDevReq_AcknowledgeEvents when read.  */
#define VMMDEV_PORT_OFF_REQUEST_FAST                        8

/** Version of VMMDevMemory structure (VMMDevMemory::u32Version). */
# define VMMDEV_MEMORY_VERSION   (1)

/** @name VMMDev events.
 *
 * Used mainly by VMMDevReq_AcknowledgeEvents/VMMDevEvents and version 1.3 of
 * VMMDevMemory.
 *
 * @{
 */
/** Host mouse capabilities has been changed. */
#define VMMDEV_EVENT_MOUSE_CAPABILITIES_CHANGED             RT_BIT(0)
/** HGCM event. */
#define VMMDEV_EVENT_HGCM                                   RT_BIT(1)
/** A display change request has been issued. */
#define VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST                 RT_BIT(2)
/** Credentials are available for judgement. */
#define VMMDEV_EVENT_JUDGE_CREDENTIALS                      RT_BIT(3)
/** The guest has been restored. */
#define VMMDEV_EVENT_RESTORED                               RT_BIT(4)
/** Seamless mode state changed. */
#define VMMDEV_EVENT_SEAMLESS_MODE_CHANGE_REQUEST           RT_BIT(5)
/** Memory balloon size changed. */
#define VMMDEV_EVENT_BALLOON_CHANGE_REQUEST                 RT_BIT(6)
/** Statistics interval changed. */
#define VMMDEV_EVENT_STATISTICS_INTERVAL_CHANGE_REQUEST     RT_BIT(7)
/** VRDP status changed. */
#define VMMDEV_EVENT_VRDP                                   RT_BIT(8)
/** New mouse position data available. */
#define VMMDEV_EVENT_MOUSE_POSITION_CHANGED                 RT_BIT(9)
/** CPU hotplug event occurred. */
#define VMMDEV_EVENT_CPU_HOTPLUG                            RT_BIT(10)
/** The mask of valid events, for sanity checking. */
#define VMMDEV_EVENT_VALID_EVENT_MASK                       UINT32_C(0x000007ff)
/** @} */


/**
 * The layout of VMMDEV RAM region that contains information for guest.
 */
typedef struct VMMDevMemory
{
    /** The size of this structure. */
    uint32_t u32Size;
    /** The structure version. (VMMDEV_MEMORY_VERSION) */
    uint32_t u32Version;

    union
    {
        struct
        {
            /** Flag telling that VMMDev set the IRQ and acknowlegment is required */
            uint32_t fHaveEvents;
        } V1_04;

        struct
        {
            /** Pending events flags, set by host. */
            uint32_t u32HostEvents;
            /** Mask of events the guest wants to see, set by guest. */
            uint32_t u32GuestEventMask;
        } V1_03;
    } V;

    //VBVAMEMORY vbvaMemory;

} VMMDevMemory;

typedef enum VBOXOSTYPE
{
    VBOXOSTYPE_Unknown          = 0,
    VBOXOSTYPE_Unknown_x64      = 0x00100,
    /** @name DOS and it's descendants
     * @{ */
    VBOXOSTYPE_DOS              = 0x10000,
    VBOXOSTYPE_Win31            = 0x15000,
    VBOXOSTYPE_Win9x            = 0x20000,
    VBOXOSTYPE_Win95            = 0x21000,
    VBOXOSTYPE_Win98            = 0x22000,
    VBOXOSTYPE_WinMe            = 0x23000,
    VBOXOSTYPE_WinNT            = 0x30000,
    VBOXOSTYPE_WinNT_x64        = 0x30100,
    VBOXOSTYPE_WinNT3x          = 0x30800,
    VBOXOSTYPE_WinNT4           = 0x31000,
    VBOXOSTYPE_Win2k            = 0x32000,
    VBOXOSTYPE_WinXP            = 0x33000,
    VBOXOSTYPE_WinXP_x64        = 0x33100,
    VBOXOSTYPE_Win2k3           = 0x34000,
    VBOXOSTYPE_Win2k3_x64       = 0x34100,
    VBOXOSTYPE_WinVista         = 0x35000,
    VBOXOSTYPE_WinVista_x64     = 0x35100,
    VBOXOSTYPE_Win2k8           = 0x36000,
    VBOXOSTYPE_Win2k8_x64       = 0x36100,
    VBOXOSTYPE_Win7             = 0x37000,
    VBOXOSTYPE_Win7_x64         = 0x37100,
    VBOXOSTYPE_Win8             = 0x38000,
    VBOXOSTYPE_Win8_x64         = 0x38100,
    VBOXOSTYPE_Win2k12_x64      = 0x39100,
    VBOXOSTYPE_Win81            = 0x3A000,
    VBOXOSTYPE_Win81_x64        = 0x3A100,
    VBOXOSTYPE_Win10            = 0x3B000,
    VBOXOSTYPE_Win10_x64        = 0x3B100,
    VBOXOSTYPE_Win2k16_x64      = 0x3C100,
    VBOXOSTYPE_Win2k19_x64      = 0x3D100,
    VBOXOSTYPE_Win11_x64        = 0x3E100,
    VBOXOSTYPE_OS2              = 0x40000,
    VBOXOSTYPE_OS2Warp3         = 0x41000,
    VBOXOSTYPE_OS2Warp4         = 0x42000,
    VBOXOSTYPE_OS2Warp45        = 0x43000,
    VBOXOSTYPE_ECS              = 0x44000,
    VBOXOSTYPE_ArcaOS           = 0x45000,
    VBOXOSTYPE_OS21x            = 0x48000,
    /** @} */
    /** @name Unixy related OSes
     * @{ */
    VBOXOSTYPE_Linux            = 0x50000,
    VBOXOSTYPE_Linux_x64        = 0x50100,
    VBOXOSTYPE_Linux22          = 0x51000,
    VBOXOSTYPE_Linux24          = 0x52000,
    VBOXOSTYPE_Linux24_x64      = 0x52100,
    VBOXOSTYPE_Linux26          = 0x53000,
    VBOXOSTYPE_Linux26_x64      = 0x53100,
    VBOXOSTYPE_ArchLinux        = 0x54000,
    VBOXOSTYPE_ArchLinux_x64    = 0x54100,
    VBOXOSTYPE_Debian           = 0x55000,
    VBOXOSTYPE_Debian_x64       = 0x55100,
    VBOXOSTYPE_OpenSUSE         = 0x56000,
    VBOXOSTYPE_OpenSUSE_x64     = 0x56100,
    VBOXOSTYPE_FedoraCore       = 0x57000,
    VBOXOSTYPE_FedoraCore_x64   = 0x57100,
    VBOXOSTYPE_Gentoo           = 0x58000,
    VBOXOSTYPE_Gentoo_x64       = 0x58100,
    VBOXOSTYPE_Mandriva         = 0x59000,
    VBOXOSTYPE_Mandriva_x64     = 0x59100,
    VBOXOSTYPE_RedHat           = 0x5A000,
    VBOXOSTYPE_RedHat_x64       = 0x5A100,
    VBOXOSTYPE_Turbolinux       = 0x5B000,
    VBOXOSTYPE_Turbolinux_x64   = 0x5B100,
    VBOXOSTYPE_Ubuntu           = 0x5C000,
    VBOXOSTYPE_Ubuntu_x64       = 0x5C100,
    VBOXOSTYPE_Xandros          = 0x5D000,
    VBOXOSTYPE_Xandros_x64      = 0x5D100,
    VBOXOSTYPE_Oracle           = 0x5E000,
    VBOXOSTYPE_Oracle_x64       = 0x5E100,
    VBOXOSTYPE_FreeBSD          = 0x60000,
    VBOXOSTYPE_FreeBSD_x64      = 0x60100,
    VBOXOSTYPE_OpenBSD          = 0x61000,
    VBOXOSTYPE_OpenBSD_x64      = 0x61100,
    VBOXOSTYPE_NetBSD           = 0x62000,
    VBOXOSTYPE_NetBSD_x64       = 0x62100,
    VBOXOSTYPE_Netware          = 0x70000,
    VBOXOSTYPE_Solaris          = 0x80000,
    VBOXOSTYPE_Solaris_x64      = 0x80100,
    VBOXOSTYPE_OpenSolaris      = 0x81000,
    VBOXOSTYPE_OpenSolaris_x64  = 0x81100,
    VBOXOSTYPE_Solaris11_x64    = 0x82100,
    VBOXOSTYPE_L4               = 0x90000,
    VBOXOSTYPE_QNX              = 0xA0000,
    VBOXOSTYPE_MacOS            = 0xB0000,
    VBOXOSTYPE_MacOS_x64        = 0xB0100,
    VBOXOSTYPE_MacOS106         = 0xB2000,
    VBOXOSTYPE_MacOS106_x64     = 0xB2100,
    VBOXOSTYPE_MacOS107_x64     = 0xB3100,
    VBOXOSTYPE_MacOS108_x64     = 0xB4100,
    VBOXOSTYPE_MacOS109_x64     = 0xB5100,
    VBOXOSTYPE_MacOS1010_x64    = 0xB6100,
    VBOXOSTYPE_MacOS1011_x64    = 0xB7100,
    VBOXOSTYPE_MacOS1012_x64    = 0xB8100,
    VBOXOSTYPE_MacOS1013_x64    = 0xB9100,
    /** @} */
    /** @name Other OSes and stuff
     * @{ */
    VBOXOSTYPE_JRockitVE        = 0xC0000,
    VBOXOSTYPE_Haiku            = 0xD0000,
    VBOXOSTYPE_Haiku_x64        = 0xD0100,
    VBOXOSTYPE_VBoxBS_x64       = 0xE0100,
    /** @} */

/** The bit number which indicates 64-bit or 32-bit. */
#define VBOXOSTYPE_x64_BIT       8
    /** The mask which indicates 64-bit. */
    VBOXOSTYPE_x64              = 1 << VBOXOSTYPE_x64_BIT,

    /** The usual 32-bit hack. */
    VBOXOSTYPE_32BIT_HACK = 0x7fffffff
} VBOXOSTYPE;

typedef enum VMMDevRequestType
{
    VMMDevReq_InvalidRequest             =  0,
    VMMDevReq_GetMouseStatus             =  1,
    VMMDevReq_SetMouseStatus             =  2,
    VMMDevReq_SetPointerShape            =  3,
    VMMDevReq_GetHostVersion             =  4,
    VMMDevReq_Idle                       =  5,
    VMMDevReq_GetHostTime                = 10,
    VMMDevReq_GetHypervisorInfo          = 20,
    VMMDevReq_SetHypervisorInfo          = 21,
    VMMDevReq_RegisterPatchMemory        = 22, /**< @since version 3.0.6 */
    VMMDevReq_DeregisterPatchMemory      = 23, /**< @since version 3.0.6 */
    VMMDevReq_SetPowerStatus             = 30,
    VMMDevReq_AcknowledgeEvents          = 41,
    VMMDevReq_CtlGuestFilterMask         = 42,
    VMMDevReq_ReportGuestInfo            = 50,
    VMMDevReq_ReportGuestInfo2           = 58, /**< @since version 3.2.0 */
    VMMDevReq_ReportGuestStatus          = 59, /**< @since version 3.2.8 */
    VMMDevReq_ReportGuestUserState       = 74, /**< @since version 4.3 */
    VMMDevReq_SizeHack                   = 0x7fffffff
} VMMDevRequestType;

typedef struct VMMDevRequestHeader
{
    /** IN: Size of the structure in bytes (including body).
     * (VBGLREQHDR uses this for input size and output if reserved1 is zero). */
    uint32_t size;
    /** IN: Version of the structure.  */
    uint32_t version;
    /** IN: Type of the request.
     * @note VBGLREQHDR uses this for optional output size. */
    VMMDevRequestType requestType;
    /** OUT: VBox status code. */
    int32_t  rc;
    /** Reserved field no.1. MBZ.
     * @note VBGLREQHDR uses this for optional output size, however never for a
     *       real VMMDev request, only in the I/O control interface. */
    uint32_t reserved1;
    /** IN: Requestor information (VMMDEV_REQUESTOR_XXX) when
     * VBOXGSTINFO2_F_REQUESTOR_INFO is set, otherwise ignored by the host. */
    uint32_t fRequestor;
} VMMDevRequestHeader;

/**
 * Mouse status request structure.
 *
 * Used by VMMDevReq_GetMouseStatus and VMMDevReq_SetMouseStatus.
 */
typedef struct
{
    /** header */
    VMMDevRequestHeader header;
    /** Mouse feature mask. See VMMDEV_MOUSE_*. */
    uint32_t mouseFeatures;
    /** Mouse x position. */
    int32_t pointerXPos;
    /** Mouse y position. */
    int32_t pointerYPos;
} VMMDevReqMouseStatus;
AssertCompileSize(VMMDevReqMouseStatus, 24+12);

/** @name Mouse capability bits (VMMDevReqMouseStatus::mouseFeatures).
 * @{ */
/** The guest can (== wants to) handle absolute coordinates.  */
#define VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE                     RT_BIT(0)
/** The host can (== wants to) send absolute coordinates.
 * (Input not captured.) */
#define VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE                    RT_BIT(1)
/** The guest can *NOT* switch to software cursor and therefore depends on the
 * host cursor.
 *
 * When guest additions are installed and the host has promised to display the
 * cursor itself, the guest installs a hardware mouse driver. Don't ask the
 * guest to switch to a software cursor then. */
#define VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR                RT_BIT(2)
/** The host does NOT provide support for drawing the cursor itself. */
#define VMMDEV_MOUSE_HOST_CANNOT_HWPOINTER                  RT_BIT(3)
/** The guest can read VMMDev events to find out about pointer movement */
#define VMMDEV_MOUSE_NEW_PROTOCOL                           RT_BIT(4)
/** If the guest changes the status of the
 * VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR bit, the host will honour this */
#define VMMDEV_MOUSE_HOST_RECHECKS_NEEDS_HOST_CURSOR        RT_BIT(5)
/** The host supplies an absolute pointing device.  The Guest Additions may
 * wish to use this to decide whether to install their own driver */
#define VMMDEV_MOUSE_HOST_HAS_ABS_DEV                       RT_BIT(6)
/** The mask of all VMMDEV_MOUSE_* flags */
#define VMMDEV_MOUSE_MASK                                   UINT32_C(0x0000007f)
/** The mask of guest capability changes for which notification events should
 * be sent */
#define VMMDEV_MOUSE_NOTIFY_HOST_MASK \
      (VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE | VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR)
/** The mask of all capabilities which the guest can legitimately change */
#define VMMDEV_MOUSE_GUEST_MASK \
      (VMMDEV_MOUSE_NOTIFY_HOST_MASK | VMMDEV_MOUSE_NEW_PROTOCOL)
/** The mask of host capability changes for which notification events should
 * be sent */
#define VMMDEV_MOUSE_NOTIFY_GUEST_MASK \
      VMMDEV_MOUSE_HOST_WANTS_ABSOLUTE
/** The mask of all capabilities which the host can legitimately change */
#define VMMDEV_MOUSE_HOST_MASK \
      (  VMMDEV_MOUSE_NOTIFY_GUEST_MASK \
       | VMMDEV_MOUSE_HOST_CANNOT_HWPOINTER \
       | VMMDEV_MOUSE_HOST_RECHECKS_NEEDS_HOST_CURSOR \
       | VMMDEV_MOUSE_HOST_HAS_ABS_DEV)
/** @} */

/** @name Absolute mouse reporting range
 * @{ */
/** @todo Should these be here?  They are needed by both host and guest. */
/** The minumum value our pointing device can return. */
#define VMMDEV_MOUSE_RANGE_MIN 0
/** The maximum value our pointing device can return. */
#define VMMDEV_MOUSE_RANGE_MAX 0xFFFF
/** The full range our pointing device can return. */
#define VMMDEV_MOUSE_RANGE (VMMDEV_MOUSE_RANGE_MAX - VMMDEV_MOUSE_RANGE_MIN)
/** @} */

/** @name VBVAMOUSEPOINTERSHAPE::fu32Flags
 * @note The VBOX_MOUSE_POINTER_* flags are used in the guest video driver,
 *       values must be <= 0x8000 and must not be changed. (try make more sense
 *       of this, please).
 * @{
 */
/** pointer is visible */
#define VBOX_MOUSE_POINTER_VISIBLE (0x0001)
/** pointer has alpha channel */
#define VBOX_MOUSE_POINTER_ALPHA   (0x0002)
/** pointerData contains new pointer shape */
#define VBOX_MOUSE_POINTER_SHAPE   (0x0004)

/**
 * Mouse pointer shape/visibility change request.
 *
 * Used by VMMDevReq_SetPointerShape. The size is variable.
 */
typedef struct VMMDevReqMousePointer
{
    /** Header. */
    VMMDevRequestHeader header;
    /** VBOX_MOUSE_POINTER_* bit flags from VBox/Graphics/VBoxVideo.h. */
    uint32_t fFlags;
    /** x coordinate of hot spot. */
    uint32_t xHot;
    /** y coordinate of hot spot. */
    uint32_t yHot;
    /** Width of the pointer in pixels. */
    uint32_t width;
    /** Height of the pointer in scanlines. */
    uint32_t height;
    /** Pointer data.
     *
     ****
     * The data consists of 1 bpp AND mask followed by 32 bpp XOR (color) mask.
     *
     * For pointers without alpha channel the XOR mask pixels are 32 bit values: (lsb)BGR0(msb).
     * For pointers with alpha channel the XOR mask consists of (lsb)BGRA(msb) 32 bit values.
     *
     * Guest driver must create the AND mask for pointers with alpha channel, so if host does not
     * support alpha, the pointer could be displayed as a normal color pointer. The AND mask can
     * be constructed from alpha values. For example alpha value >= 0xf0 means bit 0 in the AND mask.
     *
     * The AND mask is 1 bpp bitmap with byte aligned scanlines. Size of AND mask,
     * therefore, is cbAnd = (width + 7) / 8 * height. The padding bits at the
     * end of any scanline are undefined.
     *
     * The XOR mask follows the AND mask on the next 4 bytes aligned offset:
     * uint8_t *pXor = pAnd + (cbAnd + 3) & ~3
     * Bytes in the gap between the AND and the XOR mask are undefined.
     * XOR mask scanlines have no gap between them and size of XOR mask is:
     * cXor = width * 4 * height.
     ****
     *
     * Preallocate 4 bytes for accessing actual data as p->pointerData.
     */
    char pointerData[4];
} VMMDevReqMousePointer;
AssertCompileSize(VMMDevReqMousePointer, 24+24);

/**
 * Pending events structure.
 *
 * Used by VMMDevReq_AcknowledgeEvents.
 */
typedef struct
{
    /** Header. */
    VMMDevRequestHeader header;
    /** OUT: Pending event mask. */
    uint32_t events;
} VMMDevEvents;
AssertCompileSize(VMMDevEvents, 24+4);


/**
 * Guest event filter mask control.
 *
 * Used by VMMDevReq_CtlGuestFilterMask.
 */
typedef struct
{
    /** Header. */
    VMMDevRequestHeader header;
    /** Mask of events to be added to the filter. */
    uint32_t u32OrMask;
    /** Mask of events to be removed from the filter. */
    uint32_t u32NotMask;
} VMMDevCtlGuestFilterMask;
AssertCompileSize(VMMDevCtlGuestFilterMask, 24+8);

/**
 * Guest information structure.
 *
 * Used by VMMDevReportGuestInfo and PDMIVMMDEVCONNECTOR::pfnUpdateGuestVersion.
 */
typedef struct VBoxGuestInfo
{
    /** The VMMDev interface version expected by additions.
      * *Deprecated*, do not use anymore! Will be removed. */
    uint32_t interfaceVersion;
    /** Guest OS type. */
    VBOXOSTYPE osType;
} VBoxGuestInfo;
AssertCompileSize(VBoxGuestInfo, 8);

/**
 * Guest information report.
 *
 * Used by VMMDevReq_ReportGuestInfo.
 */
typedef struct
{
    /** Header. */
    VMMDevRequestHeader header;
    /** Guest information. */
    VBoxGuestInfo guestInfo;
} VMMDevReportGuestInfo;
AssertCompileSize(VMMDevReportGuestInfo, 24+8);


/**
 * Guest information structure, version 2.
 *
 * Used by VMMDevReportGuestInfo2 and PDMIVMMDEVCONNECTOR::pfnUpdateGuestVersion2.
 */
typedef struct VBoxGuestInfo2
{
    /** Major version. */
    uint16_t additionsMajor;
    /** Minor version. */
    uint16_t additionsMinor;
    /** Build number. */
    uint32_t additionsBuild;
    /** SVN revision. */
    uint32_t additionsRevision;
    /** Feature mask, VBOXGSTINFO2_F_XXX. */
    uint32_t additionsFeatures;
    /** The intentional meaning of this field was:
     * Some additional information, for example 'Beta 1' or something like that.
     *
     * The way it was implemented was implemented: VBOX_VERSION_STRING.
     *
     * This means the first three members are duplicated in this field (if the guest
     * build config is sane). So, the user must check this and chop it off before
     * usage.  There is, because of the Main code's blind trust in the field's
     * content, no way back. */
    char     szName[128];
} VBoxGuestInfo2;
AssertCompileSize(VBoxGuestInfo2, 144);

#endif
