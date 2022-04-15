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

#pragma pack(push,4)

#define AssertCompile(expr)           typedef int STATIC_ASSERT_FAILED[(expr) ? 1 : -1]
#define AssertCompileSize(type, size) AssertCompile(sizeof(type) == size)

#define RT_BIT(bit)                             ( 1U << (bit) )

#define VINF_SUCCESS                        0

/** General failure - DON'T USE THIS!!! */
#define VERR_GENERAL_FAILURE                (-1)
/** Invalid parameter. */
#define VERR_INVALID_PARAMETER              (-2)
/** Invalid parameter. */
#define VWRN_INVALID_PARAMETER              2
/** Invalid magic or cookie. */
#define VERR_INVALID_MAGIC                  (-3)
/** Invalid magic or cookie. */
#define VWRN_INVALID_MAGIC                  3
/** Invalid loader handle. */
#define VERR_INVALID_HANDLE                 (-4)
/** Invalid loader handle. */
#define VWRN_INVALID_HANDLE                 4
/** Failed to lock the address range. */
#define VERR_LOCK_FAILED                    (-5)
/** Invalid memory pointer. */
#define VERR_INVALID_POINTER                (-6)
/** Failed to patch the IDT. */
#define VERR_IDT_FAILED                     (-7)
/** Memory allocation failed. */
#define VERR_NO_MEMORY                      (-8)
/** Already loaded. */
#define VERR_ALREADY_LOADED                 (-9)
/** Permission denied. */
#define VERR_PERMISSION_DENIED              (-10)
/** Permission denied. */
#define VINF_PERMISSION_DENIED              10
/** Version mismatch. */
#define VERR_VERSION_MISMATCH               (-11)
/** The request function is not implemented. */
#define VERR_NOT_IMPLEMENTED                (-12)
/** The request function is not implemented. */
#define VINF_NOT_IMPLEMENTED                12
/** Invalid flags was given. */
#define VERR_INVALID_FLAGS                  (-13)

/** Not equal. */
#define VERR_NOT_EQUAL                      (-18)
/** The specified path does not point at a symbolic link. */
#define VERR_NOT_SYMLINK                    (-19)
/** Failed to allocate temporary memory. */
#define VERR_NO_TMP_MEMORY                  (-20)
/** Invalid file mode mask (RTFMODE). */
#define VERR_INVALID_FMODE                  (-21)
/** Incorrect call order. */
#define VERR_WRONG_ORDER                    (-22)
/** There is no TLS (thread local storage) available for storing the current thread. */
#define VERR_NO_TLS_FOR_SELF                (-23)
/** Failed to set the TLS (thread local storage) entry which points to our thread structure. */
#define VERR_FAILED_TO_SET_SELF_TLS         (-24)
/** Not able to allocate contiguous memory. */
#define VERR_NO_CONT_MEMORY                 (-26)
/** No memory available for page table or page directory. */
#define VERR_NO_PAGE_MEMORY                 (-27)
/** Already initialized. */
#define VINF_ALREADY_INITIALIZED            28
/** The specified thread is dead. */
#define VERR_THREAD_IS_DEAD                 (-29)
/** The specified thread is not waitable. */
#define VERR_THREAD_NOT_WAITABLE            (-30)
/** Pagetable not present. */
#define VERR_PAGE_TABLE_NOT_PRESENT         (-31)
/** Invalid context.
 * Typically an API was used by the wrong thread. */
#define VERR_INVALID_CONTEXT                (-32)
/** The per process timer is busy. */
#define VERR_TIMER_BUSY                     (-33)
/** Address conflict. */
#define VERR_ADDRESS_CONFLICT               (-34)
/** Unresolved (unknown) host platform error. */
#define VERR_UNRESOLVED_ERROR               (-35)
/** Invalid function. */
#define VERR_INVALID_FUNCTION               (-36)
/** Not supported. */
#define VERR_NOT_SUPPORTED                  (-37)
/** Not supported. */
#define VINF_NOT_SUPPORTED                  37
/** Access denied. */
#define VERR_ACCESS_DENIED                  (-38)
/** Call interrupted. */
#define VERR_INTERRUPTED                    (-39)
/** Call interrupted. */
#define VINF_INTERRUPTED                    39
/** Timeout. */
#define VERR_TIMEOUT                        (-40)
/** Timeout. */
#define VINF_TIMEOUT                        40
/** Buffer too small to save result. */
#define VERR_BUFFER_OVERFLOW                (-41)
/** Buffer too small to save result. */
#define VINF_BUFFER_OVERFLOW                41
/** Data size overflow. */
#define VERR_TOO_MUCH_DATA                  (-42)
/** Max threads number reached. */
#define VERR_MAX_THRDS_REACHED              (-43)
/** Max process number reached. */
#define VERR_MAX_PROCS_REACHED              (-44)
/** The recipient process has refused the signal. */
#define VERR_SIGNAL_REFUSED                 (-45)
/** A signal is already pending. */
#define VERR_SIGNAL_PENDING                 (-46)
/** The signal being posted is not correct. */
#define VERR_SIGNAL_INVALID                 (-47)
/** The state changed.
 * This is a generic error message and needs a context to make sense. */
#define VERR_STATE_CHANGED                  (-48)
/** Warning, the state changed.
 * This is a generic error message and needs a context to make sense. */
