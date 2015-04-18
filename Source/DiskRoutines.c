//
//  VirtuaWin - Virtual Desktop Manager (virtuawin.sourceforge.net)
//  DiskRoutines.c - File reading an writing routines.
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

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#include "VirtuaWin.h"
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <errno.h>
#include <assert.h>
#include <shlobj.h>  // for SHGetFolderPath
#include <direct.h>  // for mkdir

#include "DiskRoutines.h"
#include "ConfigParameters.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif
#define VIRTUAWIN_SUBDIR vwVIRTUAWIN_NAME

#define vwWindowRuleDefaultCount 7
static TCHAR *vwWindowRuleDefault0Names[vwWindowRuleDefaultCount]={
	_T("XLMAIN"),
	_T("AdobeAcrobat"),
	_T("WindowsForms10."),
	_T("ExploreWClass"),
	_T("IEFrame"),
	_T("CabinetWClass"),
	_T("BaseBar")
} ;
static vwUInt vwWindowRuleDefaultFlags[vwWindowRuleDefaultCount]={
	(vwWTFLAGS_ENABLED|vwWTFLAGS_MAIN_WIN),
	(vwWTFLAGS_ENABLED|vwWTFLAGS_MAIN_WIN),
	(vwWTFLAGS_ENABLED|vwWTFLAGS_HIDEWIN_MOVE|vwWTFLAGS_HIDETSK_TOOLWN|vwWTFLAGS_CN_EVAR),
	(vwWTFLAGS_ENABLED|vwWTFLAGS_HIDEWIN_MOVE|vwWTFLAGS_HIDETSK_TOOLWN),
	(vwWTFLAGS_ENABLED|vwWTFLAGS_HIDEWIN_MOVE|vwWTFLAGS_HIDETSK_TOOLWN),
	(vwWTFLAGS_ENABLED|vwWTFLAGS_HIDEWIN_MOVE|vwWTFLAGS_HIDETSK_TOOLWN),
	(vwWTFLAGS_ENABLED|vwWTFLAGS_DONT_MANAGE)
} ;

TCHAR *VirtuaWinPath=NULL ;
TCHAR *UserAppPath=NULL ;
#ifdef _UNICODE
char *VirtuaWinPathStr=NULL ;
char *UserAppPathStr=NULL ;
#endif

/**************************************************
 * Gets the local application settings path for the current user.
 * Calling function MUST pre-allocate the return string!
 */
static void getUserAppPath(TCHAR *path)
{
	LPITEMIDLIST idList ;
	TCHAR buff[MAX_PATH], *ss, *se, cc ;
	FILE *fp;
	size_t pssSize; 
	int len ;
	
	path[0] = '\0' ;
	
	/* look for a userpath.cfg file */
	_tcscpy_s(buff, 260, VirtuaWinPath);
	_tcscat_s(buff, 260, _T("userpath.cfg"));
	if ((fp = _tfopen(buff, _T("r"))) != NULL)
	{
		if((_fgetts(buff,MAX_PATH,fp) == 0) && (buff[0] != '\0'))
		{
			len = 0 ;
			ss = buff ;
			while(((cc=*ss++) != '\0') && (cc !='\n'))
			{
				if((cc == '$') && (*ss == '{') && ((se=_tcschr(ss,'}')) != NULL))
				{
					*se++ = '\0' ;
					if(!_tcscmp(ss+1,_T("VIRTUAWIN_PATH")))
						ss = VirtuaWinPath ;
					else
						_tgetenv_s(&pssSize, ss, 260, ss + 1);
					if(ss != NULL)
					{
						_tcscpy_s(path + len, 260, ss);
						len += _tcslen(ss) ;
					}
					ss = se ;
				}
				else
				{
					if(cc == '/')
						cc = '\\' ;
					if((cc != '\\') || (len <= 1) || (path[len-1] != '\\'))
						path[len++] = cc ;
				}
			}
			if(len && (path[len-1] != '\\'))
				path[len++] = '\\' ;
			path[len] = '\0' ;
		}
		fclose(fp) ;
	}
	if(path[0] == '\0')
	{
		if(SUCCEEDED(SHGetSpecialFolderLocation(NULL,CSIDL_APPDATA,&idList)) && (idList != NULL))
		{
			IMalloc *im ;
			SHGetPathFromIDList(idList,path);
			if(SUCCEEDED(SHGetMalloc(&im)) && (im != NULL))
			{
				im->lpVtbl->Free(im,idList) ;
				im->lpVtbl->Release(im);
			}
		}
		if(path[0] != '\0')
		{
			len = _tcslen(path) ;
			if(path[len - 1] != '\\')
				path[len++] = '\\' ;
			_tcsncpy_s(path + len, 200, VIRTUAWIN_SUBDIR, MAX_PATH - len);
			len += _tcslen(path+len) ;
			path[len++] = '\\' ;
			path[len] = '\0' ;
		}
	}
}

