//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  vwHook.h - Windows Hook for sending window activation events.
// 
//  Copyright (c) 2007-2012 VirtuaWin (VirtuaWin@home.se)
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

#ifndef __VWHOOK_H
#define __VWHOOK_H

#ifdef vwHOOK_BUILD
#define vwHOOK_EXPORT __declspec(dllexport) extern
#else
#define vwHOOK_EXPORT __declspec(dllimport) extern
#endif

vwHOOK_EXPORT void
vwHookSetup(HWND ivwHWnd, int ivwHookUse) ;
vwHOOK_EXPORT int
vwHookInstall(void) ;
vwHOOK_EXPORT void
vwHookUninstall(void) ;

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif
#endif /* __VWHOOK_H */
