//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  VirtuaWin.h - Main variable and function definitions.
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

#ifndef _VIRTUAWIN_H_
#define _VIRTUAWIN_H_

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#include "Defines.h"

/* externally accessible variables */
extern HWND hWnd;                                 // The handle to VirtuaWin 
extern int screenLeft;	                          // the screen dimensions, from VirtuaWin.h
extern int screenRight;	  
extern int screenTop;	  
extern int screenBottom;
extern int deskImageCount;


extern vwWindow *windowList;                      // list of managed windows
extern vwWindowBase *windowBaseList;              // list of all windows
extern vwWindowRule *windowRuleList;              // list for holding window rules
extern vwMenuItem *ctlMenuItemList;               // List of module inserted control menu items
extern vwHotkey hotkeyList[vwHOTKEY_MAX];         // list for holding hotkeys

extern int curDisabledMod;                        // how many disabled modules we have
extern vwModule moduleList[MAXMODULES];           // list that holds modules
extern vwDisModule disabledModules[MAXMODULES*2]; // list with disabled modules
extern HWND ichangeHWnd;                          // handle to module hangling desktop changes

/* logging defines & macros */
extern vwUByte vwLogFlag ;
extern FILE *vwLogFile ;

#define vwLogEnabled()        (vwLogFile != NULL)
#define vwLogBasic(a)         (vwLogEnabled() ? vwLogPrint a :0)

#ifdef vwLOG_VERBOSER
#define vwLogVerboser(a)       vwLogVerbose(a)
#else
#define vwLogVerboser(a)
#endif

#ifdef vwLOG_VERBOSE
#define vwLogVerbose(a)       (vwLogEnabled() ? vwLogPrint a :0)
#else
#define vwLogVerbose(a)
#endif

/* Prototypes from VirtuaWin.c */
void vwLogPrint(const TCHAR *format, ...) ;
void vwMutexLock(void) ;
void vwMutexRelease(void) ;
HWND vwFindWindow(TCHAR *className, TCHAR *windowText, int printOut) ;
void enableMouse(int turnOn) ;
void setMouseKey(void) ;
void vwHookSetup(void) ;
void vwIconLoad(void) ;
void vwIconSet(int deskNumber, int hungCount) ;
void vwTaskbarHandleGet(void) ;
void vwHotkeyRegister(int warnAll) ;
void vwHotkeyUnregister(int unregAll) ;
void getWorkArea(void) ;
int  windowListUpdate(void) ;
void vwWindowRuleReapply(void) ;
void vwLogWindows(void); 
int  disableDeskImage(int count) ;
int  createDeskImage(int deskNo, int createDefault) ;
void setForegroundWin(HWND theWin, int makeTop) ;
int  assignWindow(HWND theWin, int theDesk, vwUByte follow, vwUByte force, vwUByte setActive);
int  gotoDesk(int theDesk, vwUByte force);
void showHelp(HWND aHWnd, TCHAR *topic);

/* Prototypes from SetupDialog.c */
void createSetupDialog(HINSTANCE theHinst, HWND theHwndOwner) ;
void initDesktopProperties(void) ;
void storeDesktopProperties(void) ;

/* Prototypes from WinRuleDialog.c */
void createWindowRuleDialog(HINSTANCE theHinst, HWND theHwndOwner, vwWindowRule *wtype, HWND theWin) ;
void updateWindowRuleDialog(HINSTANCE theHinst, HWND theHwndOwner, vwWindowRule *wtype, HWND theWin) ;

/* Prototypes from ModuleRoutines.c */
void vwModulesLoad(void);
void vwModulesSendMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
void vwModulesPostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
void vwModuleLoad(int moduleIdx, TCHAR *path) ;

#endif
