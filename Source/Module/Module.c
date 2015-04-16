/*
 *  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
 *  Module.c - Example user module for VirtuaWin.
 * 
 *  Copyright (c) 1999-2005 Johan Piculell
 *  Copyright (c) 2006-2012 VirtuaWin (VirtuaWin@home.se)
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 * 
 *************************************************************************
 * 
 * Simple module skeleton for VirtuaWin. These are the minimal requirements.
 * It is a simple application with a hidden window that receives messages from virtuawin
 * Look in Messages.h to see what can be sent to and from VirtuaWin
 * 
 * Note that the classname must be the same as the filename including the '.exe'
 */
#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>
#include "Messages.h"

int initialised=0 ;
HWND vwHandle;                 /* Handle to VirtuaWin */
TCHAR installPath[MAX_PATH] ;  /* VW install path */
TCHAR userAppPath[MAX_PATH] ;  /* User's config path */
TCHAR deskName[MAX_PATH] ;     /* Current desktop's name */

static void
DisplayInfo(HWND hwnd, TCHAR *msgLabel)
{
    TCHAR buff[MAX_PATH+MAX_PATH+MAX_PATH];
    int deskSize, deskX, deskY, currentDesk ;
    deskSize = SendMessage(vwHandle, VW_DESKTOP_SIZE,0,0) ;
    deskX = SendMessage(vwHandle, VW_DESKX,0,0) ;
    deskY = SendMessage(vwHandle, VW_DESKY,0,0) ;
    currentDesk = SendMessage(vwHandle, VW_CURDESK,0,0) ;
    if(!SendMessage(vwHandle, VW_DESKNAME, (WPARAM) hwnd, 0))
    {
        MessageBox(hwnd,_T("VirtuaWin failed to send the current Desktop name."),_T("Module Error"), MB_ICONWARNING);
        exit(1) ;
    }
    if(deskName[0] == '\0')
        _stprintf_s(deskName,_T("<Desktop %d>"),currentDesk) ;
    _stprintf_s(buff,_T("Desktop Size:\t%d\nInstall path:\t%s\t\nUser path:\t%s\t\n\nVW Message:\t%s\nDesk Layout:\t%d x %d\nCurrent Desk:\t%d\nDesk Name:\t%s"),
              deskSize,installPath,userAppPath,msgLabel,deskX,deskY,currentDesk,deskName) ;
    MessageBox(hwnd,buff,_T("VirtuaWin Module Example"),0);
}


LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case MOD_INIT: 
        /* This must be taken care of in order to get the handle to VirtuaWin. */
        /* The handle to VirtuaWin comes in the wParam */
        vwHandle = (HWND) wParam; /* Should be some error handling here if NULL */
        if(!initialised)
        {
            /* Get the VW Install path and then the user's path - give VirtuaWin 10 seconds to do this */
            if(!SendMessage(vwHandle, VW_INSTALLPATH, (WPARAM) hwnd, 0) || ((initialised & 1) == 0))
            {
                MessageBox(hwnd,_T("VirtuaWin failed to send install path."),_T("Module Error"), MB_ICONWARNING);
                exit(1) ;
            }
            if(!SendMessage(vwHandle, VW_USERAPPPATH, (WPARAM) hwnd, 0) || ((initialised & 2) == 0))
            {
                MessageBox(hwnd,_T("VirtuaWin failed to send the UserApp path."),_T("Module Error"), MB_ICONWARNING);
                exit(1) ;
            }
            
        }
        DisplayInfo(hwnd,_T("Init")) ;
        break;
    
    case WM_COPYDATA:
        {
            COPYDATASTRUCT *cds;         
            cds = (COPYDATASTRUCT *) lParam ;         
            if((cds->dwData == (0-VW_INSTALLPATH)) && ((initialised & 1) == 0))
            {
                if((cds->cbData < 2) || (cds->lpData == NULL))
                    return FALSE ;
                initialised |= 1 ;
                /* the paths are always returned in a multibyte string so we do not need to know
                 * whether we are talking to a unicode VW */
#ifdef _UNICODE
                MultiByteToWideChar(CP_ACP,0,(char *) cds->lpData,-1,installPath,MAX_PATH) ;
#else
                strncpy(installPath,(char *) cds->lpData,MAX_PATH) ;
#endif
                installPath[MAX_PATH-1] = '\0' ;
            }
            else if((cds->dwData == (0-VW_USERAPPPATH)) && ((initialised & 2) == 0))
            {
                if((cds->cbData < 2) || (cds->lpData == NULL))
                    return FALSE ;
                initialised |= 2 ;
#ifdef _UNICODE
                MultiByteToWideChar(CP_ACP,0,(char *) cds->lpData,-1,userAppPath,MAX_PATH) ;
#else
                strncpy(userAppPath,(char *) cds->lpData,MAX_PATH) ;
#endif
                userAppPath[MAX_PATH-1] = '\0' ;
            }
            else if(cds->dwData == (0-VW_DESKNAME))
            {
                if(cds->lpData == NULL)
                    deskName[0] = '\0' ;
                else
                {
#ifdef _UNICODE
                    MultiByteToWideChar(CP_ACP,0,(char *) cds->lpData,-1,deskName,MAX_PATH) ;
#else
                    strncpy(deskName,(char *) cds->lpData,MAX_PATH) ;
#endif
                    deskName[MAX_PATH-1] = '\0' ;
                }
            }
        }
        return TRUE ;
        
    case MOD_QUIT:
        /* This must be handled, otherwise VirtuaWin can't shut down the module */
        PostQuitMessage(0);
        break;
    case MOD_SETUP:
        /* Optional */
        DisplayInfo(hwnd,_T("Setup")) ;
        break;
    case MOD_CFGCHANGE:
        DisplayInfo(hwnd,_T("Config Change")) ;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

/*
 * Main startup function
 */
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    WNDCLASS wc;
    MSG msg;
    
    memset(&wc, 0, sizeof(WNDCLASS));
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.hInstance = hInstance ;
    /* IMPORTANT! The classname must be the same as the filename since VirtuaWin uses 
       this for locating the window */
    wc.lpszClassName = _T("Module.exe");
  
    if (!RegisterClass(&wc))
        return 0;
  
    /* In this example, the window is never shown */
    if (CreateWindow(_T("Module.exe"), 
                     _T("Module"), 
                     WS_POPUP,
                     CW_USEDEFAULT, 
                     0, 
                     CW_USEDEFAULT, 
                     0,
                     NULL,
                     NULL,
                     hInstance,
                     NULL) == (HWND) 0)
        return 0;
    
    /* main messge loop */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