/************************************************
 * Generates a path + username for the requested file type, fills in
 * the supplied string parameter, that MUST be pre-allocated.  Caller
 * is responsible for memory allocation.  It is suggested that the
 * caller allocate the string on the stack, not dynamically on the
 * heap so that it is cleaned up automatically.  Strings can be up to
 * MAX_PATH in length.
 *
 * Return is 1 if successful, 0 otherwise.  (suggestion, convert to
 * type bool after porting to cpp)
 */

void
GetFilename(eFileNames filetype, int location, TCHAR *outStr)
{
	static TCHAR *subPath[vwFILE_COUNT] = {
		_T("modules\\*.exe"), _T("virtuawin.chm"), _T("virtuawin.cfg"), _T("window.cfg"), _T("module.cfg")
	};
	DWORD len ;
	
	if(UserAppPath == NULL)
	{
		/* initialization of paths, find the installation and user paths,
		 * exit on failure - initialization happens so early on it is safe to
		 * simply exit */
		TCHAR path[MAX_PATH], *ss ;
		
		GetModuleFileName(GetModuleHandle(NULL),path,MAX_PATH) ;
		ss = _tcsrchr(path,'\\') ;
		ss[1] = '\0' ;
		if((VirtuaWinPath = _tcsdup(path)) != NULL)
		{
			getUserAppPath(path) ;
			if(path[0] == '\0')
				UserAppPath = VirtuaWinPath ;
			else
				UserAppPath = _tcsdup(path) ;
		}
		if((VirtuaWinPath == NULL) || (UserAppPath == NULL))
		{
			MessageBox(hWnd,_T("Memory resources appear to be very low, try rebooting.\nIf you still have problems, send a mail to \n") vwVIRTUAWIN_EMAIL,vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
			exit(1);
		}
#ifdef _UNICODE
		if(WideCharToMultiByte(CP_ACP,0,VirtuaWinPath,-1,(char *) path,MAX_PATH, 0, 0))
			VirtuaWinPathStr = _strdup((char *) path) ;
		if(UserAppPath == VirtuaWinPath)
			UserAppPathStr = VirtuaWinPathStr ;
		else if(WideCharToMultiByte(CP_ACP,0,UserAppPath,-1,(char *) path,MAX_PATH, 0, 0))
			UserAppPathStr = _strdup((char *) path) ;
#endif
	}
	
	_tcsncpy(outStr, (location) ? UserAppPath:VirtuaWinPath, MAX_PATH);
	if(filetype < vwFILE_COUNT)
	{
		len = MAX_PATH - _tcslen(outStr) ;
		_tcsncat_s(outStr,260, subPath[filetype],len) ;
	}
}


/*************************************************
 * Loads module names that should be disabled
 */
int
loadDisabledModules(vwDisModule *theDisList)
{
	TCHAR buff[MAX_PATH];
	int len, nOfDisMod = 0;
	FILE *fp;
	
	GetFilename(vwMODULE_CFG,1,buff);
	
	if ((fp = _tfopen(buff, _T("r"))) != NULL)
	{
		while(_fgetts(buff,MAX_PATH,fp) != NULL)
		{
			if((len = _tcslen(buff)) > 1)
			{
				if(len > vwMODULENAME_MAX)
					buff[vwMODULENAME_MAX] = '\0' ;
				else if(buff[len-1] == '\n')
					buff[len-1] = '\0' ;
				_tcscpy_s(theDisList[nOfDisMod++].moduleName, 80, buff);
				if(nOfDisMod == (MAXMODULES * 2))
					break ;
			}
		}
		fclose(fp);
	}
	return nOfDisMod;
}

/*************************************************
 * Write out the disabled modules
 */
void
saveDisabledList(int theNOfModules, vwModule *theModList)
{
	TCHAR DisabledFileList[MAX_PATH];
	FILE* fp;
	
	GetFilename(vwMODULE_CFG,1,DisabledFileList);
	if ((fp = _tfopen(DisabledFileList, _T("w"))) == NULL)
		MessageBox(hWnd,_T("Error saving disabled module state"),vwVIRTUAWIN_NAME _T(" Error"),MB_ICONERROR);
	else
	{
		int i;
		for(i = 0; i < theNOfModules; ++i)
			if(theModList[i].disabled)
				_ftprintf(fp,_T("%s\n"),theModList[i].description);
		fclose(fp);
	}
}

/*************************************************
 * Reads window rules from window.cfg file
 */
void
loadWindowConfig(void)
{
	vwWindowRule *wt, *pwt ;
	vwWindow *win ;
	TCHAR buff[1024], *ss ;
	int ii, ll, mallocErr=0 ;
	FILE *fp ;
	
	if(windowRuleList != NULL)
	{
		/* free current list first */
		while((wt = windowRuleList) != NULL)
		{
			windowRuleList = wt->next ;
			ii = vwWTNAME_COUNT - 1 ;
			do {
				if(wt->name[ii] != NULL)
					free(wt->name[ii]) ;
			} while(--ii >= 0) ;
			free(wt) ;
		}
		/* set all window zOrder[0] (used to store the vwWindowRules) to 0 */
		win = (vwWindow *) windowBaseList ;
		while(win != NULL)
		{
			if(win->flags & vwWINFLAGS_WINDOW)
				win->zOrder[0] = 0 ;
			win = win->next ;
		}
	}
	
	GetFilename(vwWINDOW_CFG,1,buff);
	if ((fp = _tfopen(buff, _T("r"))) != 0)
	{
		pwt = wt = NULL ;
		while(_fgetts(buff,1024,fp) != NULL)
		{
			if(!_tcsncmp(buff,_T("flags# "),7))
			{
				/* start of a new windowRule */
				if((wt = calloc(1,sizeof(vwWindowRule))) == NULL)
				{
					mallocErr = 1 ;
					break ;
				}
				if(pwt == NULL)
					windowRuleList = wt ;
				else
					pwt->next = wt ;
				pwt = wt ;
				wt->flags = _ttoi(buff+7) ;
			}
			else if(wt != NULL)
			{
				if((buff[0] >= '0') && (buff[0] <= '3') && !_tcsncmp(buff+1,_T("n# "),3))
				{
					ll = _tcslen(buff+4) ;
					if(buff[ll+3] == '\n')
					{
						buff[ll+3] = '\0' ;
						ll-- ;
					}
					if((ss = _tcsdup(buff+4)) == NULL)
					{
						mallocErr = 1 ;
						break ;
					}
					ii = buff[0] - '0' ;
					wt->name[ii] = ss ;
					wt->nameLen[ii] = ll ;
				}
				else if(!_tcsncmp(buff,_T("desk# "),6) &&
						((wt->desk = _ttoi(buff+6)) >= vwDESKTOP_SIZE))
					wt->desk = 0 ;
			}
		}
		fclose(fp) ;
	}
	else
	{
		/* no window.cfg file yet create the default */
		ii = vwWindowRuleDefaultCount ;
		while(--ii >= 0)
		{
			if(((wt = calloc(1,sizeof(vwWindowRule))) == NULL) ||
			   ((wt->name[0] = _tcsdup(vwWindowRuleDefault0Names[ii])) == NULL))
			{
				mallocErr = 1 ;
				break ;
			}
			wt->nameLen[0] = _tcslen(vwWindowRuleDefault0Names[ii]) ;
			wt->flags = vwWindowRuleDefaultFlags[ii] ;
			wt->next = windowRuleList ;
			windowRuleList = wt ;
		}
	}
	if(mallocErr)
		MessageBox(hWnd,_T("System resources are low, failed to load configuration."),vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
}

/*************************************************
 * Writes the window rule list to window.cfg file
 */
void
saveWindowConfig(void)
{
	TCHAR fname[MAX_PATH];
	vwWindowRule *wt;
	FILE *fp = NULL;
	int ii ;
	
	GetFilename(vwWINDOW_CFG,1,fname);
	if ((fp = _tfopen(fname, _T("w"))) == NULL) {
		wchar_t buff[40];
		_stprintf(buff, _T("Error writing %s. \nReason: %s"), fname, _tcserror(errno) );
		MessageBox(NULL, buff, vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
	}
	else
	{
		wt = windowRuleList ;
		while(wt != NULL)
		{
			fprintf(fp, "flags# %d\n", wt->flags);
			for(ii=0 ; ii<vwWTNAME_COUNT ; ii++)
				if(wt->name[ii] != NULL)
					_ftprintf(fp,_T("%dn# %s\n"),ii,wt->name[ii]);
			if(wt->desk > 0)
				fprintf(fp, "desk# %d\n", wt->desk);
			wt = wt->next ;
		}
		fclose(fp);
	}
}

/*************************************************
 * Reads a saved configuration from file
 */
/* Get the list of hotkey commands */
#define VW_COMMAND(a, b, c, d) a = b ,
enum {
#include "vwCommands.def".desk = 
} ;
#undef  VW_COMMAND

/* horrible macro, used as it makes the code more legible - use with care! */
#define vwConfigReadInt(fp,buff,tmpI,var) if(fscanf_s(fp, "%s%i", (char *) (buff), 2048, &(tmpI)) == 2) (var) = (tmpI)

static void
addOldHotkey(int key, int mod, int win, int cmd, int desk)
{
	if(key != 0)
	{
		hotkeyList[hotkeyCount].key = (vwUByte) key ;
		hotkeyList[hotkeyCount].modifier = 0 ;
		if(mod & HOTKEYF_ALT)
			hotkeyList[hotkeyCount].modifier |= vwHOTKEY_ALT ;
		if(mod & HOTKEYF_CONTROL)
			hotkeyList[hotkeyCount].modifier |= vwHOTKEY_CONTROL;
		if(mod & HOTKEYF_SHIFT)
			hotkeyList[hotkeyCount].modifier |= vwHOTKEY_SHIFT;
		if(mod & HOTKEYF_EXT)
			hotkeyList[hotkeyCount].modifier |= vwHOTKEY_EXT;
		if(win)
			hotkeyList[hotkeyCount].modifier |= vwHOTKEY_WIN;
		hotkeyList[hotkeyCount].command = (vwUByte) cmd ;
		hotkeyList[hotkeyCount].desk = (vwUByte) desk ;
		hotkeyCount++ ;
	}
}

void
loadVirtuawinConfig(void)
{
	TCHAR buff[MAX_PATH], buff2[2048], err[40], * ss;
	FILE *fp = NULL, *wfp = NULL;
	int ii, jj, ll, hk[4] ;
	
	GetFilename(vwVIRTUAWIN_CFG,1,buff);
	if(GetFileAttributes(buff) == INVALID_FILE_ATTRIBUTES)
	{
		static char *defaultCfg="ver# 2\nhotkeyCount# 6\nhotkey1# 37 19 1 0\nhotkey2# 39 19 2 0\nhotkey3# 38 19 3 0\nhotkey4# 40 19 4 0\nhotkey5# 37 25 13 0\nhotkey6# 39 25 15 0\ndesktopNameCount# 0\n" ;
		/* config file does not exist - new user, setup configuration, check
		 * the user path exists first and if not try to create it - note that
		 * multiple levels may need to be created due to the userpath.cfg */
		if((buff[0] == '\\') && (buff[1] == '\\') && ((ss = _tcschr(buff+2,'\\')) != NULL) && (ss[1] != '\0'))
			;
		else if(buff[1] == ':')
			ss = buff + 2 ;
		else
			ss = buff ;
		while((ss = _tcschr(ss+1,'\\')) != NULL)
		{
			*ss = '\0' ;
			if(((GetFileAttributes(buff) & (0xf0000000|FILE_ATTRIBUTE_DIRECTORY)) != FILE_ATTRIBUTE_DIRECTORY) &&
			   (CreateDirectory(buff,NULL) == 0))
			{
				_stprintf_s(buff2,2048, vwVIRTUAWIN_NAME _T(" cannot create the user config directory:\n\n    %s\n\nPlease check file permissions. If you continue to have problems, send e-mail to:\n\n    ") vwVIRTUAWIN_EMAIL,buff);
				MessageBox(hWnd,buff2,vwVIRTUAWIN_NAME _T(" Error"),MB_ICONERROR);
				exit(1) ;
			}
			*ss = '\\' ;
		}
		
		/* If the user path is not the installation path then copy all the
		 * config files across to the user area */
		fp = NULL ;
		ii = vwFILE_COUNT ;
		if (_tcsicmp(VirtuaWinPath, UserAppPath))
		{
			while (--ii >= vwVIRTUAWIN_CFG)
			{

				GetFilename(ii, 0, buff);
				OutputDebugString(_T("Trying to open "));
				OutputDebugString(buff);
				OutputDebugString(_T("\n"));
				if ((fp = _tfopen(buff, _T("rb"))) != 0)
				{
					GetFilename(ii, 1, buff2);
					OutputDebugString(_T("\nTrying to create "));
					OutputDebugString(buff2);
					OutputDebugString(_T("\n"));
					if ((wfp = _tfopen(buff2, _T("wb"))) == 0)
						break;
					OutputDebugString(_T("Copying new file.\n"));
					for (;;)
					{

						
						if ((jj = fread(buff2, 1, 2048, fp)) <= 0)
							break;
						if (fwrite(buff2, 1, jj, wfp) != (size_t)jj)
						{
							jj = -1;
							break;
						}
					}
					fclose(fp);
					if ((fclose(wfp) != 0) || (jj < 0))
						break;
				}
				else {
					_tcserror_s(err, 40, errno);
					_stprintf_s(buff2, 2048, _T("Could not open. Reason: %s"), err);
					OutputDebugString(buff2);
				}
			}
		}
		else
		{
			/* must create the main VirtuaWin.cfg file */
			ii = vwVIRTUAWIN_CFG - 1 ;
			fp = NULL ;
		}
		GetFilename(vwVIRTUAWIN_CFG,1,buff);
		/* check a main config file has been copied, if not create a dummy one */

		_stprintf_s(buff2, 20, _T("ii = %i"), ii);
		if ((ii < vwVIRTUAWIN_CFG) && (fp == NULL)) {			
			if ((wfp = _tfopen(buff, _T("wb"))) != 0) {
				size_t written = fwrite(defaultCfg, strlen(defaultCfg), 1, wfp);
				if (written > 1) {
					if (fclose(wfp) != 0) {
						ii = vwVIRTUAWIN_CFG;
						_stprintf_s(buff2, 20, _T("No error!"));
					}
					else {
						_stprintf_s(buff2, 20, _T("Error closing file"));
					}
				}
				else {
					_stprintf_s(buff2, 20, _T("Error writing file"));
				}
			}
			else {
				_stprintf_s(buff2, 20, _T("Error opening file"));
			}
		}
		

			

		/* check we did not break out due to an error and virtuawin.cfg was found */
		if (ii >= vwVIRTUAWIN_CFG)
		{
			_stprintf_s(buff2, 2048, _T("Error creating new user configuration, please check installation & file permissions.\nIf you continue to have problems, send e-mail to:\n\n    " vwVIRTUAWIN_EMAIL), buff2);
			MessageBox(hWnd, buff2, vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR);
			exit(1);
		}
		_stprintf_s(buff2,2048, _T("Welcome to %s\n\nA new user configuration has been created in directory:\n\n    %s\n\nRight click on tray icon to access the Setup dialog."),vwVIRTUAWIN_NAME_VERSION,UserAppPath) ;
		MessageBox(hWnd,buff2,vwVIRTUAWIN_NAME,MB_ICONINFORMATION);
	}
	

	/* Is file readable at all? */
	if ((fp = _tfopen(buff, _T("r"))) == 0)
	{
		_tcserror_s(err, 40, errno);
		_stprintf_s(buff2,2048, _T("Error reading config file:\n\n    %s\n\nReason: %s.\n\n If you continue to have problems, send e-mail to:\n\n    ") vwVIRTUAWIN_EMAIL,buff, err);
		MessageBox(hWnd,buff2,vwVIRTUAWIN_NAME _T(" Error"),MB_ICONERROR) ;
		exit(1) ;
	}
	/* Try readying the first line which is either a version or mouseDelay if pre-v4 */
	if(fscanf(fp, "%s%d", (char *) buff2, &ii) != 2)
	{
		fclose(fp);
		_stprintf_s(buff2,2048, _T("Error reading config file:\n\n    %s\n\nFile empty or corrupt, please remove."), buff);
		MessageBox(hWnd, buff2, vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR) ;
		exit(1) ;
	}
	/* Is it a version line? */
	if(strcmp((char *) buff2,"ver#"))
	{
		/* Nope, there was no version */
		int kk, hkc[5], hks[3] ;
		
		mouseEnable = ii ;
		hotkeyCount = 0 ;
		vwConfigReadInt(fp,buff2,ii,mouseDelay);
		fscanf(fp, "%s%i", (char *) buff, &jj);
		vwConfigReadInt(fp,buff,ii,releaseFocus);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, hk + 0);
		fscanf(fp, "%s%i", (char *) buff, hk + 1);
		fscanf(fp, "%s%i", (char *) buff, hk + 2);
		fscanf(fp, "%s%i", (char *) buff, hk + 3);
		if(jj)
		{
			jj = hk[0] | hk[1] | hk[2] | hk[3] | vwHOTKEY_EXT ;
			ii = 3 ;
			do {
				hotkeyList[ii].modifier = jj ;
				hotkeyList[ii].command = vwCMD_NAV_MOVE_LEFT + ii ;
				hotkeyList[ii].desk = 0 ;
			} while(--ii >= 0) ;
			hotkeyList[0].key = VK_LEFT ;
			hotkeyList[1].key = VK_RIGHT ;
			hotkeyList[2].key = VK_UP ;
			hotkeyList[3].key = VK_DOWN ;
			hotkeyCount = 4 ;
		}
		vwConfigReadInt(fp,buff,ii,mouseJumpLength);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		vwConfigReadInt(fp,buff,ii,nDesksY);
		vwConfigReadInt(fp,buff,ii,nDesksX);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		for(ii=1,jj=9,kk=0 ; ii<=jj ; ii++)
		{
			fscanf(fp, "%s%i", (char *) buff, hk + 1);
			if((ii==1) && !strcmp((char *) buff,"Desk_count#"))
			{
				jj = hk[1] ;
				hk[1] = 0 ;
				ii = 0 ;
				kk = 1 ;
			}
			else
			{
				fscanf(fp, "%s%i", (char *) buff, hk + 2);
				fscanf(fp, "%s%i\n", (char *) buff, hk + 3);
				if(hk[1])
					addOldHotkey(hk[1],hk[2],hk[3],vwCMD_NAV_MOVE_DESKTOP,ii) ;
				if(kk && (_fgetts(buff2,2048,fp) != NULL) && ((ss=_tcschr(buff2,' ')) != NULL) &&
				   ((ll=_tcslen(++ss)) > 1))
				{
					if(ss[ll-1] == '\n')
						ss[ll-1] = '\0' ;
					desktopName[ii] = _tcsdup(ss) ;
				}
			}
		}
		vwConfigReadInt(fp,buff,ii,mouseModifierUsed);
		if(fscanf(fp, "%s%i", (char *) buff, &ii) == 2)
			mouseModifier = 0 ;
		if(ii)  mouseModifier |= vwHOTKEY_ALT ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		if(ii)  mouseModifier |= vwHOTKEY_SHIFT ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		if(ii)  mouseModifier |= vwHOTKEY_CONTROL ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		vwConfigReadInt(fp,buff,ii,refreshOnWarp);
		if(fscanf(fp, "%s%d", (char *) buff, &ii) == 2)
			mouseWarp = (ii == 0) ;
		fscanf(fp, "%s%i", (char *) buff, hks + 0);
		fscanf(fp, "%s%i", (char *) buff, hks + 1);
		vwConfigReadInt(fp,buff,ii,preserveZOrder);
		vwConfigReadInt(fp,buff,ii,deskWrap);
		vwConfigReadInt(fp,buff,ii,invertY);
		if(fscanf(fp, "%s%i", (char *) buff, &ii) == 2)
			winListContent = 0 ;
		if(ii)  winListContent |= vwWINLIST_STICKY ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		if(ii)  winListContent |= vwWINLIST_ASSIGN ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		if(ii)  winListContent |= vwWINLIST_ACCESS ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, hkc + 0);
		fscanf(fp, "%s%i", (char *) buff, hkc + 1);
		fscanf(fp, "%s%i", (char *) buff, hkc + 2);
		fscanf(fp, "%s%i", (char *) buff, hkc + 3);
		fscanf(fp, "%s%i", (char *) buff, hkc + 4);
		fscanf(fp, "%s%i", (char *) buff, hk + 0);
		fscanf(fp, "%s%i", (char *) buff, hk + 1);
		fscanf(fp, "%s%i", (char *) buff, hk + 2);
		fscanf(fp, "%s%i", (char *) buff, hk + 3);
		if(hk[0])
		{
			kk = hotkeyCount ;
			addOldHotkey(hk[1],hk[2],hk[3],vwCMD_UI_WINLIST_STD,0) ;
		}
		else
			kk = -1 ;
		vwConfigReadInt(fp,buff,ii,displayTaskbarIcon);
		fscanf(fp, "%s%i", (char *) buff, hks + 2);
		vwConfigReadInt(fp,buff,ii,noTaskbarCheck);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &jj);
		if(hkc[0])
		{
			addOldHotkey(hkc[1],hkc[2],ii,vwCMD_NAV_MOVE_NEXT,0) ;
			addOldHotkey(hkc[3],hkc[4],jj,vwCMD_NAV_MOVE_PREV,0) ;
		}
		fscanf(fp, "%s%i", (char *) buff, &ii);
		if(ii)
			addOldHotkey(hks[1],hks[0],hk[2],vwCMD_WIN_STICKY,0) ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		vwConfigReadInt(fp,buff,ii,hiddenWindowAct);
		fscanf(fp, "%s%i", (char *) buff, &ii);
		fscanf(fp, "%s%i", (char *) buff, hk + 0);
		fscanf(fp, "%s%i", (char *) buff, hk + 1);
		fscanf(fp, "%s%i", (char *) buff, hk + 2);
		fscanf(fp, "%s%i", (char *) buff, hk + 3);
		if(hk[0])
			addOldHotkey(hk[1],hk[2],hk[3],vwCMD_WIN_DISMISS,0) ;
		vwConfigReadInt(fp,buff,ii,vwLogFlag);
		vwConfigReadInt(fp,buff,ii,mouseKnock);
		vwConfigReadInt(fp,buff,ii,winListCompact);
		if(winListCompact && (kk >= 0))
			hotkeyList[kk].command = vwCMD_UI_WINMENU_CMP ;
		fscanf(fp, "%s%i", (char *) buff, hk + 0);
		fscanf(fp, "%s%i", (char *) buff, hk + 1);
		fscanf(fp, "%s%i", (char *) buff, hk + 2);
		fscanf(fp, "%s%i", (char *) buff, hk + 3);
		if(hk[0])
			addOldHotkey(hk[1],hk[2],hk[3],vwCMD_UI_WINMENU_STD,0) ;
		fscanf(fp, "%s%i", (char *) buff, &ii);
		if(ii)  winListContent |= vwWINLIST_SHOW ;
	}
	else if(ii == 2)
	{
		/* read the hotkeys and desktop names */
		hotkeyCount = 0 ;
		fscanf(fp, "%s%d", (char *) buff, &jj);
		for(ii=0 ; ii<jj ; ii++)
		{
			if(fscanf(fp, "%s %d %d %d %d", (char *) buff, hk+0, hk+1, hk+2, hk+3) == 5)
			{
				hotkeyList[hotkeyCount].key = hk[0] ;
				hotkeyList[hotkeyCount].modifier = hk[1] ;
				hotkeyList[hotkeyCount].command = hk[2] ;
				hotkeyList[hotkeyCount].desk = hk[3] ;
				hotkeyCount++ ;
			}
		}
		ii = vwDESKTOP_SIZE ;
		while(--ii >= 0)
		{
			if(desktopName[ii] != NULL)
			{
				free(desktopName[ii]) ;
				desktopName[ii] = NULL ;
			}
		}
		fscanf(fp, "%s%d\n", (char *) buff, &jj);
		for(ii=1 ; ii<=jj ; ii++)
		{
			if((_fgetts(buff2,2048,fp) != NULL) && ((ss=_tcschr(buff2,' ')) != NULL) &&
			   ((ll=_tcslen(++ss)) > 1))
			{
				if(ss[ll-1] == '\n')
					ss[ll-1] = '\0' ;
				desktopName[ii] = _tcsdup(ss) ;
			}
		}
		/* now read all the simple flags */
		vwConfigReadInt(fp,buff,ii,nDesksX);
		vwConfigReadInt(fp,buff,ii,nDesksY);
		vwConfigReadInt(fp,buff,ii,deskWrap);
		vwConfigReadInt(fp,buff,ii,useWindowRules);
		vwConfigReadInt(fp,buff,ii,taskButtonAct);
		vwConfigReadInt(fp,buff,ii,winListContent);
		vwConfigReadInt(fp,buff,ii,winListCompact);
		vwConfigReadInt(fp,buff,ii,mouseEnable);
		vwConfigReadInt(fp,buff,ii,mouseJumpLength);
		vwConfigReadInt(fp,buff,ii,mouseDelay);
		vwConfigReadInt(fp,buff,ii,mouseWarp);
		vwConfigReadInt(fp,buff,ii,mouseKnock);
		vwConfigReadInt(fp,buff,ii,mouseModifierUsed);
		vwConfigReadInt(fp,buff,ii,mouseModifier);
		vwConfigReadInt(fp,buff,ii,preserveZOrder);
		vwConfigReadInt(fp,buff,ii,hiddenWindowAct);
		vwConfigReadInt(fp,buff,ii,releaseFocus);
		vwConfigReadInt(fp,buff,ii,refreshOnWarp);
		vwConfigReadInt(fp,buff,ii,invertY);
		vwConfigReadInt(fp,buff,ii,noTaskbarCheck);
		vwConfigReadInt(fp,buff,ii,displayTaskbarIcon);
		vwConfigReadInt(fp,buff,ii,vwLogFlag);
		vwConfigReadInt(fp,buff,ii,winMenuCompact);
		vwConfigReadInt(fp,buff,ii,useDynButtonRm);
		vwConfigReadInt(fp,buff,ii,hotkeyMenuLoc);
		vwConfigReadInt(fp, buff, ii, aggressiveRules);
		vwConfigReadInt(fp,buff,ii,vwHookUse);
		vwConfigReadInt(fp,buff,ii,useDskChgModRelease);
		vwConfigReadInt(fp,buff,ii,initialDesktop);
		vwConfigReadInt(fp,buff,ii,lastDeskNoDelay);
		vwConfigReadInt(fp,buff,ii,minWinHide);
		vwConfigReadInt(fp,buff,ii,ctlMenuCompact);
	}
	else
	{
		fclose(fp);
		_stprintf_s(buff2,2048, _T("Error reading config file:\n\n    %s\n\nUnsupported version %d, please remove."), buff, ii);
		MessageBox(hWnd, buff2, vwVIRTUAWIN_NAME _T(" Error"), MB_ICONERROR) ;
		exit(1);
	}
	
	nDesks = nDesksX * nDesksY ;
	fclose(fp);
}