#define VWRN_STATE_CHANGED                  48
/** Error while parsing UUID string */
#define VERR_INVALID_UUID_FORMAT            (-49)
/** The specified process was not found. */
#define VERR_PROCESS_NOT_FOUND              (-50)
/** The process specified to a non-block wait had not exited. */
#define VERR_PROCESS_RUNNING                (-51)
/** Retry the operation. */
#define VERR_TRY_AGAIN                      (-52)
/** Retry the operation. */
#define VINF_TRY_AGAIN                      52
/** Generic parse error. */
#define VERR_PARSE_ERROR                    (-53)
/** Value out of range. */
#define VERR_OUT_OF_RANGE                   (-54)
/** A numeric conversion encountered a value which was too big for the target. */
#define VERR_NUMBER_TOO_BIG                 (-55)
/** A numeric conversion encountered a value which was too big for the target. */
#define VWRN_NUMBER_TOO_BIG                 55
/** The number begin converted (string) contained no digits. */
#define VERR_NO_DIGITS                      (-56)
/** The number begin converted (string) contained no digits. */
#define VWRN_NO_DIGITS                      56
/** Encountered a '-' during conversion to an unsigned value. */
#define VERR_NEGATIVE_UNSIGNED              (-57)
/** Encountered a '-' during conversion to an unsigned value. */
#define VWRN_NEGATIVE_UNSIGNED              57
/** Error while characters translation (unicode and so). */
#define VERR_NO_TRANSLATION                 (-58)
/** Error while characters translation (unicode and so). */
#define VWRN_NO_TRANSLATION                 58
/** Encountered unicode code point which is reserved for use as endian indicator (0xffff or 0xfffe). */
#define VERR_CODE_POINT_ENDIAN_INDICATOR    (-59)
/** Encountered unicode code point in the surrogate range (0xd800 to 0xdfff). */
#define VERR_CODE_POINT_SURROGATE           (-60)
/** A string claiming to be UTF-8 is incorrectly encoded. */
#define VERR_INVALID_UTF8_ENCODING          (-61)
/** A string claiming to be in UTF-16 is incorrectly encoded. */
#define VERR_INVALID_UTF16_ENCODING         (-62)
/** Encountered a unicode code point which cannot be represented as UTF-16. */
#define VERR_CANT_RECODE_AS_UTF16           (-63)
/** Got an out of memory condition trying to allocate a string. */
#define VERR_NO_STR_MEMORY                  (-64)
/** Got an out of memory condition trying to allocate a UTF-16 (/UCS-2) string. */
#define VERR_NO_UTF16_MEMORY                (-65)
/** Get an out of memory condition trying to allocate a code point array. */
#define VERR_NO_CODE_POINT_MEMORY           (-66)
/** Can't free the memory because it's used in mapping. */
#define VERR_MEMORY_BUSY                    (-67)
/** The timer can't be started because it's already active. */
#define VERR_TIMER_ACTIVE                   (-68)
/** The timer can't be stopped because it's already suspended. */
#define VERR_TIMER_SUSPENDED                (-69)
/** The operation was cancelled by the user (copy) or another thread (local ipc). */
#define VERR_CANCELLED                      (-70)
/** Failed to initialize a memory object.
 * Exactly what this means is OS specific. */
#define VERR_MEMOBJ_INIT_FAILED             (-71)
/** Out of memory condition when allocating memory with low physical backing. */
#define VERR_NO_LOW_MEMORY                  (-72)
/** Out of memory condition when allocating physical memory (without mapping). */
#define VERR_NO_PHYS_MEMORY                 (-73)
/** The address (virtual or physical) is too big. */
#define VERR_ADDRESS_TOO_BIG                (-74)
/** Failed to map a memory object. */
#define VERR_MAP_FAILED                     (-75)
/** Trailing characters. */
#define VERR_TRAILING_CHARS                 (-76)
/** Trailing characters. */
#define VWRN_TRAILING_CHARS                 76
/** Trailing spaces. */
#define VERR_TRAILING_SPACES                (-77)
/** Trailing spaces. */
#define VWRN_TRAILING_SPACES                77
/** Generic not found error. */
#define VERR_NOT_FOUND                      (-78)
/** Generic not found warning. */
#define VWRN_NOT_FOUND                      78
/** Generic invalid state error. */
#define VERR_INVALID_STATE                  (-79)
/** Generic invalid state warning. */
#define VWRN_INVALID_STATE                  79
/** Generic out of resources error. */
#define VERR_OUT_OF_RESOURCES               (-80)
/** Generic out of resources warning. */
#define VWRN_OUT_OF_RESOURCES               80
/** No more handles available, too many open handles. */
#define VERR_NO_MORE_HANDLES                (-81)
/** Preemption is disabled.
 * The requested operation can only be performed when preemption is enabled. */
#define VERR_PREEMPT_DISABLED               (-82)
/** End of string. */
#define VERR_END_OF_STRING                  (-83)
/** End of string. */
#define VINF_END_OF_STRING                  83
/** A page count is out of range. */
#define VERR_PAGE_COUNT_OUT_OF_RANGE        (-84)
/** Generic object destroyed status. */
#define VERR_OBJECT_DESTROYED               (-85)
/** Generic object was destroyed by the call status. */
#define VINF_OBJECT_DESTROYED               85
/** Generic dangling objects status. */
#define VERR_DANGLING_OBJECTS               (-86)
/** Generic dangling objects status. */
#define VWRN_DANGLING_OBJECTS               86
/** Invalid Base64 encoding. */
#define VERR_INVALID_BASE64_ENCODING        (-87)
/** Return instigated by a callback or similar. */
#define VERR_CALLBACK_RETURN                (-88)
/** Return instigated by a callback or similar. */
#define VINF_CALLBACK_RETURN                88
/** Authentication failure. */
#define VERR_AUTHENTICATION_FAILURE         (-89)
/** Not a power of two. */
#define VERR_NOT_POWER_OF_TWO               (-90)
/** Status code, typically given as a parameter, that isn't supposed to be used. */
#define VERR_IGNORED                        (-91)
/** Concurrent access to the object is not allowed. */
#define VERR_CONCURRENT_ACCESS              (-92)
/** The caller does not have a reference to the object.
 * This status is used when two threads is caught sharing the same object
 * reference. */
#define VERR_CALLER_NO_REFERENCE            (-93)
/** Generic no change error. */
#define VERR_NO_CHANGE                      (-95)
/** Generic no change info. */
#define VINF_NO_CHANGE                      95
/** Out of memory condition when allocating executable memory. */
#define VERR_NO_EXEC_MEMORY                 (-96)
/** The alignment is not supported. */
#define VERR_UNSUPPORTED_ALIGNMENT          (-97)
/** The alignment is not really supported, however we got lucky with this
 * allocation. */
#define VINF_UNSUPPORTED_ALIGNMENT          97
/** Duplicate something. */
#define VERR_DUPLICATE                      (-98)
/** Something is missing. */
#define VERR_MISSING                        (-99)
/** An unexpected (/unknown) exception was caught. */

