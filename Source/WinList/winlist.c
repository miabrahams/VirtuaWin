//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  winlist.c - VirtuaWin module for restoring lost windows.
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

#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include <stdio.h>
#include <tchar.h>

#include "winlistres.h"
#include "../Defines.h"
#include "../Messages.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif

HINSTANCE hInst ;   // Instance handle
HWND hwndMain ;	    // Main window handle
HWND vwHandle ;     // Handle to VirtuaWin
HWND listHWnd ;     // Handle to the window list
HWND hwndTask ;
UINT RM_Shellhook ;
TCHAR userAppPath[MAX_PATH] ;
int deskCount ;
int deskCrrnt ;

typedef struct vwlWindow {
    struct vwlWindow *next ;
    HWND              handle ;
    TCHAR             className[vwCLASSNAME_MAX] ;
    TCHAR             windowName[vwWINDOWNAME_MAX] ;
    RECT              rect ;
    int               style ;
    int               exstyle ;
    short             flag ;
    unsigned char     state ;
    unsigned char     restored ;
} vwlWindow ;

vwlWindow *windowList ;
int runMode=0 ;
int winInitialised=0 ;
int screenLeft ;
int screenRight ;
int screenTop ;
int screenBottom ;
int sortCol ;
int sortDir ;

static void
FreeWindowList(void)
{
    vwlWindow *ww ;
    while((ww = windowList) != NULL)
    {
        windowList = ww->next ;
        free(ww) ;
    }
}


static int CALLBACK
ListCompFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    vwlWindow *w1, *w2 ;
    int ret ;
    
    w1 = (vwlWindow *) lParam1 ;
    w2 = (vwlWindow *) lParam2 ;
    if(sortCol == 0)
        ret = (((int) w2->flag) - ((int) w1->flag)) ;
    else if(sortCol == 1)
        ret = (((int) w1->state) - ((int) w2->state)) ;
    else if(sortCol == 2)
        ret = _tcsicmp(w2->className,w1->className) ;
    else
        ret = 0 ;
    if(ret == 0)
        ret = _tcsicmp(w2->windowName,w1->windowName) ;
    if(ret == 0)
        ret = (((int) w2->handle) - ((int) w1->handle)) ;
        
    return ((sortDir) ? ret:0-ret) ;
}

__inline BOOL CALLBACK
enumWindowsProc(HWND hwnd, LPARAM lParam) 
{
    vwlWindow *win ;
    TCHAR fbuff[4], sbuff[4];
    int idx, flag, state, exstyle, style = GetWindowLong(hwnd, GWL_STYLE);
    LVITEM item ;
    RECT rect ;
    
    if(style & WS_CHILD)
        return TRUE ;

    GetWindowRect(hwnd,&rect);
    exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    
    fbuff[1] = ' ' ;
    fbuff[2] = '\0' ;
    sbuff[0] = ' ' ;
    sbuff[1] = ' ' ;
    sbuff[2] = '\0' ;
    state = 0 ;
    if((vwHandle != 0) && ((flag = SendMessage(vwHandle,VW_WINGETINFO,(WPARAM) hwnd,0)) != 0))
    {
        if((flag & vwWINFLAGS_WINDOW) == 0)
            return TRUE ;
        if((flag & vwWINFLAGS_MANAGED) == 0)
        {
            // This is a known unmanaged window
            fbuff[0] = 'U' ;
            flag = 0 ;
        }
        else
        {
            if((flag & (vwWINFLAGS_ELEVATED_TEST|vwWINFLAGS_ELEVATED)) == (vwWINFLAGS_ELEVATED_TEST|vwWINFLAGS_ELEVATED))
            {
                /* window is known to be elevated */
                state = 2 ;
                sbuff[0] = 'E' ;
            }
            else if(((flag & vwWINFLAGS_SHOWN) != 0) ^ ((flag & vwWINFLAGS_SHOW) == 0))
            {
                /* if show & shown or vice versa then the window is okay */
                state = 1 ;
                sbuff[0] = 'O' ;
            }
            else
            {
                /* window is hung */
                state = 3 ;
                sbuff[0] = 'H' ;
            }
            flag = vwWindowGetInfoDesk(flag) ;
            if((flag != deskCrrnt) && (flag > deskCount))
                // on a hidden desktop
                return TRUE ;
            fbuff[0] = (flag >= 10) ? ('0' + (flag / 10)) : ' ' ;
            fbuff[1] = '0' + (flag % 10) ;
        }
    }
    else if(style & WS_VISIBLE)
    {
        fbuff[0] = 'V' ;
        flag = -1 ;
    }
    else
    {
        fbuff[0] = 'H' ;
        flag = -2 ;
    }
    if((win = malloc(sizeof(vwlWindow))) == NULL)
        return FALSE ;
    memset(&item,0,sizeof(LVITEM)) ;
    item.iItem = ListView_GetItemCount(listHWnd) ;
    item.lParam = (LPARAM) win ;
    item.pszText = fbuff ;
    item.mask = LVIF_PARAM | LVIF_TEXT ;
    if((idx = ListView_InsertItem(listHWnd, &item)) < 0)
    {
        free(win) ;
        return TRUE;
    }
    win->next = windowList ;
    windowList = win ;
    GetClassName(hwnd,win->className,vwCLASSNAME_MAX);
    if(!GetWindowText(hwnd,win->windowName,vwWINDOWNAME_MAX))
        _tcscpy_s(win->windowName, 128, _T("<None>")) ;
    
    ListView_SetItemText(listHWnd,idx,1,sbuff) ;
    ListView_SetItemText(listHWnd,idx,2,win->className) ;
    ListView_SetItemText(listHWnd,idx,3,win->windowName) ;
    
    win->handle = hwnd;
    win->style = style;
    win->exstyle = exstyle;
    win->state = (unsigned char) state ;
    win->flag = (short) flag ;
    win->rect = rect ;
    win->restored = 0 ;
    
    return TRUE;
}


