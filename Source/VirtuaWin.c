//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  VirtuaWin.c - Core VirtuaWin routines.
// 
//  Copyright (c) 1999-2005 Johan Piculell
//  Copyright (c) 2006-2012 VirtuaWin (VirtuaWin@home.se)
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
//  USA.
//


// Includes
#define _CRT_SECURE_NO_WARNINGS
#define vwLOG_VERBOSE
#include "VirtuaWin.h"
#include "DiskRoutines.h"
#include "ConfigParameters.h"
#include "Messages.h"
#include "Resource.h"

/* Get the list of hotkey commands */
#define VW_COMMAND(a, b, c, d) a = b ,
enum {
#include "vwCommands.def"
} ;
#undef  VW_COMMAND

// Standard includes
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <commctrl.h>
#include <signal.h>

/*#define _WIN32_MEMORY_DEBUG*/
#ifdef _WIN32_MEMORY_DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/* define some constants that are often missing in early compilers */
#ifndef ICON_SMALL2
#define ICON_SMALL2 2
#endif
#ifndef WS_EX_NOACTIVATE
#define WS_EX_NOACTIVATE 0x8000000
#endif
/* The virtual screen size system matrix values were only added for WINVER >= 0x0500 (Win2k) */
#ifndef SM_CMONITORS
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80
#endif
#ifndef SPI_GETFOREGROUNDLOCKTIMEOUT
#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
#endif

#define vwCalculateDesk(x,y) (((y) * nDesksX) - (nDesksX - (x)))

#define vwIsEnabled()        (vwDisabled == 0)
#define vwIsDisabled()       (vwDisabled != 0)
#define vwIsBossLocked()     ((vwDisabled & 0x02) != 0)

#define vwWindowBaseGetNext(win)      ((win)->next)
#define vwWindowGetNext(win)          ((vwWindow *) (win)->next)
#define vwWindowIsWindow(win)         (((win)->flags & vwWINFLAGS_WINDOW) != 0)
#define vwWindowIsVisible(win)        (((win)->flags & vwWINFLAGS_VISIBLE) != 0)
#define vwWindowIsMinimized(win)      (((win)->flags & vwWINFLAGS_MINIMIZED) != 0)
#define vwWindowIsMaximized(win)      (((win)->flags & vwWINFLAGS_MAXIMIZED) != 0)
#define vwWindowIsManaged(win)        (((win)->flags & vwWINFLAGS_MANAGED) != 0)
#define vwWindowIsShown(win)          (((win)->flags & vwWINFLAGS_SHOWN) != 0)
#define vwWindowIsShownNotHung(win)   (((win)->flags & vwWINFLAGS_SHOWN) && ((win)->flags & vwWINFLAGS_SHOW))
#define vwWindowIsShow(win)           (((win)->flags & vwWINFLAGS_SHOW) != 0)
#define vwWindowIsSticky(win)         (((win)->flags & vwWINFLAGS_STICKY) != 0)
#define vwWindowIsHideByHide(win)     (((win)->flags & vwWINFLAGS_HIDEWIN_MASK) == vwWINFLAGS_HIDEWIN_HIDE)
#define vwWindowIsHideByMove(win)     (((win)->flags & vwWINFLAGS_HIDEWIN_MASK) == vwWINFLAGS_HIDEWIN_MOVE)
#define vwWindowIsHideByMinim(win)    (((win)->flags & vwWINFLAGS_HIDEWIN_MASK) == vwWINFLAGS_HIDEWIN_MINIM)

#define vwWindowIsNotWindow(win)      (((win)->flags & vwWINFLAGS_WINDOW) == 0)
#define vwWindowIsNotVisible(win)     (((win)->flags & vwWINFLAGS_VISIBLE) == 0)
#define vwWindowIsNotMinimized(win)   (((win)->flags & vwWINFLAGS_MINIMIZED) == 0)
#define vwWindowIsNotMaximized(win)   (((win)->flags & vwWINFLAGS_MAXIMIZED) == 0)
#define vwWindowIsNotManaged(win)     (((win)->flags & vwWINFLAGS_MANAGED) == 0)
#define vwWindowIsNotShown(win)       (((win)->flags & vwWINFLAGS_SHOWN) == 0)
#define vwWindowIsNotShow(win)        (((win)->flags & vwWINFLAGS_SHOW) == 0)
#define vwWindowIsNotSticky(win)      (((win)->flags & vwWINFLAGS_STICKY) == 0)
#define vwWindowIsNotHideByHide(win)  (((win)->flags & vwWINFLAGS_HIDEWIN_MASK) != vwWINFLAGS_HIDEWIN_HIDE)
#define vwWindowIsNotHideByMove(win)  (((win)->flags & vwWINFLAGS_HIDEWIN_MASK) != vwWINFLAGS_HIDEWIN_MOVE)
#define vwWindowIsNotHideByMinim(win) (((win)->flags & vwWINFLAGS_HIDEWIN_MASK) != vwWINFLAGS_HIDEWIN_MINIM)

#define vwWindowDontHideTaskButton(win)   (((win)->flags & vwWINFLAGS_HIDETSK_MASK) == vwWINFLAGS_HIDETSK_DONT)

#define windowIsHung(inhHWnd,waitTime)    (SendMessageTimeout(inhHWnd,WM_NULL,0x51842145,0x5e7bdeba,SMTO_ABORTIFHUNG|SMTO_BLOCK,waitTime,NULL) == 0)
#define windowIsNotHung(inhHWnd,waitTime) (SendMessageTimeout(inhHWnd,WM_NULL,0x51842145,0x5e7bdeba,SMTO_ABORTIFHUNG|SMTO_BLOCK,waitTime,NULL))

// Variables
HWND hWnd;                                   // handle to VirtuaWin
DWORD vwThread;                              // ID of main VW thread
HANDLE hMutex;
FILE *vwLogFile ;
vwUByte vwDisabled=0;	                     // if VirtuaWin enabled or not, see vwIsEnabled macro above

int desktopWorkArea[2][4] ;

vwHotkey      hotkeyList[vwHOTKEY_MAX];      // list for holding hotkeys
vwWindowBase *windowHash[vwWINHASH_SIZE];    // hash table for all windows
vwWindowBase *windowBaseList;                // list of all windows
vwWindow     *windowList;                    // list of managed windows
vwWindow     *windowFreeList;                // list of free, ready for reuse
vwWindowBase *windowBaseFreeList;            // list of free, ready for reuse
vwWindowRule *windowRuleList;                // list for holding window rules
vwMenuItem   *ctlMenuItemList;               // List of module inserted control menu items
vwModule      moduleList[MAXMODULES];        // list that holds modules
vwDisModule   disabledModules[MAXMODULES*2]; // list with disabled modules

UINT RM_Shellhook;
UINT RM_TaskbarCreated;         // Message used to broadcast taskbar restart 

HINSTANCE hInst;		// current instance
HWND taskHWnd;                  // handle to taskbar
HWND desktopHWnd;		// handle to the desktop window
DWORD desktopThread;            // thread ID of desktop window
HWND deskIconHWnd;		// handle to the desktop window holding the icons
HWND lastFGHWnd;		// handle to the last foreground window
HWND dialogHWnd;       		// handle to the setup dialog, NULL if not open
int dialogPos[2];
vwUByte dialogOpen;         
vwUByte initialized;

HWND ichangeHWnd = NULL;        // handle to module handling desktop changes
static int ichangeDesk=0 ;
static WPARAM ichangeWParam ;

// vector holding icon handles for the systray
HICON checkIcon;                // Sticky tick icon in window list
HICON icons[vwDESKTOP_SIZE];    // 0=disabled, 1,2..=normal desks
NOTIFYICONDATA nIconD;
TCHAR *desktopName[vwDESKTOP_SIZE];
unsigned char desktopUsed[vwDESKTOP_SIZE];

#define MENU_X_PADDING 1
#define MENU_Y_PADDING 1
#define ICON_PADDING 2
#define ICON_SIZE 16

// Config parameters, see ConfigParameters.h for descriptions
int hotkeyCount = 0;
int hotkeyRegistered = 0;
int moduleCount = 0;  
int currentDeskX = 1;
int currentDeskY = 1;
int currentDesk = 1; 
int nDesks = 4;     
int nDesksX = 2;     
int nDesksY = 2;     
int lastDesk = 1; 
vwUByte lastDeskNoDelay = 0 ;          
vwUByte mouseKnock = 2 ;
vwUByte hiddenWindowAct = 2 ;
vwUByte taskButtonAct = 0 ;		
vwUByte vwLogFlag = 0 ;
vwUByte releaseFocus = 0 ;	
vwUByte refreshOnWarp = 0 ;     
vwUByte initialDesktop = 0 ;          
vwUByte deskWrap = 0 ;          
vwUByte invertY = 0 ;           
vwUByte hotkeyMenuLoc = 0 ;
vwUByte aggressiveRules = 0 ;
vwUByte winListContent = (vwWINLIST_ACCESS | vwWINLIST_ASSIGN | vwWINLIST_STICKY) ;
vwUByte winListCompact = 0 ;
vwUByte winMenuCompact = 1 ;
vwUByte ctlMenuCompact = 1 ;
vwUByte displayTaskbarIcon = 1 ;
vwUByte taskbarIconShown = 0 ;
vwUByte noTaskbarCheck = 0 ;
vwUByte useWindowRules = 1 ;
vwUByte useDynButtonRm = 0 ;
vwUByte useDskChgModRelease = 0 ;
vwUByte taskbarFixRequired = 0 ;
vwUByte preserveZOrder = 2 ;      
vwUByte minWinHide = 1 ;
vwUByte taskbarBCType;          // taskbar button container type - one of vwTASKBAR_BC_*
HWND    taskbarBCHWnd;
HANDLE  taskbarProcHdl;
LPVOID  taskbarShrdMem;
HWND   *taskbarButtonList = NULL ;
int     taskbarButtonListSize = 0 ;     

#define vwTASKBAR_BC_NONE       0    // None - no dynamic update
#define vwTASKBAR_BC_TABCONTROL 1    // Win 2000
#define vwTASKBAR_BC_TOOLBAR    2    // Win XP & Vista
#define vwTASKBAR_BC_WIN7       3    // Win7
// Undocumented flag for 9x/ME
#define VA_SHARED 0x8000000
// Undocumented offsets for Win7
#define DPA_DELTA_64 0xE0
#define DPA_DELTA_32 0x90

HANDLE mouseThread;                          // Handle to the mouse thread
vwUByte mouseEnabled = 1 ;                   // Status of the mouse thread, always running at startup 
vwUByte mouseEnable = 6 ;                    // Required mouse support
vwUByte isDragging;	                     // if we are currently dragging a window
HWND    dragHWnd;                            // handle to window being dragged
vwUByte mouseWarp = 0 ;
vwUByte mouseModifierUsed = 0 ;
vwUByte mouseModifier ;
int mouseDelay = 20;
int mouseJumpLength = 60;

int curDisabledMod = 0; 
vwUInt vwZOrder=1 ;
vwUInt timerCounter=0 ;
vwUInt updateCounter=0;

/* desk image generation variables */
int         deskImageCount=-1 ;
int         deskImageEnabled=0 ;
HBITMAP     deskImageBitmap=NULL ;
BITMAPINFO  deskImageInfo ;
void       *deskImageData=NULL ;

enum {
	OSVERSION_UNKNOWN=0,
	OSVERSION_64BIT=1,
	OSVERSION_31=2,
	OSVERSION_9X=4,
	OSVERSION_NT=6,
	OSVERSION_2000=8,
	OSVERSION_XP=10
} ;
int osVersion ;

typedef DWORD (WINAPI *vwGETMODULEFILENAMEEX)(HANDLE,HMODULE,LPTSTR,DWORD) ;
vwGETMODULEFILENAMEEX vwGetModuleFileNameEx ;

vwUByte vwHookUse ;
vwUByte vwHookInstalled ;
typedef void (*vwHOOKSETUP)(HWND,int) ;
vwHOOKSETUP vwHookSetupFunc ;
typedef int (*vwHOOKINSTALL)(void) ;
vwHOOKINSTALL vwHookInstallFunc ;
typedef void (*vwHOOKUNINSTALL)(void) ;
vwHOOKUNINSTALL vwHookUninstallFunc ;

#define vwWINSH_FLAGS_TRYHARD   0x01
#define vwWINSH_FLAGS_HIDE      0x00
#define vwWINSH_FLAGS_SHOW      vwWINFLAGS_SHOW
static int vwWindowShowHide(vwWindow *aWindow, vwUInt flags);
static int changeDesk(int newDesk, WPARAM msgWParam) ;

void
vwLogPrint(const TCHAR *format, ...)
{
	if(vwLogEnabled())
	{
		SYSTEMTIME stime;
		va_list ap;
	
		GetLocalTime (&stime);
		_ftprintf(vwLogFile,_T("[%04d-%02d-%02d %02d:%02d:%02d:%03d] "),
				  stime.wYear, stime.wMonth, stime.wDay,
				  stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds) ;
		va_start(ap, format);
		_vftprintf(vwLogFile,format,ap) ;
		fflush(vwLogFile) ;
		va_end(ap);
	}
}

/************************************************
 * Locks the window list protection
 */
void
vwMutexLock(void)
{
	if(WaitForSingleObject(hMutex,0) == WAIT_TIMEOUT)
		WaitForSingleObject(hMutex,INFINITE);
}

/************************************************
 * Releases the window list protection
 */
void
vwMutexRelease(void)
{
	ReleaseMutex(hMutex);
}


/************************************************
 * Find a window give the class name and optionally the window text.
 * Same as Window's FindWindow except more reliable and gives better error reporting.
 */
typedef struct {
	TCHAR buff[128] ;
	TCHAR *className ;
	TCHAR *windowText ;
	HWND winHWnd ;
	int printOut ;
} vwFNDWIN ;

static int CALLBACK
vwFindWindowEProc(HWND hwnd, LPARAM lParam) 
{
	vwFNDWIN *mfwd ;
	mfwd = (vwFNDWIN *) lParam ;
	mfwd->buff[0] = '\0' ;
	GetClassName(hwnd,mfwd->buff,128);
	if(mfwd->printOut)
		vwLogBasic((_T("Find CN: [%s] Win: %x [%s]\n"),mfwd->className,(int) hwnd,mfwd->buff)) ;
	if(_tcsicmp(mfwd->className,mfwd->buff))
		return TRUE ;
	
	if(mfwd->windowText != NULL)
	{
		if(!GetWindowText(hwnd,mfwd->buff,128))
			mfwd->buff[0] = '\0' ;
		if(mfwd->printOut)
			vwLogBasic((_T("Find WT: [%s] Win: %x [%s]\n"),mfwd->windowText,(int) hwnd,mfwd->buff)) ;
		if(_tcsicmp(mfwd->windowText,mfwd->buff))
			return TRUE ;
	}
	/* found the module window - quit */
	if(mfwd->printOut)
		vwLogBasic((_T("Found window: %x [%s]\n"),(int) hwnd,mfwd->className)) ;
	mfwd->winHWnd = hwnd ;
	return FALSE ;
}

HWND
vwFindWindow(TCHAR *className, TCHAR *windowText, int printOut)
{
	vwFNDWIN mfwd ;
	int rv1 ;
	
	mfwd.winHWnd = NULL ;
	mfwd.printOut = printOut ;
	mfwd.className = className ;
	mfwd.windowText = windowText ;
	rv1 = EnumWindows(vwFindWindowEProc,(LPARAM) &mfwd) ;
	
	if(mfwd.winHWnd != NULL)
		return mfwd.winHWnd ;
	if(printOut)
		vwLogBasic((_T("Failed to find window: %d %d [%s]\n"),rv1,GetLastError(),className)) ;
	return NULL ;
}

/************************************************
 * Returns a bit mask of currently pressed modifier keys
 */
static int
vwKeyboardTestModifier(vwUByte modif)
{
	if((modif & vwHOTKEY_ALT) && !HIWORD(GetAsyncKeyState(VK_MENU)))
		return FALSE ;
	if((modif & vwHOTKEY_CONTROL) && !HIWORD(GetAsyncKeyState(VK_CONTROL)))
		return FALSE ;
	if((modif & vwHOTKEY_SHIFT) && !HIWORD(GetAsyncKeyState(VK_SHIFT)))
		return FALSE ;
	if((modif & vwHOTKEY_WIN) && !HIWORD(GetAsyncKeyState(VK_LWIN)) && !HIWORD(GetAsyncKeyState(VK_RWIN)))
		return FALSE ;
	return TRUE ;
}

/* check the current position of the mouse, returns: 1 if on caption of a window, 2 if on desktop, 3 if on taskbar, 0 otherwise */
static unsigned char
checkMousePosition(void)
{
	LPARAM lParam ;
	HWND hwnd ;
	POINT pt ;
	DWORD rr ;
	
	GetCursorPos(&pt);
	if((hwnd=WindowFromPoint(pt)) == NULL)
		return 0 ;
	
	if((hwnd == deskIconHWnd) || (hwnd == desktopHWnd))
		return 2 ;
	if((hwnd == taskHWnd) || (hwnd == taskbarBCHWnd))
		return 3 ;
	
	lParam = (((int)(short) pt.y) << 16) | (0x0ffff & ((int)(short) pt.x)) ;
	if((SendMessageTimeout(hwnd,WM_NCHITTEST,0,lParam,SMTO_ABORTIFHUNG|SMTO_BLOCK,50,&rr) ||
		(Sleep(1),SendMessageTimeout(hwnd,WM_NCHITTEST,0,lParam,SMTO_ABORTIFHUNG|SMTO_BLOCK,100,&rr))) &&
	   (rr == HTCAPTION))
		return 1 ;
	
	return 0 ;
}
/*************************************************
 * Checks if mouse button pressed on title bar (i.e. dragging window)
 * Returns 0 = no button down, 1 = left but down on window caption, 2 = Middle down on window caption
 * 3 = middle down on desktop, 4 = middle down on taskbar, 5 = button(s) down and not one of the others 
 */
static unsigned char
checkMouseState(int force)
{
	static unsigned char lastState=0, lastBState=0 ;
	unsigned char thisBState, mp ;
	
	// Check the state of mouse buttons
	if(GetSystemMetrics(SM_SWAPBUTTON))
	{
		thisBState = (HIWORD(GetAsyncKeyState(VK_RBUTTON)) != 0) ;
		if(HIWORD(GetAsyncKeyState(VK_MBUTTON)))
			thisBState |= 2 ;
		if(HIWORD(GetAsyncKeyState(VK_LBUTTON)))
			thisBState |= 4 ;
	}
	else
	{
		thisBState = (HIWORD(GetAsyncKeyState(VK_LBUTTON)) != 0) ;
		if(HIWORD(GetAsyncKeyState(VK_MBUTTON)))
			thisBState |= 2 ;
		if(HIWORD(GetAsyncKeyState(VK_RBUTTON)))
			thisBState |= 4 ;
	}
	if((thisBState != lastBState) || (force && (thisBState == 1)))
	{
		if(thisBState == 1)
			lastState = (checkMousePosition() == 1) ? 1:5 ;
		else if(thisBState == 2)
			lastState = ((mp = checkMousePosition()) != 0) ? mp+1:5 ;
		else
			lastState = (thisBState) ? 5:0 ;
		//vwLogVerbose((_T("New mouse state %d (%d %d)\n"),(int) lastState,(int) lastBState,(int) thisBState)) ;
		lastBState = thisBState ;
	}
	return lastState ;
}

/*************************************************
 * The mouse thread. This function runs in a thread and checks the mouse
 * position every 50ms. If mouse support is disabled, the thread will be in 
 * suspended state.
 */