#define VERR_FILE_IO_ERROR                  (-100)
/** File/Device open failed. */
#define VERR_OPEN_FAILED                    (-101)
/** File not found. */
#define VERR_FILE_NOT_FOUND                 (-102)
/** Path not found. */
#define VERR_PATH_NOT_FOUND                 (-103)
/** Invalid (malformed) file/path name. */
#define VERR_INVALID_NAME                   (-104)
/** The object in question already exists. */
#define VERR_ALREADY_EXISTS                 (-105)
/** The object in question already exists. */
#define VWRN_ALREADY_EXISTS                 105
/** Too many open files. */
#define VERR_TOO_MANY_OPEN_FILES            (-106)
/** Seek error. */
#define VERR_SEEK                           (-107)
/** Seek below file start. */
#define VERR_NEGATIVE_SEEK                  (-108)
/** Trying to seek on device. */
#define VERR_SEEK_ON_DEVICE                 (-109)
/** Reached the end of the file. */
#define VERR_EOF                            (-110)
/** Reached the end of the file. */
#define VINF_EOF                            110
/** Generic file read error. */
#define VERR_READ_ERROR                     (-111)
/** Generic file write error. */
#define VERR_WRITE_ERROR                    (-112)
/** Write protect error. */
#define VERR_WRITE_PROTECT                  (-113)
/** Sharing violation, file is being used by another process. */
#define VERR_SHARING_VIOLATION              (-114)
/** Unable to lock a region of a file. */
#define VERR_FILE_LOCK_FAILED               (-115)
/** File access error, another process has locked a portion of the file. */
#define VERR_FILE_LOCK_VIOLATION            (-116)
/** File or directory can't be created. */
#define VERR_CANT_CREATE                    (-117)
/** Directory can't be deleted. */
#define VERR_CANT_DELETE_DIRECTORY          (-118)
/** Can't move file to another disk. */
#define VERR_NOT_SAME_DEVICE                (-119)
/** The filename or extension is too long. */
#define VERR_FILENAME_TOO_LONG              (-120)
/** Media not present in drive. */
#define VERR_MEDIA_NOT_PRESENT              (-121)
/** The type of media was not recognized. Not formatted? */
#define VERR_MEDIA_NOT_RECOGNIZED           (-122)
/** Can't unlock - region was not locked. */
#define VERR_FILE_NOT_LOCKED                (-123)
/** Unrecoverable error: lock was lost. */
#define VERR_FILE_LOCK_LOST                 (-124)
/** Can't delete directory with files. */
#define VERR_DIR_NOT_EMPTY                  (-125)
/** A directory operation was attempted on a non-directory object. */
#define VERR_NOT_A_DIRECTORY                (-126)
/** A non-directory operation was attempted on a directory object. */
#define VERR_IS_A_DIRECTORY                 (-127)
/** Tried to grow a file beyond the limit imposed by the process or the filesystem. */
#define VERR_FILE_TOO_BIG                   (-128)

/** @name Generic Directory Enumeration Status Codes
 * @{
 */
/** Unresolved (unknown) search error. */
#define VERR_SEARCH_ERROR                   (-200)
/** No more files found. */
#define VERR_NO_MORE_FILES                  (-201)
/** No more search handles available. */
#define VERR_NO_MORE_SEARCH_HANDLES         (-202)
/** RTDirReadEx() failed to retrieve the extra data which was requested. */
#define VWRN_NO_DIRENT_INFO                 203
/** @} */

/** Unresolved (unknown) device i/o error. */
#define VERR_DEV_IO_ERROR                   (-250)
/** Device i/o: Bad unit. */
#define VERR_IO_BAD_UNIT                    (-251)
/** Device i/o: Not ready. */
#define VERR_IO_NOT_READY                   (-252)
/** Device i/o: Bad command. */
#define VERR_IO_BAD_COMMAND                 (-253)
/** Device i/o: CRC error. */
#define VERR_IO_CRC                         (-254)
/** Device i/o: Bad length. */
#define VERR_IO_BAD_LENGTH                  (-255)
/** Device i/o: Sector not found. */
#define VERR_IO_SECTOR_NOT_FOUND            (-256)
/** Device i/o: General failure. */
#define VERR_IO_GEN_FAILURE                 (-257)

/** Requested service does not exist. */
#define VERR_HGCM_SERVICE_NOT_FOUND                 (-2900)
/** Service rejected client connection */
#define VINF_HGCM_CLIENT_REJECTED                   2901
/** Command address is invalid. */
#define VERR_HGCM_INVALID_CMD_ADDRESS               (-2902)
/** Service will execute the command in background. */
#define VINF_HGCM_ASYNC_EXECUTE                     2903
/** HGCM could not perform requested operation because of an internal error. */
#define VERR_HGCM_INTERNAL                          (-2904)
/** Invalid HGCM client id. */
#define VERR_HGCM_INVALID_CLIENT_ID                 (-2905)
/** The HGCM is saving state. */
#define VINF_HGCM_SAVE_STATE                        (2906)
/** Requested service already exists. */
#define VERR_HGCM_SERVICE_EXISTS                    (-2907)
/** Too many clients for the service. */
#define VERR_HGCM_TOO_MANY_CLIENTS                  (-2908)
/** Too many calls to the service from a client. */
#define VERR_HGCM_TOO_MANY_CLIENT_CALLS             (-2909)


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

	VMMDevReq_HGCMConnect                = 60,
	VMMDevReq_HGCMDisconnect             = 61,
	VMMDevReq_HGCMCall32                 = 62,
	VMMDevReq_HGCMCall64                 = 63,

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

/**
 * Idle request structure.
 *
 * Used by VMMDevReq_Idle.
 */
typedef struct
{
	/** Header. */
	VMMDevRequestHeader header;
} VMMDevReqIdle;
AssertCompileSize(VMMDevReqIdle, 24);

// HGCM

typedef uint32_t                RTGCPHYS32;
typedef uint32_t                RTGCPTR32;
typedef uint64_t                RTGCPHYS64;

/**
 * HGCM service location types.
 * @ingroup grp_vmmdev_req
 */
