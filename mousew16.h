#ifndef MOUSEW16_H
#define MOUSEW16_H

/* Win16's mouse driver interface. */

/** Contains information about the mouse, used by Inquire(). */
typedef _Packed struct MOUSEINFO
{
	/** Whether a mouse exists. */
	char    msExist;
	/** Whether the mouse returns absolute or relative coordinates. */
	char    msRelative;
	/** Number of buttons. */
	short   msNumButtons;
	/** Maximum number of events per second. */
	short   msRate;
	// Reserved:
	short   msXThreshold;
	short   msYThreshold;
	short   msXRes;
	short   msYRes;
	// The following are available in Windows >= 3.1 only:
	#if 0
	/** Specifies the COM port used, or 0 for none. */
	short   msMouseCommPort;
	#endif
} MOUSEINFO;
typedef MOUSEINFO __far *LPMOUSEINFO;

/** Movement occurred. */
#define SF_MOVEMENT 0x0001
/** Button 1 changed to down. */
#define SF_B1_DOWN  0x0002
/** Button 1 changed to up. */
#define SF_B1_UP    0x0004
/** Button 2 changed to down. */
#define SF_B2_DOWN  0x0008
/** Button 2 changed to up. */
#define SF_B2_UP    0x0010
/** Event coordinates are absolute instead of relative. */
#define SF_ABSOLUTE 0x8000

/** Driver should call this callback when there are new mouse events to report.
 *  @param Status What happened. Combination of SF_MOVEMENT, SF_ABSOLUTE, etc.
 *  @param deltaX either number of mickeys moved or absolute coordinate if SB_ABSOLUTE.
 *  @param deltaY either number of mickeys moved or absolute coordinate if SB_ABSOLUTE.
 *  @param ButtonCount number of buttons
 *  @param extra1,extra2 leave as zero
 */
typedef void (__far *LPFN_MOUSEEVENT)(unsigned short Status, short deltaX, short deltaY, short ButtonCount, short extra1, short extra2);
#pragma aux MOUSEEVENTPROC parm [ax] [bx] [cx] [dx] [di] [si]

#endif