DWORD WINAPI
vwMouseProc(LPVOID lpParameter)
{
	unsigned char mode, lastMode=0, state[4], newState, wlistState=0, wmenuState=0 ;
	int ii, newPos, pos[4], statePos[4], delayTime[4], wlistX=0, wlistY=0 ;
	POINT pt;
	
	state[0] = state[1] = state[2] = state[3] = 0 ;
	// infinite loop
	while(1)
	{
		Sleep(25); 
		
		mode = checkMouseState(0) ;
		if(mouseEnable & 0x0c)
		{
			if((mode == 3) || (mode == 4))
			{
				GetCursorPos(&pt);
				if(wlistState == 0)
				{
					wlistState = 1 ;
					wlistX = pt.x ;
					wlistY = pt.y ;
				}
				else if(mouseEnable & 8)
				{
					if((ii=(mouseJumpLength >> 1)) < 10)
						ii = 10 ;
					newPos = -1 ;
					
					if(mode == 4)
					{
						if((pt.x - wlistX) >= ii)
							newPos = 7 ;
						else if((wlistX - pt.x) >= ii)
							newPos = 6 ;
					}
					else if(abs(pt.x - wlistX) < abs(pt.y - wlistY))
					{
						if((pt.y - wlistY) >= ii)
							newPos = 3 ;
						else if((wlistY - pt.y) >= ii)
							newPos = 1 ;
					}
					else
					{
						if((pt.x - wlistX) >= ii)
							newPos = 2 ;
						else if((wlistX - pt.x) >= ii)
							newPos = 0 ;
					}
					if(newPos >= 0)
					{
						vwLogBasic((_T("Mouse mddle button desk change %d (%d,%d)\n"),newPos,pt.x - wlistX,pt.y - wlistY)) ;
						/* send the switch message and wait until done */
						SendMessage(hWnd, VW_MOUSEWARP, 0, MAKELPARAM(0,newPos)) ;
						Sleep(100) ;
						GetCursorPos(&pt);
						wlistX = pt.x ;
						wlistY = pt.y ;
						wlistState = 2 ;
					}
				}
			}
			else if(wlistState)
			{
				if((mode == 0) && (wlistState == 1) && (mouseEnable & 4) &&
				   (checkMousePosition() > 1))
				{
					vwLogBasic((_T("Mouse wlist %d\n"),wlistState)) ;
					SendMessage(hWnd, VW_MOUSEWARP, 0, MAKELPARAM(0,4)) ;
				}
				wlistState = 0 ;
			}
		}
		if((mouseEnable & 2) && ((mode == 2) || wmenuState))
		{
			if(mode == 2)
				wmenuState = 1 ;
			else
			{
				if((mode == 0) && (wmenuState == 1) && (checkMousePosition() == 1))
				{
					vwLogBasic((_T("Mouse wmenu %d\n"),wmenuState)) ;
					SendMessage(hWnd, VW_MOUSEWARP, 0, MAKELPARAM(0,5)) ;
				}
				wmenuState = 0 ;
			}
		}
		if((mouseEnable & 1) && ((mode == 1) || ((mode == 0) && ((mouseEnable & 0x10) == 0))) &&
		   (!mouseModifierUsed || vwKeyboardTestModifier(mouseModifier)))
		{
			GetCursorPos(&pt);
			pos[0] = pt.x - desktopWorkArea[mode][0] ;
			pos[1] = pt.y - desktopWorkArea[mode][1] ;
			pos[2] = desktopWorkArea[mode][2] - pt.x ;
			pos[3] = desktopWorkArea[mode][3] - pt.y ;
			ii = 3 ;
			if(mode != lastMode)
			{
				/* the state of the left button has changed, rest the mode. Must also try
				 * to handle clicks on the taskbar and other non-workarea locations */
				do
					state[ii] = (pos[ii] >= mouseJumpLength) ;
				while(--ii >= 0) ;
				lastMode = mode ;
			}
			else
			{
				do {
					if((newState = state[ii]) == 4)
					{
						if(pos[ii] > 0)
							newState = 3 ;
						else if(++delayTime[ii] >= mouseDelay)
						{
							vwLogBasic((_T("Mouse desk change on edge %d (%d,%d)\n"),ii,(int) pt.x,(int) pt.y)) ;
							/* send the switch message and wait until done */
							SendMessage(hWnd, VW_MOUSEWARP, 0, MAKELPARAM(mode|2,ii)) ;
							newState = 1 ;
						}
						else
							continue ;
					}
					else if(pos[ii] >= mouseJumpLength)
					{
						if(newState == 0)
							newState = 1 ;
					}
					else if(pos[ii] <= 0)
					{
						if((mouseKnock & 1) && ((newState == 0) || ((newState == 1) && (mouseKnock & 2))))
							newState = 2 ;
						else if((newState == 3) || (newState <= 1))
							newState = 4 ;
					}
					else if((newState == 2) && (pos[ii] >= (mouseJumpLength >> 2)))
						newState = 3 ;
					if(newState != state[ii])
					{
						newPos = (ii & 0x01) ? pt.x:pt.y ;
						if((state[ii] > 1) && (abs(statePos[ii]-newPos) > mouseJumpLength))
						{
							/* newState must also be greater than 2, check the mouse movement is accurate enough */
							vwLogBasic((_T("State %d (%d %d): Changed %d -> 1 (position: %d,%d)\n"),ii,mode,pos[ii],state[ii],(int) pt.x,(int) pt.y)) ;
							state[ii] = 1 ;
						}
						else
						{
#ifdef vwLOG_VERBOSE
							if(vwLogEnabled())
#else
							if(vwLogEnabled() && ((newState > 1) || (state[ii] > 1)))
#endif
								vwLogPrint(_T("State %d (%d %d): Change %d -> %d (%d,%d)\n"),ii,mode,pos[ii],state[ii],newState,(int) pt.x,(int) pt.y) ;
							state[ii] = newState ;
							if(newState == 2)
								statePos[ii] = newPos ;
							delayTime[ii] = 0 ;
						}
					}
					else if((state[ii] > 1) && (++delayTime[ii] >= 20))
					{
						/* burnt in 1sec timer, a knock must take no more than 1 sec second (2 * (20 * 25ms)) */
						vwLogBasic((_T("State %d (%d %d): Changed %d -> 1 (timer)\n"),ii,mode,pos[ii],newState)) ;
						state[ii] = 1 ;
					}
				} while(--ii >= 0) ;
			}
		}
		else
			lastMode = -1 ;
	}
	
	return TRUE;
}

/************************ *************************
 * Turns on/off the mouse thread. Makes sure that the the thread functions
 * only is called if needed.
 */
void
enableMouse(int turnOn)
{
	// Try to turn on thread if not already running
	if(turnOn && !mouseEnabled)
	{
		ResumeThread(mouseThread);
		mouseEnabled = TRUE;
	}
	// Try to turn of thread if already not stopped
	else if(!turnOn && mouseEnabled)
	{
		SuspendThread(mouseThread);
		mouseEnabled = FALSE;
	}
}

/*************************************************
 * Sets the icon in the systray and updates the currentDesk variable
 */
void
vwIconSet(int deskNumber, int hungCount)
{
	if(displayTaskbarIcon && ((taskbarIconShown & 0x02) == 0))
	{
		int ll ;
		if(hungCount < 0)
		{
			nIconD.hIcon = NULL ;
			hungCount = 0 - hungCount ;
		}
		else
			nIconD.hIcon = icons[deskNumber];
		ll = 0 ;
		if(hungCount)
			ll = _stprintf_s(nIconD.szTip, 128, _T("%d window%s not responding\n"),hungCount,(hungCount==1) ? _T(""):_T("s")) ;
		if(vwIsDisabled())
			_tcscpy_s(nIconD.szTip,128, vwVIRTUAWIN_NAME _T(" - Disabled")); /* Tooltip */
		else
		{
			ll += _stprintf_s(nIconD.szTip+ll,128, _T("Desktop %d"),deskNumber) ;
			if(desktopName[deskNumber] != NULL)
			{
				nIconD.szTip[ll++] = ':' ;
				nIconD.szTip[ll++] = ' ' ;
				_tcsncpy_s(nIconD.szTip+ll,128, desktopName[deskNumber],64-ll) ;
				nIconD.szTip[63] = '\0' ;
			}
		}
		if(taskbarIconShown & 0x01)
			Shell_NotifyIcon(NIM_MODIFY, &nIconD) ;
		else
		{
			// This adds the icon, try up to 3 times as systray process may not have started
			for(ll = 3 ;;)
			{
				if(Shell_NotifyIcon(NIM_ADD, &nIconD))
				{
					taskbarIconShown |= 0x01 ;
					break ;
				}
				if(--ll <= 0)
					break ;
				/* Due to the way Win7 kicks things off when logging on we can get a RM_TaskbarCreated leading to a double
				 * initialize, try deleting and if successful we should now be able to create a new one so don't sleep */ 
				if(Shell_NotifyIcon(NIM_DELETE,&nIconD))
					Sleep(2000) ;
			}
		}
	}
	else if(taskbarIconShown & 0x01)
	{
		Shell_NotifyIcon(NIM_DELETE,&nIconD) ;
		taskbarIconShown &= ~0x01 ;
	}
}

/************************ *************************
 * Loads the icons for the systray according to the current setup
 */
void
vwIconLoad(void)
{
	int xIcon = GetSystemMetrics(SM_CXSMICON);
	int yIcon = GetSystemMetrics(SM_CYSMICON);
	int ii, iconId, iconCount ;
	TCHAR buff[16], *ss ;
	
	/* must setup desktopUsed array first, this is used elsewhere */
	memset(desktopUsed+1,1,nDesks) ;
	memset(desktopUsed+nDesks+1,0,vwDESKTOP_SIZE-nDesks-1) ;
	ii = hotkeyCount ;
	while(--ii >= 0)
		if((hotkeyList[ii].command == vwCMD_NAV_MOVE_DESKTOP) &&
		   (hotkeyList[ii].desk < vwDESKTOP_SIZE))
			desktopUsed[hotkeyList[ii].desk] = 1 ;
	
	if(nDesksY != 2 || nDesksX != 2) // if 2 by 2 mode
	{
		iconId = IDI_ST_0 ;
		iconCount = 9 ;
	}
	else
	{
		if(osVersion >= OSVERSION_XP)
			iconId = IDI_ST_DIS_2 ;
		else
			iconId = IDI_ST_DIS_1 ;
		iconCount = 4 ;
	}
	_tcscpy_s(buff,16, _T("icons/")) ;
	for(ii = 0 ; ii<vwDESKTOP_SIZE ; ii++)
	{
		icons[ii] = NULL ;
		if((desktopUsed[ii] != 0) || (ii == 0))
		{
			/* Try to load user defined icons, otherwise use built in icon or disable icon */
			ss = buff+6 ;
			if(ii > 9)
			{
				*ss++ = (ii/10)+'0' ;
				*ss++ = (ii%10)+'0' ;
			}
			else
				*ss++ = ii+'0' ;
			_tcscpy_s(ss,16,_T(".ico")) ;
			if(((icons[ii] = (HICON) LoadImage(hInst, buff, IMAGE_ICON, xIcon, yIcon, LR_LOADFROMFILE)) == NULL) &&
			   ((ii > iconCount) ||
				((icons[ii] = (HICON) LoadImage(hInst, MAKEINTRESOURCE(iconId+ii), IMAGE_ICON, xIcon, yIcon, 0)) == NULL)) &&
			   ((icons[ii] = (HICON) LoadImage(hInst, MAKEINTRESOURCE(IDI_VIRTUAWIN), IMAGE_ICON, xIcon, yIcon, 0)) == NULL))
				icons[ii] = icons[0] ;
		}
	}
	// Load checkmark icon for sticky
	checkIcon=LoadIcon(hInst,MAKEINTRESOURCE(IDI_CHECK));
}


/************************************************
 * Setup vwHook - used to fix winodw activation issues
 */
void
vwHookSetup(void)
{
	if(vwHookUse != vwHookInstalled)
	{
		if(vwHookUse)
		{
			if(vwHookSetupFunc == NULL)
			{
				HINSTANCE libHandle ; 
				
				if(((libHandle = LoadLibrary(_T("vwHook"))) != NULL) &&
				   ((vwHookUninstallFunc = (vwHOOKUNINSTALL) GetProcAddress(libHandle,"vwHookUninstall")) != NULL) &&
				   ((vwHookInstallFunc = (vwHOOKINSTALL) GetProcAddress(libHandle,"vwHookInstall")) != NULL))
					vwHookSetupFunc = (vwHOOKSETUP) GetProcAddress(libHandle,"vwHookSetup") ;
			}
			if(vwHookSetupFunc != NULL)
			{
				vwHookSetupFunc(hWnd,1) ;
				if(vwHookInstallFunc() != 0)
					MessageBox(NULL,_T("Failed to install vwHook"),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONWARNING);
				else
				{
					vwLogBasic((_T("Installed vwHook\n"))) ;
					vwHookInstalled = 1 ;
				}
			}
			else
				MessageBox(NULL,_T("Failed to load vwHook"),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONWARNING);
		}
		else
		{
			if(vwHookSetupFunc != NULL)
			{
				vwHookUninstallFunc() ;
				vwLogBasic((_T("Uninstalled vwHook\n"))) ;
			}
			vwHookInstalled = 0 ;
		}
	}
}

/************************************************
 * Registering hotkeys - don't complain if registering toggle enable or boss keys as these are not always unregistered 
 */
void
vwHotkeyRegister(int warnAll)
{
	if(!hotkeyRegistered)
	{
		TCHAR buff[64] ;
		int ii ;
		hotkeyRegistered = hotkeyCount ;
		
		for(ii=0 ; ii<hotkeyCount ; ii++)
		{
			if(hotkeyList[ii].atom == 0)
			{
				_stprintf_s(buff, 260, _T("vwAtom%d"), ii);
				hotkeyList[ii].atom = GlobalAddAtom(buff);
			}
			if(hotkeyList[ii].atom == 0)
				MessageBox(hWnd,_T("Failed to create global atom"),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONWARNING);
			else if((RegisterHotKey(hWnd,hotkeyList[ii].atom,(hotkeyList[ii].modifier & vwHOTKEY_MOD_MASK),hotkeyList[ii].key) == FALSE) &&
					(warnAll || ((hotkeyList[ii].command != vwCMD_UI_ENABLESTATE) &&
								 (hotkeyList[ii].command != vwCMD_UI_BOSS_KEY) && (hotkeyList[ii].command != vwCMD_UI_UNBOSS_KEY))))
			{
				_stprintf_s(buff, 260, _T("Failed to register hotkey %d, check hotkeys."), ii + 1);
				MessageBox(hWnd,buff,vwVIRTUAWIN_NAME _T(" Error"), MB_ICONWARNING);
			}
		}
	}
}

/***************************************************************************************************
 * Unregistering hotkeys - dont unregister the toggle enable state or boss keys unless exiting
 */
void
vwHotkeyUnregister(int unregAll)
{
	if(hotkeyRegistered)
	{
		int ii ;
		ii = hotkeyRegistered ;
		hotkeyRegistered = 0 ;
		while(--ii >= 0)
			if((hotkeyList[ii].atom) && (unregAll || ((hotkeyList[ii].command != vwCMD_UI_ENABLESTATE) && 
													  (hotkeyList[ii].command != vwCMD_UI_BOSS_KEY) && (hotkeyList[ii].command != vwCMD_UI_UNBOSS_KEY))))
				UnregisterHotKey(hWnd,hotkeyList[ii].atom) ;
	}
}

/***************************************************************************************************
 * Get screen width and height and store values in global variables
 */
static void
getScreenSize(void)
{
	RECT r;
	desktopHWnd = GetDesktopWindow();
	GetClientRect(desktopHWnd,&r) ;
	dialogPos[0] = ((r.left + r.right) - 440) >> 1 ;
	dialogPos[1] = ((r.top + r.bottom) - 550) >> 1 ;
	if((desktopWorkArea[0][2] = GetSystemMetrics(SM_CXVIRTUALSCREEN)) <= 0)
	{
		/* The virtual screen size system matrix values are not supported on
		 * this OS (Win95 & NT), use the desktop window size */
		desktopWorkArea[0][0] = r.left;
		desktopWorkArea[0][1] = r.top;
		desktopWorkArea[0][2] = r.right - 1 ;
		desktopWorkArea[0][3] = r.bottom - 1 ;
	}
	else
	{
		desktopWorkArea[0][0]  = GetSystemMetrics(SM_XVIRTUALSCREEN);
		desktopWorkArea[0][1]  = GetSystemMetrics(SM_YVIRTUALSCREEN);
		desktopWorkArea[0][2] += desktopWorkArea[0][0] - 1 ;
		desktopWorkArea[0][3]  = GetSystemMetrics(SM_CYVIRTUALSCREEN) + desktopWorkArea[0][1] - 1;
	}
	vwLogBasic((_T("Got screen %x size: %d %d -> %d %d\n"),
				(int) desktopHWnd,desktopWorkArea[0][0],desktopWorkArea[0][1],desktopWorkArea[0][2],desktopWorkArea[0][3])) ;
}

/************************************************
 * Grabs and stores the workarea of the screen
 */
void
getWorkArea(void)
{
	RECT r;
	
	if((GetSystemMetrics(SM_CMONITORS) <= 1) && SystemParametersInfo(SPI_GETWORKAREA,0,&r,0))
	{
		desktopWorkArea[1][0] = r.left;
		desktopWorkArea[1][1] = r.top;
		desktopWorkArea[1][2] = r.right - 1 ; 
		desktopWorkArea[1][3] = r.bottom - 1 ;
	}
	else
	{
		desktopWorkArea[1][0] = desktopWorkArea[0][0] ;
		desktopWorkArea[1][1] = desktopWorkArea[0][1] ;
		desktopWorkArea[1][2] = desktopWorkArea[0][2] ;
		desktopWorkArea[1][3] = desktopWorkArea[0][3] ;
	}
	GetWindowRect(hWnd,&r) ;
	vwLogBasic((_T("Got work area: %d %d -> %d %d  &  %d %d -> %d %d (%d,%d)\n"),
				desktopWorkArea[0][0],desktopWorkArea[0][1],desktopWorkArea[0][2],desktopWorkArea[0][3],
				desktopWorkArea[1][0],desktopWorkArea[1][1],desktopWorkArea[1][2],desktopWorkArea[1][3],r.left,r.top)) ;
	/* make sure the VW window is still hidden */
	if(r.top > -30000)
		SetWindowPos(hWnd,0,10,-31000,0,0,(SWP_FRAMECHANGED | SWP_DEFERERASE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING)) ; 
}

/************************************************
 * Tries to locate the handle to the taskbar
 */
void
vwTaskbarHandleGet(void)
{
	/* set the window to give focus to when releasing focus on switch also used to refresh */
	if((desktopHWnd = GetDesktopWindow()) != NULL)
		desktopThread = GetWindowThreadProcessId(desktopHWnd,NULL) ;
	if(((deskIconHWnd = vwFindWindow(_T("Progman"), _T("Program Manager"),0)) != NULL) &&
	   ((deskIconHWnd = FindWindowEx(deskIconHWnd, NULL, _T("SHELLDLL_DefView"),NULL)) != NULL))
	   deskIconHWnd = FindWindowEx(deskIconHWnd, NULL, _T("SysListView32"), _T("FolderView")) ;
	
	taskbarBCType = vwTASKBAR_BC_NONE ;
	if((noTaskbarCheck & 0x01) == 0)
	{
		HWND hwndTray = FindWindowEx(NULL, NULL,_T("Shell_TrayWnd"), NULL);
		HWND hwndBar = FindWindowEx(hwndTray, NULL,_T("ReBarWindow32"), NULL );
		
		// Maybe "RebarWindow32" is not a child to "Shell_TrayWnd", then try this
		if(hwndBar == NULL)
			hwndBar = hwndTray;
		
		taskHWnd = FindWindowEx(hwndBar, NULL,_T("MSTaskSwWClass"), NULL);
		
		if((taskHWnd != NULL) && (preserveZOrder > 2))
		{
			if((hwndBar = FindWindowEx(taskHWnd,0,_T("SysTabControl32"),0)) != NULL)
			{
				taskbarBCHWnd = hwndBar ;
				taskbarBCType = vwTASKBAR_BC_TABCONTROL;
			}
			else if((hwndBar = FindWindowEx(taskHWnd,0,_T("ToolbarWindow32"),0)) != NULL)
			{
				taskbarBCHWnd = hwndBar ;
				taskbarBCType = vwTASKBAR_BC_TOOLBAR;
			}
			else if((hwndBar = FindWindowEx(taskHWnd,0,_T("MSTaskListWClass"),0)) != NULL)
			{
				taskbarBCHWnd = hwndBar ;
				taskbarBCType = vwTASKBAR_BC_WIN7;
			}
			else
				MessageBox(hWnd,_T("Failed to identify taskbar button container - dynamic taskbar order disabled."),vwVIRTUAWIN_NAME _T(" Error"),0) ;
			if(taskbarBCType)
			{
				DWORD procId ;
				
				if(taskbarShrdMem)
				{
					if(osVersion < OSVERSION_NT)
						VirtualFree(taskbarShrdMem,0,MEM_RELEASE) ;
					else
						VirtualFreeEx(taskbarProcHdl,taskbarShrdMem,0,MEM_RELEASE);
					taskbarShrdMem = NULL;
				}
				if(taskbarProcHdl == NULL)
				{
					CloseHandle(taskbarProcHdl);
					taskbarProcHdl = NULL;
				}
				if((GetWindowThreadProcessId(taskbarBCHWnd,&procId) != 0) && (procId != 0) &&
				   ((taskbarProcHdl = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, procId)) != NULL))
				{
					if(taskbarBCType != vwTASKBAR_BC_WIN7)
					{
						if(osVersion <= OSVERSION_9X)
							taskbarShrdMem = VirtualAlloc(NULL,sizeof(TBBUTTON) + sizeof(TCITEM),MEM_COMMIT | VA_SHARED, PAGE_READWRITE) ;
						else
							taskbarShrdMem = VirtualAllocEx(taskbarProcHdl,NULL,sizeof(TBBUTTON) + sizeof(TCITEM),MEM_COMMIT,PAGE_READWRITE) ;
						if(taskbarShrdMem == NULL)
						{
							MessageBox(hWnd,_T("Failed to create taskbar button transfer memory - dynamic taskbar order disabled."),vwVIRTUAWIN_NAME _T(" Error"),0) ;
							taskbarBCType = vwTASKBAR_BC_NONE ;
						}
					}
				}
			}
		}
	}
	// if on win9x the tricky windows need to be continually hidden
	taskbarFixRequired = ((osVersion <= OSVERSION_9X) && (taskHWnd != NULL)) ;
	vwLogBasic((_T("Got desktopWin %x T%d, iconWin %x, taskbar %d win %x, type %d, fix %d\n"),
				(int) desktopHWnd,(int) desktopThread,(int) deskIconHWnd,noTaskbarCheck,(int) taskHWnd,taskbarBCType,taskbarFixRequired)) ;
}