typedef enum
{
	VMMDevHGCMLoc_Invalid    = 0,
	VMMDevHGCMLoc_LocalHost  = 1,
	VMMDevHGCMLoc_LocalHost_Existing = 2,
	VMMDevHGCMLoc_SizeHack   = 0x7fffffff
} HGCMServiceLocationType;
AssertCompileSize(HGCMServiceLocationType, 4);

/**
 * HGCM host service location.
 * @ingroup grp_vmmdev_req
 */
typedef struct
{
	char achName[128]; /**< This is really szName. */
} HGCMServiceLocationHost;
AssertCompileSize(HGCMServiceLocationHost, 128);

/**
 * HGCM service location.
 * @ingroup grp_vmmdev_req
 */
typedef struct HGCMSERVICELOCATION
{
	/** Type of the location. */
	HGCMServiceLocationType type;

	union
	{
		HGCMServiceLocationHost host;
	} u;
} HGCMServiceLocation;
AssertCompileSize(HGCMServiceLocation, 128+4);


/**
 * HGCM parameter type.
 */
typedef enum
{
	VMMDevHGCMParmType_Invalid            = 0,
	VMMDevHGCMParmType_32bit              = 1,
	VMMDevHGCMParmType_64bit              = 2,
	VMMDevHGCMParmType_PhysAddr           = 3,  /**< @deprecated Doesn't work, use PageList. */
	VMMDevHGCMParmType_LinAddr            = 4,  /**< In and Out */
	VMMDevHGCMParmType_LinAddr_In         = 5,  /**< In  (read;  host<-guest) */
	VMMDevHGCMParmType_LinAddr_Out        = 6,  /**< Out (write; host->guest) */
	VMMDevHGCMParmType_LinAddr_Locked     = 7,  /**< Locked In and Out - for VBoxGuest, not host. */
	VMMDevHGCMParmType_LinAddr_Locked_In  = 8,  /**< Locked In  (read;  host<-guest) - for VBoxGuest, not host. */
	VMMDevHGCMParmType_LinAddr_Locked_Out = 9,  /**< Locked Out (write; host->guest) - for VBoxGuest, not host. */
	VMMDevHGCMParmType_PageList           = 10, /**< Physical addresses of locked pages for a buffer. */
	VMMDevHGCMParmType_Embedded           = 11, /**< Small buffer embedded in request. */
	VMMDevHGCMParmType_ContiguousPageList = 12, /**< Like PageList but with physically contiguous memory, so only one page entry. */
	VMMDevHGCMParmType_NoBouncePageList   = 13, /**< Like PageList but host function requires no bounce buffering. */
	VMMDevHGCMParmType_SizeHack           = 0x7fffffff
} HGCMFunctionParameterType;
AssertCompileSize(HGCMFunctionParameterType, 4);

typedef struct
{
	HGCMFunctionParameterType type;
	union
	{
		uint32_t   value32;
		uint64_t   value64;
		struct
		{
			uint32_t size;

			union
			{
				RTGCPHYS32 physAddr;
				RTGCPTR32  linearAddr;
			} u;
		} Pointer;
		struct
		{
			uint32_t  cb;
			RTGCPTR32 uAddr;
		} LinAddr;                      /**< Shorter version of the above Pointer structure. */
		struct
		{
			uint32_t size;              /**< Size of the buffer described by the page list. */
			uint32_t offset;            /**< Relative to the request header, valid if size != 0. */
		} PageList;
		struct
		{
			uint32_t fFlags : 8;        /**< VBOX_HGCM_F_PARM_*. */
			uint32_t offData : 24;      /**< Relative to the request header (must be a valid offset even if cbData is zero). */
			uint32_t cbData;            /**< The buffer size. */
		} Embedded;
	} u;
} HGCMFunctionParameter;
AssertCompileSize(HGCMFunctionParameter, 4+8);

#define VBOX_HGCM_REQ_DONE      0x1
#define VBOX_HGCM_REQ_CANCELLED 0x2

/**
 * HGCM request header.
 */
typedef struct VMMDevHGCMRequestHeader
{
	/** Request header. */
	VMMDevRequestHeader header;

	/** HGCM flags. */
	uint32_t fu32Flags;

	/** Result code. */
	int32_t result;
} VMMDevHGCMRequestHeader;
AssertCompileSize(VMMDevHGCMRequestHeader, 24+8);

/**
 * HGCM connect request structure.
 *
 * Used by VMMDevReq_HGCMConnect.
 */
typedef struct
{
	/** HGCM request header. */
	VMMDevHGCMRequestHeader header;

	/** IN: Description of service to connect to. */
	HGCMServiceLocation loc;

	/** OUT: Client identifier assigned by local instance of HGCM. */
	uint32_t u32ClientID;
} VMMDevHGCMConnect;
AssertCompileSize(VMMDevHGCMConnect, 32+132+4);

/**
 * HGCM disconnect request structure.
 *
 * Used by VMMDevReq_HGCMDisconnect.
 */
typedef struct
{
	/** HGCM request header. */
	VMMDevHGCMRequestHeader header;

	/** IN: Client identifier. */
	uint32_t u32ClientID;
} VMMDevHGCMDisconnect;
AssertCompileSize(VMMDevHGCMDisconnect, 32+4);

/**
 * HGCM call request structure.
 *
 * Used by VMMDevReq_HGCMCall32 and VMMDevReq_HGCMCall64.
 */
typedef struct
{
	/* request header */
	VMMDevHGCMRequestHeader header;

	/** IN: Client identifier. */
	uint32_t u32ClientID;
	/** IN: Service function number. */
	uint32_t u32Function;
	/** IN: Number of parameters. */
	uint32_t cParms;
	/** Parameters follow in form: HGCMFunctionParameter aParms[X]; */
	HGCMFunctionParameter aParms[];
} VMMDevHGCMCall;
AssertCompileSize(VMMDevHGCMCall, 32+12);

/** @name Direction of data transfer (HGCMPageListInfo::flags). Bit flags.
 * @{ */
