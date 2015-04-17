//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  SetupDialog.c - Setup Dialog routines.
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

// Must compile with define of _WIN32_IE=0x0200 otherwise the setup dialog
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

/* Get the list of hotkey commands */
#define VW_COMMAND(a, b, c, d) b ,
static unsigned char vwCommandEnum[]={
#include "vwCommands.def"
} ;
#undef  VW_COMMAND

#ifdef _UNICODE
#define VW_COMMAND(a, b, c, d) _T(d) ,
#else
#define VW_COMMAND(a, b, c, d) d ,
#endif
static TCHAR *vwCommandName[]={
#include "vwCommands.def"
} ;
#undef  VW_COMMAND

#define VW_COMMAND(a, b, c, d) c ,
static unsigned char vwCommandFlag[]={
#include "vwCommands.def"
} ;
#undef  VW_COMMAND

#define vwPROPSHEET_PAGE_COUNT 6

static int pageChangeMask=0 ;
static int pageApplyMask=0 ;
static HWND setupGeneralHWnd=NULL;
static HWND setupHotkeysHWnd=NULL;
static int vwSetupHotkeyCur ;
static unsigned char vwSetupHotkeyGotKey ;


static void
vwSetupHotKeysInitList(void)
{
    TCHAR buff[128], *ss;
    int ii, jj, cmd, deskCount ;
    
    if((deskCount = nDesks) < currentDesk)
        deskCount = currentDesk ;
    
    SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_LIST,LB_RESETCONTENT,0, 0);
    ii = 130 ;
    SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_LIST,LB_SETTABSTOPS,(WPARAM)1,(LPARAM)&ii);
    for(ii=0 ; ii<hotkeyCount ; ii++)
    {
        if(hotkeyList[ii].desk <= deskCount)
        {
            cmd = 0 ;
            while((vwCommandEnum[cmd] != 0) && (vwCommandEnum[cmd] != hotkeyList[ii].command))
                cmd++ ;
            ss = buff ;
			_tcscpy_s(ss, 128, vwCommandName[cmd]);
            if(hotkeyList[ii].desk && vwCommandEnum[cmd])
            {
                ss = _tcschr(ss,'#') ;
				ss += _stprintf_s(ss, 128, _T("%d"), hotkeyList[ii].desk);
				_tcscpy_s(ss, 128, _tcschr(vwCommandName[cmd], '#') + 1);
            }
            ss += _tcslen(ss) ;
            *ss++ = '\t' ;
            if(hotkeyList[ii].modifier & vwHOTKEY_ALT)
            {
                *ss++ = 'A' ;
                *ss++ = ' ' ;
                *ss++ = '+' ;
                *ss++ = ' ' ;
            }
            if(hotkeyList[ii].modifier & vwHOTKEY_CONTROL)
            {
                *ss++ = 'C' ;
                *ss++ = ' ' ;
                *ss++ = '+' ;
                *ss++ = ' ' ;
            }
            if(hotkeyList[ii].modifier & vwHOTKEY_SHIFT)
            {
                *ss++ = 'S' ;
                *ss++ = ' ' ;
                *ss++ = '+' ;
                *ss++ = ' ' ;
            }
            if(hotkeyList[ii].modifier & vwHOTKEY_WIN)
            {
                *ss++ = 'W' ;
                *ss++ = ' ' ;
                *ss++ = '+' ;
                *ss++ = ' ' ;
            }
            jj = MapVirtualKey((UINT) hotkeyList[ii].key,0);
            if(hotkeyList[ii].modifier & vwHOTKEY_EXT)
                jj |= 0x100 ;
            GetKeyNameText(jj << 16, ss, 20);
            SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_LIST,LB_ADDSTRING,0,(LONG) buff);
        }
    }
    if(vwSetupHotkeyCur >= 0)
    {
        for(ii=0, jj=0 ; ii != vwSetupHotkeyCur ; ii++)
            if(hotkeyList[ii].desk <= deskCount)
                jj++ ;
        SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_LIST,LB_SETCURSEL,jj,0) ;
    }
}

static void
vwSetupApply(HWND hDlg, int curPageMask)
{
    if(pageChangeMask)
    {
        pageApplyMask |= curPageMask ;
        if((pageApplyMask & pageChangeMask) == pageChangeMask)
        {
            /* All pages have now got any changes from the GUI, save them and apply */
            saveVirtuawinConfig();
            vwHotkeyUnregister(1);
            /* Need to get the taskbar again in case the order has changed to dynamic taskbar */
            vwTaskbarHandleGet();
            vwHookSetup();
            vwIconLoad();
            vwHotkeyRegister(1);
            enableMouse(mouseEnable);
            vwIconSet(currentDesk,0);
            /* update the hotkey dialog as user may have made desktops (and therefore a hidden hotkey) visible */ 
            vwSetupHotKeysInitList() ;
            /* Tell modules about the config change */
            vwModulesPostMessage(MOD_CFGCHANGE, 0, 0);
            pageChangeMask = 0 ;
            pageApplyMask = 0 ;
        }
    }
}

static void
vwSetupCancel(void)
{
    if(pageChangeMask)
    {
        // Reset to the original values.
        loadVirtuawinConfig();
        pageChangeMask = 0 ;
        pageApplyMask = 0 ;
    }
}

void
initDesktopProperties(void)
{
    TCHAR buff[32] ;
    int pcm = pageChangeMask ;
    pageChangeMask = -1 ;
	_stprintf_s(buff, 260, _T("Name of desktop %d:"), currentDesk);
    SetDlgItemText(setupGeneralHWnd, IDC_DESKTOPLBL, buff) ;
    SetDlgItemText(setupGeneralHWnd, IDC_DESKTOPNAME, (desktopName[currentDesk] != NULL) ? desktopName[currentDesk]:_T("")) ;
    pageChangeMask = pcm ;
}

void
storeDesktopProperties(void)
{
    TCHAR buff[64], *ss ;
    
    GetDlgItemText(setupGeneralHWnd,IDC_DESKTOPNAME,buff,64) ;
    if(((desktopName[currentDesk] == NULL) && (buff[0] != '\0')) ||
       ((desktopName[currentDesk] != NULL) && _tcscmp(buff,desktopName[currentDesk])))
    {
        if(desktopName[currentDesk] != NULL)
            free(desktopName[currentDesk]) ;
        if(buff[0] == '\0')
            desktopName[currentDesk] = NULL ;
        else
        {
            while((ss=_tcschr(buff,'\n')) != NULL)
                *ss = ' ' ;
            desktopName[currentDesk] = _tcsdup(buff) ;
        }
    }
}

/*************************************************
 * The "General" tab callback
 * This is the firts callback to be called when the property sheet is created
 */