/************************************************
 * desk image generation functions
 */
int
createDeskImage(int deskNo, int createDefault)
{
	HDC deskDC ;
	HDC bitmapDC ;
	HBITMAP oldmap;
	TCHAR fname[MAX_PATH] ;
	FILE *fp ;
	int ww, hh, ret ;
	
	if((deskImageEnabled == 0) || (deskNo > nDesks) ||
	   ((deskDC = GetDC(desktopHWnd)) == NULL) ||
	   ((bitmapDC = CreateCompatibleDC(deskDC)) == NULL))
		return 0 ;
	oldmap = (HBITMAP) SelectObject(bitmapDC,deskImageBitmap);
	ww = desktopWorkArea[0][2]-desktopWorkArea[0][0] + 1 ;
	hh = desktopWorkArea[0][3]-desktopWorkArea[0][1] + 1 ;
	if(createDefault)
	{
		/* Fill all using the button color (taskbar) and then draw the active desktop area (non-taskbar)
		 * the background colour, but one pixel smaller all round to creatre a boarder, this makes
		 * modules like VWPreview easier to use when only default previews are available. */
		RECT rect;
		rect.left   = 0 ;
		rect.top    = 0 ;
		rect.right  = deskImageInfo.bmiHeader.biWidth ;
		rect.bottom = deskImageInfo.bmiHeader.biHeight ;
		FillRect(bitmapDC,&rect,(HBRUSH) (COLOR_BTNFACE+1)) ;
		rect.left   = 1 + ((desktopWorkArea[1][0] - desktopWorkArea[0][0]) * deskImageInfo.bmiHeader.biWidth / ww) ;
		rect.top    = 1 + ((desktopWorkArea[1][1] - desktopWorkArea[0][1]) * deskImageInfo.bmiHeader.biHeight / hh) ;
		rect.right  = (desktopWorkArea[1][2] - desktopWorkArea[0][0]) * deskImageInfo.bmiHeader.biWidth / ww ;
		rect.bottom = (desktopWorkArea[1][3] - desktopWorkArea[0][1]) * deskImageInfo.bmiHeader.biHeight / hh ;
		FillRect(bitmapDC,&rect,(HBRUSH) (COLOR_BACKGROUND+1)) ;
	}
	else if(deskImageInfo.bmiHeader.biHeight == hh)
	{
		BitBlt(bitmapDC,0,0,ww,hh,deskDC,desktopWorkArea[0][0],desktopWorkArea[0][1],SRCCOPY) ;
	}
	else
	{
		/* set to HALFTONE for better quality, but not supported on Win95/98/Me */
		if(osVersion >= OSVERSION_NT)
		{
			SetStretchBltMode(bitmapDC,HALFTONE) ;
			SetBrushOrgEx(bitmapDC,0,0,0) ;
		}
		else
			SetStretchBltMode(bitmapDC,COLORONCOLOR) ;
		StretchBlt(bitmapDC,0,0,deskImageInfo.bmiHeader.biWidth,deskImageInfo.bmiHeader.biHeight,
				   deskDC,desktopWorkArea[0][0],desktopWorkArea[0][1],ww,hh,SRCCOPY);    
	}
	
	/* Create the desk_#.bmp file */ 
	GetFilename(vwFILE_COUNT,1,fname) ;
	_stprintf_s(fname + _tcslen(fname), 260, _T("desk_%d.bmp"), deskNo);
	if(GetDIBits(bitmapDC,deskImageBitmap,0,deskImageInfo.bmiHeader.biHeight,deskImageData,&deskImageInfo,DIB_RGB_COLORS) &&
		((fp = _tfopen(fname, _T("wb+"))) != 0))
	{
		BITMAPFILEHEADER hdr ;
		hdr.bfType = 0x4d42 ;
		hdr.bfOffBits = (DWORD) (sizeof(BITMAPFILEHEADER) + deskImageInfo.bmiHeader.biSize) ;
		hdr.bfSize = hdr.bfOffBits + deskImageInfo.bmiHeader.biSizeImage ; 
		hdr.bfReserved1 = 0 ;
		hdr.bfReserved2 = 0 ;
		
		ret = ((fwrite(&hdr,sizeof(BITMAPFILEHEADER),1,fp) == 1) &&  
			   (fwrite(&deskImageInfo,deskImageInfo.bmiHeader.biSize,1,fp) == 1) &&
			   (fwrite(deskImageData,deskImageInfo.bmiHeader.biSizeImage,1,fp) == 1)) ;
		
		fclose(fp) ;
		if(!ret)
			DeleteFile(fname) ;
	}
	else
		ret = 0 ;
	
	SelectObject(bitmapDC,oldmap);
	DeleteDC(bitmapDC);
	ReleaseDC(desktopHWnd,deskDC) ;
	
	vwLogBasic((_T("createDeskImage: %d: %d %d - %d %d\n"),ret,deskNo,createDefault,(int) deskImageInfo.bmiHeader.biWidth,(int) deskImageInfo.bmiHeader.biHeight)) ;
	return ret ;
}

int
disableDeskImage(int count)
{
	if(deskImageCount <= 0)
		return 0 ;
	if((deskImageCount -= count) <= 0)
	{
		if(deskImageData != NULL)
		{
			free(deskImageData) ;
			deskImageData = NULL ;
		}
		if(deskImageBitmap != NULL)
		{
			DeleteObject(deskImageBitmap) ;
			deskImageBitmap = NULL ;
		}
		deskImageInfo.bmiHeader.biHeight = 0 ;
		deskImageCount = 0 ;
		deskImageEnabled = 0 ;
	}
	return 1 ;
}

static int
enableDeskImage(int height)
{
	int width, biSizeImage, count=deskImageCount ;
	
	if(height <= 0)
		return 0 ;
	if((count <= 0) || (deskImageInfo.bmiHeader.biHeight < height))
	{
		HDC deskDC = GetDC(desktopHWnd) ;
		
		if(count > 0)
			disableDeskImage(count) ;
		
		if((width = (height * (desktopWorkArea[0][2]-desktopWorkArea[0][0]+1)) / (desktopWorkArea[0][3]-desktopWorkArea[0][1]+1)) <= 0)
			width = 1 ;
		/* the GetDIBits function returns 24 bit RGB even if the bitmap is set to 32 bit rgba so fix
		 * the BMP to 24 bit RGB, not sure what would happen on a palette based system. However VW
		 * crashes if deskImageData is only w*h*3, found w*h*4 avoids the problem. */
		biSizeImage = (((width * 3) + 3) & ~3) * height ; 
		if(((deskImageBitmap = CreateCompatibleBitmap(deskDC,width,height)) != NULL) &&
		   ((deskImageData = malloc(biSizeImage)) != NULL))
		{
			deskImageInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
			deskImageInfo.bmiHeader.biWidth = width ; 
			deskImageInfo.bmiHeader.biHeight = height ; 
			deskImageInfo.bmiHeader.biPlanes = 1 ; 
			deskImageInfo.bmiHeader.biBitCount = 24 ; 
			deskImageInfo.bmiHeader.biCompression = BI_RGB ;
			deskImageInfo.bmiHeader.biSizeImage = biSizeImage ;
			deskImageInfo.bmiHeader.biXPelsPerMeter = 0 ;
			deskImageInfo.bmiHeader.biYPelsPerMeter = 0 ;
			deskImageInfo.bmiHeader.biClrUsed = 0 ;
			deskImageInfo.bmiHeader.biClrImportant = 0 ;
			vwLogBasic((_T("initDeskImage succeeded: %d - %d %d\n"),
						height,(int) deskImageInfo.bmiHeader.biWidth,(int) deskImageInfo.bmiHeader.biHeight)) ;
		}
		else
			vwLogBasic((_T("initDeskImage failed: %d, %x, %x\n"),height,(int) deskImageBitmap,(int) deskImageData)) ; 
		ReleaseDC(desktopHWnd,deskDC);
		if(deskImageData == NULL)
			return 0 ;
		deskImageEnabled = 1 ;
	}
	/* if first time enabled, create default images for all desks */
	if(count < 0)
	{
		/* first time enabled, create default images for all desks */
		count = nDesks ;
		do
			createDeskImage(count,1) ;
		while(--count > 0) ;
		count = 0 ;
	}
	deskImageCount = count + 1 ;
	return 1 ;
}

/************************************************
 * Show the VirtuaWin help pages
 */
void
showHelp(HWND aHWnd, TCHAR *topic)
{
	TCHAR buff[MAX_PATH+64] ;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;  

	_tcscpy_s(buff,324, _T("\"hh\" mk:@MSITStore:")) ;
	GetFilename(vwVIRTUAWIN_HLP,0,buff+19);
	if(topic != NULL)
	{
		_tcscat_s(buff, 324, _T("::/VirtuaWin_"));
		_tcscat_s(buff, 324, topic);
	}
	memset(&si, 0, sizeof(si)); 
	si.cb = sizeof(si); 
	if(CreateProcess(NULL,buff,NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
	{
		CloseHandle(pi.hThread) ;
		CloseHandle(pi.hProcess) ;
	}
	else
		MessageBox(aHWnd,_T("Error opening on-line help."),vwVIRTUAWIN_NAME _T(" Error"),MB_ICONWARNING);
}

static void
vwWindowBaseLink(vwWindowBase *wb) 
{
	vwWindowBase *wp ;
	int idx = ((unsigned int) (wb->handle)) % vwWINHASH_SIZE ;
	
	wb->hash = windowHash[idx] ;
	windowHash[idx] = wb ;
	if(vwWindowIsManaged(wb))
	{
		wb->next = NULL ;
		if((wp = (vwWindowBase *) windowList) == NULL)
		{
			windowList = (vwWindow *) wb ;
			wp = windowBaseList ;
		}
		if(wp == NULL)
			windowBaseList = wb ;
		else
		{
			while(wp->next != NULL)
				wp = wp->next ;
			wp->next = wb ;
		}
	}
	else
	{
		wb->next = windowBaseList ;
		windowBaseList = wb ;
	}
}

static void
vwWindowBaseUnlink(vwWindowBase *wb) 
{
	vwWindowBase *wp ;
	int idx = ((unsigned int) (wb->handle)) % vwWINHASH_SIZE ;
	
	if((wp = windowHash[idx]) == wb)
		windowHash[idx] = wb->hash ;
	else
	{
		while(wp->hash != wb)
			wp = wp->hash ;
		wp->hash = wb->hash ;
	}
	
	wp = windowBaseList ;
	if(vwWindowIsManaged(wb))
	{
		if(wb == (vwWindowBase *) windowList)
			windowList = (vwWindow *) wb->next ;
		else
			wp = (vwWindowBase *) windowList ;
	}
	if(wb == wp)
		windowBaseList = wb->next ;
	else
	{
		while(wp->next != wb)
			wp = wp->next ;
		wp->next = wb->next ;
	}
}

static vwWindowBase *
vwWindowBaseCreate(vwUInt flags, HWND hwnd) 
{
	vwWindowBase *wb ;
	vwWindow *win, *pwin ;
	
	if(flags & vwWINFLAGS_WINDOW)
	{
		if((win = windowFreeList) != NULL)
		{
			windowFreeList = win->next ;
			memset(win,0,sizeof(vwWindow)) ;
		}
		else
			win = calloc(1,sizeof(vwWindow)) ;
		wb = (vwWindowBase *) win ;
	}
	else
	{
		if((wb = windowBaseFreeList) != NULL)
		{
			windowBaseFreeList = wb->next ;
			memset(wb,0,sizeof(vwWindowBase)) ;
		}
		else
			wb = calloc(1,sizeof(vwWindowBase)) ;
		win = NULL ;
	}
	if(wb == NULL)
	{
		static vwUByte printedError=FALSE ;
		if(!printedError)
		{
			printedError = TRUE ;
			MessageBox(hWnd,_T("System resources are low, windows may not be managed."),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
		}
		return NULL ;
	}
	wb->handle = hwnd ;
	wb->flags = flags ;
	if(win != NULL)
	{
		if(GetWindowThreadProcessId(hwnd,&(win->processId)) == 0)
			win->processId = 0 ;
		else if((pwin = (vwWindow *) windowBaseList) != NULL)
		{
			do
			{
				if((pwin->flags & vwWINFLAGS_WINDOW) && (win->processId == pwin->processId))
				{
					if((win->processNext = pwin->processNext) == NULL)
						win->processNext = pwin ;
					pwin->processNext = win ;
					break ;
				}
			} while((pwin = pwin->next) != NULL) ;
		}
	}
	vwWindowBaseLink(wb) ;
	
	return wb ;
}

static void
vwWindowBaseDelete(vwWindowBase *wb) 
{
	vwWindowBaseUnlink(wb) ; 
	if(vwWindowIsWindow(wb))
	{
		vwWindow *pWin, *nWin ;
		if((nWin = ((vwWindow *) wb)->processNext) != NULL)
		{
			pWin = nWin ;
			while(pWin->processNext != (vwWindow *) wb)
				pWin = pWin->processNext ;
			if(pWin == nWin)
				pWin->processNext = NULL ;
			else
				pWin->processNext = nWin ;
		}
		if((nWin = ((vwWindow *) wb)->linkedNext) != NULL)
		{
			pWin = nWin ;
			while(pWin->linkedNext != (vwWindow *) wb)
				pWin = pWin->linkedNext ;
			if(pWin == nWin)
			{
				pWin->linkedNext = NULL ;
				/* 1877997 if the taskbar button was being removed by VW stop
				 * removing it as all the linked windows have been closed */
				if(pWin->flags & vwWINFLAGS_RM_TASKBAR_BUT)
					pWin->flags &= ~(vwWINFLAGS_NO_TASKBAR_BUT | vwWINFLAGS_RM_TASKBAR_BUT) ;
			}
			else
				pWin->linkedNext = nWin ;
		}
		wb->next = (vwWindowBase *) windowFreeList ;
		windowFreeList = (vwWindow *) wb ;
	}
	else
	{
		wb->next = windowBaseFreeList ;
		windowBaseFreeList = wb ;
	}
}

static void
vwWindowLink(vwWindow *winP, vwWindow *winC)
{
	vwWindow *ww ;
	if((ww=winC->linkedNext) != NULL)
	{
		while(ww->linkedNext != winC)
		{
			if(ww == winP)
				return ;
			ww = ww->linkedNext ;
		}
	}
	else
		ww = winC ;
	if((ww->linkedNext = winP->linkedNext) == NULL)
		ww->linkedNext = winP ;
	winP->linkedNext = winC ;
}

/*************************************************
 * Returns vwWindowBase pointer if window is found in hash table
 */
static vwWindowBase *
vwWindowBaseFind(HWND hwnd)
{
	vwWindowBase *wb ;
	
	wb = windowHash[((unsigned int) hwnd) % vwWINHASH_SIZE] ;
	while(wb != NULL)
	{
		if(wb->handle == hwnd)
			return wb ;
		wb = wb->hash ;
	}
	return NULL ;
}

/*************************************************
 * Returns pointer to a vwWindow if window is found and managed, NULL otherwise
 */
static vwWindow *
vwWindowFind(HWND hwnd)
{
	vwWindowBase *wb ;
	
	wb = windowHash[((unsigned int) hwnd) % vwWINHASH_SIZE] ;
	while(wb != NULL)
	{
		if(wb->handle == hwnd)
		{
			if(vwWindowIsManaged(wb))
				return (vwWindow *) wb ;
			break ;
		}
		wb = wb->hash ;
	}
	return NULL ;
}

typedef int (*vwIFuncSST)(const TCHAR *, const TCHAR *, size_t);
static vwIFuncSST nameCompFunc[vwWTNAME_COUNT] = { _tcsncmp, _tcsncmp, _tcsnicmp } ;


//Searches the window rule list to find rule with highest precedence. 
//If we've already found a rule owt, check to see it's still active. 
//Otherwise search the rule list to find the first match. 
static vwWindowRule *
vwWindowRuleFind(HWND hwnd, vwWindowRule *owt)
{
	TCHAR name[vwWTNAME_COUNT][MAX_PATH] ;
	vwWindowRule *wt ;
	int nameLen[vwWTNAME_COUNT], infoGot=0, ii, jj, bi ;
	
	if(owt != NULL)
	{
		/* the original windowRule is stored in zOrder[0] but not maintained.
		 * If the user has triggered a reload the list will have changed, so
		 * check the value is correct */
		wt = windowRuleList ;
		while(wt != NULL)
		{
			if(wt == owt)
			{
				if(wt->flags & vwWTFLAGS_ENABLED)
					return wt ;
				break ;
			}
			wt = wt->next ;
		}
	}
		
	wt = windowRuleList;
	while(wt)
	{
		if(wt->flags & vwWTFLAGS_ENABLED)
		{
			ii = vwWTNAME_COUNT - 1 ;
			do {
				if(wt->name[ii] != NULL)
				{
					bi = 1<<ii ;
					if((infoGot & bi) == 0)
					{
						infoGot |= bi ;
						if(ii == 0)
						{
							name[0][0] = '\0' ;
							GetClassName(hwnd,name[0],MAX_PATH);
						}
						else if(ii == 1)
						{
							if(!GetWindowText(hwnd,name[1],MAX_PATH))
								name[1][0] = '\0' ;
						}
						else if(ii == 2)
						{
							HANDLE procHdl ;
							DWORD procId ;
							DWORD procSize = MAX_PATH; 
							name[2][0] = '\0' ;
							if((vwGetModuleFileNameEx != NULL) &&
							   (GetWindowThreadProcessId(hwnd,&procId) != 0) && (procId != 0) && 
							   ((procHdl=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,procId)) != NULL))
							{
								//vwGetModuleFileNameEx(procHdl,NULL,name[2],MAX_PATH) ;
								QueryFullProcessImageName(procHdl, 0, name[2], &procSize);
								CloseHandle(procHdl) ;
							}
						}
						nameLen[ii] = _tcslen(name[ii]) ;
					}
					if((ii != 1) && (nameLen[ii] == 0))
						/* failed to get className or processName - don't match */
						break ;
					if(nameLen[ii] < ((int) wt->nameLen[ii]))
						break ;
					switch((wt->flags >> (ii << 1)) & 0x03)
					{
					case 0:
						/* "<NAME>" */
						if(nameLen[ii] != ((int) wt->nameLen[ii]))
							bi = 1 ;
						else
							bi = nameCompFunc[ii](name[ii],wt->name[ii],wt->nameLen[ii]) ;
						break ;
					case 1:
						/* "*<NAME>" */
						bi = nameCompFunc[ii](name[ii] + nameLen[ii] - wt->nameLen[ii],wt->name[ii],wt->nameLen[ii]) ;
						break ;
					case 2:
						/* "<NAME>*" */
						bi = nameCompFunc[ii](name[ii],wt->name[ii],wt->nameLen[ii]) ;
						break ;
					case 3:
						/* "*<NAME>*" */
						jj = nameLen[ii] - ((int) wt->nameLen[ii]) + 1 ;
						while(--jj >= 0)
							if((bi=nameCompFunc[ii](name[ii]+jj,wt->name[ii],wt->nameLen[ii])) == 0)
								break ;
						break ;
					}
					if(bi)
						break ;
				}
			} while(--ii >= 0) ;
			if(ii < 0)
				return wt ;
		}
		wt = wt->next ;
	}
	return NULL ;
}


/*************************************************
 * Forces a window into the foreground. Must be done in this way to avoid
 * the flashing in the taskbar insted of actually changing active window.
 */
void
setForegroundWin(HWND theWin, int makeTop)
{
	HWND cwHwnd, setHwnd=theWin ;
	vwWindow *win ;
	DWORD ThreadID1, ThreadID2, timeout;
	int ii, err1, err2 ;
	
	cwHwnd = GetForegroundWindow() ;
	vwLogBasic((_T("setForegroundWin: %x -> %x makeTop: %d (%x = virtuawin, %x = desktop)\n"),(int) cwHwnd, (int) theWin,makeTop,(int) hWnd,(int) desktopHWnd)) ;
	if(theWin == NULL)
	{
		setHwnd = desktopHWnd ;
		makeTop = 0 ;
	}
	if(cwHwnd != theWin)
	{
		/* try cheap and easy way first */
		if(cwHwnd == NULL)
			BringWindowToTop(setHwnd) ;
		err1 = SetForegroundWindow(setHwnd) ; 
		cwHwnd = GetForegroundWindow() ;
		//vwLogBasic((_T("SetForegroundWindow: %x %x - %d\n"),(int) theWin,(int) cwHwnd, err1)) ;
	}
	if(cwHwnd == theWin)
	{
		/* bring to the front if requested as swapping desks can muddle the order */
		if(makeTop)
			BringWindowToTop(setHwnd);
		return ;
	}
	
	if(theWin == NULL)
		ThreadID2 = desktopThread ;
	else if((theWin == hWnd) || (theWin == dialogHWnd))
		ThreadID2 = vwThread ;
	else if(((win = vwWindowFind(theWin)) == NULL) || vwWindowIsNotShow(win) || vwWindowIsNotShown(win) ||
			(windowIsHung(theWin,50) && (Sleep(1),windowIsHung(theWin,100))))
	{
		/* don't make the foreground a hidden or non-managed or hung window */
		vwLogBasic((_T("SetForground: %8x - %d %x or HUNG\n"),(int) theWin,(win == NULL),(win == NULL) ? 0:win->flags)) ;
		return ;
	}
	else
		ThreadID2 = GetWindowThreadProcessId(theWin,NULL) ;
	
	ii = 1 ;
	for(;;ii--)
	{
		/* do not bother making it the foreground if it already is */
		if((cwHwnd = GetForegroundWindow()) == theWin)
			break ;
		if(ii < 0)
		{
			SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,0,(LPVOID) &timeout,0);
			SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,0,(LPVOID) 0,SPIF_SENDCHANGE);
		}
		/* get the Id of the current foreground process */
		if(cwHwnd == NULL)
		{
			BringWindowToTop(setHwnd) ;
			ThreadID1 = desktopThread ;
		}
		else if(windowIsHung(cwHwnd,50) && (Sleep(1),windowIsHung(cwHwnd,100)))
			ThreadID1 = 0 ;
		else
			ThreadID1 = GetWindowThreadProcessId(cwHwnd,NULL) ;
		if(ThreadID1 == ThreadID2)
		{
			SetForegroundWindow(setHwnd) ; 
			vwLogBasic((_T("Two windows owned by same thread: %x - %x (%d %d %d)\n"),(int) cwHwnd,(int) GetForegroundWindow(),ThreadID1,ThreadID2,vwThread)) ;
		}
		else
		{
			/* get foreground window ownership first */
			if((ThreadID1 != 0) && (ThreadID1 != vwThread))
			{
				err1 = AttachThreadInput(ThreadID1,vwThread,TRUE);
				err2 = SetForegroundWindow(hWnd) ;
				AttachThreadInput(ThreadID1,vwThread,FALSE);
				vwLogBasic((_T("Attached to get foreground: %x - %x (%d %d %d) %d %d\n"),(int) cwHwnd,(int) GetForegroundWindow(),ThreadID1,ThreadID2,vwThread,err1,err2)) ;
			}
			if((ThreadID2 != 0) && (ThreadID2 != vwThread))
			{
				err1 = AttachThreadInput(vwThread,ThreadID2,TRUE);
				err2 = SetForegroundWindow(setHwnd) ; 
				AttachThreadInput(vwThread,ThreadID2,FALSE);
				vwLogBasic((_T("Attached to set foreground: %x - %x (%d %d %d) %d %d\n"),(int) theWin,(int) GetForegroundWindow(),ThreadID1,ThreadID2,vwThread,err1,err2)) ;
			}
			else if(setHwnd != hWnd)
			{
				err2 = SetForegroundWindow(setHwnd) ; 
				vwLogBasic((_T("Set foreground Window: %x - %x (%d %d %d) %d\n"),(int) theWin,(int) GetForegroundWindow(),ThreadID1,ThreadID2,vwThread,err2)) ;
			}
		}
		/* SetForegroundWindow can return success (non-zero) but not succeed (GetForegroundWindow != theWin)
		 * Getting the foreground window right is really important because if the existing foreground window
		 * is left as the foreground window but hidden (common when moving the app or desk) VW will confuse
		 * it with a popup */
		cwHwnd = GetForegroundWindow() ;
		vwLogBasic((_T("Set foreground window %d: %d, %x -> %x\n"),ii,(theWin == cwHwnd),(int) theWin,(int) cwHwnd)) ;
		if(ii < 0)
		{
			SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,0,(LPVOID) timeout,SPIF_SENDCHANGE);
			break ;
		}
		else if(cwHwnd == theWin)
			break ;
		
		/* A short sleep allows the rest of the system to catch up */
		vwLogVerbose((_T("About to FG sleep\n"))) ;
		Sleep(1) ;
	}
	/* bring to the front if requested as swapping desks can muddle the order */
	if(makeTop)
		BringWindowToTop(theWin);
}