#define VBOX_HGCM_F_PARM_DIRECTION_NONE      0x00000000UL
#define VBOX_HGCM_F_PARM_DIRECTION_TO_HOST   0x00000001UL
#define VBOX_HGCM_F_PARM_DIRECTION_FROM_HOST 0x00000002UL
#define VBOX_HGCM_F_PARM_DIRECTION_BOTH      0x00000003UL
#define VBOX_HGCM_F_PARM_DIRECTION_MASK      0x00000003UL
/** Macro for validating that the specified flags are valid. */
#define VBOX_HGCM_F_PARM_ARE_VALID(fFlags) \
	(   ((fFlags) & VBOX_HGCM_F_PARM_DIRECTION_MASK) \
	 && !((fFlags) & ~VBOX_HGCM_F_PARM_DIRECTION_MASK) )
/** @} */

/**
 * VMMDevHGCMParmType_PageList points to this structure to actually describe the
 * buffer.
 */
typedef struct
{
	uint32_t flags;        /**< VBOX_HGCM_F_PARM_*. */
	uint16_t offFirstPage; /**< Offset in the first page where data begins. */
	uint16_t cPages;       /**< Number of pages. */
	RTGCPHYS64 aPages[1];  /**< Page addresses. */
} HGCMPageListInfo;
AssertCompileSize(HGCMPageListInfo, 4+2+2+8);

# define VBOX_HGCM_MAX_PARMS 32

/**
 * HGCM cancel request structure.
 *
 * The Cancel request is issued using the same physical memory address as was
 * used for the corresponding initial HGCMCall.
 *
 * Used by VMMDevReq_HGCMCancel.
 */
typedef struct
{
	/** Header. */
	VMMDevHGCMRequestHeader header;
} VMMDevHGCMCancel;
AssertCompileSize(VMMDevHGCMCancel, 32);


// SHARED FOLDERS (shfl)

/** @name Shared Folders service functions. (guest)
 * @{
 */
/** Query mappings changes.
 * @note Description is currently misleading, it will always return all
 *       current mappings with SHFL_MS_NEW status.  Only modification is the
 *       SHFL_MF_AUTOMOUNT flag that causes filtering out non-auto mounts. */
#define SHFL_FN_QUERY_MAPPINGS      (1)
/** Query the name of a map. */
#define SHFL_FN_QUERY_MAP_NAME      (2)
/** Open/create object. */
#define SHFL_FN_CREATE              (3)
/** Close object handle. */
#define SHFL_FN_CLOSE               (4)
/** Read object content. */
#define SHFL_FN_READ                (5)
/** Write new object content. */
#define SHFL_FN_WRITE               (6)
/** Lock/unlock a range in the object. */
#define SHFL_FN_LOCK                (7)
/** List object content. */
#define SHFL_FN_LIST                (8)
/** Query/set object information. */
#define SHFL_FN_INFORMATION         (9)
/** Remove object */
#define SHFL_FN_REMOVE              (11)
/** Map folder (legacy) */
#define SHFL_FN_MAP_FOLDER_OLD      (12)
/** Unmap folder */
#define SHFL_FN_UNMAP_FOLDER        (13)
/** Rename object (possibly moving it to another directory) */
#define SHFL_FN_RENAME              (14)
/** Flush file */
#define SHFL_FN_FLUSH               (15)
/** @todo macl, a description, please. */
#define SHFL_FN_SET_UTF8            (16)
/** Map folder */
#define SHFL_FN_MAP_FOLDER          (17)
/** Read symlink destination.
 * @since VBox 4.0  */
#define SHFL_FN_READLINK            (18) /**< @todo rename to SHFL_FN_READ_LINK (see struct capitalization) */
/** Create symlink.
 * @since VBox 4.0  */
#define SHFL_FN_SYMLINK             (19)
/** Ask host to show symlinks
 * @since VBox 4.0  */
#define SHFL_FN_SET_SYMLINKS        (20)
/** Query information about a map.
 * @since VBox 6.0  */
#define SHFL_FN_QUERY_MAP_INFO      (21)
/** Wait for changes to the mappings.
 * @since VBox 6.0  */
#define SHFL_FN_WAIT_FOR_MAPPINGS_CHANGES       (22)
/** Cancel all waits for changes to the mappings for the calling client.
 * The wait calls will return VERR_CANCELLED.
 * @since VBox 6.0  */
#define SHFL_FN_CANCEL_MAPPINGS_CHANGES_WAITS   (23)
/** Sets the file size.
 * @since VBox 6.0  */
#define SHFL_FN_SET_FILE_SIZE       (24)
/** Queries supported features.
 * @since VBox 6.0.6  */
#define SHFL_FN_QUERY_FEATURES      (25)
/** Copies a file to another.
 * @since VBox 6.0.6  */
#define SHFL_FN_COPY_FILE           (26)
/** Copies part of a file to another.
 * @since VBox 6.0.6  */
#define SHFL_FN_COPY_FILE_PART      (27)
/** Close handle to (optional) and remove object by path.
 * This function is tailored for Windows guests.
 * @since VBox 6.0.8  */
#define SHFL_FN_CLOSE_AND_REMOVE    (28)
/** Set the host error code style.
 * This is for more efficiently getting the correct error status when the host
 * and guest OS types differs and it won't happen naturally.
 * @since VBox 6.0.10  */
#define SHFL_FN_SET_ERROR_STYLE     (29)
/** The last function number. */
#define SHFL_FN_LAST                SHFL_FN_SET_ERROR_STYLE
/** @} */


/** @name Shared Folders service functions. (host)
 * @{
 */
/** Add shared folder mapping. */
#define SHFL_FN_ADD_MAPPING         (1)
/** Remove shared folder mapping. */
#define SHFL_FN_REMOVE_MAPPING      (2)
/** Set the led status light address. */
#define SHFL_FN_SET_STATUS_LED      (3)
/** Allow the guest to create symbolic links
 * @since VBox 4.0  */
#define SHFL_FN_ALLOW_SYMLINKS_CREATE (4)
/** @} */

/** Root handle for a mapping. Root handles are unique.
 *
 * @note Function parameters structures consider the root handle as 32 bit
 *       value. If the typedef will be changed, then function parameters must be
 *       changed accordingly. All those parameters are marked with SHFLROOT in
 *       comments.
 */
