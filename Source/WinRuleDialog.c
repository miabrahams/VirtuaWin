//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  WinRuleDialog.c - Window Rule Dialog routines.
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

// Must compile with define of _WIN32_IE=0x0200 otherwise the dialog
// will not open on NT/95 without a patch (known MS issue) 
#define _WIN32_IE 0x0200

// Includes
#include "VirtuaWin.h"
#include "Resource.h"
#include "Messages.h"
#include "DiskRoutines.h"
#include "ConfigParameters.h"

// Standard includes
#include <stdlib.h>
#include <shellapi.h>
#include <prsht.h>
#include <commctrl.h>

static int deskCount ;
static HWND initWin ;
static vwWindowRule *winRuleCur ;

static void
windowRuleDialogInitList(HWND hDlg)
{
    static int ts[3] = { 16, 20, 120 } ;
    static char wtypeNameLabel[vwWTNAME_COUNT] = "CWP" ;
    vwWindowRule *wt ;
    TCHAR buff[388], *ss;
    int ii, jj, kk, winRuleCurIdx=0 ;
    
    SendDlgItemMessage(hDlg,IDC_WTYPE_LIST,LB_RESETCONTENT,0, 0);
    SendDlgItemMessage(hDlg,IDC_WTYPE_LIST,LB_SETTABSTOPS,(WPARAM) 3,(LPARAM) ts);
    ii = 0 ;
    wt = windowRuleList ;
    while(wt != NULL)
    {
        if(((wt->flags & vwWTFLAGS_MOVE) == 0) || (wt->desk <= deskCount))
        {
            ii++ ;
            ss = buff ;
            ss += _stprintf_s(ss,_T("%d"),ii) ;
            for(jj=0 ; jj<vwWTNAME_COUNT ; jj++)
            {
                if(wt->name[jj] != NULL)
                {
                    *ss++ = '\t' ;
                    *ss++ = wtypeNameLabel[jj] ;
                    *ss++ = 'N' ;
                    *ss++ = ':' ;
                    if(wt->flags & (1 << (jj << 1)))
                        *ss++ = '*' ;
                    kk = _tcslen(wt->name[jj]) ;
                    if(kk > 120)
                    {
                        _tcsncpy(ss,wt->name[jj],120) ;
                        ss += 120 ;
                        *ss++ = '.' ;
                        *ss++ = '.' ;
                        *ss++ = '.' ;
                    }
                    else
                    {
                        _tcsncpy(ss,wt->name[jj],kk) ;
                        ss += kk ;
                    }
                    if(wt->flags & (2 << (jj << 1)))
                        *ss++ = '*' ;
                }
            }
            *ss = '\0' ;
            SendDlgItemMessage(hDlg,IDC_WTYPE_LIST,LB_ADDSTRING,0,(LONG) buff);
            if(wt == winRuleCur)
                winRuleCurIdx = ii ;
        }
        wt = wt->next ;
    }
    if(winRuleCurIdx)
        SendDlgItemMessage(hDlg,IDC_WTYPE_LIST,LB_SETCURSEL,winRuleCurIdx-1,0) ;
    else
        winRuleCur = NULL ;
}