/************************************************
 * Show the setup dialog and perform some stuff before and after display
 */
static void
showSetup(void)
{
	if(!dialogOpen)
	{
		// reload current config
		loadVirtuawinConfig();
		vwLogVerbose((_T("About to call createSetupDialog\n"))) ;
		createSetupDialog(hInst,hWnd);
		vwLogVerbose((_T("createSetupDialog returned\n"))) ;
	}
	else if((dialogHWnd != NULL) && (GetForegroundWindow() != dialogHWnd))
	{
		// setup dialog has probably been lost under the windows raise it.
		setForegroundWin(dialogHWnd,0);
		SetWindowPos(dialogHWnd,HWND_NOTOPMOST,0,0,0,0,
					 SWP_DEFERERASE | SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_NOMOVE) ;
	}
}

/************************************************
 * Show the window rule dialog and perform some stuff before and after display
 * if add=1 user clicked "add window rule," otherwise "edit window rule" 
 */
static void
showWindowRule(HWND theWin, int add)  
{
	vwWindowRule *wt = NULL;
	vwWindow *win;

	if (!dialogOpen && useWindowRules)
	{
		if ((theWin != NULL) && !add)
		{
			vwMutexLock();
			wt = NULL;
			if ((win = vwWindowFind(theWin)) != NULL)
				wt = (vwWindowRule *)win->zOrder[0];
			wt = vwWindowRuleFind(theWin, wt);
			vwMutexRelease();
			theWin = NULL;
		}
		vwLogVerbose((_T("About to open windowRule\n")));
		createWindowRuleDialog(hInst, hWnd, wt, theWin);
		vwLogVerbose((_T("createWindowRuleDialog returned\n")));
	}
	else if ((dialogHWnd != NULL))
	{
		if ((theWin != NULL) && !add)
		{
			vwMutexLock();
			wt = NULL;
			if ((win = vwWindowFind(theWin)) != NULL)
				wt = (vwWindowRule *)win->zOrder[0];
			wt = vwWindowRuleFind(theWin, wt);
			vwMutexRelease();
			theWin = NULL;
		}

		if (GetForegroundWindow() != dialogHWnd) {
			// setup dialog has probably been lost under the windows raise it.
			setForegroundWin(dialogHWnd, 0);
			SetWindowPos(dialogHWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
				SWP_DEFERERASE | SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_NOMOVE);	}
		vwLogVerbose((_T("Sending new message rule\n")));
		updateWindowRuleDialog(hInst, hWnd, wt, theWin);
	}
	else {
		vwLogVerbose((_T("dialogHwnd was null, aborting\n"))); 
	}
	

}

static int
windowSetAlwaysOnTop(HWND theWin)
{
	int ExStyle ;
	
	vwLogBasic((_T("AlwaysOnTop window: %x\n"),(int) theWin)) ;
	if((theWin == NULL) && ((theWin = GetForegroundWindow()) == NULL))
		return 0 ;
	ExStyle = GetWindowLong(theWin,GWL_EXSTYLE) ;
	SetWindowPos(theWin,(ExStyle & WS_EX_TOPMOST) ? HWND_NOTOPMOST:HWND_TOPMOST,0,0,0,0,
				 SWP_DEFERERASE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOMOVE) ;
	return 1 ;
}

/************************************************
 * Force full redraw of a window
 * VW's changing of the Toolwin flag means the window can be drawn with the wrong frame which changes the size of the client area.
 * But forcing a full redraw is tricky as the main Windows redraw functions/flags do not address the client window resize issue.
 * To force the full redrawing shrink the window size by 1 pixel and then increase it again - I'm sure there's a
 * better solution but I've not found it */
static void
WindowFullRedraw(HWND theWin)
{
	RECT pos;
	GetWindowRect(theWin,&pos) ;
	SetWindowPos(theWin,0,0,0,pos.right-pos.left,pos.bottom-pos.top-1,(SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_DEFERERASE| SWP_NOSENDCHANGING)) ; 
	SetWindowPos(theWin,0,0,0,pos.right-pos.left,pos.bottom-pos.top,(SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_DEFERERASE| SWP_NOSENDCHANGING)) ; 
}

/************************************************
 * Moves a window to a given desk
 * move can be one of the following values:
 * 0 - Window will be moved on next desktop change
 * 1 - Window will be moved immediately
 * 2 - Window will be shown on the current desktop.
 */
static int
vwWindowSetDesk(vwWindow *win, int theDesk, vwUByte move, vwUByte setActive)
{
	vwWindow *ewin ;
	HWND activeHWnd ;
	vwUInt show ;
	
	activeHWnd = GetForegroundWindow() ;
	vwLogBasic((_T("Set window %x to desk %d. move: %d (Current active window: %x)\n"),(int) win->handle,theDesk,move,(int) activeHWnd)) ;
	
	ewin = (win->linkedNext != NULL) ? win:NULL ;
	do {
		if(vwWindowIsManaged(win))
		{
			if(vwWindowIsShownNotHung(win))
				win->zOrder[theDesk] = win->zOrder[currentDesk] ;
			else if(win->desk != theDesk)
				win->zOrder[theDesk] = win->zOrder[win->desk] ;
			if(vwWindowIsNotSticky(win) && ((win->desk != theDesk) || (vwWindowIsShow(win) && (theDesk != currentDesk))))
			{
				/* if temporarily show the window on this desk */ 
				if(move > 1)
				{
					if(theDesk == currentDesk)
						show = vwWINSH_FLAGS_SHOW ;
					else
						show = vwWINSH_FLAGS_HIDE ;
				}
				else
				{
					win->desk = theDesk;
					if((currentDesk == theDesk) || (move == 0))
						show = vwWINSH_FLAGS_SHOW ;
					else
						show = vwWINSH_FLAGS_HIDE ;
				}
				if(!show && (activeHWnd == win->handle))
					setActive = TRUE ;
				vwWindowShowHide(win,show) ;
			}
		}
		win = win->linkedNext ;
	} while(win != ewin) ;
	
	if(setActive)
	{
		// we have just assigned the foreground window to a different
		// desktop, we must find a replacement as leaving this one as the
		// active foreground window can lead to problems 
		vwUInt activeZOrder=0 ;
		activeHWnd = NULL ;
		win = windowList ;
		while(win != NULL)
		{
			if(vwWindowIsShow(win) && vwWindowIsShown(win) && 
			   vwWindowIsNotMinimized(win) && (win->zOrder[currentDesk] > activeZOrder))
			{
				activeHWnd = win->handle;
				activeZOrder = win->zOrder[currentDesk];
			}
			win = vwWindowGetNext(win) ;
		}
		vwLogVerbose((_T("Looking for replacement active: %x\n"),(int) activeHWnd)) ;
		setForegroundWin(activeHWnd,0) ;
	}
	return 1 ;
}

/************************************************
 * Sets a windows sticky setting
 */
static int
vwWindowSetSticky(vwWindow *win, int state)
{
	vwWindow *ewin ;
	vwUInt zOrder ;
	int ii ;
	
	ewin = (win->linkedNext != NULL) ? win:NULL ;
	do {
		if(vwWindowIsManaged(win))
		{
			if(state < 0) // toggle sticky state - set state so all owner windows are set correctly.
				state = vwWindowIsSticky(win) ^ TRUE;
			vwLogVerbose((_T("Setting Sticky: %x theWin = %x - %d -> %d\n"),(int) win->handle,
						  (int) 1,(int) vwWindowIsSticky(win),state)) ;
			if(state)
			{
				win->flags |= vwWINFLAGS_STICKY ;
				// set its zOrder of all desks to its zOrder on its current desk
				zOrder = win->zOrder[win->desk] ;
				ii = vwDESKTOP_SIZE - 1 ;
				do
					win->zOrder[ii] = zOrder ;
				while(--ii >= 0) ;
				// if not currently set to show (i.e. was on another desktop) then make it visible
				if(vwWindowIsNotShow(win))
					vwWindowShowHide(win,vwWINSH_FLAGS_SHOW) ;
			}
			else
				win->flags &= ~vwWINFLAGS_STICKY ;
			win->desk = currentDesk;
		}
		win = win->linkedNext ;
	} while(win != ewin) ;
	
	return 1 ;
}

/*************************************************
 * Callback function. Integrates all enumerated windows
 */
static int CALLBACK
enumWindowsProc(HWND hwnd, LPARAM lParam) 
{
	vwWindowRule *wt ;
	vwWindow *win ;
	vwWindowBase *wb, *pwb ;
	HWND phwnd ;
	vwUInt flags ;
	int style, exstyle ;
	RECT pos ;
	
	if((style = GetWindowLong(hwnd, GWL_STYLE)) & WS_CHILD)
		// Ignore all windows with child flag set
		return TRUE;
	if((wb=vwWindowBaseFind(hwnd)) == NULL)
	{
		if(hwnd == dialogHWnd)
			// Dont manage VW setup
			return TRUE ;
		
		wt = vwWindowRuleFind(hwnd,NULL) ;
		exstyle = GetWindowLong(hwnd, GWL_EXSTYLE) ;
		/* Criterias for a window to be handeled by VirtuaWin
		 * Note: some apps like winamp use the WS_EX_TOOLWINDOW flag to remove themselves from
		 * the taskbar so VW will manage toolwin windows if they are not popups or have owners
		 */
		if(hwnd == hWnd)
			flags = vwWINFLAGS_FOUND ;
		else if((wt != NULL) && (wt->flags & vwWTFLAGS_MANAGE))
			flags = 0 ;
		else if((wt != NULL) && (wt->flags & vwWTFLAGS_DONT_MANAGE))
			flags = vwWINFLAGS_FOUND|vwWINFLAGS_FORCE_NOT_MNGD ;
		else if((exstyle & WS_EX_TOOLWINDOW) && ((style & WS_POPUP) || (GetWindow(hwnd,GW_OWNER) != NULL)))
			flags = vwWINFLAGS_FOUND ;
		else if(((phwnd=GetParent(hwnd)) == NULL) || (phwnd == desktopHWnd) || ((pwb=vwWindowBaseFind(phwnd)) == NULL))
			flags = 0 ;
		else if(vwWindowIsManaged(pwb))
			flags = (vwWindowIsHideByMove(pwb)) ? 0:vwWINFLAGS_FOUND ;
		else if(((pwb)->flags & vwWINFLAGS_FORCE_NOT_MNGD) != 0)
			flags = vwWINFLAGS_FOUND|vwWINFLAGS_FORCE_NOT_MNGD ;
		else if(((phwnd=GetParent(phwnd)) != NULL) && ((pwb=vwWindowBaseFind(phwnd)) != NULL) && 
				vwWindowIsManaged(pwb) && vwWindowIsNotHideByMove(pwb))
			flags = vwWINFLAGS_FOUND ;
		else
			flags = 0 ;
			
		if(flags == 0)
		{
			if(style & WS_VISIBLE)
				// manage this window
				flags = vwWINFLAGS_FOUND | vwWINFLAGS_VISIBLE | vwWINFLAGS_WINDOW | vwWINFLAGS_MANAGED | vwWINFLAGS_SHOWN | vwWINFLAGS_SHOW ;
			else
				// would manage this window if it became visible so need to create a vwWindow for it
				flags = vwWINFLAGS_FOUND | vwWINFLAGS_WINDOW ;
			if(style & WS_MINIMIZE)
				flags |= vwWINFLAGS_MINIMIZED ;
			if(style & WS_MAXIMIZE)
				flags |= vwWINFLAGS_MAXIMIZED ;
		}
		if(((wb = vwWindowBaseCreate(flags,hwnd)) != NULL) && vwWindowIsWindow(wb))
		{
			win = (vwWindow *) wb ;
			win->exStyle = exstyle ;
			win->wt = wt ;
			//if((style & WS_VISIBLE) == 0)
				//vwLogBasic((_T("Got new unmanaged window %8x Proc %d Flg %x %x (%08x) %x\n"),
				//			(int)win->handle,(int)win->processId,(int)win->flags,(int) exstyle,(int)style,(int)win->wt)) ;
		}
		if(wt != NULL)
		{
			if(wt->flags & vwWTFLAGS_CLOSE)
			   SendMessageTimeout(hwnd,WM_CLOSE,0,0L,SMTO_ABORTIFHUNG|SMTO_BLOCK,100,NULL) ;
			if((wt->flags & vwWTFLAGS_ALWAYSONTOP) && ((exstyle & WS_EX_TOPMOST) == 0))
				windowSetAlwaysOnTop(hwnd) ;
		}
		return TRUE;
	}
	wb->flags |= vwWINFLAGS_FOUND ;
	if(vwWindowIsNotWindow(wb))
		return TRUE ;
	win = (vwWindow *) wb ;
	// vwLogVerbose((_T("Updating win %x Flgs %08x %08x %08x\n"),(int) hwnd,(int)win->flags,(int)win->exStyle,(int)style)) ;
	if(vwWindowIsNotManaged(win))
	{
		if((style & WS_VISIBLE) == 0)
			return TRUE ;
		/* window is visible, start to manage it... */
		vwWindowBaseUnlink(wb) ;
		wb->flags = (wb->flags & ~(vwWINFLAGS_INITIALIZED|vwWINFLAGS_MINIMIZED|vwWINFLAGS_MAXIMIZED)) | (vwWINFLAGS_VISIBLE | vwWINFLAGS_MANAGED | vwWINFLAGS_SHOWN | vwWINFLAGS_SHOW) ;
		if(style & WS_MINIMIZE)
			win->flags |= vwWINFLAGS_MINIMIZED ;
		if(style & WS_MAXIMIZE)
			win->flags |= vwWINFLAGS_MAXIMIZED ;
		win->exStyle = GetWindowLong(hwnd, GWL_EXSTYLE) ;
		memset(&(win->zOrder[1]),0,(vwDESKTOP_SIZE-1)*sizeof(vwUInt)) ;
		win->wt = vwWindowRuleFind(hwnd, win->wt);
		win->menuId = 0 ;
		vwLogBasic((_T("Started managing window %8x Proc %d Flg %x %x (%08x) %x\n"),
					(int)win->handle,(int)win->processId,(int)win->flags,(int)win->exStyle,(int)style,(int)win->wt)) ;
		vwWindowBaseLink(wb) ;
		return TRUE ;
	}
	if(!(style & WS_VISIBLE))
	{
		if(vwWindowIsVisible(win))
		{
			if(vwWindowIsShown(win))
			{
				/* this window has been hidden by someone else - stop handling it unless VirtuaWin
				 * knows its not visible (which means it is probably a hung process so keep it.) */
				vwLogBasic((_T("Stopped managing window %8x Proc %d Flg %x %x (%08x) Desk %d\n"),
							(int)win->handle,(int)win->processId,(int)win->flags,(int) win->exStyle,(int)style,(int)win->desk)) ;
				vwWindowBaseUnlink(wb) ;
				/* should check for visible children? */
				win->flags &= ~(vwWINFLAGS_MANAGED | vwWINFLAGS_VISIBLE) ;
				vwWindowBaseLink(wb) ;
			}
			else
				win->flags &= ~vwWINFLAGS_VISIBLE ;
		}
		else if(vwWindowIsShown(win) && vwWindowIsShow(win))
			/* This is likely to be a HUNG window, make it hung so we actively monitor it */
			win->flags &= ~vwWINFLAGS_SHOWN ;
			
	}
	else if(vwWindowIsNotShow(win))
	{
		if((style & WS_MINIMIZE) == 0)
		{
			if(vwWindowIsHideByMinim(win) || (vwWindowIsMinimized(win) && minWinHide))
			{
				vwLogBasic((_T("Got minim-window state change: %x %d (%d) %x -> %x\n"),
							(int) win->handle,win->desk,currentDesk,win->flags,style)) ;
				wb->flags |= vwWINFLAGS_ACTIVATED ;
			}
			else if(vwWindowIsHideByMove(win))
			{
				GetWindowRect(hwnd,&pos) ;
				if(pos.top >= -5000)
				{
					/* Something has moved this window back into a visible area (or
					 * at least outside VirtuaWins domain) so make it belong to this
					 * desktop, update the list entry, also check the location as the
					 * app may have only made the window visible */
					vwLogBasic((_T("Got move-window state change: %x %d (%d) %x -> %d %d\n"),
								(int) win->handle,win->desk,currentDesk,win->flags,(int)pos.left,(int)pos.top)) ;
					wb->flags |= vwWINFLAGS_ACTIVATED ;
				}
			}
			else if(vwWindowIsNotVisible(win))
			{
				/* Something has made this window visible so make it belong to this desktop, update the list entry,
				 * also check the location as the app may have only made the window visible */
				vwLogBasic((_T("Got window state change: %x %d (%d) %x\n"),
							(int) win->handle,win->desk,currentDesk,win->flags)) ;
				wb->flags |= vwWINFLAGS_ACTIVATED | vwWINFLAGS_VISIBLE ;
			}
			else if(vwWindowIsNotShown(win))
				/* This is likely to be a HUNG window, make it hung so we actively monitor it */
				win->flags |= vwWINFLAGS_SHOWN ;
		}
	}
	else if(vwWindowIsNotVisible(win))
		/* if visible and shown store the latest style & exStyle flags */
		win->flags |= vwWINFLAGS_VISIBLE ;
	else if(vwWindowIsShown(win))
	{
		if(((style & WS_MINIMIZE) != 0) ^ ((win->flags & vwWINFLAGS_MINIMIZED) != 0))
			win->flags ^= vwWINFLAGS_MINIMIZED ;
		if(((style & WS_MAXIMIZE) != 0) ^ ((win->flags & vwWINFLAGS_MAXIMIZED) != 0))
			win->flags ^= vwWINFLAGS_MAXIMIZED ;
		style = GetWindowLong(hwnd, GWL_EXSTYLE) ;
		if((win->exStyle & WS_EX_TOOLWINDOW) ^ (style & WS_EX_TOOLWINDOW))
			vwLogBasic((_T("TOOLWIN flag about to change! %x (%x -> %x)\n"),
						(int) win->handle,win->exStyle,style)) ;
		win->exStyle = style ;
	}
	return TRUE;
}