typedef uint32_t SHFLROOT;

/** NIL shared folder root handle. */
#define SHFL_ROOT_NIL ((SHFLROOT)~0)


/** A shared folders handle for an opened object. */
typedef uint64_t SHFLHANDLE;

#define SHFL_HANDLE_NIL  ((SHFLHANDLE)~0LL)
#define SHFL_HANDLE_ROOT ((SHFLHANDLE)0LL)

/** Hardcoded maximum length (in chars) of a shared folder name. */
#define SHFL_MAX_LEN         (256)
/** Hardcoded maximum number of shared folder mapping available to the guest. */
#define SHFL_MAX_MAPPINGS    (64)

/**
 * Shared folder string buffer structure.
 */
typedef struct _SHFLSTRING
{
	/** Allocated size of the String member in bytes. */
	uint16_t u16Size;

	/** Length of string without trailing nul in bytes. */
	uint16_t u16Length;

	/** UTF-8 or UTF-16 string. Nul terminated. */
	char ach[];
} SHFLSTRING;
AssertCompileSize(SHFLSTRING, 4);

/**
 * The available additional information in a SHFLFSOBJATTR object.
 */
typedef enum SHFLFSOBJATTRADD
{
	/** No additional information is available / requested. */
	SHFLFSOBJATTRADD_NOTHING = 1,
	/** The additional unix attributes (SHFLFSOBJATTR::u::Unix) are
	 *  available / requested. */
	SHFLFSOBJATTRADD_UNIX,
	/** The additional extended attribute size (SHFLFSOBJATTR::u::EASize) is
	 *  available / requested. */
	SHFLFSOBJATTRADD_EASIZE,
	/** The last valid item (inclusive).
	 * The valid range is SHFLFSOBJATTRADD_NOTHING thru
	 * SHFLFSOBJATTRADD_LAST. */
	SHFLFSOBJATTRADD_LAST = SHFLFSOBJATTRADD_EASIZE,

	/** The usual 32-bit hack. */
	SHFLFSOBJATTRADD_32BIT_SIZE_HACK = 0x7fffffff
} SHFLFSOBJATTRADD;


/* Assert sizes of the IRPT types we're using below. */
//AssertCompileSize(RTFMODE,      4);
//AssertCompileSize(RTFOFF,       8);
//AssertCompileSize(RTINODE,      8);
//AssertCompileSize(RTTIMESPEC,   8);
//AssertCompileSize(RTDEV,        4);
//AssertCompileSize(RTUID,        4);

/**
 * Shared folder filesystem object attributes.
 */
#pragma pack(push, 1)
typedef struct SHFLFSOBJATTR
{
	/** Mode flags (st_mode). RTFS_UNIX_*, RTFS_TYPE_*, and RTFS_DOS_*.
	 * @remarks We depend on a number of RTFS_ defines to remain unchanged.
	 *          Fortuntately, these are depending on windows, dos and unix
	 *          standard values, so this shouldn't be much of a pain. */
	uint32_t          fMode;

	/** The additional attributes available. */
	SHFLFSOBJATTRADD  enmAdditional;

	/**
	 * Additional attributes.
	 *
	 * Unless explicitly specified to an API, the API can provide additional
	 * data as it is provided by the underlying OS.
	 */
	union SHFLFSOBJATTRUNION
	{
		/** Additional Unix Attributes
		 * These are available when SHFLFSOBJATTRADD is set in fUnix.
		 */
		 struct SHFLFSOBJATTRUNIX
		 {
			/** The user owning the filesystem object (st_uid).
			 * This field is ~0U if not supported. */
			uint32_t        uid;

			/** The group the filesystem object is assigned (st_gid).
			 * This field is ~0U if not supported. */
			uint32_t        gid;

			/** Number of hard links to this filesystem object (st_nlink).
			 * This field is 1 if the filesystem doesn't support hardlinking or
			 * the information isn't available.
			 */
			uint32_t        cHardlinks;

			/** The device number of the device which this filesystem object resides on (st_dev).
			 * This field is 0 if this information is not available. */
			uint32_t        INodeIdDevice;

			/** The unique identifier (within the filesystem) of this filesystem object (st_ino).
			 * Together with INodeIdDevice, this field can be used as a OS wide unique id
			 * when both their values are not 0.
			 * This field is 0 if the information is not available. */
			uint64_t       INodeId;

			/** User flags (st_flags).
			 * This field is 0 if this information is not available. */
			uint32_t        fFlags;

			/** The current generation number (st_gen).
			 * This field is 0 if this information is not available. */
			uint32_t        GenerationId;

			/** The device number of a character or block device type object (st_rdev).
			 * This field is 0 if the file isn't of a character or block device type and
			 * when the OS doesn't subscribe to the major+minor device idenfication scheme. */
			uint32_t        Device;
		} Unix;

		/**
		 * Extended attribute size.
		 */
		struct SHFLFSOBJATTREASIZE
		{
			/** Size of EAs. */
			uint64_t        cb;
		} EASize;
	} u;
} SHFLFSOBJATTR;
#pragma pack (pop)
AssertCompileSize(SHFLFSOBJATTR, 44);
/** Pointer to a shared folder filesystem object attributes structure. */
typedef SHFLFSOBJATTR *PSHFLFSOBJATTR;
/** Pointer to a const shared folder filesystem object attributes structure. */
typedef const SHFLFSOBJATTR *PCSHFLFSOBJATTR;


/**
 * Filesystem object information structure.
 */
#pragma pack(push, 1)
typedef struct SHFLFSOBJINFO
{
   /** Logical size (st_size).
	* For normal files this is the size of the file.
	* For symbolic links, this is the length of the path name contained
	* in the symbolic link.
	* For other objects this fields needs to be specified.
	*/
   uint64_t     cbObject;

   /** Disk allocation size (st_blocks * DEV_BSIZE). */
   uint64_t     cbAllocated;

   /** Time of last access (st_atime).
	* @remarks  Here (and other places) we depend on the IPRT timespec to
	*           remain unchanged. */
   int64_t     AccessTime;

   /** Time of last data modification (st_mtime). */
   int64_t     ModificationTime;

   /** Time of last status change (st_ctime).
	* If not available this is set to ModificationTime.
	*/
   int64_t     ChangeTime;

   /** Time of file birth (st_birthtime).
	* If not available this is set to ChangeTime.
	*/
   int64_t     BirthTime;

   /** Attributes. */
   SHFLFSOBJATTR Attr;

} SHFLFSOBJINFO;
#pragma pack (pop)
AssertCompileSize(SHFLFSOBJINFO, 92);
/** Pointer to a shared folder filesystem object information structure. */
typedef SHFLFSOBJINFO *PSHFLFSOBJINFO;
/** Pointer to a const shared folder filesystem object information
 *  structure. */