/************************************************
 * Writes down the current configuration on file
 */
void
saveVirtuawinConfig(void)
{
	TCHAR VWConfigFile[MAX_PATH];
	FILE* fp;
	int ii, jj ;
	
	GetFilename(vwVIRTUAWIN_CFG,1,VWConfigFile);
	if ((fp = _tfopen(VWConfigFile, _T("w"))) == 0)
	{
		MessageBox(NULL,_T("Error writing virtuawin.cfg file"),vwVIRTUAWIN_NAME _T(" Error"),MB_ICONERROR);
	}
	else
	{
		fprintf(fp, "ver# 2\n") ;
		fprintf(fp, "hotkeyCount# %d\n", hotkeyCount);
		for(ii=0 ; ii<hotkeyCount ; ii++)
			fprintf(fp, "hotkey%d# %d %d %d %d\n",ii+1,hotkeyList[ii].key,hotkeyList[ii].modifier,hotkeyList[ii].command,hotkeyList[ii].desk) ;
		jj = vwDESKTOP_MAX ;
		while(jj && (desktopName[jj] == NULL))
			jj-- ;
		fprintf(fp, "desktopNameCount# %d\n",jj);
		for(ii=1 ; ii<=jj ; ii++)
			_ftprintf(fp, _T("desktopName%d# %s\n"),ii,(desktopName[ii] == NULL) ? _T(""):desktopName[ii]);
		fprintf(fp, "deskX# %d\n", nDesksX);
		fprintf(fp, "deskY# %d\n", nDesksY);
		fprintf(fp, "deskWrap# %d\n", deskWrap);
		fprintf(fp, "useWindowRules# %d\n", useWindowRules);
		fprintf(fp, "taskButtonAct# %d\n", taskButtonAct);
		fprintf(fp, "winListContent# %d\n", winListContent);
		fprintf(fp, "winListCompact# %d\n", winListCompact);
		fprintf(fp, "mouseEnable# %d\n", mouseEnable);
		fprintf(fp, "mouseJumpLength# %d\n", mouseJumpLength);
		fprintf(fp, "mouseDelay# %d\n", mouseDelay);
		fprintf(fp, "mouseWarp# %d\n", mouseWarp);
		fprintf(fp, "mouseKnock# %d\n", mouseKnock);
		fprintf(fp, "mouseModifierUsed# %d\n", mouseModifierUsed);
		fprintf(fp, "mouseModifier# %d\n", mouseModifier);
		fprintf(fp, "preserveZOrder# %d\n", preserveZOrder);
		fprintf(fp, "hiddenWindowAct# %d\n", hiddenWindowAct);
		fprintf(fp, "releaseFocus# %d\n", releaseFocus);
		fprintf(fp, "refreshOnWarp# %d\n", refreshOnWarp);
		fprintf(fp, "invertY# %d\n", invertY);
		fprintf(fp, "noTaskbarCheck# %d\n", noTaskbarCheck & 0x01);
		fprintf(fp, "displayTaskbarIcon# %d\n", displayTaskbarIcon);
		fprintf(fp, "logFlag# %d\n", vwLogFlag);
		fprintf(fp, "winMenuCompact# %d\n", winMenuCompact);
		fprintf(fp, "useDynButtonRm# %d\n", useDynButtonRm);
		fprintf(fp, "hotkeyMenuLoc# %d\n", hotkeyMenuLoc);
		fprintf(fp, "aggressiveRules# %d\n", aggressiveRules);
		fprintf(fp, "vwHookUse# %d\n", vwHookUse);
		fprintf(fp, "useDskChgModRelease# %d\n", useDskChgModRelease);
		fprintf(fp, "initialDesktop# %d\n", initialDesktop);
		fprintf(fp, "lastDeskNoDelay# %d\n", lastDeskNoDelay);
		fprintf(fp, "minWinHide# %d\n", minWinHide);
		fprintf(fp, "ctlMenuCompact# %d\n", ctlMenuCompact);
		fclose(fp);
	}
}
