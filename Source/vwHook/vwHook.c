//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  vwHook.c - Windows Hook for sending window activation events.
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
#define vwHOOK_BUILD
#define vwHOOK_TEST 0

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "Messages.h"
#include "vwHook.h"

#ifdef _MSC_VER
#pragma data_seg(".VWSHR")
#define SHARED
#else
#define SHARED __attribute__((section(".VWSHR"), shared)) 
#endif

static int vwHookUse SHARED = 0 ;
static HWND vwHWnd SHARED = NULL ;

#ifdef _MSC_VER
#pragma data_seg()
#pragma comment(linker, "/SECTION:.VWSHR,RWS")
#endif

#if vwHOOK_TEST
FILE *logfp ;
#endif

HINSTANCE hookHInst ;
HHOOK hookCallWndProc ;
clock_t vwShowHideTime ;
/* allow up to 0.25secs for VW to change the visibility state of a window */
#define vwCHANGE_STATE_TIME (CLOCKS_PER_SEC >> 2)
BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpvReserved)
{
    if(fdwReason == DLL_PROCESS_ATTACH)
    {
        hookHInst = hInst ;
#if vwHOOK_TEST
        if(logfp == NULL)
            logfp = fopen("c:\\vwHook.log","w+") ;
        fprintf(logfp,"DllMain: %p\n",hookHInst) ;
        fflush(logfp) ;
#endif
    }
    return TRUE ;
}


LRESULT CALLBACK
vwHookCallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if((nCode == HC_ACTION) && vwHookUse)
    {
        CWPSTRUCT *cwp = (CWPSTRUCT *) lParam ;
        HWND vwh = vwHWnd ;
        clock_t cc ;

#if vwHOOK_TEST
        {
            SYSTEMTIME stime;
            GetLocalTime (&stime);
            fprintf(logfp,"[%04d-%02d-%02d %02d:%02d:%02d:%03d] vwHookCallWndProc %x: %d %d %d (%x %x %x) - %x\n",
                     stime.wYear, stime.wMonth, stime.wDay, stime.wHour, stime.wMinute, stime.wSecond, stime.wMilliseconds,
                     cwp->hwnd,cwp->message,cwp->wParam,cwp->lParam,cwp->message,cwp->wParam,cwp->lParam,
                     GetWindowLong(cwp->hwnd, GWL_STYLE)) ;
            fflush(logfp) ;
        }
#endif
        if(cwp->hwnd == vwh)
            /* dont report any VW window events back to VW */
            ;
        else if((cwp->message == WM_ACTIVATE) && (LOWORD(cwp->wParam) != WA_INACTIVE) &&
                (((cc=clock()) - vwShowHideTime) > vwCHANGE_STATE_TIME))
        {
#if vwHOOK_TEST
            fprintf(logfp,"     Activate: %d %d\n",vwShowHideTime,cc) ;
            fflush(logfp) ;
#endif
            /* set time to this time to avoid sending more than one message at a time */
            vwShowHideTime = cc ;
            PostMessage(vwh,VW_ACCESSWIN,(WPARAM) cwp->hwnd,-1) ;
        }
        else if((cwp->message == WM_NULL) && (cwp->wParam == 0x51842145) && (cwp->lParam == 0x5e7bdeba))
            vwShowHideTime = clock() ;
            
    }
    
    return CallNextHookEx(hookCallWndProc,nCode,wParam,lParam) ;
}

vwHOOK_EXPORT void
vwHookSetup(HWND ivwHWnd, int ivwHookUse)
{
    vwHWnd = ivwHWnd ;
    vwHookUse = ivwHookUse ;
}

vwHOOK_EXPORT int
vwHookInstall(void)
{
#if vwHOOK_TEST
    /* only hook the vwTestApp */
    DWORD vwHookThreadId ;
    HWND hWnd ;
    
    if(logfp == NULL)
        logfp = fopen("c:\\vwHook.log","w+") ;
    fprintf(logfp,"vwHookInstall: %p\n",hookHInst) ;
    fflush(logfp) ;
    
    if(((hWnd = FindWindow("#32770","vwTestApp")) == NULL) ||
       ((vwHookThreadId = GetWindowThreadProcessId(hWnd,NULL)) == 0))
        return 1 ;
    fprintf(logfp,"vwHookInstall: %p: %x %d\n",hookHInst,hWnd,vwHookThreadId) ;
    fflush(logfp) ;
#else
#define vwHookThreadId 0
#endif
    if((hookCallWndProc = SetWindowsHookEx(WH_CALLWNDPROC,vwHookCallWndProc,hookHInst,vwHookThreadId)) == NULL)
        return 2 ;
    return 0 ;
}

vwHOOK_EXPORT void
vwHookUninstall(void)
{
    UnhookWindowsHookEx(hookCallWndProc) ;
}
