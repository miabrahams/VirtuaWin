//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  ConfigParameters.h - Declaration of configured variables
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

#ifndef _CONFIGPARAMETERS_H_
#define _CONFIGPARAMETERS_H_

#define vwWINLIST_ACCESS   0x01
#define vwWINLIST_ASSIGN   0x02
#define vwWINLIST_SHOW     0x04
#define vwWINLIST_STICKY   0x08
#define vwWINLIST_TITLELN  0x40

extern HWND    dialogHWnd;         // handle to the setup dialog, NULL if not open
extern int     dialogPos[2];       // Where to place the dialog         
extern vwUByte dialogOpen;         // Flags whether a dialog is open 

extern int hotkeyCount;            // Number of hotkeys
extern int moduleCount;            // Number of loaded modules
extern int currentDesk;            // Current desktop
extern int nDesks;                 // indicates the total number of desks (nDesksX * nDesksY)
extern int nDesksX;                // indicates the number of desks wide the virtual area is
extern int nDesksY;                // indicates the number of desks tall the virtual area is
extern int mouseJumpLength;        // How far to jump into new desktop
extern int mouseDelay;             // Mouse change delay 50ms*mouseDelay 
extern vwUByte mouseKnock;         // mouse edge kncking mode
extern vwUByte mouseEnable;        // Required mouse support
extern vwUByte mouseWarp;          // if we don't want to move the mouse pointer after switch
extern vwUByte mouseModifierUsed;  // if user must use a modify key to warp with mouse
extern vwUByte mouseModifier ;     // modify key required to warp with mouse
extern vwUByte preserveZOrder;     // Should we preserve the window Z order
extern vwUByte hiddenWindowAct;    // Hidden window activation action
extern vwUByte taskButtonAct;	   // Hidden window but visible task button activation action
extern vwUByte releaseFocus;       // release focus on switch
extern vwUByte refreshOnWarp;      // if we should refresh desktop after switch
extern vwUByte initialDesktop;     // The desktop to start on, usually 1
extern vwUByte lastDeskNoDelay;    // Don't wait a second before updating lastDesk
extern vwUByte deskWrap;           // If we want to have desktop cycling
extern vwUByte invertY;            // if up/down should be inverted
extern vwUByte displayTaskbarIcon; // Should we display the systray icon
extern vwUByte noTaskbarCheck;     // Should we skip the taskbar search
extern vwUByte useWindowRules;     // Use window rules
extern vwUByte useDynButtonRm;     // Use dynamic taskbar button removal
extern vwUByte useDskChgModRelease;// Use automatic desktop change modifier release
extern vwUByte winListContent;     // Required content of the winodw list menu
extern vwUByte winListCompact;     // if window list menu should be compact
extern vwUByte winMenuCompact;     // if current window menu should be compact
extern vwUByte ctlMenuCompact;     // if control menu should be compact
extern vwUByte hotkeyMenuLoc;      // Location to use if menu/list is opened via a hotkey
extern vwUByte aggressiveRules;	   // Aggressive rule enforcement
extern vwUByte minWinHide;         // Flag to determine how to hide minimized windows
extern vwUByte vwHookUse;          // Use vwHook to resolve activate issues

extern TCHAR *desktopName[vwDESKTOP_SIZE];

#endif