/*************************************************
* Central window list update procedure
*/
int
windowListUpdate(void)
{
	HWND activeHWnd ;
	vwWindowRule *wt ;
	vwWindowBase *wb, *wbn ;
	vwWindow *nw, *win, *ww ;
	RECT pos ;
	TCHAR cname[vwCLASSNAME_MAX], wname[vwWINDOWNAME_MAX] ;
	int newDesk=0, j, hungCount=0 ;
	
	//vwLogVerbose((_T("Updating winList fgw %x tpw %x\n"),(int) GetForegroundWindow(),(int) GetTopWindow(NULL))) ;
	/* We now own the mutex. */
	wb = windowBaseList ;
	while(wb != NULL)
	{
		wb->flags &= ~vwWINFLAGS_FOUND ;
		wb = vwWindowBaseGetNext(wb) ;
	}
	// Get all windows
	if(EnumWindows(enumWindowsProc,0) == 0)
	{
		vwLogBasic((_T("Call to EnumWindows failed: %x\n"),GetLastError())) ;
		/* best we can do is assume all windows are still there */
		wb = windowBaseList ;
		while(wb != NULL)
		{
			wb->flags |= vwWINFLAGS_FOUND ;
			wb = vwWindowBaseGetNext(wb) ;
		}
	}
	
	// finish the initialisation of new windows
	nw = windowList ;
	while(nw != NULL)
	{
		if((nw->flags & vwWINFLAGS_INITIALIZED) == 0)
			break ;
		nw = vwWindowGetNext(nw) ;
	}
	if(nw != NULL)
	{
		win = nw ;
		while (win != NULL)
		{
			wt = win->wt;
			win->desk = currentDesk;
			win->zOrder[currentDesk] = 1;
			if (wt != NULL)
			{
				win->flags |= (wt->flags & (vwWTFLAGS_HIDEWIN_MASK | vwWTFLAGS_HIDETSK_MASK | vwWTFLAGS_STICKY | vwWTFLAGS_MAIN_WIN | vwWTFLAGS_GROUP_APP | vwWTFLAGS_HWACT_MASK));
				if (vwWindowIsNotSticky(win) && (wt->flags & vwWTFLAGS_MOVE) && (desktopUsed[wt->desk] != 0))
				{
					win->flags |= (wt->flags & vwWINFLAGS_MOVE_IMMEDIATE);
					win->desk = wt->desk;
				}
			}
			win = vwWindowGetNext(win);
		}
		// now finish of initialization of linked windows
		win = nw ;
		while(win != NULL)
		{
			HWND owner = GetWindow(win->handle,GW_OWNER) ;
			if((win->exStyle & (WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE)) || ((owner != NULL) && ((win->exStyle & WS_EX_APPWINDOW) == 0)))
				win->flags |= vwWINFLAGS_NO_TASKBAR_BUT ;
			if(win->processNext != NULL)
			{
				if(owner != NULL)
				{
					/* link the owner if it is from the same process */
					ww = win->processNext ;
					do {
						if(ww->handle == owner)
						{
							if(vwWindowIsWindow(ww) && (ww->flags & vwWINFLAGS_FOUND) && vwWindowIsShown(ww))
							{
								vwWindowLink(ww,win) ;
								if(vwWindowIsNotHideByHide(win) && vwWindowIsHideByHide(ww) && vwWindowIsShown(ww))
								{
									// if an owned window is flagged as hidden by move or minimize we must make the parent window
									// hide by a non-hide method otherwise the call to ShowOwnedPopups is likely to break things
									ww->flags = (ww->flags & ~vwWINFLAGS_HIDEWIN_MASK) | vwWTFLAGS_HIDEWIN_MOVE ;
									if((ww->flags & vwWINFLAGS_HIDETSK_MASK) == vwWINFLAGS_HIDETSK_HIDE)
										ww->flags |= vwWINFLAGS_HIDETSK_TOOLWN ;
									vwLogBasic((_T("Making %x parent window %x hide by move or minim\n"),(int) win->handle,(int) ww->handle)) ;
								}
							}
							break ;
						}
						else if((ww = ww->processNext) == win)
							owner = GetWindow(owner,GW_OWNER) ;
					} while(owner != NULL) ;
				}
				if(win->flags & vwWINFLAGS_MINIMIZED)
				{
					/* some apps like Excel and Adobe Reader create one main window
					 * and a minimized hidden tricky position window per file open for the
					 * taskbar, if this is one make it owned by the main window */
					ww = win->processNext ;
					do {
						if((ww->flags & (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWTFLAGS_MAIN_WIN)) == (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWTFLAGS_MAIN_WIN))
						{
							vwLogBasic((_T("Making %x the main window of %x\n"),(int) ww->handle,(int) win->handle)) ;
							ww->flags |= vwWINFLAGS_NO_TASKBAR_BUT | vwWINFLAGS_RM_TASKBAR_BUT ;
							vwWindowLink(ww,win) ;
							break ;
						}
					} while((ww = ww->processNext) != win) ;
				}
				/* now see if any of this window's process' windows have
				 * already been initialized with an GROUP_APP flag */
				ww = win->processNext ;
				do {
					if((ww->flags & (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_GROUP_APP|vwWINFLAGS_INITIALIZED)) == (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_GROUP_APP|vwWINFLAGS_INITIALIZED))
					{
						vwLogBasic((_T("Linking %x to app %d window %x\n"),(int) win->handle,win->processId,(int) ww->handle)) ;
						vwWindowLink(ww,win) ;
						break ;
					}
				} while((ww = ww->processNext) != win) ;
				
				if((ww == win) && (win->flags & vwWINFLAGS_GROUP_APP))
				{
					/* first window of this app to get the GROUP_APP flag, link all the initialized & managed windows together */
					vwLogBasic((_T("Linking all window %x app windows\n"),(int) win->handle,win->processId)) ;
					ww = win->processNext ;
					do {
						if((ww->flags & (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_INITIALIZED)) == (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_INITIALIZED))
							vwWindowLink(ww,win) ;
					} while((ww = ww->processNext) != win);
				}
				if(vwWindowIsNotSticky(win) && ((ww=win->linkedNext) != NULL))
				{
					/* if one of the linked windows is sticky then make them all sticky */
					do {
						if((ww->flags & (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_STICKY)) == (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_STICKY))
						{
							ww->flags |= vwWINFLAGS_STICKY ;
							break ;
						}
					} while((ww = ww->linkedNext) != win) ;
					if(ww == win)
					{
						/* not sticky, look for an initialized window and set this
						 * window's desk to be the initialized window set activated
						 * flag if not currently on the current desktop */
						ww = win->linkedNext ;
						do {
							if((ww->flags & (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_INITIALIZED)) == (vwWINFLAGS_FOUND|vwWINFLAGS_MANAGED|vwWINFLAGS_INITIALIZED))
							{
								if((win->desk = ww->desk) != currentDesk)
									win->flags |= vwWINFLAGS_ACTIVATED ;
								break ;
							}
						} while((ww = ww->linkedNext) != win) ;
					}
				}
			}
			win = vwWindowGetNext(win) ;
		}
		/* finally we can apply any auto stick or assignments */
		win = nw ;
		while(win != NULL)
		{
			win->flags |= vwWINFLAGS_INITIALIZED ;
			if(vwWindowIsSticky(win))
			{
				/* flagged as sticky in cfg or linked window is sticky */ 
				win->desk = currentDesk ;
				vwWindowSetSticky(win,TRUE) ;
			}
			else if(win->desk != currentDesk)
			{
				j = win->desk ;
				win->desk = currentDesk ;
				if(j > nDesks)
					vwWindowSetDesk(win,j,1,FALSE) ;
				else
				{
					vwUByte moveImmediately = ((initialized == 0) || (win->flags & vwWINFLAGS_MOVE_IMMEDIATE)) ;
					vwWindowSetDesk(win,j,moveImmediately,FALSE);
					if(moveImmediately)
					{
						// set this window's zorder on the new desktop so it will be top
						if(newDesk == 0)
						{
							win->zOrder[j] = ++vwZOrder ;
							if((((win->flags & vwWINFLAGS_HWACT_MASK) == 0) && (hiddenWindowAct == 3)) ||
							   ((win->flags & vwWINFLAGS_HWACT_MASK) == ((3+1) << vwWINFLAGS_HWACT_BITROT)))
								newDesk = j ;
						}
						else
							win->zOrder[j] = vwZOrder-1 ;
					}
				}
			}
			win = vwWindowGetNext(win) ;
		}
		if(vwLogEnabled())
		{
			win = nw ;
			while(win != NULL)
			{
				GetWindowRect(win->handle,&pos) ;
				GetClassName(win->handle,cname,vwCLASSNAME_MAX);
				if(!GetWindowText(win->handle,wname,vwWINDOWNAME_MAX))
					_tcscpy_s(wname, 128, vwWTNAME_NONE);
				vwLogBasic((_T("Got new window %8x %08x %08x Flg %x Desk %d Proc %d %x Link %x Pos %d %d\n  Class \"%s\" Title \"%s\"\n"),
							(int)win->handle,(int)GetWindowLong(win->handle, GWL_STYLE),(int)win->exStyle,
							(int)win->flags,(int)win->desk,(int)win->processId,(int)((win->processNext == NULL) ? 0:win->processNext->handle),
							(int)((win->linkedNext == NULL) ? 0:win->linkedNext->handle),(int) pos.left,(int) pos.top,cname,wname)) ;
				win = vwWindowGetNext(win) ;
			}
		}
	}
	
	// remove windows that have gone.
	// Note that when a window is closed it takes a while for the window to
	// disappear, windows will then find another app to make current and the
	// order of events is fairly random. The problem for us is that if
	// windows selects a hidden app (i.e. on another desktop) it is very
	// difficult to differentiate between this and a genuine pop-up event.
	if(((activeHWnd = GetForegroundWindow()) == NULL) || (activeHWnd == hWnd))
		lastFGHWnd = activeHWnd = NULL ;
	else if(activeHWnd == lastFGHWnd)
		activeHWnd = NULL ;
	wb = windowBaseList ;
	while(wb != NULL)
	{
		wbn = vwWindowBaseGetNext(wb) ;
		if((wb->flags & vwWINFLAGS_FOUND) == 0)
		{
			if(wb->flags & vwWINFLAGS_WINDOW)
				vwLogBasic((_T("Lost window %8x Flg %x\n"),(int) wb->handle,wb->flags)) ;
			vwWindowBaseDelete(wb) ;
		}
		wb = wbn ;
	}
	
	// Handle the re-assignment of any popped up window, set the zorder and count hung windows
	win = windowList ;
	while(win != NULL)
	{
		if(win->flags & vwWINFLAGS_ACTIVATED)
		{
			vwUByte activateAction ;
			/* if not hiding minimized windows the an activation of a minimized window should always move the window */
			if((minWinHide & 0x02) && vwWindowIsMinimized(win))
				activateAction = 1 ;
			else if((win->flags & vwWINFLAGS_HWACT_MASK) == 0)
				activateAction = hiddenWindowAct ;
			else
				activateAction = ((win->flags & vwWINFLAGS_HWACT_MASK) >> vwWINFLAGS_HWACT_BITROT) - 1 ;
			win->flags &= ~vwWINFLAGS_ACTIVATED ;
			if((win->desk != currentDesk) && ((activateAction == 0) || (win->desk > nDesks)))
			{
				vwLogBasic((_T("Ignore SWin Activation: %x (%d %d %x %x)\n"),(int) win->handle,(int) activateAction,(int) win->desk,win->flags,minWinHide)) ;
				j = win->desk ;
				/* remove any link as we need to push this window back in isolation */
				nw = win->linkedNext ;
				win->linkedNext = NULL ;
				/* must officially bring it to this desktop first to restore flags etc, then push it back */
				vwWindowSetDesk(win,currentDesk,2,FALSE) ;
				vwWindowSetDesk(win,j,1,FALSE) ;
				win->linkedNext = nw ;
				if(win->handle == activeHWnd)
					activeHWnd = NULL ;
			}
			else
			{
				vwUInt flags=win->flags ; 
				vwLogBasic((_T("Got SWin Activation: %x (%d %d %x %x)\n"),(int) win->handle,(int) activateAction,(int) win->desk,win->flags,minWinHide)) ;
				if((activateAction == 3) && (newDesk == 0))
					newDesk = win->desk ;
				vwWindowSetDesk(win,currentDesk,activateAction,FALSE) ;
				if(((flags & (vwWINFLAGS_SHOWN|vwWINFLAGS_HIDETSK_MASK)) == vwWINFLAGS_HIDETSK_TOOLWN) || (vwWindowIsMinimized(win) && minWinHide))
					/* VW's setting of the Toolwin flag means the frame will have been drawn wrong, force a redraw. */
					WindowFullRedraw(win->handle) ;
			}
		}
		if(win->handle == activeHWnd)
		{
			vwUByte activateAction ;
			if((j = (win->flags & vwWINFLAGS_HWACT_MASK)) == 0)
				activateAction = hiddenWindowAct ;
			else
				activateAction = (j >> vwWINFLAGS_HWACT_BITROT) - 1 ;

			//Experimental: review the rules to make sure we're not on the wrong desk. 
			wt = vwWindowRuleFind(activeHWnd, NULL);
			if (wt != NULL)
				win->wt = wt;
			if (aggressiveRules && (wt != NULL) && vwWindowIsNotSticky(win) && (wt->flags & vwWTFLAGS_MOVE) 
				&& (desktopUsed[wt->desk] != 0) && (wt->desk != win->desk)) {
				vwLogBasic((_T("Moving window %x from desktop = %d to match rule desktop = %d.\n"), (int)activeHWnd, (int)win->desk, (int)wt->desk));
				vwWindowSetDesk(win, wt->desk, 1, TRUE);
				setForegroundWin(NULL, 0);
			}
			else if ((win->desk != currentDesk) && ((activateAction == 0) || (win->desk > nDesks)))
			{
				vwLogBasic((_T("Ignore NWin Activation: %x (%d %d %x %x)\n"),(int) activeHWnd,(int) activateAction,(int) win->desk,win->flags,minWinHide)) ;
				setForegroundWin(NULL,0) ;
			}
			else
			{
				vwLogBasic((_T("NWin Activation: %x (action: %d, desk: %d, flags: %4x, minWinHide: %x)\n"),(int) activeHWnd,(int) activateAction,(int) win->desk,win->flags,minWinHide)) ;
				if(vwWindowIsNotShown(win) && activateAction)
				{
					if((activateAction == 3) && (newDesk == 0))
						newDesk = win->desk ;
					vwWindowSetDesk(win,currentDesk,activateAction,FALSE) ;
					if((win->flags & vwWINFLAGS_HIDETSK_MASK) == vwWINFLAGS_HIDETSK_TOOLWN)
						/* VW's setting of the Toolwin flag means the frame will have been drawn wrong, force a redraw. */
						WindowFullRedraw(win->handle) ;
				}
				win->zOrder[currentDesk] = ++vwZOrder ;
				// if this is only a temporary display increase its zorder in its main desk
				if(win->desk != currentDesk)
					win->zOrder[win->desk] = vwZOrder ;
			}



		}
		if((win->flags & (vwWINFLAGS_ELEVATED_TEST|vwWINFLAGS_ELEVATED)) == vwWINFLAGS_ELEVATED)
		{
			/* we use a dummy WM_NCHITTEST message to check if we have access */
			DWORD rr ;
			if(SendMessageTimeout(win->handle,WM_NCHITTEST,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,50,&rr))
				win->flags = (win->flags & ~vwWINFLAGS_ELEVATED) | vwWINFLAGS_ELEVATED_TEST ;
			else if(GetLastError() == ERROR_ACCESS_DENIED)
			{
				/* failure was not a timeout - consider this an elevated window . May need to
				 * change this to test the error is ERROR_ACCESS_DENIED */
				win->flags |= vwWINFLAGS_ELEVATED_TEST ;
				vwLogBasic((_T("Found window %x elevated: %x"),win->handle,win->flags)) ;
			}
		}
		if(vwWindowIsShow(win))
		{
			if(vwWindowIsNotShown(win))
				hungCount++ ;
		}
		else if(vwWindowIsShown(win) &&
				((win->flags & (vwWINFLAGS_ELEVATED|vwWINFLAGS_ELEVATED_TEST)) != (vwWINFLAGS_ELEVATED|vwWINFLAGS_ELEVATED_TEST)))
			hungCount++ ;
		win = vwWindowGetNext(win) ;
	}
	if(activeHWnd != NULL)
		lastFGHWnd = activeHWnd ;
	vwLogVerboser((_T("Update winList.  hung: %d, new desk: %d, fgwin: %x, topwin: %x \n"), hungCount, newDesk, (int) GetForegroundWindow(),(int) GetTopWindow(NULL)));
	return ((newDesk << 16) | hungCount) ;
}

/*************************************************
 * Makes all windows visible
 */
static void
vwWindowShowAll(vwUShort shwFlags)
{
	vwWindow *win ;
	vwMutexLock();
	windowListUpdate() ;
	win = windowList ;
	while(win != NULL)
	{
		// still ignore windows on a private desktop unless exiting (vwWINSH_FLAGS_TRYHARD)
		if((win->desk <= nDesks) || ((shwFlags & 0x09) == vwWINSH_FLAGS_TRYHARD))
		{
			win->desk = currentDesk ;
			vwWindowShowHide(win,shwFlags|vwWINSH_FLAGS_SHOW);
		}
		win = vwWindowGetNext(win) ;
	}
	vwMutexRelease();
}

/************************************************
 * Does necessary stuff before shutting down
 */
static void
shutDown(void)
{
	// Remove the timer & tell all modules to quit
	KillTimer(hWnd, 0x29a);
	if(vwHookInstalled)
		vwHookUninstallFunc() ;
	vwModulesPostMessage(MOD_QUIT, 0, 0);
	vwHotkeyUnregister(1);
	// gather all windows quickly & remove icon
	vwWindowShowAll(0) ;
	Shell_NotifyIcon(NIM_DELETE, &nIconD);
	// try harder to gather remaining ones before exiting
	vwWindowShowAll((vwUShort) (taskButtonAct|vwWINSH_FLAGS_TRYHARD)) ;
	// Delete loaded icon resource
	DeleteObject(checkIcon);
	if(taskbarShrdMem)
	{
		if(osVersion < OSVERSION_NT)
			VirtualFree(taskbarShrdMem,0,MEM_RELEASE) ;
		else
			VirtualFreeEx(taskbarProcHdl,taskbarShrdMem,0,MEM_RELEASE);
		taskbarShrdMem = NULL;
	}
	if(taskbarProcHdl == NULL)
	{
		CloseHandle(taskbarProcHdl);
		taskbarProcHdl = NULL;
	}
	PostQuitMessage(0);
}