BOOL APIENTRY
setupGeneral(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    static int tmpDesksY;
    static int tmpDesksX;
    WORD wPar;
    int maxDesk ;
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            dialogHWnd = GetParent(hDlg) ;
            setupGeneralHWnd = hDlg ;
            pageChangeMask = 0 ;
            pageApplyMask = 0 ;
            
            SetDlgItemInt(hDlg, IDC_DESKY, nDesksY, FALSE);
            SetDlgItemInt(hDlg, IDC_DESKX, nDesksX, FALSE);
            tmpDesksY = nDesksY;
            tmpDesksX = nDesksX;
            // Set the spin buddy controls
            SendMessage(GetDlgItem(hDlg, IDC_SLIDERX), UDM_SETBUDDY, (LONG) GetDlgItem(hDlg, IDC_DESKX), 0L );
            SendMessage(GetDlgItem(hDlg, IDC_SLIDERY), UDM_SETBUDDY, (LONG) GetDlgItem(hDlg, IDC_DESKY), 0L );
            // Set spin ranges
            SendMessage(GetDlgItem(hDlg, IDC_SLIDERX), UDM_SETRANGE, 0L, MAKELONG(vwDESKTOP_MAX, 1));
            SendMessage(GetDlgItem(hDlg, IDC_SLIDERY), UDM_SETRANGE, 0L, MAKELONG(vwDESKTOP_MAX, 1));
            // Set init values
            SendMessage(GetDlgItem(hDlg, IDC_SLIDERX), UDM_SETPOS, 0L, MAKELONG(nDesksX, 0));
            SendMessage(GetDlgItem(hDlg, IDC_SLIDERY), UDM_SETPOS, 0L, MAKELONG(nDesksY, 0));
            
            if(deskWrap)
                SendDlgItemMessage(hDlg, IDC_DESKCYCLE, BM_SETCHECK, 1, 0);
            if(hotkeyMenuLoc)
                SendDlgItemMessage(hDlg, IDC_HOTKEYMENULOC, BM_SETCHECK, 1, 0);
            if(winListCompact)
                SendDlgItemMessage(hDlg, IDC_COMPACTWLIST, BM_SETCHECK, 1, 0);
            if(winMenuCompact)
                SendDlgItemMessage(hDlg, IDC_COMPACTWMENU, BM_SETCHECK, 1, 0);
            if(ctlMenuCompact)
                SendDlgItemMessage(hDlg, IDC_COMPACTCMENU, BM_SETCHECK, 1, 0);
            if(winListContent & vwWINLIST_ACCESS)
                SendDlgItemMessage(hDlg, IDC_MENUACCESS, BM_SETCHECK, 1, 0);
            if(winListContent & vwWINLIST_ASSIGN)
                SendDlgItemMessage(hDlg, IDC_MENUMOVE, BM_SETCHECK, 1, 0);
            if(winListContent & vwWINLIST_SHOW)
                SendDlgItemMessage(hDlg, IDC_MENUSHOW, BM_SETCHECK, 1, 0);
            if(winListContent & vwWINLIST_STICKY)
                SendDlgItemMessage(hDlg, IDC_MENUSTICKY, BM_SETCHECK, 1, 0);
            if(winListContent & vwWINLIST_TITLELN)
                SendDlgItemMessage(hDlg, IDC_WLUSETTLLN, BM_SETCHECK, 1, 0);

            /* Currect desktop properties */
            initDesktopProperties() ;
            setForegroundWin(dialogHWnd,1);
            SetWindowPos(dialogHWnd,HWND_NOTOPMOST,dialogPos[0],dialogPos[1],0,0,SWP_NOSIZE | SWP_NOACTIVATE) ;
            return TRUE ;
        }
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE: // Getting focus
            // Initialize the controls.
            break;
        case PSN_APPLY: // Apply, OK
            tmpDesksY = GetDlgItemInt(hDlg, IDC_DESKY, NULL, FALSE);
            tmpDesksX = GetDlgItemInt(hDlg, IDC_DESKX, NULL, FALSE);
            maxDesk = tmpDesksX * tmpDesksY ;
            if(maxDesk < nDesks)
            {
                vwWindow *win ;
                
                if(currentDesk > maxDesk)
                    // user is on an invalid desk, move
                    gotoDesk(1,TRUE) ;
                vwMutexLock();
                windowListUpdate() ;
                win = windowList ;
                while(win != NULL)
                {
                    if((win->desk > maxDesk) && (win->desk <= nDesks))
                    {
                        // This window is on an invalid desk, move
                        vwMutexRelease();
                        assignWindow(win->handle,1,FALSE,TRUE,FALSE);
                        // must start again as the list will have been updated
                        vwMutexLock();
                        win = windowList ;
                    }
                    win = win->next ;
                }
                vwMutexRelease();
            }
            else if((maxDesk > nDesks) && (deskImageCount >= 0))
            {
                do {
                    nDesks++ ;
                    createDeskImage(nDesks,1) ;
                } while(nDesks < maxDesk) ;
            }
            nDesksY = tmpDesksY;
            nDesksX = tmpDesksX;
            nDesks = maxDesk ;
            
            deskWrap = (SendDlgItemMessage(hDlg, IDC_DESKCYCLE, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            hotkeyMenuLoc = (SendDlgItemMessage(hDlg, IDC_HOTKEYMENULOC, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            winListCompact = (SendDlgItemMessage(hDlg, IDC_COMPACTWLIST, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            winMenuCompact = (SendDlgItemMessage(hDlg, IDC_COMPACTWMENU, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            ctlMenuCompact = (SendDlgItemMessage(hDlg, IDC_COMPACTCMENU, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            winListContent = 0 ;
            if(SendDlgItemMessage(hDlg, IDC_MENUACCESS, BM_GETCHECK, 0, 0) == BST_CHECKED)
                winListContent |= vwWINLIST_ACCESS ;
            if(SendDlgItemMessage(hDlg, IDC_MENUMOVE, BM_GETCHECK, 0, 0) == BST_CHECKED)
                winListContent |= vwWINLIST_ASSIGN ;
            if(SendDlgItemMessage(hDlg, IDC_MENUSHOW, BM_GETCHECK, 0, 0) == BST_CHECKED)
                winListContent |= vwWINLIST_SHOW ;
            if(SendDlgItemMessage(hDlg, IDC_MENUSTICKY, BM_GETCHECK, 0, 0) == BST_CHECKED)
                winListContent |= vwWINLIST_STICKY ;
            if(SendDlgItemMessage(hDlg, IDC_WLUSETTLLN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                winListContent |= vwWINLIST_TITLELN ;
            storeDesktopProperties() ;
            
            vwSetupApply(hDlg,0x01) ;
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;
        case PSN_KILLACTIVE: // Switch tab sheet
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;
        case PSN_RESET: // Cancel
            vwSetupCancel() ;
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;
        case PSN_HELP:
            showHelp(hDlg,_T("SetupDialog.htm#General")) ;
            break;
        }
        break;
        
    case WM_COMMAND:
        wPar = LOWORD(wParam);
        if(wPar == IDC_DESKX)
        {
            if(HIWORD(wParam) == EN_CHANGE)
                pageChangeMask |= 0x01 ;
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            tmpDesksX = SendMessage(GetDlgItem(hDlg, IDC_SLIDERX), UDM_GETPOS, 0, 0);
            if((tmpDesksX * tmpDesksY) > vwDESKTOP_MAX)
            {
                while((tmpDesksX * tmpDesksY) > vwDESKTOP_MAX)
                    tmpDesksY--;
                SendMessage(GetDlgItem(hDlg, IDC_SLIDERY), UDM_SETPOS, 0L, MAKELONG( tmpDesksY, 0));
            }
            SetFocus(hDlg) ;
        }
        else if(wPar == IDC_DESKY)
        {
            if(HIWORD(wParam) == EN_CHANGE)
                pageChangeMask |= 0x01 ;
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            tmpDesksY = SendMessage(GetDlgItem(hDlg, IDC_SLIDERY), UDM_GETPOS, 0, 0);
            if((tmpDesksY * tmpDesksX) > vwDESKTOP_MAX)
            {
                while ((tmpDesksY * tmpDesksX) > vwDESKTOP_MAX)
                    tmpDesksX--;
                SendMessage(GetDlgItem(hDlg, IDC_SLIDERX), UDM_SETPOS, 0L, MAKELONG( tmpDesksX, 0));
            }
            SetFocus(hDlg) ;
        }
        else if((pageChangeMask >= 0) &&
                ((wPar == IDC_DESKCYCLE)   || (wPar == IDC_COMPACTWLIST)  ||
                 (wPar == IDC_MENUMOVE)    || (wPar == IDC_COMPACTWMENU)  ||
                 (wPar == IDC_MENUSHOW)    || (wPar == IDC_COMPACTCMENU)  ||
                 (wPar == IDC_MENUACCESS)  || (wPar == IDC_HOTKEYMENULOC) ||
                 (wPar == IDC_WLUSETTLLN)  || (wPar == IDC_MENUSTICKY)    ||
                 (wPar == IDC_DESKTOPNAME  && HIWORD(wParam) == EN_CHANGE)))
        {
            pageChangeMask |= 0x01 ;
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L); // Enable apply button
        }
        else if(wPar == IDC_DESKTOPBTN)
        {
            if((maxDesk = currentDesk + 1) > (tmpDesksX * tmpDesksY))
                maxDesk = 1 ;
            gotoDesk(maxDesk,TRUE) ;
        }
        break;
    }
    return (FALSE);
}

/*************************************************
 * The "Hotkeys" tab callback
 */
static void
vwSetupHotKeysInit(void)
{
    TCHAR buff[10] ;
    int ii, jj ;
    
    for(ii=0 ; vwCommandEnum[ii] != 0 ; ii++)
        SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_CMD,CB_ADDSTRING,0,(LONG) vwCommandName[ii]) ;
    SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_CMD,CB_SETCURSEL,0,0) ;
    
    if((jj = nDesks) < currentDesk)
        jj = currentDesk ;
    for(ii=1 ; ii<=jj ; ii++)
    {
        _stprintf_s(buff, 10, _T("%d"),ii) ;
        SendDlgItemMessage(setupHotkeysHWnd, IDC_HOTKEY_DSK, CB_ADDSTRING, 0, (LONG) buff) ;
    }
    SendDlgItemMessage(setupHotkeysHWnd,IDC_HOTKEY_DSK,CB_SETCURSEL,currentDesk-1,0) ;
    EnableWindow(GetDlgItem(setupHotkeysHWnd,IDC_HOTKEY_DSK),FALSE) ;
    EnableWindow(GetDlgItem(setupHotkeysHWnd,IDC_HOTKEY_ADD),FALSE) ;
    EnableWindow(GetDlgItem(setupHotkeysHWnd,IDC_HOTKEY_MOD),FALSE) ;
    EnableWindow(GetDlgItem(setupHotkeysHWnd,IDC_HOTKEY_DEL),FALSE) ;
    vwSetupHotkeyCur = -1 ;
    vwSetupHotkeyGotKey = 0 ;
    vwSetupHotKeysInitList() ;
}


static void
vwSetupHotKeysSetCommand(HWND hDlg)
{
    int ii ;
    if((ii=SendDlgItemMessage(hDlg,IDC_HOTKEY_CMD,CB_GETCURSEL,0,0)) != CB_ERR)
    {
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_DSK),(vwCommandFlag[ii] & 0x01)) ;
        if(vwCommandFlag[ii] & 0x02)
            EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_WUM),1) ;
        else
        {
            EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_WUM),0) ;
            SendDlgItemMessage(hDlg,IDC_HOTKEY_WUM,BM_SETCHECK,((vwCommandFlag[ii] & 0x04) != 0), 0);
        }
        if((vwSetupHotkeyCur >= 0) && vwSetupHotkeyGotKey &&
           (vwCommandEnum[ii] != hotkeyList[vwSetupHotkeyCur].command))
        {
            EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_ADD),TRUE) ;
            EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),TRUE) ;
        }
    }
}