typedef const SHFLFSOBJINFO *PCSHFLFSOBJINFO;


/** Result of an open/create request.
 *  Along with handle value the result code
 *  identifies what has happened while
 *  trying to open the object.
 */
typedef enum _SHFLCREATERESULT
{
	SHFL_NO_RESULT,
	/** Specified path does not exist. */
	SHFL_PATH_NOT_FOUND,
	/** Path to file exists, but the last component does not. */
	SHFL_FILE_NOT_FOUND,
	/** File already exists and either has been opened or not. */
	SHFL_FILE_EXISTS,
	/** New file was created. */
	SHFL_FILE_CREATED,
	/** Existing file was replaced or overwritten. */
	SHFL_FILE_REPLACED,
	/** Blow the type up to 32-bit. */
	SHFL_32BIT_HACK = 0x7fffffff
} SHFLCREATERESULT;
AssertCompile(SHFL_NO_RESULT == 0);
AssertCompileSize(SHFLCREATERESULT, 4);


/** @name Open/create flags.
 *  @{
 */

/** No flags. Initialization value. */
#define SHFL_CF_NONE                  (0x00000000)

/** Lookup only the object, do not return a handle. All other flags are ignored. */
#define SHFL_CF_LOOKUP                (0x00000001)

/** Open parent directory of specified object.
 *  Useful for the corresponding Windows FSD flag
 *  and for opening paths like \\dir\\*.* to search the 'dir'.
 *  @todo possibly not needed???
 */
#define SHFL_CF_OPEN_TARGET_DIRECTORY (0x00000002)

/** Create/open a directory. */
#define SHFL_CF_DIRECTORY             (0x00000004)

/** Open/create action to do if object exists
 *  and if the object does not exists.
 *  REPLACE file means atomically DELETE and CREATE.
 *  OVERWRITE file means truncating the file to 0 and
 *  setting new size.
 *  When opening an existing directory REPLACE and OVERWRITE
 *  actions are considered invalid, and cause returning
 *  FILE_EXISTS with NIL handle.
 */
#define SHFL_CF_ACT_MASK_IF_EXISTS      (0x000000F0)
#define SHFL_CF_ACT_MASK_IF_NEW         (0x00000F00)

/** What to do if object exists. */
#define SHFL_CF_ACT_OPEN_IF_EXISTS      (0x00000000)
#define SHFL_CF_ACT_FAIL_IF_EXISTS      (0x00000010)
#define SHFL_CF_ACT_REPLACE_IF_EXISTS   (0x00000020)
#define SHFL_CF_ACT_OVERWRITE_IF_EXISTS (0x00000030)

/** What to do if object does not exist. */
#define SHFL_CF_ACT_CREATE_IF_NEW       (0x00000000)
#define SHFL_CF_ACT_FAIL_IF_NEW         (0x00000100)

/** Read/write requested access for the object. */
#define SHFL_CF_ACCESS_MASK_RW          (0x00003000)

/** No access requested. */
#define SHFL_CF_ACCESS_NONE             (0x00000000)
/** Read access requested. */
#define SHFL_CF_ACCESS_READ             (0x00001000)
/** Write access requested. */
#define SHFL_CF_ACCESS_WRITE            (0x00002000)
/** Read/Write access requested. */
#define SHFL_CF_ACCESS_READWRITE        (SHFL_CF_ACCESS_READ | SHFL_CF_ACCESS_WRITE)

/** Requested share access for the object. */
#define SHFL_CF_ACCESS_MASK_DENY        (0x0000C000)

/** Allow any access. */
#define SHFL_CF_ACCESS_DENYNONE         (0x00000000)
/** Do not allow read. */
#define SHFL_CF_ACCESS_DENYREAD         (0x00004000)
/** Do not allow write. */
#define SHFL_CF_ACCESS_DENYWRITE        (0x00008000)
/** Do not allow access. */
#define SHFL_CF_ACCESS_DENYALL          (SHFL_CF_ACCESS_DENYREAD | SHFL_CF_ACCESS_DENYWRITE)

/** Requested access to attributes of the object. */
#define SHFL_CF_ACCESS_MASK_ATTR        (0x00030000)

/** No access requested. */
#define SHFL_CF_ACCESS_ATTR_NONE        (0x00000000)
/** Read access requested. */
#define SHFL_CF_ACCESS_ATTR_READ        (0x00010000)
/** Write access requested. */
#define SHFL_CF_ACCESS_ATTR_WRITE       (0x00020000)
/** Read/Write access requested. */
#define SHFL_CF_ACCESS_ATTR_READWRITE   (SHFL_CF_ACCESS_ATTR_READ | SHFL_CF_ACCESS_ATTR_WRITE)

/** The file is opened in append mode. Ignored if SHFL_CF_ACCESS_WRITE is not set. */
#define SHFL_CF_ACCESS_APPEND           (0x00040000)

/** @} */

#pragma pack(push,1)
typedef struct _SHFLCREATEPARMS
{
	/* Returned handle of opened object. */
	SHFLHANDLE Handle;

	/* Returned result of the operation */
	SHFLCREATERESULT Result;

	/* SHFL_CF_* */
	uint32_t CreateFlags;

	/* Attributes of object to create and
	 * returned actual attributes of opened/created object.
	 */
	SHFLFSOBJINFO Info;

} SHFLCREATEPARMS;
#pragma pack(pop)