static int wtypeNameEntry[vwWTNAME_COUNT] = { IDC_WTYPE_CNAME, IDC_WTYPE_WNAME, IDC_WTYPE_PNAME } ;
static void
windowRuleDialogInitItem(HWND hDlg)
{
    vwWindowRule *wt ;
    TCHAR buff[1024] ;
    int ii ;
    
    if(winRuleCur != NULL)
    {
        ii = 0 ;
        wt = windowRuleList ;
        while(wt != NULL)
        {
            if(((wt->flags & vwWTFLAGS_MOVE) == 0) || (wt->desk <= deskCount))
            {
                if(wt == winRuleCur)
                    break ;
                ii++ ;
            }
            wt = wt->next ;
        }
        
        if(wt != NULL)
        {
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_UP),(ii > 0)) ;
            ii = vwWTNAME_COUNT-1 ;
            do {
                buff[0] = '\0' ;
                if(wt->name[ii] != NULL)
                {
                    if(wt->flags & (1 << (ii << 1)))
                    {
                        buff[0] = '*' ;
                        _tcscpy(buff+1,wt->name[ii]) ;
                    }
                    else
                        _tcscpy(buff,wt->name[ii]) ;
                    if(wt->flags & (2 << (ii << 1)))
                        _tcscat(buff,_T("*")) ;
                    if(buff[0] == '\0')
                        _tcscpy(buff,vwWTNAME_NONE);
                }
                SetDlgItemText(hDlg,wtypeNameEntry[ii],buff) ;
            } while(--ii >= 0) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_MOD),TRUE) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_DEL),TRUE) ;
            SendDlgItemMessage(hDlg,IDC_WTYPE_ENABLE,BM_SETCHECK,((wt->flags & vwWTFLAGS_ENABLED) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_CLOSE,BM_SETCHECK,((wt->flags & vwWTFLAGS_CLOSE) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_ALONTOP,BM_SETCHECK,((wt->flags & vwWTFLAGS_ALWAYSONTOP) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_NMANAGE,BM_SETCHECK,((wt->flags & vwWTFLAGS_DONT_MANAGE) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_AMANAGE,BM_SETCHECK,((wt->flags & vwWTFLAGS_MANAGE) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_VMANAGE,BM_SETCHECK,((wt->flags & (vwWTFLAGS_DONT_MANAGE|vwWTFLAGS_MANAGE)) == 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_MAINWIN,BM_SETCHECK,((wt->flags & vwWTFLAGS_MAIN_WIN) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_STICKY,BM_SETCHECK,((wt->flags & vwWTFLAGS_STICKY) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_GRPAPP,BM_SETCHECK,((wt->flags & vwWTFLAGS_GROUP_APP) != 0), 0);
            if((ii = ((wt->flags & vwWTFLAGS_MOVE) != 0)) && wt->desk)
                SendDlgItemMessage(hDlg,IDC_WTYPE_AMDSK,CB_SETCURSEL,wt->desk-1, 0) ;
            SendDlgItemMessage(hDlg,IDC_WTYPE_AMOVE,BM_SETCHECK,ii,0);
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_AMDSK),ii) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_AMIMM),ii) ;
            SendDlgItemMessage(hDlg,IDC_WTYPE_AMIMM,BM_SETCHECK,((wt->flags & vwWTFLAGS_MOVE_IMMEDIATE) != 0), 0);
            SendDlgItemMessage(hDlg,IDC_WTYPE_HWACT,CB_SETCURSEL,((wt->flags & vwWTFLAGS_HWACT_MASK) >> vwWTFLAGS_HWACT_BITROT), 0) ;
            SendDlgItemMessage(hDlg,IDC_WTYPE_WHIDE,CB_SETCURSEL,((wt->flags & vwWTFLAGS_HIDEWIN_MASK) >> vwWTFLAGS_HIDEWIN_BITROT), 0) ;
            SendDlgItemMessage(hDlg,IDC_WTYPE_THIDE,CB_SETCURSEL,((wt->flags & vwWTFLAGS_HIDETSK_MASK) >> vwWTFLAGS_HIDETSK_BITROT), 0) ;
            
            while((wt=wt->next) != NULL)
                if(((wt->flags & vwWTFLAGS_MOVE) == 0) || (wt->desk <= deskCount))
                    break ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_DOWN),(wt != NULL)) ;
            return ;
        }
    }
    winRuleCur = NULL ;
    EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_UP),FALSE) ;
    EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_DOWN),FALSE) ;
    EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_MOD),FALSE) ;
    EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_DEL),FALSE) ;
}