static void
vwSetupHotKeysSetDesk(HWND hDlg)
{
    int ii ;
    if((vwSetupHotkeyCur >= 0) && vwSetupHotkeyGotKey &&
       ((ii=SendDlgItemMessage(hDlg,IDC_HOTKEY_DSK,CB_GETCURSEL,0,0)) != CB_ERR) &&
       ((ii + 1) != hotkeyList[vwSetupHotkeyCur].desk))
    {
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_ADD),TRUE) ;
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),TRUE) ;
    }
}

static void
vwSetupHotKeysSetKey(HWND hDlg)
{
    vwSetupHotkeyGotKey = ((SendDlgItemMessage(hDlg,IDC_HOTKEY_ENT,HKM_GETHOTKEY,0,0) & 0x0ff) != 0) ;
    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_ADD),vwSetupHotkeyGotKey) ;
    if(vwSetupHotkeyCur >= 0)
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),vwSetupHotkeyGotKey) ;
}

static void
vwSetupHotKeysSetItem(HWND hDlg)
{
    int ii, deskCount ;
    
    if((ii=SendDlgItemMessage(hDlg,IDC_HOTKEY_LIST,LB_GETCURSEL,0,0)) != LB_ERR)
    {
        if((deskCount = nDesks) < currentDesk)
            deskCount = currentDesk ;
        
        for(vwSetupHotkeyCur = 0 ; ; vwSetupHotkeyCur++)
        {
            if(hotkeyList[vwSetupHotkeyCur].desk > deskCount)
                ii++ ;
            if(vwSetupHotkeyCur == ii)
                break ;
        } 
        if(vwSetupHotkeyCur >= hotkeyCount)
            vwSetupHotkeyCur = -1 ;
    }
    else
        vwSetupHotkeyCur = -1 ;
    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_DEL),(vwSetupHotkeyCur >= 0)) ;
    if(vwSetupHotkeyCur >= 0)
    {
        ii = 0 ;
        while(vwCommandEnum[ii] != hotkeyList[vwSetupHotkeyCur].command)
        {
            if(vwCommandEnum[ii] == 0)
            {
                ii = 0 ;
                break ;
            }
            ii++ ;
        }
        SendDlgItemMessage(hDlg, IDC_HOTKEY_CMD, CB_SETCURSEL, ii, 0) ;
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_DSK),(hotkeyList[vwSetupHotkeyCur].desk > 0)) ;
        if(hotkeyList[vwSetupHotkeyCur].desk > 0)
            SendDlgItemMessage(hDlg,IDC_HOTKEY_DSK,CB_SETCURSEL,hotkeyList[vwSetupHotkeyCur].desk - 1,0) ;
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_WUM),((vwCommandFlag[ii] & 0x2) != 0)) ;
        SendDlgItemMessage(hDlg,IDC_HOTKEY_WUM,BM_SETCHECK,((hotkeyList[vwSetupHotkeyCur].modifier & vwHOTKEY_WIN_MOUSE) != 0), 0);
        
        vwSetupHotkeyGotKey = 1 ;
        ii = 0 ;
        if(hotkeyList[vwSetupHotkeyCur].modifier & vwHOTKEY_ALT)
            ii |= HOTKEYF_ALT ;
        if(hotkeyList[vwSetupHotkeyCur].modifier & vwHOTKEY_CONTROL)
            ii |= HOTKEYF_CONTROL;
        if(hotkeyList[vwSetupHotkeyCur].modifier & vwHOTKEY_SHIFT)
            ii |= HOTKEYF_SHIFT;
        if(hotkeyList[vwSetupHotkeyCur].modifier & vwHOTKEY_EXT)
            ii |= HOTKEYF_EXT;
        SendDlgItemMessage(hDlg,IDC_HOTKEY_ENT,HKM_SETHOTKEY,MAKEWORD(hotkeyList[vwSetupHotkeyCur].key,ii), 0);
        SendDlgItemMessage(hDlg,IDC_HOTKEY_WIN,BM_SETCHECK,((hotkeyList[vwSetupHotkeyCur].modifier & vwHOTKEY_WIN) != 0), 0);
    }
    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),FALSE) ;
    
}