#define vwProcessMemoryRead(hProcess,lpBaseAddress,lpBuffer,nSize,rSize) \
((ReadProcessMemory(hProcess,lpBaseAddress,lpBuffer,nSize,&rSize) != 0) && (rSize == nSize))

static int
vwTaskbarButtonListUpdate(void)
{
	int ii, jj, tbCount=0, itemCount, bgsCount, psz=4 ;
	vwUByte empty = ' '; 
	vwUByte *dp1, *dp2 = &empty, *bgs = &empty;
	DWORD rSize;
	TCITEM tcItem;
	TBBUTTON tbItem;
	HWND tbHWnd;
	
	if(taskbarBCType == vwTASKBAR_BC_WIN7)
	{
		/* Win7: Msg result -> the dpa -> button groups -> button group -> buttons */
	if((dp1 = (vwUByte *) GetWindowLong(taskbarBCHWnd,DWL_MSGRESULT)) == NULL)
		{
			vwLogBasic((_T("Win7BC Err0: %d\n"),GetLastError())) ;
			return 0 ;
		}
		if(osVersion & OSVERSION_64BIT)
		{
			dp1 += DPA_DELTA_64 ;
			psz = 8 ;
		}
		else
			dp1 += DPA_DELTA_32 ;
		if(!vwProcessMemoryRead(taskbarProcHdl,dp1,&dp2,sizeof(void *),rSize) || (dp2 == NULL) || // the dpa
		   !vwProcessMemoryRead(taskbarProcHdl,dp2,&bgsCount, sizeof(int),rSize) || // button groups count and pointer
		   !vwProcessMemoryRead(taskbarProcHdl,dp2+psz,&bgs, sizeof(void *),rSize))
		{
			vwLogBasic((_T("Win7BC Err1: %p %p -> %d %p\n"),dp1,dp2,bgsCount,bgs)) ;
			return 0 ;
		}
		itemCount = 0 ;
		for(ii=0; ii<bgsCount; ii++)
		{
			if(!vwProcessMemoryRead(taskbarProcHdl,bgs+(ii*psz),&dp1,sizeof(void *),rSize) || // button group
			   !vwProcessMemoryRead(taskbarProcHdl,dp1+(6*psz),&jj, sizeof(int),rSize)) // button group type
			{
				vwLogBasic((_T("Win7BC Err2: %d %p -> %p %d\n"),ii,bgs,dp1,jj)) ;
				return 0 ;
			}
			else if((jj == 1) || (jj == 3))
			{
				if(!vwProcessMemoryRead(taskbarProcHdl,dp1+(5*psz),&dp2,sizeof(void *),rSize) ||
				   !vwProcessMemoryRead(taskbarProcHdl,dp2,&jj,sizeof(int),rSize)) // button count
				{
					vwLogBasic((_T("Win7BC Err3: %d %p -> %p %d\n"),ii,bgs,dp1,jj)) ;
					return 0 ;
				}
				itemCount += jj ;
			}
		}
	}
	else if(taskbarBCType == vwTASKBAR_BC_TABCONTROL)
		itemCount = TabCtrl_GetItemCount(taskbarBCHWnd) ;
	else
		itemCount = SendMessage(taskbarBCHWnd,TB_BUTTONCOUNT,0,0) ;
	if(itemCount <= 0)
		return 0 ;
	if(itemCount >= taskbarButtonListSize)
	{
		if(taskbarButtonList != NULL)
			free(taskbarButtonList) ;
		taskbarButtonListSize = itemCount + 0x10 ;
		taskbarButtonList = malloc(taskbarButtonListSize * sizeof(HWND)) ;
		if(taskbarButtonList == NULL)
		{
			taskbarButtonListSize = 0 ;
			return 0 ;
		}
	}
	if(taskbarBCType == vwTASKBAR_BC_WIN7)
	{
		for(ii=0; ii<bgsCount; ii++)
		{
			if(!vwProcessMemoryRead(taskbarProcHdl,bgs+(ii*psz),&dp1,sizeof(void *),rSize) || // button group
			   !vwProcessMemoryRead(taskbarProcHdl,dp1+(6*psz),&jj, sizeof(int),rSize)) // button group type
			{
				vwLogBasic((_T("Win7BC Err4: %d %p -> %p %d\n"),ii,bgs,dp1,jj)) ;
				return 0 ;
			}
			else if((jj == 1) || (jj == 3))
			{
				if(!vwProcessMemoryRead(taskbarProcHdl,dp1+(5*psz),&dp2,sizeof(void *),rSize) || 
				   !vwProcessMemoryRead(taskbarProcHdl,dp2,&itemCount,sizeof(int),rSize) ||  // button count
				   !vwProcessMemoryRead(taskbarProcHdl,dp2+(1*psz),&dp1,sizeof(void *),rSize)) // buttons
				{
					vwLogBasic((_T("Win7BC Err5: %d %p -> %p -> %d %p\n"),ii,bgs,dp2,itemCount,dp1)) ;
					return 0 ;
				}
				for(jj=0; jj<itemCount; jj++)
				{
					if(vwProcessMemoryRead(taskbarProcHdl,dp1+(jj*psz),&dp2,sizeof(void *),rSize) && 
					   vwProcessMemoryRead(taskbarProcHdl,dp2+(3*psz),&dp2,sizeof(void *),rSize) &&
					   vwProcessMemoryRead(taskbarProcHdl,dp2+psz,&tbHWnd,sizeof(HWND),rSize))
					   taskbarButtonList[tbCount++] = tbHWnd ;
				}
			}
		}
	}
	else if(taskbarBCType == vwTASKBAR_BC_TABCONTROL)
	{
		for(ii = 0 ; ii < itemCount ; ++ii)
		{
			tcItem.mask = TCIF_PARAM;
			if(WriteProcessMemory(taskbarProcHdl,taskbarShrdMem,&tcItem,sizeof(TCITEM),NULL) &&
			   TabCtrl_GetItem(taskbarBCHWnd,ii,taskbarShrdMem) &&
			   vwProcessMemoryRead(taskbarProcHdl,taskbarShrdMem,&tcItem,sizeof(TCITEM),rSize) &&
			   (tcItem.lParam != 0))
				taskbarButtonList[tbCount++] = (HWND) tcItem.lParam ;
		}
	}
	else
	{
		for(ii = 0 ; ii < itemCount ; ++ii)
		{
			if(SendMessage(taskbarBCHWnd,TB_GETBUTTON,ii,(LPARAM)taskbarShrdMem) &&
			   vwProcessMemoryRead(taskbarProcHdl,taskbarShrdMem,&tbItem,sizeof(TBBUTTON),rSize))
			{
				if(osVersion & OSVERSION_64BIT)
				{
					/* On 64bit OS there is an extra 4 bytes padding so the dwData shifts into the iString member,
					 * but this may not work all the time as the pointer should be 64bit and this assumes the higher
					 * 32 bits are 0 */
					if(vwProcessMemoryRead(taskbarProcHdl,(LPCVOID) tbItem.iString,&tbHWnd,sizeof(HWND),rSize) &&
					   (tbHWnd != NULL))
						taskbarButtonList[tbCount++] = tbHWnd ;
				}
				else if(vwProcessMemoryRead(taskbarProcHdl,(LPCVOID) tbItem.dwData,&tbHWnd,sizeof(HWND),rSize) &&
						(tbHWnd != NULL))
					taskbarButtonList[tbCount++] = tbHWnd ;
			}
		}
	}
	taskbarButtonList[tbCount] = NULL ;
	return tbCount ;
}
/************************************************
 * Callback for new window assignment and taskbar fix (removes bogus reappearances of tasks on win9x).
 * Run a few housekeeping tasks for the first 
 */
static VOID CALLBACK
monitorTimerProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	static vwUInt mtCount=0 ;
	vwWindow *win ;
	int ii, hungCount ;
	
	timerCounter++ ;
	if(((deskIconHWnd == NULL) || ((hwnd=taskHWnd) == NULL)) && (noTaskbarCheck == 0) && ((timerCounter & 0x3) == 0))
	{
		vwTaskbarHandleGet() ;
		if((hwnd == NULL) && (taskHWnd != NULL))
			vwIconSet(currentDesk,0) ;
		if(timerCounter >= 20)
		{
			if(taskHWnd == NULL)
				MessageBox(hWnd,_T("Could not locate handle to the taskbar.\n This will disable the ability to hide troublesome windows correctly."),vwVIRTUAWIN_NAME _T(" Error"), 0); 
			noTaskbarCheck = 2 ;
		}
	}
	vwMutexLock();
	if((timerCounter == 2) && taskbarBCType && useDynButtonRm)
	{
		/* check the taskbar buttons in the taskbar are correct */
		int tbCount ;
		tbCount = vwTaskbarButtonListUpdate() ;
#ifdef vwLOG_VERBOSE
		if(vwLogEnabled())
		{
			vwLogBasic((_T("Timer Taskbar-Buttons %d:"),tbCount)) ;
			for(ii = 0 ; ii<tbCount ; ii++)
				_ftprintf(vwLogFile,_T(" %x"),taskbarButtonList[ii]) ;
			_ftprintf(vwLogFile,_T("\n")) ;
			fflush(vwLogFile) ;
		}
#endif
		for(ii = 0 ; ii<tbCount ; ii++)
		{
			if(((win = vwWindowFind(taskbarButtonList[ii])) != NULL) &&
			   ((win->flags & (vwWINFLAGS_NO_TASKBAR_BUT|vwWINFLAGS_RM_TASKBAR_BUT)) == (vwWINFLAGS_NO_TASKBAR_BUT|vwWINFLAGS_RM_TASKBAR_BUT)) &&
			   vwWindowIsShownNotHung(win))
				PostMessage(taskHWnd,RM_Shellhook,HSHELL_WINDOWDESTROYED,(LPARAM) win->handle) ;
		}
	}
	hungCount = windowListUpdate() ;
	if((ii = (hungCount >> 16)) > 0)
	{
		vwMutexRelease();
		changeDesk(ii,MOD_CHANGEDESK);
		return ;
	}
	if(hungCount > 0)
	{
		/* there's a hung process, try to handle it - every second to start
		 * with, after 32 seconds only try once every 8 seconds */
		mtCount++ ;
		ii = (mtCount & ~0x7f) ? 31:3;
		if((mtCount & ii) == 0)
		{
			win = windowList ;
			while(win != NULL)
			{
				if(vwWindowIsShow(win))
				{
					if(vwWindowIsNotShown(win))
						vwWindowShowHide(win,vwWINSH_FLAGS_SHOW) ;
				}
				else if(vwWindowIsShown(win) &&
						((win->flags & (vwWINFLAGS_ELEVATED|vwWINFLAGS_ELEVATED_TEST)) != (vwWINFLAGS_ELEVATED|vwWINFLAGS_ELEVATED_TEST)) &&
						vwWindowShowHide(win,0) && ((win->flags & vwWINFLAGS_ELEVATED_TEST) == 0))
				{
					/* UAC support - VW may not have the required privileges to hide the window -
					 * we don't know as we have not tested this window. So if the window is shown
					 * and we're trying to hide it and vwWindowShowHide returns not-hung the flag
					 * the window as potentially elevated and assume we can't hide it so don't
					 * complain */
					win->flags |= vwWINFLAGS_ELEVATED ;
					hungCount-- ;
				}
				win = vwWindowGetNext(win) ;
			}
			/* flash the icon for half a second each time we try */
			vwIconSet(currentDesk,0-hungCount) ;
		}
		else if(((mtCount-1) & ii) == 0)
		{
			/* restore the flashing icon */
			vwIconSet(currentDesk,hungCount) ;
		}
	}
	else if(mtCount)
	{
		/* hung process problem has been resolved. */
		mtCount = 0 ;
		vwIconSet(currentDesk,0);
	}

	if(taskbarFixRequired)
	{
		win = windowList ;
		while(win != NULL)
		{
			if(vwWindowIsNotHideByHide(win) && vwWindowIsNotShown(win))
				PostMessage(taskHWnd, RM_Shellhook, HSHELL_WINDOWDESTROYED, (LPARAM) win->handle);
			win = vwWindowGetNext(win) ;
		}
	}
	vwMutexRelease();
}

/*************************************************
 * Helper function for all the step* functions below
 * Does the actual switching work 
 */