static void
windowRuleDialogInit(HWND hDlg, int firstTime)
{
    TCHAR buff[MAX_PATH] ;
    HANDLE procHdl ;
    DWORD procId ;
    int ii ;
    
    if(firstTime)
    {
        SendDlgItemMessage(hDlg, IDC_WTYPE_HWACT, CB_ADDSTRING, 0, (LONG) _T("Default - configured in main Setup"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_HWACT, CB_ADDSTRING, 0, (LONG) _T("Ignore the event"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_HWACT, CB_ADDSTRING, 0, (LONG) _T("Move window to current desktop"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_HWACT, CB_ADDSTRING, 0, (LONG) _T("Show window on current desktop"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_HWACT, CB_ADDSTRING, 0, (LONG) _T("Change to window's desktop"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_WHIDE, CB_ADDSTRING, 0, (LONG) _T("Hide using standard method"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_WHIDE, CB_ADDSTRING, 0, (LONG) _T("Hide by move window"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_WHIDE, CB_ADDSTRING, 0, (LONG) _T("Hide by minimizing window"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_THIDE, CB_ADDSTRING, 0, (LONG) _T("Hide using standard method"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_THIDE, CB_ADDSTRING, 0, (LONG) _T("Show - Keep taskbar button visible"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_THIDE, CB_ADDSTRING, 0, (LONG) _T("Hide by using toolwin flag"));
        SendDlgItemMessage(hDlg, IDC_WTYPE_ENABLE,BM_SETCHECK,1,0);
        SendDlgItemMessage(hDlg, IDC_WTYPE_VMANAGE,BM_SETCHECK,1,0);
        SendDlgItemMessage(hDlg, IDC_WTYPE_HWACT, CB_SETCURSEL, 0, 0) ;
        SendDlgItemMessage(hDlg, IDC_WTYPE_WHIDE, CB_SETCURSEL, 0, 0) ;
        SendDlgItemMessage(hDlg, IDC_WTYPE_THIDE, CB_SETCURSEL, 0, 0) ;
        EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_AMDSK),FALSE) ;
        EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_AMIMM),FALSE) ;
        EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_APPLY),FALSE) ;
    }
    SendDlgItemMessage(hDlg,IDC_WTYPE_AMDSK,CB_RESETCONTENT,0, 0);
    for(ii=1 ; ii<=deskCount ; ii++)
    {
        _stprintf(buff,_T("%d"),ii) ;
        SendDlgItemMessage(hDlg, IDC_WTYPE_AMDSK, CB_ADDSTRING, 0, (LONG) buff) ;
    }
    SendDlgItemMessage(hDlg, IDC_WTYPE_AMDSK, CB_SETCURSEL, 0, 0) ;
    windowRuleDialogInitList(hDlg) ;
    windowRuleDialogInitItem(hDlg) ;
    if(initWin != NULL)
    {
        typedef DWORD (WINAPI *vwGETMODULEFILENAMEEX)(HANDLE,HMODULE,LPTSTR,DWORD) ;
        extern vwGETMODULEFILENAMEEX vwGetModuleFileNameEx ;
        
        buff[0] = 0 ;
        GetClassName(initWin,buff,MAX_PATH);
        SetDlgItemText(hDlg,wtypeNameEntry[0],buff) ;
        buff[0] = 0 ;
        GetWindowText(initWin,buff,MAX_PATH);
        if(buff[0] == 0)
            _tcscpy(buff,vwWTNAME_NONE);
        SetDlgItemText(hDlg,wtypeNameEntry[1],buff) ;
        buff[0] = 0 ;
        if((vwGetModuleFileNameEx != NULL) &&
           (GetWindowThreadProcessId(initWin,&procId) != 0) && (procId != 0) && 
           ((procHdl=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ,FALSE,procId)) != NULL))
        {
            vwGetModuleFileNameEx(procHdl,NULL,buff,MAX_PATH) ;
            CloseHandle(procHdl) ;
        }
        SetDlgItemText(hDlg,wtypeNameEntry[2],buff) ;
        initWin = NULL ;
    }
}

static void
windowRuleDialogSetItem(HWND hDlg)
{
    int ii, jj ;
    
    if((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_LIST,LB_GETCURSEL,0,0)) != LB_ERR)
    {
        jj = 0 ;
        winRuleCur = windowRuleList ;
        while(winRuleCur != NULL)
        {
            if((winRuleCur->flags & vwWTFLAGS_MOVE) && (winRuleCur->desk > deskCount))
                ii++ ;
            if(jj == ii)
                break ;
            jj++ ;
            winRuleCur = winRuleCur->next ;
        } 
    }
    else
        winRuleCur = NULL ;
    windowRuleDialogInitItem(hDlg) ;
}

static void
windowRuleDialogMoveUp(HWND hDlg)
{
    vwWindowRule *wt, *pwt, *ppwt ;
    
    if(winRuleCur != NULL)
    {
        ppwt = pwt = NULL ;
        wt = windowRuleList ;
        while((wt != winRuleCur) && (wt != NULL))
        {
            if(((wt->flags & vwWTFLAGS_MOVE) == 0) || (wt->desk <= deskCount))
            {
                ppwt = pwt ;
                pwt = wt ;
            }
            wt = wt->next ;
        }
        if(wt == NULL)
            winRuleCur = NULL ;
        else if(pwt != NULL)
        {
            wt = pwt ;
            while(wt->next != winRuleCur)
                wt = wt->next ;
            wt->next = winRuleCur->next ;
            if(pwt == windowRuleList)
                windowRuleList = winRuleCur ;
            else
            {
                wt = windowRuleList ;
                while(wt->next != pwt)
                    wt = wt->next ;
                wt->next = winRuleCur ;
            }
            winRuleCur->next = pwt ;
            if(ppwt == NULL)
                EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_UP),FALSE) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_DOWN),TRUE) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_APPLY),TRUE) ;
        }
    }
    windowRuleDialogInitList(hDlg) ;
}