static void
vwSetupHotKeysAddMod(HWND hDlg, int add)
{
    int ii, key, mod, cmd, dsk ;
    
    if(!add && (vwSetupHotkeyCur < 0))
        return ;
    if(((key=SendDlgItemMessage(hDlg,IDC_HOTKEY_ENT,HKM_GETHOTKEY,0,0)) > 0) &&
       ((cmd=SendDlgItemMessage(hDlg,IDC_HOTKEY_CMD,CB_GETCURSEL,0,0)) != CB_ERR) &&
       ((dsk=SendDlgItemMessage(hDlg,IDC_HOTKEY_DSK,CB_GETCURSEL,0,0)) != CB_ERR))
    {
        ii = HIBYTE(key) ;
        mod = 0 ;
        if(ii & HOTKEYF_ALT)
            mod |= vwHOTKEY_ALT ;
        if(ii & HOTKEYF_CONTROL)
            mod |= vwHOTKEY_CONTROL;
        if(ii & HOTKEYF_SHIFT)
            mod |= vwHOTKEY_SHIFT;
        if(ii & HOTKEYF_EXT)
            mod |= vwHOTKEY_EXT;
        if(SendDlgItemMessage(hDlg, IDC_HOTKEY_WIN, BM_GETCHECK, 0, 0) == BST_CHECKED)
            mod |= vwHOTKEY_WIN ;
        key = LOBYTE(key) ;
        ii = hotkeyCount ;
        while(--ii >= 0)
        {
            if((hotkeyList[ii].key == key) && ((hotkeyList[ii].modifier & (vwHOTKEY_MOD_MASK|vwHOTKEY_EXT)) == mod) &&
               (add || (ii != vwSetupHotkeyCur)))
            {
                MessageBox(hWnd,_T("Another command is already bound to this hotkey."),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
                return ;
            }
        }
        if(add)
        {
            if(hotkeyCount == vwHOTKEY_MAX)
            {
                MessageBox(hWnd,_T("Maximum number of hotkeys has been reached.\n\nPlease report this to problem."),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
                return ;
            }
            vwSetupHotkeyCur = hotkeyCount++ ;
        }
        if((vwCommandFlag[cmd] & 0x04) || ((vwCommandFlag[cmd] & 0x02) && (SendDlgItemMessage(hDlg, IDC_HOTKEY_WUM, BM_GETCHECK, 0, 0) == BST_CHECKED)))
            mod |= vwHOTKEY_WIN_MOUSE ;
        hotkeyList[vwSetupHotkeyCur].key = key ;
        hotkeyList[vwSetupHotkeyCur].modifier = mod ;
        hotkeyList[vwSetupHotkeyCur].command = vwCommandEnum[cmd] ;
        hotkeyList[vwSetupHotkeyCur].desk = (vwCommandFlag[cmd] & 0x01) ? dsk+1:0 ;
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_ADD),FALSE) ;
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),FALSE) ;
        EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_DEL),TRUE) ;
        vwSetupHotKeysInitList() ;
        pageChangeMask |= 0x02 ;
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
    }
}

static void
vwSetupHotKeysDelete(HWND hDlg)
{
    int ii ;
    
    if((ii=vwSetupHotkeyCur) < 0)
        return ;
    while(++ii < hotkeyCount)
    {
        hotkeyList[ii-1].key = hotkeyList[ii].key ;
        hotkeyList[ii-1].modifier = hotkeyList[ii].modifier ;
        hotkeyList[ii-1].command = hotkeyList[ii].command ;
        hotkeyList[ii-1].desk = hotkeyList[ii].desk ;
    }
    hotkeyCount-- ;
    vwSetupHotkeyCur = -1 ;
    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_ADD),TRUE) ;
    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),FALSE) ;
    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_DEL),FALSE) ;
    vwSetupHotKeysInitList() ;
    pageChangeMask |= 0x02 ;
    SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
}

BOOL APIENTRY setupHotkeys(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        setupHotkeysHWnd = hDlg ;
        vwSetupHotKeysInit() ;
        return TRUE;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_SETACTIVE:
            // Initialize the controls.
            break;
        case PSN_APPLY:
            vwSetupApply(hDlg,0x02) ;
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;
        case PSN_KILLACTIVE:
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;
        case PSN_RESET:
            vwSetupCancel() ;
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;
        case PSN_HELP:
            showHelp(hDlg,_T("SetupDialog.htm#Hotkeys")) ;
            break;
        }
        break;
        
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_HOTKEY_LIST:
            if(HIWORD(wParam) == LBN_SELCHANGE)
                vwSetupHotKeysSetItem(hDlg) ;
            break ;
        
        case IDC_HOTKEY_CMD:
            if(HIWORD(wParam) == CBN_SELCHANGE)
                vwSetupHotKeysSetCommand(hDlg) ;
            break;
        
        case IDC_HOTKEY_DSK:
            if(HIWORD(wParam) == CBN_SELCHANGE)
                vwSetupHotKeysSetDesk(hDlg) ;
            break;
        
        case IDC_HOTKEY_ENT:
            if(HIWORD(wParam) == EN_CHANGE)
                vwSetupHotKeysSetKey(hDlg) ;
            break;
        
        case IDC_HOTKEY_WIN:
        case IDC_HOTKEY_WUM:
            if(vwSetupHotkeyGotKey)
            {
                EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_ADD),TRUE) ;
                if(vwSetupHotkeyCur >= 0)
                    EnableWindow(GetDlgItem(hDlg,IDC_HOTKEY_MOD),TRUE) ;
            }
            break;
        
        case IDC_HOTKEY_ADD:
            vwSetupHotKeysAddMod(hDlg,1) ;
            break;
        case IDC_HOTKEY_MOD:
            vwSetupHotKeysAddMod(hDlg,0) ;
            break;
        case IDC_HOTKEY_DEL:
            vwSetupHotKeysDelete(hDlg) ;
            break;
        }
        break;
    }
    return (FALSE);
}