static int
ichangeDeskProc(int newDesk, WPARAM msgWParam)
{
	HWND activeHWnd, lzh ;
	vwWindow *win, *bwn ;
	vwUInt activeZOrder=0, cno, czo, bno, bzo, lno, lzo ;
	int notHung ;
	
	vwLogBasic((_T("Step Desk Start: %d -> %d (%d,%x)\n"),currentDesk,newDesk,isDragging,(int)dragHWnd)) ;
	
	vwMutexLock();
	windowListUpdate() ;
	if(taskbarBCType && (timerCounter >= 4))
	{
		vwWindow *nwin, *pwin ;
		int ii, jj, tbCount ;
		tbCount = vwTaskbarButtonListUpdate() ;
#if 0
		if(vwLogEnabled())
		{
			vwLogBasic((_T("Step Desk Taskbar-Buttons %d:"),tbCount)) ;
			for(ii = 0 ; ii<tbCount ; ii++)
				_ftprintf(vwLogFile,_T(" %x"),taskbarButtonList[ii]) ;
			_ftprintf(vwLogFile,_T("\n")) ;
			fflush(vwLogFile) ;
		}
#endif
		ii = 0 ;
		win = windowList ;
		while(win != NULL)
		{
			nwin = vwWindowGetNext(win) ;
			if(vwWindowIsShownNotHung(win))
			{
				if(win->handle == taskbarButtonList[ii])
				{
					win->flags &= ~vwWINFLAGS_NO_TASKBAR_BUT ;
					ii++ ;
				}
				else
				{
					jj = tbCount ;
					while((--jj >= 0) && (taskbarButtonList[jj] != win->handle))
						;
					if(jj < 0)
						win->flags |= vwWINFLAGS_NO_TASKBAR_BUT ;
					else
					{
						win->flags &= ~vwWINFLAGS_NO_TASKBAR_BUT ;
						
						pwin = NULL ;
						if(((jj == 0) ||
							((pwin = vwWindowFind(taskbarButtonList[jj-1])) == NULL)) &&
						   ((jj+1) < tbCount) &&
						   ((bwn = vwWindowFind(taskbarButtonList[jj+1])) != NULL))
						{
							if(bwn == windowList)
								pwin = (vwWindow *) windowBaseList ;
							else
								pwin = windowList ;
							while(pwin->next != bwn)
								pwin = pwin->next ;
						}
						if((pwin != NULL) && (pwin != win) && (pwin->next != win)) 
						{
							if(win == windowList)
							{
								windowList = win->next ;
								bwn = (vwWindow *) windowBaseList ;
							}
							else
								bwn = windowList ;
							while(bwn->next != win)
								bwn = bwn->next ;
							bwn->next = win->next ;
							win->next = pwin->next ;
							pwin->next = win ;
							if(win->next == windowList)
								windowList = win ;
						}
					}
				}
			}
			win = nwin ;
		}
	}
	timerCounter = 0 ;
	
	activeHWnd = NULL;
	if(isDragging || (checkMouseState(1) == 1))
	{
		/* move the app we are dragging to the new desktop, must handle linked windows */
		/* keep isDragging == 1 if using a mouse to drag */
		isDragging = (dragHWnd == 0) ;
		if(((dragHWnd != 0) || ((dragHWnd = GetForegroundWindow()) != 0)) &&
		   ((win = vwWindowFind(dragHWnd)) != NULL))
		{
			bwn = (win->linkedNext == NULL) ? NULL:win ;
			do {
				win->desk = newDesk;
				win->zOrder[newDesk] = win->zOrder[currentDesk] ;
			} while((win = win->linkedNext) != bwn) ;
			activeZOrder = 0xffffffff ;
			activeHWnd = dragHWnd ;
		}
		else
			isDragging = 0 ;
		dragHWnd = NULL ;
	}
	if(dialogOpen)
		storeDesktopProperties() ;
	
	/* must lose the current focus and regain at the end to get things right, however this must be
	 * compromised if using the mouse to drag a window as this window must not lose focus or it
	 * will be dropped */
	if(!isDragging)
		setForegroundWin(hWnd,0) ;
	isDragging = 0 ;
	
	currentDesk = newDesk;
	/* Calculations for getting the x and y positions */
	currentDeskY = ((currentDesk - 1)/nDesksX) + 1 ;
	currentDeskX = nDesksX + currentDesk - (currentDeskY * nDesksX);
	
	if(preserveZOrder == 1)
	{
		win = windowList ;
		while(win != NULL)
		{
			if(vwWindowIsSticky(win))
				win->desk = currentDesk ;
			if((win->desk != currentDesk) && vwWindowIsShown(win))
				// Hide these, not iconic or "Switch minimized" enabled
				vwWindowShowHide(win,vwWINSH_FLAGS_HIDE) ;
			win = vwWindowGetNext(win) ;
		}
		lzo = lno = 0xffffffff ;
		lzh = HWND_NOTOPMOST ;
		for(;;)
		{
			bwn = NULL ;
			bzo = bno = cno = 0 ;
			win = windowList ;
			while(win != NULL)
			{
				if((win->desk == currentDesk) && ((win->exStyle & WS_EX_TOPMOST) == 0) &&
				   ((czo=win->zOrder[currentDesk]) >= bzo) && ((czo < lzo) || ((czo == lzo) && (cno < lno))))
				{
					bwn = win ;
					bzo = czo ;
					bno = cno ;
				}
				win = vwWindowGetNext(win) ;
				cno++ ;
			}
			if(bwn == NULL)
				break ;
			// Show these windows
			if(vwWindowIsNotShown(bwn))
				notHung = vwWindowShowHide(bwn,vwWINSH_FLAGS_SHOW) ;
			else
				notHung = windowIsNotHung(bwn->handle,100) ;
			vwLogVerbose((_T("ZOrder: %d %d %x -> %d %d %x (%d)\n"),lno,lzo,lzh,bno,bzo,bwn->handle,notHung)) ;
			if(notHung)
			{
				SetWindowPos(bwn->handle,lzh,0,0,0,0,SWP_DEFERERASE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOMOVE) ;
				lzh = bwn->handle ;
			}
			else
				vwLogBasic((_T("ZOrder: %8x - HUNG\n"),(int) bwn->handle)) ;
			if((activeHWnd == NULL) && ((bwn->flags & (vwWINFLAGS_SHOWN|vwWINFLAGS_SHOW|vwWINFLAGS_MINIMIZED)) == (vwWINFLAGS_SHOWN|vwWINFLAGS_SHOW)))
			{
				activeHWnd = bwn->handle;
				activeZOrder = bwn->zOrder[currentDesk];
			}
			lno = bno ;
			lzo = bzo ;
		}
	}
	else
	{
		bno = 0 ;
		win = windowList ;
		while(win != NULL)
		{
			if(vwWindowIsSticky(win))
				win->desk = currentDesk ;
			if(win->desk == currentDesk)
			{
				// Show these windows
				if(vwWindowIsShown(win))
				{
					if(vwWindowIsNotShow(win))
					{
						/* currently hung */
						win->flags |= vwWINFLAGS_SHOW ;
					}
					else
					{
						if((win->zOrder[currentDesk] > activeZOrder) && vwWindowIsNotMinimized(win))
						{
							activeHWnd = win->handle;
							activeZOrder = win->zOrder[currentDesk];
						}
						if(bno && ((win->flags & vwWINFLAGS_NO_TASKBAR_BUT) == 0) && (taskHWnd != NULL))
						{
							/* It is visible but its position in the taskbar is wrong - fix by removing from the taskbar and then adding again */
							PostMessage(taskHWnd,RM_Shellhook,HSHELL_WINDOWDESTROYED,(LPARAM) win->handle) ;
							Sleep(1);
							PostMessage(taskHWnd,RM_Shellhook,HSHELL_WINDOWCREATED,(LPARAM) win->handle) ;
						}
					}
				}
				else if(vwWindowIsNotShow(win) &&
						vwWindowShowHide(win,vwWINSH_FLAGS_SHOW))
				{
					bno = 1 ;
					if((win->zOrder[currentDesk] > activeZOrder) && vwWindowIsNotMinimized(win))
					{
						activeHWnd = win->handle;
						activeZOrder = win->zOrder[currentDesk];
					}
				}
			}
			else if(vwWindowIsShown(win))
				// Hide these, not iconic or "Switch minimized" enabled
				vwWindowShowHide(win,vwWINSH_FLAGS_HIDE) ;
			win = vwWindowGetNext(win) ;
		}
		if((preserveZOrder == 2) || (preserveZOrder == 4))
		{
			// very small sleep to allow system to catch up
			Sleep(1);
			
			lzo = lno = 0xffffffff ;
			lzh = HWND_NOTOPMOST ;
			for(;;)
			{
				bwn = NULL ;
				bzo = 0 ;
				cno = 0 ;
				win = windowList ;
				while(win != NULL)
				{
					if((win->desk == currentDesk) && ((win->exStyle & WS_EX_TOPMOST) == 0) &&
					   ((czo=win->zOrder[currentDesk]) >= bzo) && ((czo < lzo) || ((czo == lzo) && (cno < lno))))
					{
						bwn = win ;
						bzo = czo ;
						bno = cno ;
					}
					win = vwWindowGetNext(win) ;
					cno++ ;
				}
				if(bwn == NULL)
					break ;
				// Show these windows
				notHung = windowIsNotHung(bwn->handle,100) ;
				vwLogVerbose((_T("TBZOrder: %d %d %x -> %d %d %x (%d)\n"),lno,lzo,lzh,bno,bzo,bwn->handle,notHung)) ;
				if(notHung)
				{
					SetWindowPos(bwn->handle,lzh,0,0,0,0,SWP_DEFERERASE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOSENDCHANGING|SWP_NOMOVE) ;
					lzh = bwn->handle ;
				}
				else
					vwLogBasic((_T("TBZOrder: %8x - HUNG\n"),(int) bwn->handle)) ;
				lzo = bzo ;
				lno = bno ;
			}
		}
	}
	if(releaseFocus)
		activeHWnd = NULL ;
	vwLogBasic((_T("Active found: %x (%d,%d)\n"),(int) activeHWnd,(int)activeZOrder,releaseFocus)) ;
	/* Take a quick sleep here to give everything a chance to catch up, particularly explorer */
	Sleep(1) ;
	setForegroundWin(activeHWnd,TRUE) ;
	
	if(useDskChgModRelease && (activeHWnd != 0))
	{
		/* assumes the left key is pressed which could lead to problems */
		if(HIWORD(GetAsyncKeyState(VK_MENU)))
		{
			PostMessage(activeHWnd,WM_KEYUP,VK_MENU,(LPARAM) 0xC0380001) ;
			PostMessage(activeHWnd,WM_KEYUP,VK_MENU,(LPARAM) 0xC1380001) ;
		}
		if(HIWORD(GetAsyncKeyState(VK_CONTROL)))
		{
			PostMessage(activeHWnd,WM_KEYUP,VK_CONTROL,(LPARAM) 0xC01D0001) ;
			PostMessage(activeHWnd,WM_KEYUP,VK_CONTROL,(LPARAM) 0xC11D0001) ;
		}
		if(HIWORD(GetAsyncKeyState(VK_SHIFT)))
		{
			PostMessage(activeHWnd,WM_KEYUP,VK_SHIFT,(LPARAM) 0xC02A0001) ;
			PostMessage(activeHWnd,WM_KEYUP,VK_SHIFT,(LPARAM) 0xC0360001) ;
		}
		if(HIWORD(GetAsyncKeyState(VK_LWIN)))
			PostMessage(activeHWnd,WM_KEYUP,VK_LWIN,(LPARAM) 0xC15B0001) ;
		if(HIWORD(GetAsyncKeyState(VK_RWIN)))
			PostMessage(activeHWnd,WM_KEYUP,VK_RWIN,(LPARAM) 0xC15C0001) ;
	}
	
	/* reset the monitor timer to give the system a chance to catch up first */
	SetTimer(hWnd, 0x29a, 250, monitorTimerProc);
	vwMutexRelease();
	
	if(dialogOpen)
	{
		initDesktopProperties() ;
		showSetup() ;
	}
	vwIconSet(currentDesk,0) ;
	if(refreshOnWarp)
		RedrawWindow( NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
	
	vwModulesPostMessage(MOD_CHANGEDESK,msgWParam,currentDesk);
	if(taskHWnd != NULL)
	{
		/* this strange piece of code resolves bad pop-up problems, by changing the style
		 * the VW window gets back onto Explorer's window list at a higher point than any
		 * other tricky hidden window so when explorer is looking for a replacement it will
		 * choose VW rather than any other hidden window so now we can easily determine if
		 * a pop-up is a real pop-up or explorer looking for a replacement */
		SetWindowLong(hWnd, GWL_EXSTYLE,0) ;
		SetWindowLong(hWnd, GWL_EXSTYLE,WS_EX_TOOLWINDOW) ;
		PostMessage(taskHWnd, RM_Shellhook, HSHELL_WINDOWDESTROYED, (LPARAM) hWnd);
	}
	vwLogBasic((_T("Step Desk End (%x)\n"),(int)GetForegroundWindow())) ;
	
	return currentDesk ;
}

static VOID CALLBACK
ichangeDeskTimeoutProc(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	int newDesk = ichangeDesk ;
	ichangeDesk = -1  ;
	KillTimer(hWnd,0x29b) ;
	
	if((newDesk > 0) && (newDesk != currentDesk))
		ichangeWParam = ichangeDeskProc(newDesk,ichangeWParam) ;
	else
		ichangeWParam = 0 ;
	ichangeDesk = 0  ;
}

static int
changeDesk(int newDesk, WPARAM msgWParam)
{
	MSG msg ;
	
	ichangeDesk = 0 ;
	KillTimer(hWnd,0x29b) ;
	
	if(newDesk == currentDesk)
		// Nothing to do
		return 0;
	
	/* don't bother updating last desktop or generating an image unless the
	 * user has been on the desk for at least a second */
	if(timerCounter >= 4)
	{
		if(lastDesk != currentDesk)
			lastDesk = currentDesk ;
		if(deskImageEnabled > 0)
			createDeskImage(currentDesk,0) ;
	}
	else if(lastDeskNoDelay && (lastDesk != currentDesk))
		lastDesk = currentDesk ;
	
	if((ichangeHWnd == NULL) || isDragging || (currentDesk > nDesks) || (newDesk > nDesks))
		return ichangeDeskProc(newDesk,msgWParam) ;
		
	vwLogBasic((_T("IChange Desk: %d -> %d (%d,%x)\n"),currentDesk,newDesk,msgWParam,(int)ichangeHWnd)) ;
	ichangeDesk = newDesk ;
	ichangeWParam = msgWParam ;
	PostMessage(ichangeHWnd,VW_ICHANGEDESK,currentDesk,newDesk);
	SetTimer(hWnd, 0x29b, 250, ichangeDeskTimeoutProc);
	while(ichangeDesk && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return ichangeWParam ;
}

/*************************************************
 * Goto a specified desktop specifying desk number
 */
int
gotoDesk(int theDesk, vwUByte force)
{
	if((theDesk <= 0) || (theDesk >= vwDESKTOP_SIZE) || ((theDesk > nDesks) && !force))
		return 0;
	
	return changeDesk(theDesk,MOD_CHANGEDESK);
}

/*************************************************
 * Goto a specified desktop specifying desk number
 */
static int
stepDelta(int delta)
{
	int newDesk ;
	if(currentDesk > nDesks)
		/* on a private desktop - go to first if delta is +ve, last otherwise */
		newDesk = (delta < 0) ? nDesks:1 ;
	else if((newDesk=currentDesk+delta) < 1)
	{
		if(!deskWrap)
			return 0 ;
		newDesk = nDesks ;
	}
	else if(newDesk > nDesks)
	{
		if(!deskWrap)
			return 0 ;
		newDesk = 1 ;
	}
	return changeDesk(newDesk,MOD_CHANGEDESK);
}


/*************************************************
 * Step on desk to the right
 */
static int
stepRight(void)
{
	int deskX, deskY=currentDeskY ;
	
	if(currentDesk > nDesks)
	{   /* on a private desktop - go to first */
		deskX = 1;
		deskY = 1;
	}
	else if((deskX=currentDeskX + 1) > nDesksX)
	{
		if(!deskWrap)
			return 0;
		deskX = 1;
	}
	return changeDesk(vwCalculateDesk(deskX,deskY),MOD_STEPRIGHT);
}

/*************************************************
 * Step one desk to the left
 */
static int
stepLeft(void)
{
	int deskX, deskY=currentDeskY ;
	
	if(currentDesk > nDesks)
	{   /* on a private desktop - go to last */
		deskX = nDesksX;
		deskY = nDesksY;
	}
	else if((deskX=currentDeskX - 1) < 1)
	{
		if(!deskWrap)
			return 0;
		deskX = nDesksX;
	}
	return changeDesk(vwCalculateDesk(deskX,deskY),MOD_STEPLEFT);
}

/*************************************************
 * Step one desk down
 */
static int
stepDown(void)
{
	int deskX=currentDeskX, deskY ;
	
	if(currentDesk > nDesks)
	{   /* on a private desktop - go to first */
		deskX = 1;
		deskY = 1;
	}
	else if((deskY = currentDeskY + 1) > nDesksY)
	{
		if(!deskWrap)
			return 0;
		deskY = 1;
	}
	return changeDesk(vwCalculateDesk(deskX,deskY),MOD_STEPDOWN);
}

/*************************************************
 * Step one desk up
 */
static int
stepUp(void)
{
	int deskX=currentDeskX, deskY ;
	
	if(currentDesk > nDesks)
	{   /* on a private desktop - go to last */
		deskX = nDesksX;
		deskY = nDesksY;
	}
	else if((deskY = currentDeskY - 1) < 1)
	{
		if(!deskWrap)
			return 0;
		deskY = nDesksY;
	}
	return changeDesk(vwCalculateDesk(deskX,deskY),MOD_STEPUP);
}


/*************************************************
 * Pull out the menu handling code
 */ 
#include "vwMenus.c"

static int
vwHotkeyExecute(vwUByte command, vwUByte desk, vwUByte modifier)
{
	HWND hwnd, theWin = NULL ;
	
	if(modifier & vwHOTKEY_WIN_MOUSE)
	{
		POINT pt;
		GetCursorPos(&pt);
		if((theWin = WindowFromPoint(pt)) == NULL)
			return 0 ;
		while((GetWindowLong(theWin, GWL_STYLE) & WS_CHILD) && 
			  ((hwnd = GetParent(theWin)) != NULL) && (hwnd != desktopHWnd))
			theWin = hwnd ;
	}
	switch(command)
	{
	case vwCMD_NAV_MOVE_LEFT:
		return stepLeft();
	case vwCMD_NAV_MOVE_RIGHT:
		return stepRight();
	case vwCMD_NAV_MOVE_UP:
	case vwCMD_NAV_MOVE_DOWN:
		if((command == vwCMD_NAV_MOVE_UP) ^ (invertY != 0))
			return stepUp();
		return stepDown();
	case vwCMD_NAV_MOVE_PREV:
		return stepDelta(-1) ;
	case vwCMD_NAV_MOVE_NEXT:
		return stepDelta(1) ;
	case vwCMD_NAV_MOVE_DESKTOP:
		return gotoDesk(desk,TRUE);
	case vwCMD_WIN_STICKY:
		return windowSetSticky(theWin,-1);
	case vwCMD_WIN_DISMISS:
		return windowDismiss(theWin);
	case vwCMD_WIN_MOVE_DESKTOP:
	case vwCMD_WIN_MOVE_DESK_FOL:
		return assignWindow(theWin,desk,(vwUByte) (command == vwCMD_WIN_MOVE_DESK_FOL),TRUE,FALSE);
	case vwCMD_WIN_MOVE_PREV:
	case vwCMD_WIN_MOVE_PREV_FOL:
		return assignWindow(theWin,VW_STEPPREV,(vwUByte) (command == vwCMD_WIN_MOVE_PREV_FOL),TRUE,FALSE);
	case vwCMD_WIN_MOVE_NEXT:
	case vwCMD_WIN_MOVE_NEXT_FOL:
		return assignWindow(theWin,VW_STEPNEXT,(vwUByte) (command == vwCMD_WIN_MOVE_NEXT_FOL),TRUE,FALSE);
	case vwCMD_UI_WINMENU_STD:
		popupWindowMenu(GetForegroundWindow(),0x10);
		break ;
	case vwCMD_UI_WINMENU_CMP:
		popupWindowMenu(GetForegroundWindow(),0x11);
		break ;
	case vwCMD_UI_WINLIST_STD:
		popupWinListMenu(hWnd,0x14) ;
		break ;
	case vwCMD_UI_WINLIST_CMP:
		popupWinListMenu(hWnd,0x15) ;
		break ;
	case vwCMD_UI_WINLIST_MRU:
		popupWinListMenu(hWnd,0x16) ;
		break ;
	case vwCMD_UI_SETUP:
		showSetup();
		break ;
	case vwCMD_WIN_PUSHTOBOTTOM:
		return windowPushToBottom(theWin) ;
	case vwCMD_WIN_ALWAYSONTOP:
		return windowSetAlwaysOnTop(theWin) ;
	case vwCMD_WIN_GATHER_ALL:
		vwWindowShowAll(0);
		break ;
	case vwCMD_WIN_REAPPLY_RULES:
		vwWindowRuleReapply() ;
		break ;
	case vwCMD_UI_WTYPE_SETUP:
		showWindowRule(theWin,0) ;
		break ;
	case vwCMD_UI_CTLMENU_CMP:
		return popupControlMenu(hWnd,0x11) ;
	case vwCMD_WIN_GATHER:
		windowGather(theWin);
		break ;
	case vwCMD_WIN_MOVE_LEFT:
	case vwCMD_WIN_MOVE_LEFT_FOL:
		return assignWindow(theWin,VW_STEPLEFT,(vwUByte) (command & 0x01),TRUE,FALSE);
	case vwCMD_WIN_MOVE_RIGHT:
	case vwCMD_WIN_MOVE_RIGHT_FOL:
		return assignWindow(theWin,VW_STEPRIGHT,(vwUByte) (command & 0x01),TRUE,FALSE);
	case vwCMD_WIN_MOVE_UP:
	case vwCMD_WIN_MOVE_UP_FOL:
	case vwCMD_WIN_MOVE_DOWN:
	case vwCMD_WIN_MOVE_DOWN_FOL:
		if((command >= vwCMD_WIN_MOVE_DOWN) ^ (invertY == 0))
			return assignWindow(theWin,VW_STEPUP,(vwUByte) (command & 0x01),TRUE,FALSE);
		return assignWindow(theWin,VW_STEPDOWN,(vwUByte) (command & 0x01),TRUE,FALSE);
	case vwCMD_UI_ENABLESTATE:
		return vwToggleEnabled(0) ;
	case vwCMD_NAV_MOVE_LAST:
		{
			int nld ; 
			if(lastDesk == currentDesk)
				return 0 ;
			nld = currentDesk ;
			gotoDesk(lastDesk,FALSE);
			lastDesk = nld ;
			break ;
		}
	case vwCMD_UI_SYSTRAYICON:
		taskbarIconShown ^= 0x02 ;
		vwIconSet(currentDesk,0) ;
		return (taskbarIconShown & 0x01) ;
	case vwCMD_UI_BOSS_KEY:
		return vwToggleEnabled(1) ;
	case vwCMD_UI_UNBOSS_KEY:
		return vwToggleEnabled(2) ;
	case vwCMD_UI_EXIT:
		shutDown() ;
		break ;
	case vwCMD_WIN_BRINGTOTOP:
		if(theWin == NULL)
			return 0 ;
		setForegroundWin(theWin,1) ;
		break ;
	case vwCMD_UI_CTLMENU_STD:
		return popupControlMenu(hWnd,0x10) ;
	default:
		return 0 ;
	}
	return 1 ;
}

/*************************************************
 * Main window callback, this is where all main window messages are taken care of
 */
static LRESULT CALLBACK
wndProc(HWND aHWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case VW_MOUSEWARP:
		// Is virtuawin enabled
		if(vwIsEnabled())
		{
			POINT pt;
			GetCursorPos(&pt);
			
			isDragging = (LOWORD(lParam) & 0x1) ;
			switch(HIWORD(lParam))
			{
			case 0:
				/* left */
				if((stepLeft() != 0) && (LOWORD(lParam) & 0x2))
				{
					if(mouseWarp)
						SetCursorPos(desktopWorkArea[0][2] - mouseJumpLength, pt.y);
					else
						SetCursorPos(pt.x + mouseJumpLength, pt.y);
				}
				break;
				
			case 1:
				/* up */
				if(invertY)
					wParam = stepDown();
				else
					wParam = stepUp();
				if((wParam != 0) && (LOWORD(lParam) & 0x2))
				{
					if(mouseWarp)
						SetCursorPos(pt.x, desktopWorkArea[0][3] - mouseJumpLength);
					else
						SetCursorPos(pt.x, pt.y + mouseJumpLength);
				}
				break;
				
			case 2:
				/* right */
				if((stepRight() != 0) && (LOWORD(lParam) & 0x2))
				{
					if(mouseWarp)
						SetCursorPos(desktopWorkArea[0][0] + mouseJumpLength, pt.y);
					else
						SetCursorPos(pt.x - mouseJumpLength, pt.y);
				}
				break;
				
			case 3:
				/* down */
				if(invertY)
					wParam = stepUp();
				else
					wParam = stepDown();
				if((wParam != 0) && (LOWORD(lParam) & 0x2))
				{
					if(mouseWarp)
						SetCursorPos(pt.x, desktopWorkArea[0][1] + mouseJumpLength);
					else
						SetCursorPos(pt.x, pt.y - mouseJumpLength);
				}
				break;
			
			case 4:
				/* window list */
				popupWinListMenu(aHWnd,(HIWORD(GetKeyState(VK_CONTROL))) ? 6:(HIWORD(GetKeyState(VK_SHIFT))) ? (winListCompact ^ 5):(winListCompact | 4)) ;
				break;
			case 5:
				/* window menu */
				popupWindowMenu(GetForegroundWindow(),winMenuCompact);
				break;
			case 6:
				/* previous */
				stepDelta(-1) ;
				break;
			case 7:
				/* next */
				stepDelta(1) ;
				break;
			}
			/* reset isDragging to 0 as the call to step desk may have failed and will 
			 * result in a window being dragged on next change if left set to 1 */
			isDragging = 0 ;
		}
		return TRUE;
		
	case WM_HOTKEY:				// A hot key was pressed
		{
			int ii = hotkeyCount ;
			while(--ii >= 0)
				if(hotkeyList[ii].atom == wParam)
					break ;
			if(ii < 0)
				return FALSE ;
			vwHotkeyExecute(hotkeyList[ii].command,hotkeyList[ii].desk,hotkeyList[ii].modifier) ;
			return TRUE ;
		}
		// Plugin messages
	case VW_CHANGEDESK: 
		if(vwIsDisabled())
			return FALSE ;
		switch (wParam)
		{
		case VW_STEPPREV:
			return stepDelta(-1) ;
		case VW_STEPNEXT:
			return stepDelta(1) ;
		case VW_STEPLEFT:
			return stepLeft();
		case VW_STEPRIGHT:
			return stepRight();
		case VW_STEPUP:
			return stepUp();
		case VW_STEPDOWN:
			return stepDown();
		default:
			return gotoDesk(wParam,FALSE);
		}
		return FALSE ;
		
	case VW_CLOSE: 
		shutDown();
		return TRUE;
		
	case VW_SETUP:
		if(vwIsDisabled())
			return FALSE ;
		showSetup();
		return TRUE;
		
	case VW_DELICON:
		taskbarIconShown |= 0x02 ;
		vwIconSet(currentDesk,0) ;
		return TRUE;
		
	case VW_SHOWICON:
		taskbarIconShown &= ~0x02 ;
		vwIconSet(currentDesk,0) ;
		return TRUE;
		
	case VW_HELP:
		showHelp(aHWnd,NULL) ;
		return TRUE;
		
	case VW_GATHER:
		vwWindowShowAll(0);
		return TRUE;
		
	case VW_DESKX:
		return nDesksX;
		
	case VW_DESKY:
		return nDesksY;
		
	case VW_CURDESK:
		return currentDesk;
		
	case VW_ASSIGNWIN:
		if(vwIsDisabled())
			return FALSE ;
		if(lParam < 0)
			return assignWindow((HWND) wParam,0-lParam,TRUE,FALSE,FALSE);
		return assignWindow((HWND) wParam,(vwUByte) lParam,FALSE,FALSE,FALSE);
		
	case VW_ACCESSWIN:
		if(vwIsDisabled())
			return FALSE ;
		return accessWindow((HWND)wParam,(int) lParam,FALSE);
		
	case VW_SETSTICKY:
		if(vwIsDisabled())
			return FALSE ;
		return windowSetSticky((HWND)wParam,(int) lParam);
		
	case VW_WINGETINFO:
		{
			int ii ;
			vwWindowBase *win ;
			vwMutexLock();
			if((win = vwWindowBaseFind((HWND)wParam)) == NULL)
				ii = 0;
			else
			{
				ii = (int) (win->flags & 0x00ffffff) ;
				if(vwWindowIsManaged(win))
					ii |= (int) (((vwUInt) (((vwWindow *) win)->desk)) << 24) ;
			}
			vwMutexRelease();
			return ii ;
		}
		
	case VW_FOREGDWIN:
		if(wParam == 0)
		{
			HWND activeHWnd=NULL ;
			vwUInt activeZOrder=0 ;
			vwWindow *win ;
			
			vwMutexLock();
			
			win = windowList ;
			while(win != NULL)
			{
				if(vwWindowIsShow(win) && vwWindowIsShown(win) && 
				   vwWindowIsNotMinimized(win) && (win->zOrder[currentDesk] > activeZOrder))
				{
					activeHWnd = win->handle;
					activeZOrder = win->zOrder[currentDesk];
				}
				win = vwWindowGetNext(win) ;
			}
			setForegroundWin(activeHWnd,0) ;
			vwMutexRelease();
			return 1 ;
		}
		else if(lParam < 0)
		{
			int ii ;
			if((ii=IsWindow((HWND)wParam)) != 0)
				setForegroundWin((HWND)wParam,0);
			return ii ;
		}
		else
		{
			int ii ;
			vwWindow *win ;
			vwMutexLock();
			if((win = vwWindowFind((HWND)wParam)) != NULL)
			{
				/* found the window, if visible bring to the front otherwise set the zorder */
				if(lParam == 0)
					lParam = currentDesk ;
				if((lParam == currentDesk) && vwWindowIsShown(win))
					setForegroundWin((HWND)wParam,0);
				win->zOrder[lParam] = ++vwZOrder ;
				ii = 1 ;
			}
			else
				ii = 0 ;
			vwMutexRelease();
			return ii ;
		}
	
	case VW_WINLIST:
		/* Send over the window list with WM_COPYDATA - retired
		 * This message was too dependent on the Window data structure, creating modules which
		 * are very version dependent. As to v4.0 support for this message has been removed,
		 * Module writers are encouraged to use the VW_WINGETINFO message instead see SF bug
		 * 1915723 for more information */
		return 0 ;
		
	case VW_INSTALLPATH:
	case VW_USERAPPPATH:
		{
			// Send over the VirtuaWin install path with WM_COPYDATA
			//  - always use a byte string so unicode/non-uncode modules can work together
			COPYDATASTRUCT cds;
			DWORD ret ;
			char *ss = (message == VW_INSTALLPATH) ? VirtuaWinPathStr:UserAppPathStr ;
			cds.dwData = 0 - message ;
			cds.cbData = strlen(ss) + 1 ;
			cds.lpData = (void*)ss;
			if(wParam == 0)
			{
				vwModulesSendMessage(WM_COPYDATA, (WPARAM) aHWnd, (LPARAM)&cds); 
				ret = TRUE ;
			}
			else if(!SendMessageTimeout((HWND)wParam,WM_COPYDATA,(WPARAM) aHWnd,(LPARAM) &cds,SMTO_ABORTIFHUNG|SMTO_BLOCK,10000,&ret))
				ret = 0 ;
			vwLogVerbose((_T("Sent path %d to %d, returning %d\n"),message,(int) wParam, ret)) ;
			return ret ;
		}
	
	case VW_DESKIMAGE:
		if(wParam == 0)
			return deskImageCount ;
		else if(wParam == 1)
			return enableDeskImage(lParam) ;
		else if(wParam == 6)
			return desktopWorkArea[0][3]-desktopWorkArea[0][1]+1 ;
		else if(wParam == 7)
			return desktopWorkArea[0][2]-desktopWorkArea[0][0]+1 ;
		else if(deskImageCount == 0)
			return 0 ;
		else if(wParam == 2)
			return disableDeskImage(1) ;
		else if(wParam == 3)
			return createDeskImage(currentDesk,0) ;
		else if(wParam == 4)
			return deskImageInfo.bmiHeader.biHeight ;
		else if(wParam == 5)
			return deskImageInfo.bmiHeader.biWidth ;
		else if(wParam == 8)
			deskImageEnabled = lParam ;
		else
			return 0 ;
		
	case VW_ENABLE_STATE:
		lParam = vwIsEnabled() ;
		if((wParam == 1) || ((wParam == 2) && vwIsEnabled()) || ((wParam == 3) && vwIsDisabled()))
			vwToggleEnabled(0) ;
		return lParam ;
		
	case VW_DESKNAME:
		{
			// Send over the VirtuaWin install path with WM_COPYDATA
			//  - always use a byte string so unicode/non-uncode modules can work together
			COPYDATASTRUCT cds;
			DWORD ret ;
#ifdef _UNICODE
			char buff[128] ;
#endif
			if(wParam == 0)
				ret = FALSE ;
			else
			{
				ret = TRUE ;
				cds.dwData = 0 - message ;
				if(lParam == 0)
					lParam = currentDesk ;
				if((lParam >= 0) && (lParam < vwDESKTOP_SIZE) &&
				   (desktopName[lParam] != NULL))
				{
#ifdef _UNICODE
					if(!WideCharToMultiByte(CP_ACP,0,desktopName[lParam],-1,(char *) buff,128, 0, 0))
						ret = FALSE ;
					else
					{
						cds.cbData = strlen(buff) + 1 ;
						cds.lpData = (void *) buff ;
					}
#else
					cds.cbData = strlen(desktopName[lParam]) + 1 ;
					cds.lpData = (void *) desktopName[lParam] ;
#endif
				}
				else
				{
					cds.cbData = 0 ;
					cds.lpData = (void*) NULL ;
				}
				if(ret && !SendMessageTimeout((HWND)wParam,WM_COPYDATA,(WPARAM) aHWnd,(LPARAM) &cds,SMTO_ABORTIFHUNG|SMTO_BLOCK,10000,&ret))
					ret = FALSE ;
			}
			vwLogVerbose((_T("Sent deskname %d to %d, returning %d\n"),(int) lParam,(int) wParam, ret)) ;
			return ret ;
		}
		
	case VW_DESKTOP_SIZE:
		return vwDESKTOP_SIZE;
		
	case VW_WINMANAGE:
		return windowSetManage((HWND)wParam,(int) lParam);
		
	case VW_HOTKEY:
		return vwHotkeyExecute((vwUByte) wParam,(vwUByte) LOWORD(lParam),(vwUByte) HIWORD(lParam)) ;
		
	case VW_CMENUITEM:
		{
			vwMenuItem *mi, *pi, *ni ;
			vwUShort pos = (vwUShort) lParam ;
			HWND module = (HWND) wParam ;            
			
			pi = NULL ;
			mi = ctlMenuItemList ;
			while(mi != NULL)
			{
				ni = mi->next ;
				if((mi->module == module) && ((pos == 0) || (mi->position == pos)))
				{
					if(pi == NULL)
						ctlMenuItemList = ni ;
					else
						pi->next = ni ;
					free(mi) ;
				}
				else
					pi = mi ;
				mi = ni ;
			}
			return TRUE ;
		}
	
	case VW_ICHANGEDESK:
		if(wParam == 0)
			return (LRESULT) ichangeHWnd ;
		else if(wParam == 3)
		{
			if(ichangeDesk > 0)
				SetTimer(hWnd, 0x29b, 1, ichangeDeskTimeoutProc);
		}
		else if(wParam == 1)
		{
			if((ichangeHWnd == NULL) || (ichangeHWnd == (HWND) lParam))
			{
				ichangeHWnd = (HWND) lParam ;
				return 2 ;
			}
			return 1 ;
		}
		else if((wParam == 2) && (ichangeHWnd == (HWND) lParam))
			ichangeHWnd = NULL ;
		return 0 ;
		
	case VW_APPLYRULES:
		vwWindowRuleReapply() ;
		return TRUE ;

	case VW_INVERTY:
		return invertY ;

		// End plugin messages
		
	case WM_CREATE:		       // when main window is created
		return TRUE;
		
	case WM_MOVE:
		/* ensure the VW window remains hidden */
		if(((short) HIWORD(lParam)) > -30000)
			SetWindowPos(aHWnd,0,10,-31000,0,0,(SWP_FRAMECHANGED | SWP_DEFERERASE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING)) ; 
		return 0 ;
		
	case WM_ENDSESSION:
		if(wParam)
			shutDown();
		return TRUE;
	
	case WM_CLOSE:
		if(MessageBox(aHWnd,_T("Really exit ") vwVIRTUAWIN_NAME _T("?"),vwVIRTUAWIN_NAME, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES)
			return FALSE ;
		break ;
		
	case WM_DESTROY:	  // when application is closed
		shutDown();            
		return TRUE;
		
	case VW_SYSTRAY:		   // We are being notified of mouse activity over the icon
		switch (lParam)
		{
		case WM_LBUTTONDOWN:               // Show the window list
			if(vwIsEnabled())
				popupWinListMenu(aHWnd,(HIWORD(GetKeyState(VK_CONTROL))) ? 2:(HIWORD(GetKeyState(VK_SHIFT))) ? (winListCompact ^ 1):winListCompact) ;
			break;
		
		case WM_LBUTTONDBLCLK:             // double click on icon
			if(vwIsEnabled())
				showSetup();
			break;
			
		case WM_MBUTTONUP:		   // Move to the next desktop
			if(vwIsEnabled())
				stepDelta((HIWORD(GetKeyState(VK_SHIFT))) ? -1:1) ;
			break;
			
		case WM_RBUTTONUP:		   // Open the control menu
			return popupControlMenu(aHWnd,ctlMenuCompact) ;
		}
		return TRUE;
		
	case WM_DISPLAYCHANGE:
		/* screen size has changed, get the new size and set the mouse work area */
		getScreenSize();
		if(deskImageCount > 0)
		{
			int hh = deskImageInfo.bmiHeader.biHeight ;
			deskImageInfo.bmiHeader.biHeight -= 1 ;
			enableDeskImage(hh) ;
		}
		/* no break */
	case WM_SETTINGCHANGE:
		/* the position and size of the taskbar may have changed */ 
		if(initialized)
			getWorkArea();
		return TRUE;
	case WM_MEASUREITEM:
		measureMenuItem(aHWnd,(MEASUREITEMSTRUCT*)lParam);
		break;
	case WM_DRAWITEM:
		renderMenuItem((DRAWITEMSTRUCT*)lParam);        
		break;
	
	case WM_COPYDATA:
		if(lParam != 0)
		{
			COPYDATASTRUCT *cds = (COPYDATASTRUCT *) lParam ;
			if(cds->dwData == VW_CMENUITEM)
			{
				vwMenuItemMsg *mim ;
				vwMenuItem *mi, *pi, *ni ;
				int ii ;
				
				if((cds->cbData > 4) && ((mim=cds->lpData) != NULL) &&
				   ((mi = malloc(sizeof(vwMenuItem))) != NULL))
				{
					mi->module = (HWND) wParam ;
					mi->submenu = NULL ;
					mi->position = mim->position ;
					mi->message = mim->message ;
					mi->id = 0 ;
#ifdef _UNICODE
					MultiByteToWideChar(CP_ACP,0,mim->label,-1,mi->label,vwMENU_LABEL_MAX) ;
#else
					strncpy(mi->label,mim->label,vwMENU_LABEL_MAX) ;
#endif
					mi->label[vwMENU_LABEL_MAX-1] = '\0' ;
					ii = 0 ;
					pi = NULL ;
					ni = ctlMenuItemList ;
					while(ni != NULL)
					{
						if(ni->position == mi->position)
						{
							if(ni->module == mi->module)
								ii = 1 ;
							else if(ii)
								break ;
						}
						else if(ni->position > mi->position)
							break ;
						pi = ni ;
						ni = ni->next ;
					}
					mi->next = ni ;
					if(pi == NULL)
						ctlMenuItemList = mi ;
					else
						pi->next = mi ;
				}
				return TRUE ;
			}
		}
		break ;
	default:
		// If taskbar restarted
		if(message == RM_TaskbarCreated)
		{
			taskbarIconShown &= ~0x01 ;
			vwTaskbarHandleGet();
			if(taskHWnd != NULL)
				vwIconSet(currentDesk,0) ;
			else if(deskIconHWnd == NULL)
				timerCounter = 0 ;
		}
		break;
	}
	return DefWindowProc(aHWnd, message, wParam, lParam);
}

static void 
vwCrashHandler(int sig)
{
	static vwUByte count=0 ;
	if(!count)
	{
		/* only attempt this once, the window list could be corrupt */
		count = 1 ;
		if(windowList != NULL)
		{
			vwWindow *win ;
			/* lock mutex if not already locked, but don't wait */
			WaitForSingleObject(hMutex,0) ;
			win = windowList ;
			while(win != NULL)
			{
				// still ignore windows on a private desktop unless exiting (vwWINSH_FLAGS_TRYHARD)
				if((win->desk <= nDesks) || ((taskButtonAct & 0x01) == 0))
				{
					win->desk = currentDesk ;
					vwWindowShowHide(win,vwWINSH_FLAGS_SHOW|vwWINSH_FLAGS_TRYHARD) ;
				}
				win = vwWindowGetNext(win) ;
			}
			vwLogBasic((_T("Received signal %d, terminating\n"),sig)) ;
		}
		exit(1) ;
	}
	else
		_exit(1) ;
}

	
#define WIN64_KEY "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"
int
vwWindowsIs64Bit(void)
{
	TCHAR buff[128] ;
	DWORD size, type;
	HKEY base_key;
	int ret=0 ;
	
	if(RegOpenKey(HKEY_LOCAL_MACHINE,_T(WIN64_KEY),&base_key) == ERROR_SUCCESS)
	{
		size = 128 ;
		if(RegQueryValueEx(base_key,_T("Identifier"),(LPDWORD) 0,&type,(LPBYTE)&buff,&size) == ERROR_SUCCESS)
		{
			if(!_tcsncmp(buff,_T("AMD64"),5) || !_tcsncmp(buff,_T("EM64T"),5) || !_tcsncmp(buff,_T("Intel64"),7))
				ret = OSVERSION_64BIT ;
		}
		RegCloseKey(base_key);
	}
	return ret ;
}

static VOID CALLBACK
VirtuaWinInitContinue(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
	/* finish off initialization and create the list of windows */
	DWORD threadID;
	
	/* make sure this is not called twice! */
	KillTimer(hWnd,0x29a);
	
	/* register message for explorer/systray crash restart (>=IE4) & taskbar window button manipulation */
	RM_TaskbarCreated = RegisterWindowMessage(_T("TaskbarCreated"));
	RM_Shellhook = RegisterWindowMessage(_T("SHELLHOOK"));
	
	vwHookSetup();
	vwHotkeyRegister(1);
	getScreenSize();
	getWorkArea();
	vwTaskbarHandleGet();
	vwIconSet(currentDesk,0) ;
	
	/* Create the thread responsible for mouse monitoring */   
	mouseThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) vwMouseProc, NULL, 0, &threadID); 	
	mouseEnabled = TRUE;
	enableMouse(mouseEnable);
	
	/* always move windows immediately on startup */
	vwMutexLock() ;
	windowListUpdate() ;
	vwMutexRelease() ;
	
	/* Load user modules */
	vwModulesLoad();
	
	SetTimer(hWnd,0x29a,1000,monitorTimerProc) ; 
	initialized = TRUE ;
}