static int
GenerateWinList(HWND hDlg, int update)
{
    /* The virtual screen size system matrix values were only added for WINVER >= 0x0500 (Win2k) */
#ifndef SM_XVIRTUALSCREEN
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#endif
    FreeWindowList() ;
    if((screenRight  = GetSystemMetrics(SM_CXVIRTUALSCREEN)) <= 0)
    {
        /* The virtual screen size system matrix values are not supported on
         * this OS (Win95 & NT), use the desktop window size */
        RECT r;
        GetClientRect(GetDesktopWindow(), &r);
        screenLeft   = r.left;
        screenRight  = r.right;
        screenTop    = r.top;
        screenBottom = r.bottom;
    }
    else
    {
        screenLeft   = GetSystemMetrics(SM_XVIRTUALSCREEN);
        screenRight += screenLeft;
        screenTop    = GetSystemMetrics(SM_YVIRTUALSCREEN);
        screenBottom = GetSystemMetrics(SM_CYVIRTUALSCREEN)+screenTop;
    }
    
    listHWnd = GetDlgItem(hDlg,ID_WINLIST) ;
    if(update)
        ListView_DeleteAllItems(listHWnd) ;
    else
    {
        LVCOLUMN col;
        ListView_SetExtendedListViewStyleEx(listHWnd,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);
        
        col.mask = LVCF_FMT | LVCF_TEXT;
        col.fmt = LVCFMT_LEFT;
        col.pszText = _T("F") ;
        ListView_InsertColumn(listHWnd,0,&col);
        
        col.pszText = _T("S") ;
        ListView_InsertColumn(listHWnd,1,&col);
        
        col.pszText = _T("Class") ;
        ListView_InsertColumn(listHWnd,2,&col);
        
        col.pszText = _T("Title") ;
        ListView_InsertColumn(listHWnd,3,&col);
    }
    if(vwHandle != 0)
    {
        deskCount = SendMessage(vwHandle, VW_DESKX, 0, 0) * SendMessage(vwHandle, VW_DESKY, 0, 0) ;
        deskCrrnt = SendMessage(vwHandle, VW_CURDESK, 0, 0) ;
    }
    
    EnumWindows(enumWindowsProc, (LPARAM) hDlg);   // get all windows
    
    ListView_SortItems(listHWnd,ListCompFunc,0) ;
    
    ListView_SetColumnWidth(listHWnd,0,LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(listHWnd,1,LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(listHWnd,2,LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(listHWnd,3,LVSCW_AUTOSIZE);
    
    EnableWindow(GetDlgItem(hDlg,IDUNDO),FALSE) ;

    return 1;
}


BOOL CALLBACK
enumWindowsSaveListProc(HWND hwnd, LPARAM lParam) 
{
    FILE *wdFp=(FILE *) lParam ;
    DWORD procId, threadId ;
    int style, exstyle, desk ;
    RECT pos ;
#ifdef _UNICODE
    WCHAR nameW[vwWINDOWNAME_MAX] ;
#endif
    char text[vwWINDOWNAME_MAX] ;
    char class[vwCLASSNAME_MAX] ;
    
    
    style = GetWindowLong(hwnd, GWL_STYLE);
    exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    threadId = GetWindowThreadProcessId(hwnd,&procId) ;
    GetWindowRect(hwnd,&pos);
#ifdef _UNICODE
    GetClassName(hwnd,nameW,vwCLASSNAME_MAX);
    WideCharToMultiByte(CP_ACP,0,nameW,-1,class,vwCLASSNAME_MAX, 0, 0) ;
    if(GetWindowText(hwnd,nameW,vwWINDOWNAME_MAX))
        WideCharToMultiByte(CP_ACP,0,nameW,-1,text,vwWINDOWNAME_MAX, 0, 0) ;
    else
#else
    GetClassName(hwnd,class,vwCLASSNAME_MAX);
    if(!GetWindowText(hwnd,text,vwWINDOWNAME_MAX))
#endif
        strcpy_s(text,128, "<None>");
    
    if(vwHandle != 0)
        desk = SendMessage(vwHandle,VW_WINGETINFO,(WPARAM) hwnd,0) ;
    else
        desk = 0 ;
    fprintf(wdFp,"%8x %08x %08x %8x %8x %8x %6x %s\n%8d %8x %8d %8d %8d %8d %6d %s\n",
            (int)hwnd,style,exstyle,(int)GetParent(hwnd),
            (int)GetWindow(hwnd,GW_OWNER),(int)GetClassLong(hwnd,GCL_HICON),(desk & 0x00ffffff),text,
            (int)procId,(int)threadId,(int)pos.top,(int)pos.bottom,(int)pos.left,(int)pos.right,
            ((desk >> vwWTFLAGS_HIDEWIN_BITROT) & 0x00ff),class) ;
    
    return TRUE;
}

static vwlWindow *
GetListSelection(HWND hwndDlg)
{
    LVITEM item ;
    int ii ;
    
    listHWnd = GetDlgItem(hwndDlg,ID_WINLIST) ;
    if(ListView_GetSelectedCount(listHWnd) != 1)
        return NULL ;
    
    ii = ListView_GetItemCount(listHWnd) ;
    while(--ii >=0)
    {
        memset(&item,0,sizeof(LVITEM)) ;
        item.mask = LVIF_STATE | LVIF_PARAM ;
        item.stateMask = LVIS_SELECTED ;
        item.iItem = ii ;
        ListView_GetItem(listHWnd,&item) ;
        if(item.state & LVIS_SELECTED)
            return ((vwlWindow *) item.lParam) ;
    }
    return NULL ;
}

/* This is the main function for the dialog. */
static BOOL CALLBACK
DialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    vwlWindow *curw ;
    
    switch(msg)
    {
    case WM_INITDIALOG:
        GenerateWinList(hwndDlg,0);
        return TRUE;
        
    case WM_NOTIFY:
        if(wParam == ID_WINLIST)
        {
            NM_LISTVIEW *nm = (NM_LISTVIEW *) lParam ;
            if(nm->hdr.code == LVN_COLUMNCLICK)
            {
                if(nm->iSubItem != sortCol)
                {
                    sortCol = nm->iSubItem ;
                    sortDir = 0 ;
                }
                else
                    sortDir ^= 1 ;
                ListView_SortItems(nm->hdr.hwndFrom,ListCompFunc,0) ;
                return TRUE;
            }
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDREFRESH:
            GenerateWinList(hwndDlg,1) ;
            return TRUE;
            
        case IDOK:
            if((curw = GetListSelection(hwndDlg)) != NULL)
            {
                int left, top;
                if(curw->flag > 0)
                    /* make VW display the window */
                    SendMessage(vwHandle,VW_ACCESSWIN,(WPARAM) curw->handle,1) ;
                if((curw->style & WS_VISIBLE) == 0)
                {
                    ShowWindow(curw->handle, SW_SHOWNA);
                    ShowOwnedPopups(curw->handle, SW_SHOWNA);
                }
                if((curw->style & WS_VISIBLE) && (curw->exstyle & WS_EX_TOOLWINDOW))
                {
                    // Restore the window mode
                    SetWindowLong(curw->handle, GWL_EXSTYLE, (curw->exstyle & (~WS_EX_TOOLWINDOW))) ;  
                    // Notify taskbar of the change
                    if(hwndTask != NULL)
                        PostMessage(hwndTask, RM_Shellhook, 1, (LPARAM) curw->handle);
                }
                left = curw->rect.left ;
                top = curw->rect.top ;
                /* some apps hide the window by pushing it to -32000,
                 * VirtuaWin does not move these windows */
                if((left != -32000) || (top != -32000))
                {
                    if(top < -5000)
                        top += 25000 ;
                    if(left < screenLeft)
                        left = screenLeft + 10 ;
                    else if(left > screenRight)
                        left = screenLeft + 10 ;
                    if(top < screenTop)
                        top = screenTop + 10 ;
                    else if(top > screenBottom)
                        top = screenTop + 10 ;
                    SetWindowPos(curw->handle, 0, left, top, 0, 0,
                                 SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE ); 
                }
                SetForegroundWindow(curw->handle);
                curw->restored = 1 ;
                EnableWindow(GetDlgItem(hwndDlg,IDUNDO),TRUE) ;
            }
            return 1;
        case IDUNDO:
            if((curw = GetListSelection(hwndDlg)) != NULL)
            {
                if(curw->restored)
                {
                    if(curw->flag > 0)
                        /* make VW display the window */
                        SendMessage(vwHandle,VW_ASSIGNWIN,(WPARAM) curw->handle, (LPARAM) curw->flag) ;
                    if((curw->style & WS_VISIBLE) == 0)
                    {
                        ShowWindow(curw->handle, SW_HIDE);
                        ShowOwnedPopups(curw->handle, SW_HIDE);
                    }
                    if((curw->style & WS_VISIBLE) && (curw->exstyle & WS_EX_TOOLWINDOW))
                    {
                        // Restore the window mode
                        SetWindowLong(curw->handle, GWL_EXSTYLE, curw->exstyle) ;  
                        // Notify taskbar of the change
                        if(hwndTask != NULL)
                            PostMessage(hwndTask, RM_Shellhook, 2, (LPARAM) curw->handle);
                    }
                    SetWindowPos(curw->handle, 0, curw->rect.left, curw->rect.top, 0, 0,
                                 SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE ); 
                    curw->restored = 0 ;
                }
                else
                    MessageBox(hwndDlg, _T("Cannot undo, not restored."), _T("VirtuaWinList Error"), MB_ICONWARNING);
            }
            return 1;
        
        case IDSAVE:
            if(winInitialised)
            {
                TCHAR fname[MAX_PATH] ;
                FILE *wdFp ;
                int ii=1 ;
                while(ii < 1000)
                {
                    _stprintf_s(fname,260, _T("%sWinList_%d.log"),(winInitialised) ? userAppPath:_T(""),ii) ;
                    if(GetFileAttributes(fname) == INVALID_FILE_ATTRIBUTES)
                        break ;
                    ii++ ;
                }
                if(ii == 1000)
                    MessageBox(hwndDlg, _T("Cannot create a WinList_#.log file, please clean up your user directory."), _T("VirtuaWinList Error"), MB_ICONWARNING);
                else
                {
					_tfopen_s(&wdFp, fname, _T("w+"));
                    EnumWindows(enumWindowsSaveListProc,(LPARAM) wdFp) ;
                    fclose(wdFp) ;
                }
            }
            return 1;
        case IDCANCEL:
            EndDialog(hwndDlg,0);
            if(runMode)
                exit(0) ;
            FreeWindowList() ;
            return 1;
        }
        break;
        
    case WM_CLOSE:
        EndDialog(hwndDlg,0);
        if(runMode)
            exit(0) ;
        FreeWindowList() ;
        return TRUE;
	
    }
    return FALSE;
}


static void
goGetTheTaskbarHandle(void)
{
    HWND hwndTray = FindWindowEx(NULL, NULL,_T("Shell_TrayWnd"), NULL);
    HWND hwndBar = FindWindowEx(hwndTray, NULL,_T("ReBarWindow32"), NULL );
    
    // Maybe "RebarWindow32" is not a child to "Shell_TrayWnd", then try this
    if(hwndBar == NULL)
        hwndBar = hwndTray;
    
    hwndTask = FindWindowEx(hwndBar, NULL,_T("MSTaskSwWClass"), NULL);
    
    /* SF 1843056 - don't complain if not found, may be using a different
     * shell, leave it to VirtuaWin to complain if appropriate */
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        
    case MOD_INIT:
        // The handle to VirtuaWin comes in the wParam 
        vwHandle = (HWND) wParam;
        // If not yet winInitialised get the user path and initialize.
        if(!winInitialised)
        {
            SendMessage(vwHandle, VW_USERAPPPATH, (WPARAM) hwnd, 0) ;
            if(!winInitialised)
                /* do not exit as this maybe caused by a problem we are trying to fix */
                MessageBox(hwnd, _T("VirtuaWin failed to send the UserApp path."), _T("VirtuaWinList Error"), MB_ICONWARNING);
        }
        break;
    case WM_COPYDATA:
        if(!winInitialised)
        {
            COPYDATASTRUCT *cds;         
            cds = (COPYDATASTRUCT *) lParam ;         
            if(cds->dwData == (0-VW_USERAPPPATH))
            {
                if((cds->cbData < 2) || (cds->lpData == NULL))
                    return FALSE ;
                winInitialised = 1 ;
#ifdef _UNICODE
                MultiByteToWideChar(CP_ACP,0,(char *) cds->lpData,-1,userAppPath,MAX_PATH) ;
#else
                strcpy_s(userAppPath,260,(char *) cds->lpData) ;
#endif
            }
        }
        return TRUE ;
    case MOD_QUIT: // This must be handeled, otherwise VirtuaWin can't shut down the module 
        PostQuitMessage(0);
        break;
    case MOD_SETUP: // Optional
        if(wParam != 0)
            hwnd = (HWND) wParam ;
        else
            hwnd = (HWND) hwndMain ;
        SetForegroundWindow(hwnd) ;
        DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), hwnd, (DLGPROC) DialogFunc);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

/* Initializes the window */ 
static BOOL
WinListInit(void)
{
    WNDCLASS wc;
    
    InitCommonControls();
    memset(&wc, 0, sizeof(WNDCLASS));
    wc.style = 0;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;
    wc.hInstance = hInst;
    /* IMPORTANT! The classname must be the same as the filename since VirtuaWin uses 
       this for locating the window */
    wc.lpszClassName = _T("WinList.exe");
    
    if (!RegisterClass(&wc))
        return 0;
    
    // the window is never shown
    if ((hwndMain = CreateWindow(_T("WinList.exe"), 
                                 _T("WinList"), 
                                 WS_POPUP,
                                 CW_USEDEFAULT, 
                                 0, 
                                 CW_USEDEFAULT, 
                                 0,
                                 NULL,
                                 NULL,
                                 hInst,
                                 NULL)) == (HWND)0)
        return 0;
    
    RM_Shellhook = RegisterWindowMessage(_T("SHELLHOOK"));
    goGetTheTaskbarHandle() ;
    
    return 1;
}

/*
 * Main startup function
 */
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    MSG msg;
    
    hInst = hInstance;
    if(!WinListInit())
        return 0;
    
    runMode = (strstr(lpCmdLine,"-module") == NULL) ;
    if(runMode)
    {
        SetForegroundWindow(hwndMain) ;
        DialogBox(hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), hwndMain, (DLGPROC) DialogFunc);
    }
    
    // main messge loop
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