/*************************************************
 * The "Mouse" tab callback
 */
BOOL APIENTRY
setupMouse(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    TCHAR buff[5];
    
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDC_TIME, mouseDelay * 25, FALSE);
        SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETRANGE, TRUE, MAKELONG(1, 80));
        SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_SETPOS, TRUE, mouseDelay >> 1);
        SetDlgItemInt(hDlg, IDC_JUMP, mouseJumpLength, TRUE);
        
        if(mouseEnable & 1)
            SendDlgItemMessage(hDlg, IDC_ENABLEMOUSE, BM_SETCHECK, 1,0);
        if(mouseEnable & 0x10)
            SendDlgItemMessage(hDlg, IDC_MOUSEDRAG, BM_SETCHECK, 1,0);
        if(mouseModifierUsed)
            SendDlgItemMessage(hDlg, IDC_KEYCONTROL, BM_SETCHECK, 1,0);
        if(mouseModifier & vwHOTKEY_ALT)
            SendDlgItemMessage(hDlg, IDC_MALT, BM_SETCHECK, 1,0);
        if(mouseModifier & vwHOTKEY_CONTROL)
            SendDlgItemMessage(hDlg, IDC_MCTRL, BM_SETCHECK, 1,0);
        if(mouseModifier & vwHOTKEY_SHIFT)
            SendDlgItemMessage(hDlg, IDC_MSHIFT, BM_SETCHECK, 1,0);
        if(mouseModifier & vwHOTKEY_WIN)
            SendDlgItemMessage(hDlg, IDC_MWIN, BM_SETCHECK, 1,0);
        if(mouseKnock & 1) 
            SendDlgItemMessage(hDlg, IDC_KNOCKMODE1, BM_SETCHECK, 1,0);
        if(mouseKnock & 2) 
            SendDlgItemMessage(hDlg, IDC_KNOCKMODE2, BM_SETCHECK, 1,0);
        if(mouseWarp)
            SendDlgItemMessage(hDlg, IDC_MOUSEWARP, BM_SETCHECK, 1,0);
        if(mouseEnable & 8)
            SendDlgItemMessage(hDlg, IDC_MOUSEMDCHNG, BM_SETCHECK, 1,0);
        if(mouseEnable & 4)
            SendDlgItemMessage(hDlg, IDC_MOUSEWLIST, BM_SETCHECK, 1,0);
        if(mouseEnable & 2)
            SendDlgItemMessage(hDlg, IDC_MOUSEWMENU, BM_SETCHECK, 1,0);
        return TRUE;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE:
            // Initialize the controls. Only if we want to reinitialize on tab change.
            break;
            
        case PSN_APPLY:
            mouseEnable = (SendDlgItemMessage(hDlg, IDC_ENABLEMOUSE, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            if(SendDlgItemMessage(hDlg, IDC_MOUSEDRAG, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseEnable |= 0x10 ;
            if(SendDlgItemMessage(hDlg, IDC_MOUSEWMENU, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseEnable |= 2 ;
            if(SendDlgItemMessage(hDlg, IDC_MOUSEWLIST, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseEnable |= 4 ;
            if(SendDlgItemMessage(hDlg, IDC_MOUSEMDCHNG, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseEnable |= 8 ;
            GetDlgItemText(hDlg, IDC_JUMP, buff, 4);
            mouseJumpLength = _ttoi(buff);
            mouseDelay = (SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_GETPOS, 0, 0)) << 1 ;
            mouseWarp = (SendDlgItemMessage(hDlg, IDC_MOUSEWARP, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            mouseKnock = (SendDlgItemMessage(hDlg, IDC_KNOCKMODE1, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            if(mouseKnock && (mouseJumpLength < 24))
            {
                mouseKnock = 0 ;
                SendDlgItemMessage(hDlg, IDC_KNOCKMODE1, BM_SETCHECK, 0,0);
                MessageBox(hDlg,_T("Mouse jump length is too small to support knocking, must be 24 or greater."),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR); 
            }
            if(SendDlgItemMessage(hDlg, IDC_KNOCKMODE2, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseKnock |= 2 ;
                
            mouseModifierUsed = (SendDlgItemMessage(hDlg, IDC_KEYCONTROL, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            mouseModifier = 0 ;
            if(SendDlgItemMessage(hDlg, IDC_MALT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseModifier |= vwHOTKEY_ALT ;
            if(SendDlgItemMessage(hDlg, IDC_MCTRL, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseModifier |= vwHOTKEY_CONTROL ;
            if(SendDlgItemMessage(hDlg, IDC_MSHIFT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseModifier |= vwHOTKEY_SHIFT ;
            if(SendDlgItemMessage(hDlg, IDC_MWIN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                mouseModifier |= vwHOTKEY_WIN ;
            
            vwSetupApply(hDlg,0x04) ;
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;
        case PSN_KILLACTIVE:
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;
        case PSN_RESET:
            vwSetupCancel() ;
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;
        case PSN_HELP:
            showHelp(hDlg,_T("SetupDialog.htm#Mouse")) ;
            break;
        }
        break;
        
    case WM_COMMAND:
        if(LOWORD(wParam) == IDC_JUMP       || LOWORD(wParam) == IDC_ENABLEMOUSE ||
           LOWORD(wParam) == IDC_MALT       || LOWORD(wParam) == IDC_KEYCONTROL  ||
           LOWORD(wParam) == IDC_MCTRL      || LOWORD(wParam) == IDC_KNOCKMODE1  ||
           LOWORD(wParam) == IDC_MSHIFT     || LOWORD(wParam) == IDC_KNOCKMODE2  ||
           LOWORD(wParam) == IDC_MWIN       || LOWORD(wParam) == IDC_MOUSEWMENU  ||
           LOWORD(wParam) == IDC_MOUSEWARP  || LOWORD(wParam) == IDC_MOUSEWLIST  ||
           LOWORD(wParam) == IDC_MOUSEDRAG  || LOWORD(wParam) == IDC_MOUSEMDCHNG )
        {
            pageChangeMask |= 0x04 ;
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L); // Enable apply
        }
        break;
        
    case WM_HSCROLL:
        pageChangeMask |= 0x04 ;
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        SetDlgItemInt(hDlg, IDC_TIME, (SendDlgItemMessage(hDlg, IDC_SLIDER, TBM_GETPOS, 0, 0) * 50), TRUE);
        break;
    }
    return FALSE;
}

/*************************************************
 * The "Modules" tab callback
 */
static void
setupModulesList(HWND hDlg)
{
    int index;
    TCHAR tmpName[128];
    
    SendDlgItemMessage(hDlg, IDC_MODLIST, LB_RESETCONTENT, 0, 0);
    for(index = 0; index < moduleCount; index++)
    {
        _tcsncpy_s(tmpName,128, moduleList[index].description,100) ;
        tmpName[100] = '\0' ;
        if(moduleList[index].disabled)
			_tcscat_s(tmpName, 100, _T(" (disabled)"));
        else if(moduleList[index].handle == NULL)
			_tcscat_s(tmpName, 100, _T(" (failed to run)"));
        SendDlgItemMessage(hDlg, IDC_MODLIST, LB_ADDSTRING, 0, (LONG)tmpName);
    }
}

BOOL APIENTRY
setupModules(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    int index;
    
    switch (message)
    {
    case WM_INITDIALOG:
        setupModulesList(hDlg) ;
        return TRUE;
    
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE:
            // Initialize the controls.
            break;
        case PSN_APPLY:
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;
        case PSN_KILLACTIVE:
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;
        case PSN_RESET:
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;
        case PSN_HELP:
            showHelp(hDlg,_T("SetupDialog.htm#Modules")) ;
            break;
        }
        break;
        
    case WM_COMMAND:
        if(LOWORD((wParam) == IDC_MODCONFIG || HIWORD(wParam) == LBN_DBLCLK ))
        {   // Show config
            int curSel = SendDlgItemMessage(hDlg, IDC_MODLIST, LB_GETCURSEL, 0, 0);
            if((curSel != LB_ERR) && (moduleList[curSel].handle != NULL))
                PostMessage(moduleList[curSel].handle, MOD_SETUP, (UINT) hDlg, 0);
        }
        else if(LOWORD((wParam) == IDC_MODRELOAD))
        {   
            /* Unload all modules currently running */
            vwMenuItem *mi ;
            vwModulesSendMessage(MOD_QUIT, 0, 0);
            for(index = 0; index < MAXMODULES; index++)
            {
                moduleList[index].handle = NULL;
                moduleList[index].description[0] = '\0';
            }
            moduleCount = 0;
            /* free off all module inserted menu items and reset ICHANGE and image generation */
            while((mi = ctlMenuItemList) != NULL)
            {
                ctlMenuItemList = mi->next ;
                free(mi) ;
            }
            ichangeHWnd = NULL ;
            disableDeskImage(deskImageCount) ;
            /* sleep for a second to allow the modules to exit cleanly */
            Sleep(1000) ;
            curDisabledMod = loadDisabledModules(disabledModules);
            vwModulesLoad();
            setupModulesList(hDlg) ;
        }
        else if(LOWORD((wParam) == IDC_MODDISABLE))
        {   // Enable/Disable
            int curSel = SendDlgItemMessage(hDlg, IDC_MODLIST, LB_GETCURSEL, 0, 0);
            if(curSel != LB_ERR)
            {
                if(moduleList[curSel].disabled == FALSE)
                {   // let's disable
                    moduleList[curSel].disabled = TRUE;
                    if(moduleList[curSel].handle != NULL)
                    {
                        PostMessage(moduleList[curSel].handle, MOD_QUIT, 0, 0);
                        if(ichangeHWnd == moduleList[curSel].handle)
                            ichangeHWnd = NULL ;
                    }
                }
                else
                {   // let's enable & load
                    moduleList[curSel].disabled = FALSE;
                    vwModuleLoad(curSel,NULL) ;
                }
                saveDisabledList(moduleCount, moduleList);
                setupModulesList(hDlg) ;
                SendDlgItemMessage(hDlg,IDC_MODLIST,LB_SETCURSEL,curSel,0) ;
            }
        }
        else if(LOWORD(wParam) == IDC_GETMODS)
        {
            HINSTANCE h = ShellExecute(NULL,_T("open"),vwVIRTUAWIN_MODULES_URL,NULL,NULL,SW_SHOWNORMAL);
            if((UINT)h < 33)
                MessageBox(hDlg,_T("Error opening modules link."),vwVIRTUAWIN_NAME _T(" Error"),MB_ICONWARNING);
        }
        break;
    }
    
    return (FALSE);
}

/*************************************************
 * The "Expert" tab callback
 */
BOOL APIENTRY
setupExpert(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    int oldLogFlag ;
    
    switch (message)
    {
    case WM_INITDIALOG:
        SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_ADDSTRING, 0, (LONG) _T("Static taskbar order"));
        SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_ADDSTRING, 0, (LONG) _T("Z order"));
        SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_ADDSTRING, 0, (LONG) _T("Static taskbar & Z order"));
        SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_ADDSTRING, 0, (LONG) _T("Dynamic taskbar order"));
        SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_ADDSTRING, 0, (LONG) _T("Dynamic taskbar & Z order"));
        SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_SETCURSEL, preserveZOrder, 0) ;
        SendDlgItemMessage(hDlg, IDC_HIDWINACT, CB_ADDSTRING, 0, (LONG) _T("Ignore the event"));
        SendDlgItemMessage(hDlg, IDC_HIDWINACT, CB_ADDSTRING, 0, (LONG) _T("Move window to current desktop"));
        SendDlgItemMessage(hDlg, IDC_HIDWINACT, CB_ADDSTRING, 0, (LONG) _T("Show window on current desktop"));
        SendDlgItemMessage(hDlg, IDC_HIDWINACT, CB_ADDSTRING, 0, (LONG) _T("Move to window's desktop"));
        SendDlgItemMessage(hDlg, IDC_HIDWINACT, CB_SETCURSEL, hiddenWindowAct, 0) ;
        if(!releaseFocus)
            SendDlgItemMessage(hDlg, IDC_FOCUS, BM_SETCHECK, 1,0);
        if(refreshOnWarp)
            SendDlgItemMessage(hDlg, IDC_REFRESH, BM_SETCHECK, 1,0);
        if(invertY)
            SendDlgItemMessage(hDlg, IDC_INVERTY, BM_SETCHECK, 1,0);
        if(!displayTaskbarIcon)
            SendDlgItemMessage(hDlg, IDC_DISPLAYICON, BM_SETCHECK, 1,0);
        if((noTaskbarCheck & 0x01) == 0)
            SendDlgItemMessage(hDlg, IDC_TASKBARDETECT, BM_SETCHECK, 1,0);
        if(vwHookUse)
            SendDlgItemMessage(hDlg, IDC_USEVWHOOK, BM_SETCHECK, 1,0);
        if(useWindowRules)
            SendDlgItemMessage(hDlg, IDC_USEWINRULES, BM_SETCHECK, 1,0);
        if(useDynButtonRm)
            SendDlgItemMessage(hDlg, IDC_DYNBUTTONRM, BM_SETCHECK, 1,0);
        if(useDskChgModRelease)
            SendDlgItemMessage(hDlg, IDC_USEDCMODREL, BM_SETCHECK, 1,0);
        if(minWinHide & 0x01)
            SendDlgItemMessage(hDlg, IDC_NOHIDEMINWIN, BM_SETCHECK, 1,0);
        if((minWinHide & 0x02) == 0)
            SendDlgItemMessage(hDlg, IDC_HIDEMINWIN, BM_SETCHECK, 1,0);
        if(vwLogFlag)
            SendDlgItemMessage(hDlg, IDC_DEBUGLOGGING, BM_SETCHECK, 1,0);
        return TRUE;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_SETACTIVE:
            // Initialize the controls.
            break;
        case PSN_APPLY:
            oldLogFlag = vwLogFlag ;
            preserveZOrder = (vwUByte) SendDlgItemMessage(hDlg, IDC_PRESORDER, CB_GETCURSEL, 0, 0) ;
            hiddenWindowAct = (vwUByte) SendDlgItemMessage(hDlg, IDC_HIDWINACT, CB_GETCURSEL, 0, 0) ;
            releaseFocus = (SendDlgItemMessage(hDlg, IDC_FOCUS, BM_GETCHECK, 0, 0) != BST_CHECKED) ;
            refreshOnWarp = (SendDlgItemMessage(hDlg, IDC_REFRESH, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            invertY = (SendDlgItemMessage(hDlg, IDC_INVERTY, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            noTaskbarCheck = (SendDlgItemMessage(hDlg, IDC_TASKBARDETECT, BM_GETCHECK, 0, 0) != BST_CHECKED) ;
            vwHookUse = (SendDlgItemMessage(hDlg, IDC_USEVWHOOK, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            useWindowRules = (SendDlgItemMessage(hDlg, IDC_USEWINRULES, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            useDynButtonRm = (SendDlgItemMessage(hDlg, IDC_DYNBUTTONRM, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            useDskChgModRelease = (SendDlgItemMessage(hDlg, IDC_USEDCMODREL, BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            minWinHide &= ~0x03 ;
            if(SendDlgItemMessage(hDlg, IDC_NOHIDEMINWIN, BM_GETCHECK, 0, 0) == BST_CHECKED)
                minWinHide |= 0x01 ;
            if(SendDlgItemMessage(hDlg, IDC_HIDEMINWIN, BM_GETCHECK, 0, 0) != BST_CHECKED)
                minWinHide |= 0x02 ;
            vwLogFlag = (SendDlgItemMessage(hDlg,IDC_DEBUGLOGGING,BM_GETCHECK, 0, 0) == BST_CHECKED) ;
            displayTaskbarIcon = (SendDlgItemMessage(hDlg, IDC_DISPLAYICON, BM_GETCHECK, 0, 0) != BST_CHECKED) ;
            
            vwSetupApply(hDlg,0x08) ;
            if(vwLogFlag != oldLogFlag)
            {
                MessageBox(hDlg, vwVIRTUAWIN_NAME _T(" must be restarted for the event logging change to take effect."),
                           vwVIRTUAWIN_NAME _T(" Note"), MB_ICONINFORMATION);
            }
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE);
            break;
        case PSN_KILLACTIVE:
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            return 1;
        case PSN_RESET:
            vwSetupCancel() ;
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE);
            break;
        case PSN_HELP:
            showHelp(hDlg,_T("SetupDialog.htm#Expert")) ;
            break;
        }
        break;
        
    case WM_COMMAND:
        if(LOWORD((wParam) == IDC_EXPLORECNFG))
        {   // Explore Config
            STARTUPINFO si;
            PROCESS_INFORMATION pi;  
            TCHAR cmdLn[MAX_PATH+9], *ss ;
            
            _tcscpy_s(cmdLn,269,_T("explorer ")) ;
            GetFilename(vwVIRTUAWIN_CFG,1,cmdLn+9) ;
            if((ss = _tcsrchr(cmdLn,'\\')) != NULL)
                *ss = '\0' ;
            memset(&si, 0, sizeof(si)); 
            si.cb = sizeof(si); 
            if(CreateProcess(NULL,cmdLn, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
            else
            {
				_stprintf_s(cmdLn, 260, _T("Failed to launch explorer. (Err %d)"), (int)GetLastError());
                MessageBox(hWnd,cmdLn,vwVIRTUAWIN_NAME _T(" Error"),MB_ICONWARNING) ;
            }
        }
        else if(LOWORD((wParam) == IDC_LOGWINDOWS))
        {   // Log Windows
            TCHAR cname[vwCLASSNAME_MAX], wname[vwWINDOWNAME_MAX] ;
            vwWindowBase *wb ;
            vwWindow *win ;
            RECT pos ;
                
            vwMutexLock();
            windowListUpdate() ;
            wb = windowBaseList ;
            while(wb != NULL)
            {
                GetWindowRect(wb->handle,&pos) ;
                GetClassName(wb->handle,cname,vwCLASSNAME_MAX);
                if(!GetWindowText(wb->handle,wname,vwWINDOWNAME_MAX))
                    _tcscpy_s(wname,128, vwWTNAME_NONE);
                if(wb->flags & vwWINFLAGS_MANAGED)
                {
                    win = (vwWindow *) wb ;
                    vwLogBasic((_T("MNG-WIN: %8x Flg %08x %08x %08x Pos %d %d Proc %d %x Own %x Link %x Desk %d\n        Class \"%s\" Title \"%s\"\n"),
                                (int) wb->handle,(int) wb->flags,(int) GetWindowLong(wb->handle, GWL_STYLE),(int) GetWindowLong(wb->handle, GWL_EXSTYLE),
                                (int) pos.left, (int) pos.top,(int)win->processId,(int)((win->processNext == NULL) ? 0:win->processNext->handle),
                                (int) GetWindow(wb->handle,GW_OWNER),(int)((win->linkedNext == NULL) ? 0:win->linkedNext->handle),(int) win->desk,cname,wname)) ;
                }
                else
                    vwLogBasic((_T("%s-WIN: %8x Flg %08x %08x %08x Pos %d %d Own %x\n        Class \"%s\" Title \"%s\"\n"),
                                (wb->flags & vwWINFLAGS_WINDOW) ? "UNM":"NON",
                                (int) wb->handle,(int) wb->flags,(int) GetWindowLong(wb->handle, GWL_STYLE),(int) GetWindowLong(wb->handle, GWL_EXSTYLE),
                                (int) pos.left,(int) pos.top, (int) GetWindow(wb->handle,GW_OWNER), cname, wname)) ;
                wb = wb->next ;
            }
            vwMutexRelease();
        }
        else if(LOWORD(wParam) == IDC_FOCUS       || LOWORD(wParam) == IDC_USEWINRULES ||
                LOWORD(wParam) == IDC_DISPLAYICON || LOWORD(wParam) == IDC_DEBUGLOGGING ||
                LOWORD(wParam) == IDC_INVERTY     || LOWORD(wParam) == IDC_TASKBARDETECT ||
                LOWORD(wParam) == IDC_REFRESH     || LOWORD(wParam) == IDC_DYNBUTTONRM ||
                LOWORD(wParam) == IDC_USEVWHOOK   || LOWORD(wParam) == IDC_NOHIDEMINWIN ||
                LOWORD(wParam) == IDC_USEDCMODREL || LOWORD(wParam) == IDC_HIDEMINWIN ||
                (LOWORD(wParam) == IDC_HIDWINACT  && HIWORD(wParam) == CBN_SELCHANGE) ||
                (LOWORD(wParam) == IDC_PRESORDER  && HIWORD(wParam) == CBN_SELCHANGE) )
        {
            pageChangeMask |= 0x08 ;
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        }
        break;
    }
    return FALSE ;
}

/*************************************************
 * The "About" tab callback
 */
BOOL APIENTRY
setupAbout(HWND hDlg, UINT message, UINT wParam, LONG lParam)
{
    TCHAR license[] = _T("This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\r\n \r\nThis program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details. \r\n \r\nYou should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.");
    
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_LICENSE, license);
        return TRUE;
        
    case WM_CTLCOLORSTATIC:
        if((HWND)lParam == GetDlgItem(hDlg, IDC_MAILTO) ||
           (HWND)lParam == GetDlgItem(hDlg, IDC_HTTP))
        {
            SetBkMode((HDC)wParam, TRANSPARENT); // Don't overwrite background
            SetTextColor((HDC)wParam, RGB(0, 0, 255)); // Blue 
            return (BOOL) GetStockObject(HOLLOW_BRUSH);
        }
        return FALSE;
        
    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {
        case PSN_HELP:
            showHelp(hDlg,_T("SetupDialog.htm")) ;
            break;
        }
        break;
    
    case WM_COMMAND:
        if(LOWORD(wParam) == IDC_MAILTO)
        {
            HINSTANCE h = ShellExecute(NULL,_T("open"),_T("mailto:") vwVIRTUAWIN_EMAIL,NULL,NULL,SW_SHOWNORMAL);
            if((UINT)h < 33)
                MessageBox(hDlg,_T("Error executing mail program."),vwVIRTUAWIN_NAME _T(" Error"),MB_ICONWARNING);
        }
        else if(LOWORD(wParam) == IDC_HTTP)
        {
            HINSTANCE h = ShellExecute(NULL,_T("open"),vwVIRTUAWIN_WEBSITE,NULL,NULL,SW_SHOWNORMAL);
            if((UINT)h < 33)
                MessageBox(hDlg,_T("Error opening website link."),vwVIRTUAWIN_NAME _T(" Error"),MB_ICONWARNING);
        }
        return TRUE;
    }
    return FALSE;
}

/*************************************************
 * Initialize callback function for the property sheet
 * Used for removing the "?" in the title bar
 */
int CALLBACK
propCallBack( HWND hwndDlg, UINT uMsg, LPARAM lParam )
{
    DLGTEMPLATE* ptr;
    
    switch(uMsg)
    {
    case PSCB_PRECREATE:
        // Removes the question mark button in the title bar
        ptr = (DLGTEMPLATE*)lParam;
        ptr->style = ptr->style ^ (DS_CONTEXTHELP | DS_CENTER); 
        
        break;  
    }
    return 0;
}

/*************************************************
 * Creates the property sheet that holds the setup dialog
 */
void
createSetupDialog(HINSTANCE theHinst, HWND theHwndOwner)
{
    PROPSHEETPAGE psp[vwPROPSHEET_PAGE_COUNT];
    PROPSHEETHEADER psh;
    
    int xIcon = GetSystemMetrics(SM_CXSMICON);
    int yIcon = GetSystemMetrics(SM_CYSMICON);
    
    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE|PSP_HASHELP;
    psp[0].hInstance = theHinst;
    psp[0].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_GENERAL);
    psp[0].pszIcon = NULL;
    psp[0].pfnDlgProc = setupGeneral;
    psp[0].pszTitle = _T("General");
    psp[0].lParam = 0;
    
    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE|PSP_HASHELP;
    psp[1].hInstance = theHinst;
    psp[1].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_HOTKEYS);
    psp[1].pszIcon = NULL;
    psp[1].pfnDlgProc = setupHotkeys;
    psp[1].pszTitle = _T("Hotkeys");
    psp[1].lParam = 0;
    
    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE|PSP_HASHELP;
    psp[2].hInstance = theHinst;
    psp[2].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_MOUSE);
    psp[2].pszIcon = NULL;
    psp[2].pfnDlgProc = setupMouse;
    psp[2].pszTitle = _T("Mouse");
    psp[2].lParam = 0;
    
    psp[3].dwSize = sizeof(PROPSHEETPAGE);
    psp[3].dwFlags = PSP_USETITLE|PSP_HASHELP;
    psp[3].hInstance = theHinst;
    psp[3].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_MODULES);
    psp[3].pszIcon = NULL;
    psp[3].pfnDlgProc = setupModules;
    psp[3].pszTitle = _T("Modules");
    psp[3].lParam = 0;
    
    psp[4].dwSize = sizeof(PROPSHEETPAGE);
    psp[4].dwFlags = PSP_USETITLE|PSP_HASHELP;
    psp[4].hInstance = theHinst;
    psp[4].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_EXPERT);
    psp[4].pszIcon = NULL;
    psp[4].pfnDlgProc = setupExpert;
    psp[4].pszTitle = _T("Expert");
    psp[4].lParam = 0;
    
    psp[5].dwSize = sizeof(PROPSHEETPAGE);
    psp[5].dwFlags = PSP_USETITLE|PSP_HASHELP;
    psp[5].hInstance = theHinst;
    psp[5].pszTemplate = MAKEINTRESOURCE(IDD_PROPPAGE_ABOUT);
    psp[5].pszIcon = NULL;
    psp[5].pfnDlgProc = setupAbout;
    psp[5].pszTitle = _T("About");
    psp[5].lParam = 0;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_USECALLBACK | PSH_PROPSHEETPAGE | PSH_USEHICON ;
    psh.hwndParent = theHwndOwner;
    psh.hInstance = theHinst;
    psh.pszIcon = NULL;
    psh.pszCaption = _T("VirtuaWin - Setup") ;
    psh.nPages = vwPROPSHEET_PAGE_COUNT ;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
    psh.nStartPage = 0;
    psh.hIcon = (HICON) LoadImage(theHinst, MAKEINTRESOURCE(IDI_VIRTUAWIN), IMAGE_ICON, xIcon, yIcon, 0);
    psh.pfnCallback = (PFNPROPSHEETCALLBACK)propCallBack;
    
    setupGeneralHWnd = NULL;
    setupHotkeysHWnd = NULL;
    dialogOpen = TRUE;
    PropertySheet(&psh);
    dialogOpen = FALSE;
    dialogHWnd = NULL;
}