/** Lock mode bit mask. */
#define SHFL_LOCK_MODE_MASK  (0x3)
/** Cancel lock on the given range. */
#define SHFL_LOCK_CANCEL     (0x0)
/** Acquire read only lock. Prevent write to the range. */
#define SHFL_LOCK_SHARED     (0x1)
/** Acquire write lock. Prevent both write and read to the range. */
#define SHFL_LOCK_EXCLUSIVE  (0x2)

/** Do not wait for lock if it can not be acquired at the time. */
#define SHFL_LOCK_NOWAIT     (0x0)
/** Wait and acquire lock. */
#define SHFL_LOCK_WAIT       (0x4)

/** Lock the specified range. */
#define SHFL_LOCK_PARTIAL    (0x0)
/** Lock entire object. */
#define SHFL_LOCK_ENTIRE     (0x8)

#define SHFL_RENAME_FILE                (0x1)
#define SHFL_RENAME_DIR                 (0x2)
#define SHFL_RENAME_REPLACE_IF_EXISTS   (0x4)

/** @name Shared Folders mappings.
 * @{
 */

/** The mapping has been added since last query. */
#define SHFL_MS_NEW        (1)
/** The mapping has been deleted since last query. */
#define SHFL_MS_DELETED    (2)

/** Validation mask.  Needs to be adjusted
  * whenever a new SHFL_MF_ flag is added. */
#define SHFL_MF_MASK       (0x00000011)
/** UTF-16 enconded strings. */
#define SHFL_MF_UCS2       (0x00000000)
/** Guest uses UTF8 strings, if not set then the strings are unicode (UCS2). */
#define SHFL_MF_UTF8       (0x00000001)
/** Just handle the auto-mounted folders. */
#define SHFL_MF_AUTOMOUNT  (0x00000010)

typedef struct _SHFLMAPPING
{
	/** Mapping status.
	 * @note Currently always set to SHFL_MS_NEW.  */
	uint32_t u32Status;
	/** Root handle. */
	SHFLROOT root;
} SHFLMAPPING;
AssertCompileSize(SHFLMAPPING, 8);
/** Pointer to a SHFLMAPPING structure. */
typedef SHFLMAPPING *PSHFLMAPPING;

/** @} */

/** @name Shared Folder directory information
 * @{
 */

typedef struct _SHFLDIRINFO
{
	/** Full information about the object. */
	SHFLFSOBJINFO   Info;
	/** The length of the short field (number of RTUTF16 chars).
	 * It is 16-bit for reasons of alignment. */
	uint16_t        cucShortName;
	/** The short name for 8.3 compatibility.
	 * Empty string if not available.
	 */
	uint16_t        uszShortName[14];

	SHFLSTRING      name;
} SHFLDIRINFO, *PSHFLDIRINFO;

/**
 * Shared folder filesystem properties.
 */
typedef struct SHFLFSPROPERTIES
{
	/** The maximum size of a filesystem object name.
	 * This does not include the '\\0'. */
	uint32_t cbMaxComponent;

	/** True if the filesystem is remote.
	 * False if the filesystem is local. */
	bool    fRemote;

	/** True if the filesystem is case sensitive.
	 * False if the filesystem is case insensitive. */
	bool    fCaseSensitive;

	/** True if the filesystem is mounted read only.
	 * False if the filesystem is mounted read write. */
	bool    fReadOnly;

	/** True if the filesystem can encode unicode object names.
	 * False if it can't. */
	bool    fSupportsUnicode;

	/** True if the filesystem is compresses.
	 * False if it isn't or we don't know. */
	bool    fCompressed;

	/** True if the filesystem compresses of individual files.
	 * False if it doesn't or we don't know. */
	bool    fFileCompression;

	/** @todo more? */
} SHFLFSPROPERTIES;
AssertCompileSize(SHFLFSPROPERTIES, 12);
/** Pointer to a shared folder filesystem properties structure. */
typedef SHFLFSPROPERTIES *PSHFLFSPROPERTIES;
/** Pointer to a const shared folder filesystem properties structure. */
typedef SHFLFSPROPERTIES const *PCSHFLFSPROPERTIES;

typedef struct _SHFLVOLINFO
{
	uint64_t       ullTotalAllocationBytes;
	uint64_t       ullAvailableAllocationBytes;
	uint32_t       ulBytesPerAllocationUnit;
	uint32_t       ulBytesPerSector;
	uint32_t       ulSerial;
	SHFLFSPROPERTIES fsProperties;
} SHFLVOLINFO, *PSHFLVOLINFO;

#define SHFL_LIST_NONE          0
#define SHFL_LIST_RETURN_ONE    1
#define SHFL_LIST_RESTART       2

/** Mask of Set/Get bit. */
#define SHFL_INFO_MODE_MASK    (0x1)
/** Get information */
#define SHFL_INFO_GET          (0x0)
/** Set information */
#define SHFL_INFO_SET          (0x1)

/** Get name of the object. */
#define SHFL_INFO_NAME         (0x2)
/** Set size of object (extend/trucate); only applies to file objects */
#define SHFL_INFO_SIZE         (0x4)
/** Get/Set file object info. */
#define SHFL_INFO_FILE         (0x8)
/** Get volume information. */
#define SHFL_INFO_VOLUME       (0x10)

#define SHFL_REMOVE_FILE        (0x1)
#define SHFL_REMOVE_DIR         (0x2)
#define SHFL_REMOVE_SYMLINK     (0x4)

/** Query flag: Guest prefers drive letters as mount points. */
#define SHFL_MIQF_DRIVE_LETTER      RT_BIT(0)
/** Query flag: Guest prefers paths as mount points. */
#define SHFL_MIQF_PATH              RT_BIT(1)

/** Set if writable. */
#define SHFL_MIF_WRITABLE           RT_BIT(0)
/** Indicates that the mapping should be auto-mounted. */
#define SHFL_MIF_AUTO_MOUNT         RT_BIT(1)
/** Set if host is case insensitive. */
#define SHFL_MIF_HOST_ICASE         RT_BIT(2)
/** Set if guest is case insensitive. */
#define SHFL_MIF_GUEST_ICASE        RT_BIT(3)
/** Symbolic link creation is allowed. */
#define SHFL_MIF_SYMLINK_CREATION   RT_BIT(4)

#pragma pack (pop)

#endif