static void
VirtuaWinInit(HINSTANCE hInstance, LPSTR cmdLine)
{

	OSVERSIONINFO os;
	HINSTANCE libHandle ; 
	WNDCLASSEX wc;
	hInst = hInstance;
	

#ifdef _WIN32_MEMORY_DEBUG
	/* Enable heap checking on each allocate and free */
	_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF|_CRTDBG_DELAY_FREE_MEM_DF|
					_CRTDBG_LEAK_CHECK_DF|_CRTDBG_DELAY_FREE_MEM_DF);
#endif
	/* Is this call to VirtuaWin just to send a message to an already ruinning VW? */
	cmdLine = strstr(cmdLine,"-msg") ;
	
	/* Only one instance may be started */
	hMutex = CreateMutex(NULL, FALSE, vwVIRTUAWIN_NAME _T("PreventSecond"));
	if((hMutex == (HANDLE) 0) || (GetLastError() == ERROR_ALREADY_EXISTS))
	{
		UINT message=VW_SETUP ;
		WPARAM wParam=0 ;
		LPARAM lParam=0 ;
		
		if((hWnd = vwFindWindow(vwVIRTUAWIN_CLASSNAME,NULL,0)) == NULL)
		{
			MessageBox(hWnd,_T("Failed to find ") vwVIRTUAWIN_NAME _T(" window."),vwVIRTUAWIN_NAME _T(" Error"),0) ;
			exit(-2) ;
		}
		/* get the message from the command-line, default is to display configuration window... */
		if(cmdLine != NULL)
			sscanf_s(cmdLine+4,"%i %i %li",&message,&wParam,&lParam) ;
		/* post message and quit */
		exit(SendMessage(hWnd,message,wParam,lParam)) ;
	}
	if(cmdLine != NULL)
		/* VW not running, can't post message, return error */
		exit(-1) ;
	
	/* install a crash handler to avoid loosing windows whenever possible */
	signal(SIGINT,vwCrashHandler);
	signal(SIGTERM,vwCrashHandler);
	signal(SIGILL,vwCrashHandler);
	signal(SIGABRT,vwCrashHandler);
	signal(SIGSEGV,vwCrashHandler);
	vwThread = GetCurrentThreadId() ;
	
	os.dwOSVersionInfoSize = sizeof(os);
	// GetVersionEx(&os);
	osVersion = OSVERSION_NT ;
	osVersion |= vwWindowsIs64Bit() ;
	
	if((libHandle = LoadLibrary(_T("psapi"))) != NULL)
#ifdef _UNICODE
		vwGetModuleFileNameEx = (vwGETMODULEFILENAMEEX) GetProcAddress(libHandle,"GetModuleFileNameExW") ;
#else
		vwGetModuleFileNameEx = (vwGETMODULEFILENAMEEX) GetProcAddress(libHandle,"GetModuleFileNameExA") ;
#endif
	
	/* Create a window class for the window that receives systray notifications.
	 * The window will never be displayed */
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = wndProc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VIRTUAWIN));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = vwVIRTUAWIN_CLASSNAME;
	wc.hIconSm = (HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_VIRTUAWIN), IMAGE_ICON,
								   GetSystemMetrics(SM_CXSMICON),
								   GetSystemMetrics(SM_CYSMICON), 0);

	if(RegisterClassEx(&wc) == 0)
	{
		MessageBox(hWnd,_T("Failed to register class!"),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONWARNING);
		exit(1) ;
	}
	/* Create window. Note that the window is visible to solve pop-up issues, but is placed off sceen */
	if((hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, vwVIRTUAWIN_CLASSNAME, vwVIRTUAWIN_CLASSNAME, WS_VISIBLE,
							  10, -31000, 10, 10, NULL, NULL, hInstance, NULL)) == NULL)
	{
		MessageBox(NULL,_T("Failed to create window!"),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONWARNING);
		exit(2) ;
	}
	
	/* Initials the systray icon structure */
	nIconD.cbSize = sizeof(NOTIFYICONDATA); // size
	nIconD.hWnd = hWnd;		    // window to receive notifications
	nIconD.uID = 1;		    // application-defined ID for icon (can be any UINT value)
	nIconD.uFlags = NIF_MESSAGE |   // nIconD.uCallbackMessage is valid, use it
		  NIF_ICON |		    // nIconD.hIcon is valid, use it
		  NIF_TIP;		    // nIconD.szTip is valid, use it
	nIconD.uCallbackMessage = VW_SYSTRAY;  // message sent to nIconD.hWnd
	
	loadVirtuawinConfig() ;
	if(initialDesktop && (initialDesktop <= nDesks))
	{
		currentDesk = initialDesktop ;
		currentDeskY = ((currentDesk - 1) / nDesksX) + 1 ;
		currentDeskX = nDesksX + currentDesk - (currentDeskY * nDesksX) ;
	}
	if(vwLogFlag)
	{
		TCHAR logFname[MAX_PATH] = _T("");
		GetFilename(vwVIRTUAWIN_CFG,1,logFname) ;
		_tcscpy_s(logFname+_tcslen(logFname)-3,260, _T("log")) ;
		vwLogFile = _tfopen(logFname, _T("w+"));
		vwLogBasic((vwVIRTUAWIN_NAME_VERSION _T("\n"))) ;
	}
	if(useWindowRules)
		loadWindowConfig() ;
	curDisabledMod = loadDisabledModules(disabledModules) ;
	vwIconLoad() ;
	
	/* Wait a second before trying to initialize any more, this gives the system a chance to sort itself out first */
	SetTimer(hWnd,0x29a,1000,VirtuaWinInitContinue) ;
}

/*************************************************
 * VirtuaWin start point
 */
int APIENTRY
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg ;
	
	VirtuaWinInit(hInstance,lpCmdLine) ;
	
	/* Main message loop */
	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	CloseHandle(hMutex);
	return msg.wParam;
}