static void
windowRuleDialogMoveDown(HWND hDlg)
{
    vwWindowRule *wt, *pwt ;
    
    if(winRuleCur != NULL)
    {
        wt = windowRuleList ;
        while((wt != winRuleCur) && (wt != NULL))
            wt = wt->next ;
        if(wt == NULL)
            winRuleCur = NULL ;
        else
        {
            while(((wt = wt->next) != NULL) && (wt->flags & vwWTFLAGS_MOVE) && (wt->desk > deskCount))
                ;
            if(wt != NULL)
            {
                if(winRuleCur == windowRuleList)
                    windowRuleList = winRuleCur->next ;
                else
                {
                    pwt = windowRuleList ;
                    while(pwt->next != winRuleCur)
                        pwt = pwt->next ;
                    pwt->next = winRuleCur->next ;
                }
                winRuleCur->next = wt->next ;
                wt->next = winRuleCur ;
                wt = winRuleCur ;
                while(((wt = wt->next) != NULL) && (wt->flags & vwWTFLAGS_MOVE) && (wt->desk > deskCount))
                    ;
                if(wt == NULL)
                    EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_DOWN),FALSE) ;
                EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_UP),TRUE) ;
                EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_APPLY),TRUE) ;
            }
        }
    }
    windowRuleDialogInitList(hDlg) ;
}

static void
windowRuleDialogAddMod(HWND hDlg, int add)
{
    vwWindowRule *wt ;
    int ii, ll, mallocErr=0 ;
    TCHAR buff[1024], *ss ;
    vwUInt flags ;
    
    if(add)
    {
        if((wt = calloc(1,sizeof(vwWindowRule))) == NULL)
            mallocErr = 1 ;
        else
        {
            wt->next = windowRuleList ;
            windowRuleList = wt ;
        }
    }
    else
    {
        wt = windowRuleList ;
        while((wt != winRuleCur) && (wt != NULL))
            wt = wt->next ;
    }
    if(wt != NULL)
    {
        flags = 0 ;
        
        ii = vwWTNAME_COUNT-1 ;
        do {
            GetDlgItemText(hDlg,wtypeNameEntry[ii],buff,1024) ;
            ss = buff ;
            if(ss[0] != '\0')
            {
                if(!_tcscmp(buff,vwWTNAME_NONE))
                    ss[0] = '\0' ;
                if(ss[0] == '*')
                {
                    flags |= 1 << (ii << 1) ;
                    ss++ ;
                }
                ll = _tcslen(ss) ;
                if((ll > 0) && (ss[ll-1] == '*'))
                {
                    flags |= 2 << (ii << 1) ;
                    ss[--ll] = '\0' ;
                }
                if((wt->name[ii] != NULL) && _tcscmp(ss,wt->name[ii]))
                {
                    free(wt->name[ii]) ;
                    wt->name[ii] = NULL ;
                    wt->nameLen[ii] = 0 ;
                }
                if(wt->name[ii] == NULL)
                {
                    if((wt->name[ii] = _tcsdup(ss)) == NULL)
                        mallocErr = 1 ;
                    else
                        wt->nameLen[ii] = ll ;
                }
            }
            else if(wt->name[ii] != NULL)
            {
                free(wt->name[ii]) ;
                wt->name[ii] = NULL ;
                wt->nameLen[ii] = 0 ;
            }
        } while(--ii >= 0) ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_ENABLE,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_ENABLED ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_CLOSE,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_CLOSE ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_ALONTOP,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_ALWAYSONTOP ;
        else if(SendDlgItemMessage(hDlg,IDC_WTYPE_NMANAGE,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_DONT_MANAGE ;
        else if(SendDlgItemMessage(hDlg,IDC_WTYPE_AMANAGE,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_MANAGE ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_MAINWIN,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_MAIN_WIN ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_STICKY,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_STICKY ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_GRPAPP,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_GROUP_APP ;
        wt->desk = 0 ;
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_AMOVE,BM_GETCHECK,0,0) == BST_CHECKED)
        {
            flags |= vwWTFLAGS_MOVE ;
            if((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_AMDSK,CB_GETCURSEL,0,0)) != CB_ERR)
                wt->desk = ii + 1 ;
        }
        if(SendDlgItemMessage(hDlg,IDC_WTYPE_AMIMM,BM_GETCHECK,0,0) == BST_CHECKED)
            flags |= vwWTFLAGS_MOVE_IMMEDIATE ;
        
        if((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_HWACT,CB_GETCURSEL,0,0)) != CB_ERR)
            flags |= ii << vwWTFLAGS_HWACT_BITROT ;
        if((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_WHIDE,CB_GETCURSEL,0,0)) != CB_ERR)
            flags |= ii << vwWTFLAGS_HIDEWIN_BITROT ;
        if((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_THIDE,CB_GETCURSEL,0,0)) != CB_ERR)
            flags |= ii << vwWTFLAGS_HIDETSK_BITROT ;
        wt->flags = flags ;
        winRuleCur = wt ;
        EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_APPLY),TRUE) ;
    }
    if(mallocErr)
        MessageBox(hWnd,_T("System resources are low, failed to create new window rule."),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
    windowRuleDialogInitList(hDlg) ;
    windowRuleDialogInitItem(hDlg) ;
}

static void
windowRuleDialogDelete(HWND hDlg)
{
    vwWindowRule *wt, *pwt ;
    int ii ;
    if(winRuleCur != NULL)
    {
        pwt = NULL ;
        wt = windowRuleList ;
        while((wt != winRuleCur) && (wt != NULL))
        {
            pwt = wt ;
            wt = wt->next ;
        }
        if(wt != NULL)
        {
            if(pwt == NULL)
                windowRuleList = wt->next ;
            else
                pwt->next = wt->next ;
            
            ii = vwWTNAME_COUNT-1 ;
            do {
                if(wt->name[ii] != NULL)
                    free(wt->name[ii]) ;
            } while(--ii >= 0) ;
            free(wt) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_APPLY),TRUE) ;
        }
        winRuleCur = NULL ;
    }
    windowRuleDialogInitList(hDlg) ;
}

static BOOL CALLBACK
windowRuleDialogFunc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int ii, jj ;
    switch (msg)
    {
    case WM_INITDIALOG:
        dialogHWnd = hDlg ;
        SetWindowPos(hDlg, 0, dialogPos[0], dialogPos[1], 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
        windowRuleDialogInit(hDlg,1) ;
        return TRUE;
        
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_WTYPE_LIST:
            if(HIWORD(wParam) == LBN_SELCHANGE)
                windowRuleDialogSetItem(hDlg) ;
            break ;
            
        case IDC_WTYPE_UP:
            windowRuleDialogMoveUp(hDlg) ;
            break ;
        
        case IDC_WTYPE_DOWN:
            windowRuleDialogMoveDown(hDlg) ;
            break ;
            
        case IDC_WTYPE_ADD:
            windowRuleDialogAddMod(hDlg,1) ;
            break ;
        
        case IDC_WTYPE_MOD:
            windowRuleDialogAddMod(hDlg,0) ;
            break ;
        
        case IDC_WTYPE_DEL:
            windowRuleDialogDelete(hDlg) ;
            break ;
        
        case IDC_WTYPE_AMOVE:
            ii = (SendDlgItemMessage(hDlg,IDC_WTYPE_AMOVE,BM_GETCHECK,0,0) == BST_CHECKED) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_AMDSK),ii) ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_AMIMM),ii) ;
            break ;
        
        case IDC_WTYPE_WHIDE:
            if((HIWORD(wParam) == CBN_SELCHANGE) &&
               ((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_WHIDE,CB_GETCURSEL,0,0)) != CB_ERR) &&
               ((jj=SendDlgItemMessage(hDlg,IDC_WTYPE_THIDE,CB_GETCURSEL,0,0)) != CB_ERR))
            {
                if((ii > 0) && (jj == 0))
                    SendDlgItemMessage(hDlg, IDC_WTYPE_THIDE, CB_SETCURSEL, 2, 0) ;
                else if((ii == 0) && (jj == 1))
                    SendDlgItemMessage(hDlg, IDC_WTYPE_THIDE, CB_SETCURSEL, 0, 0) ;
            }
            break ;
            
        case IDC_WTYPE_THIDE:
            if((HIWORD(wParam) == CBN_SELCHANGE) &&
               ((ii=SendDlgItemMessage(hDlg,IDC_WTYPE_THIDE,CB_GETCURSEL,0,0)) != CB_ERR) &&
               ((jj=SendDlgItemMessage(hDlg,IDC_WTYPE_WHIDE,CB_GETCURSEL,0,0)) != CB_ERR))
            {
                if((ii == 1) && (jj == 0))
                    SendDlgItemMessage(hDlg, IDC_WTYPE_WHIDE, CB_SETCURSEL, 2, 0) ;
                else if((ii == 0) && (jj != 0))
                    SendDlgItemMessage(hDlg, IDC_WTYPE_WHIDE, CB_SETCURSEL, 0, 0) ;
            }
            break ;

        case IDC_WTYPE_OK:
            if(IsWindowEnabled(GetDlgItem(hDlg,IDC_WTYPE_APPLY)))
                saveWindowConfig() ; 
            vwWindowRuleReapply() ;
            EndDialog(hDlg,0);
            return TRUE;
            
        case IDCANCEL:
            /* load original config back in */ 
            loadWindowConfig() ; 
            EndDialog(hDlg,0);
            return TRUE;
        
        case IDC_WTYPE_APPLY:
            saveWindowConfig() ; 
            vwWindowRuleReapply() ;
            EnableWindow(GetDlgItem(hDlg,IDC_WTYPE_APPLY),FALSE) ;
            break ;
            
        case IDC_WTYPE_HELP:
            showHelp(hDlg,_T("WindowRulesDialog.htm")) ;
            break ;
        }
        break ;
        
    case WM_CLOSE:
        /* load original config back in */ 
        loadWindowConfig() ; 
        EndDialog(hDlg,0);
        return TRUE;
	
    }
    return FALSE;
}

void
createWindowRuleDialog(HINSTANCE theHinst, HWND theHwndOwner, vwWindowRule *wtype, HWND theWin)
{
    if((deskCount = nDesks) < currentDesk)
        deskCount = currentDesk ;
    winRuleCur = wtype ;
    initWin = theWin ;
    dialogOpen = TRUE ;
    DialogBox(theHinst,MAKEINTRESOURCE(IDD_WINDOWRULEDIALOG),theHwndOwner,(DLGPROC) windowRuleDialogFunc) ;
    dialogOpen = FALSE ;
    dialogHWnd = NULL ;
}
