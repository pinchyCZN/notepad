// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#include "precompiledHeaders.h"
#include "ShortcutMapper.h"
#include "Notepad_plus.h"

void ShortcutMapper::initTabs() {
	HWND hTab = _hTabCtrl = ::GetDlgItem(_hSelf, IDC_BABYGRID_TABBAR);
	TCITEM tie;
	tie.mask = TCIF_TEXT;
	tie.pszText = tabNames[0];
	::SendMessage(hTab, TCM_INSERTITEM, 0, (LPARAM)(&tie) );
	tie.pszText = tabNames[1];
	::SendMessage(hTab, TCM_INSERTITEM, 1, (LPARAM)(&tie) );
	tie.pszText = tabNames[2];
	::SendMessage(hTab, TCM_INSERTITEM, 2, (LPARAM)(&tie) );
	tie.pszText = tabNames[3];
	::SendMessage(hTab, TCM_INSERTITEM, 3, (LPARAM)(&tie) );
	tie.pszText = tabNames[4];
	::SendMessage(hTab, TCM_INSERTITEM, 4, (LPARAM)(&tie) );

    TabCtrl_SetCurSel(_hTabCtrl, int(_currentState));
}

void ShortcutMapper::translateTab(int index, const TCHAR * newname) {
	if (index < 0 || index > 4)
		return;
	generic_strncpy(tabNames[index], newname, maxTabName);
}

void ShortcutMapper::initBabyGrid() {
	RECT rect;
	getClientRect(rect);
	
	_babygrid.init(_hInst, _hSelf, IDD_BABYGRID_ID1);
	//_babygrid.reSizeTo(rect);
	_babygrid.reSizeToWH(rect);
	_babygrid.hideCursor();
	_babygrid.makeColAutoWidth();
	_babygrid.setColsNumbered(false);
	_babygrid.setColWidth(0, 30);
	_babygrid.setColWidth(1, 250);
}

static const TCHAR * GetShortcutName(GridState gs,int i,NppParameters *nppParam)
{
	switch(gs){
		case STATE_MENU: 
		{
			vector<CommandShortcut> & cshortcuts = nppParam->getUserShortcuts();
			return cshortcuts[i].getName();
		}
		case STATE_MACRO: 
		{
			vector<MacroShortcut> & cshortcuts = nppParam->getMacroList();
			return cshortcuts[i].getName();
		}
		case STATE_USER: 
		{
			vector<UserCommand> & cshortcuts = nppParam->getUserCommandList();
			return cshortcuts[i].getName();
		}
		case STATE_PLUGIN: 
		{
			vector<PluginCmdShortcut> & cshortcuts = nppParam->getPluginCommandList();
			return cshortcuts[i].getName();
		}
		case STATE_SCINTILLA:
		{
			vector<ScintillaKeyMap> & cshortcuts = nppParam->getScintillaKeyList();
			return cshortcuts[i].getName();
		}
	}
	return 0;
}
static bool GetShortcutKeys(GridState gs,int i,NppParameters *nppParam,TCHAR *str,int len)
{
	switch(gs){
		case STATE_MENU: 
		{
			vector<CommandShortcut> & cshortcuts = nppParam->getUserShortcuts();
			_tcsncpy_s(str,len,cshortcuts[i].toString().c_str(),_TRUNCATE);
			return TRUE ;
		}
		case STATE_MACRO: 
		{
			vector<MacroShortcut> & cshortcuts = nppParam->getMacroList();
			_tcsncpy_s(str,len,cshortcuts[i].toString().c_str(),_TRUNCATE);
			_tcsncpy_s(str,len,cshortcuts[i].toString().c_str(),_TRUNCATE);
			return TRUE ;
		}
		case STATE_USER: 
		{
			vector<UserCommand> & cshortcuts = nppParam->getUserCommandList();
			_tcsncpy_s(str,len,cshortcuts[i].toString().c_str(),_TRUNCATE);
			return TRUE ;
		}
		case STATE_PLUGIN: 
		{
			vector<PluginCmdShortcut> & cshortcuts = nppParam->getPluginCommandList();
			_tcsncpy_s(str,len,cshortcuts[i].toString().c_str(),_TRUNCATE);
			return TRUE ;
		}
		case STATE_SCINTILLA:
		{
			vector<ScintillaKeyMap> & cshortcuts = nppParam->getScintillaKeyList();
			_tcsncpy_s(str,len,cshortcuts[i].toString().c_str(),_TRUNCATE);
			return TRUE ;
		}
	}
	return FALSE;
}
static bool CheckFilter(const TCHAR *str,const TCHAR *filter)
{
	TCHAR tstr[80]={0},tfilter[80]={0};
	_tcsncpy_s(tstr,sizeof(tstr)/sizeof(TCHAR),str,_TRUNCATE);
	_tcsncpy_s(tfilter,sizeof(tfilter)/sizeof(TCHAR),filter,_TRUNCATE);
	_tcslwr_s(tstr,sizeof(tstr)/sizeof(TCHAR));
	_tcslwr_s(tfilter,sizeof(tfilter)/sizeof(TCHAR));
	if(_tcsstr(tstr,tfilter))
		return TRUE;
	else
		return FALSE;
}
void ShortcutMapper::fillOutBabyGrid()
{
	TCHAR filter1[40]={0},filter2[40]={0};
	NppParameters *nppParam = NppParameters::getInstance();
	size_t nrItems = 0;

	switch(_currentState) {
		case STATE_MENU: {
			nrItems = nppParam->getUserShortcuts().size();
			break; }
		case STATE_MACRO: {
			nrItems = nppParam->getMacroList().size();
			break; }
		case STATE_USER: {
			nrItems = nppParam->getUserCommandList().size();
			break; }
		case STATE_PLUGIN: {
			nrItems = nppParam->getPluginCommandList().size();
			break; }
		case STATE_SCINTILLA: {
			nrItems = nppParam->getScintillaKeyList().size();
			break; }
	}
	_babygrid.clear();
	//_babygrid.setLineColNumber(nrItems, 3);
	_babygrid.setLineColNumber(0, 3);

	_babygrid.setText(0, 1, TEXT("Name"));
	_babygrid.setText(0, 2, TEXT("Shortcut"));
	_babygrid.setText(0, 3, TEXT("Shortcut Index"));

	GetWindowText(GetDlgItem(_hSelf,IDC_BABYGRID_FILTER1),filter1,sizeof(filter1)/sizeof(TCHAR));
	GetWindowText(GetDlgItem(_hSelf,IDC_BABYGRID_FILTER2),filter2,sizeof(filter2)/sizeof(TCHAR));
	_tcslwr(filter1);
	_tcslwr(filter2);
	if(nrItems==0){
        ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), false);
        ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
		return;
	}

	switch(_currentState) {
		case STATE_MENU: {
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
			break; }
		case STATE_MACRO: {
            bool shouldBeEnabled = nrItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
			break; }
		case STATE_USER: {
            bool shouldBeEnabled = nrItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), shouldBeEnabled);
			break; }
		case STATE_PLUGIN: {
            bool shouldBeEnabled = nrItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
			break; }
		case STATE_SCINTILLA: {
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDM_BABYGRID_DELETE), false);
			break; }
	}
	int index=0;
	for(size_t i = 0; i < nrItems; i++) {
		TCHAR keys[40]={0};
		const TCHAR *name=GetShortcutName(_currentState,i,nppParam);
		GetShortcutKeys(_currentState,i,nppParam,keys,sizeof(keys)/sizeof(TCHAR));
		if((filter1[0]==L'\0' && filter2[0]==L'\0') 
			|| (filter1[0]!=L'\0' && CheckFilter(name,filter1))
			|| (filter2[0]!=L'\0' && CheckFilter(keys,filter2))){
			TCHAR str[10]={0};
			_babygrid.setText(index+1, 1, name);
			_babygrid.setText(index+1, 2, keys);
			_sntprintf_s(str,sizeof(str)/sizeof(TCHAR),_TRUNCATE,L"%i",i+1);
			_babygrid.setText(index+1, 3, str);
			index++;
		}
	}
}

BOOL CALLBACK ShortcutMapper::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			initBabyGrid();
			initTabs();
			fillOutBabyGrid();
			_babygrid.display();	
			goToCenter();
			SetWindowText(GetDlgItem(_hSelf,IDC_BABYGRID_FILTER1),L"");
			SetWindowText(GetDlgItem(_hSelf,IDC_BABYGRID_FILTER2),L"");
			return TRUE;
		}

		case WM_NOTIFY: {
			NMHDR nmh = *((NMHDR*)lParam);
			if (nmh.hwndFrom == _hTabCtrl) {
				if (nmh.code == TCN_SELCHANGE) {
					int index = TabCtrl_GetCurSel(_hTabCtrl);
					switch (index) {
						case 0:
							_currentState = STATE_MENU;
							break;
						case 1:
							_currentState = STATE_MACRO;
							break;
						case 2:
							_currentState = STATE_USER;
							break;
						case 3:
							_currentState = STATE_PLUGIN;
							break;
						case 4:
							_currentState = STATE_SCINTILLA;
							break;
					}
					fillOutBabyGrid();
				}
			}
			break; }

		case WM_COMMAND : 
		{
			switch (LOWORD(wParam))
			{
				case IDCANCEL :
				{
					::EndDialog(_hSelf, -1);
					return TRUE;
				}
				case IDOK :
				{
					::EndDialog(_hSelf, 0);
					return TRUE;
				}
				case IDC_BABYGRID_FILTER2:
				case IDC_BABYGRID_FILTER1:
					if(HIWORD(wParam)==EN_CHANGE)
						fillOutBabyGrid();
					break;
				case IDM_BABYGRID_MODIFY :
				{
					NppParameters *nppParam = NppParameters::getInstance();
					int row = _babygrid.getSelectedRow();
					TCHAR str[10]={0};
					_babygrid.getText(row,3,str);
					if(str[0]!=L'\0')
						row=_tstoi(str);
					else
						break;
					

					switch(_currentState) {
						case STATE_MENU: {
							//Get CommandShortcut corresponding to row
							vector<CommandShortcut> & shortcuts = nppParam->getUserShortcuts();
							CommandShortcut csc = shortcuts[row - 1], prevcsc = shortcuts[row - 1];
							csc.init(_hInst, _hSelf);
							if (csc.doDialog() != -1 && prevcsc != csc) {	//shortcut was altered
								nppParam->addUserModifiedIndex(row-1);
								shortcuts[row - 1] = csc;
								_babygrid.setText(row, 2, csc.toString().c_str());
								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								
							}
							break; }
						case STATE_MACRO: {
							//Get MacroShortcut corresponding to row
							vector<MacroShortcut> & shortcuts = nppParam->getMacroList();
							MacroShortcut msc = shortcuts[row - 1], prevmsc = shortcuts[row - 1];
							msc.init(_hInst, _hSelf);
							if (msc.doDialog() != -1 && prevmsc != msc) {	//shortcut was altered
								shortcuts[row - 1] = msc;
								_babygrid.setText(row, 1, msc.getName());
								_babygrid.setText(row, 2, msc.toString().c_str());

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								
							}
							break; }
						case STATE_USER: {
							//Get UserCommand corresponding to row
							vector<UserCommand> & shortcuts = nppParam->getUserCommandList();
							UserCommand ucmd = shortcuts[row - 1], prevucmd = shortcuts[row - 1];
							ucmd.init(_hInst, _hSelf);
							prevucmd = ucmd;
							if (ucmd.doDialog() != -1 && prevucmd != ucmd) {	//shortcut was altered
								shortcuts[row - 1] = ucmd;
								_babygrid.setText(row, 1, ucmd.getName());
								_babygrid.setText(row, 2, ucmd.toString().c_str());

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								
							}
							break; }
						case STATE_PLUGIN: {
							//Get PluginCmdShortcut corresponding to row
							vector<PluginCmdShortcut> & shortcuts = nppParam->getPluginCommandList();
							if(shortcuts.empty())
								break;
							PluginCmdShortcut pcsc = shortcuts[row - 1], prevpcsc = shortcuts[row - 1];
							pcsc.init(_hInst, _hSelf);
							prevpcsc = pcsc;
							if (pcsc.doDialog() != -1 && prevpcsc != pcsc) {	//shortcut was altered
								nppParam->addPluginModifiedIndex(row-1);
								shortcuts[row - 1] = pcsc;
								_babygrid.setText(row, 2, pcsc.toString().c_str());

								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								unsigned long cmdID = pcsc.getID();
								ShortcutKey shortcut;
								shortcut._isAlt = pcsc.getKeyCombo()._isAlt;
								shortcut._isCtrl = pcsc.getKeyCombo()._isCtrl;
								shortcut._isShift = pcsc.getKeyCombo()._isShift;
								shortcut._key = pcsc.getKeyCombo()._key;

								::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED, cmdID, (LPARAM)&shortcut);
							}
							break; }
						case STATE_SCINTILLA: {
							//Get ScintillaKeyMap corresponding to row
							vector<ScintillaKeyMap> & shortcuts = nppParam->getScintillaKeyList();
							ScintillaKeyMap skm = shortcuts[row - 1], prevskm = shortcuts[row-1];
							skm.init(_hInst, _hSelf);
							if (skm.doDialog() != -1 && prevskm != skm) 
							{
								//shortcut was altered
								nppParam->addScintillaModifiedIndex(row-1);
								shortcuts[row-1] = skm;
								_babygrid.setText(row, 2, skm.toString().c_str());

								//Notify current Accelerator class to update key
								nppParam->getScintillaAccelerator()->updateKeys();
							}
							break; 
						}
					}
					return TRUE;
				}
				case IDM_BABYGRID_DELETE :
				{
					NppParameters *nppParam = NppParameters::getInstance();
					if (::MessageBox(_hSelf, TEXT("Are you sure you want to delete this shortcut?"), TEXT("Are you sure?"), MB_OKCANCEL) == IDOK)
					{
						const int row = _babygrid.getSelectedRow();
						int shortcutIndex = row-1;
						DWORD cmdID = 0;
						
						// Menu data
						size_t posBase = 0;
						size_t nbElem = 0;
						HMENU hMenu = NULL;
                        int modifCmd = IDM_SETTING_SHORTCUT_MAPPER_RUN;
						switch(_currentState) 
						{
							case STATE_MENU:
							case STATE_PLUGIN:
							case STATE_SCINTILLA: 
							{
								return FALSE;			//this is bad
							}
							case STATE_MACRO: 
							{
								vector<MacroShortcut> & theMacros = nppParam->getMacroList();
								vector<MacroShortcut>::iterator it = theMacros.begin();
								cmdID = theMacros[shortcutIndex].getID();
								theMacros.erase(it + shortcutIndex);
								fillOutBabyGrid();
								
								// preparing to remove from menu
								posBase = 6;
								nbElem = theMacros.size();
								hMenu = ::GetSubMenu((HMENU)::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_MACRO);
                                modifCmd = IDM_SETTING_SHORTCUT_MAPPER_MACRO;
								for (size_t i = shortcutIndex ; i < nbElem ; i++)	//lower the IDs of the remaining items so there are no gaps
								{
									MacroShortcut ms = theMacros[i];
									ms.setID(ms.getID() - 1);	//shift all IDs
									theMacros[i] = ms;
								}
								break; 
							}
							case STATE_USER: 
							{
								vector<UserCommand> & theUserCmds = nppParam->getUserCommandList();
								vector<UserCommand>::iterator it = theUserCmds.begin();
								cmdID = theUserCmds[shortcutIndex].getID();
								theUserCmds.erase(it + shortcutIndex);
								fillOutBabyGrid();
							
								// preparing to remove from menu
								posBase = 2;
								nbElem = theUserCmds.size();
								hMenu = ::GetSubMenu((HMENU)::SendMessage(_hParent, NPPM_INTERNAL_GETMENU, 0, 0), MENUINDEX_RUN);
                                modifCmd = IDM_SETTING_SHORTCUT_MAPPER_RUN;
								for (size_t i = shortcutIndex ; i < nbElem ; i++)	//lower the IDs of the remaining items so there are no gaps
								{
									UserCommand uc = theUserCmds[i];
									uc.setID(uc.getID() - 1);	//shift all IDs
									theUserCmds[i] = uc;
								}
								break;
							}
						}

                        // updateShortcuts() will update all menu item - the menu items will be shifted
						nppParam->getAccelerator()->updateShortcuts();

                        // All menu items are shifted up. So we delete the last item
                        ::RemoveMenu(hMenu, posBase + nbElem, MF_BYPOSITION);

                        if (nbElem == 0) 
                        {
                            ::RemoveMenu(hMenu, modifCmd, MF_BYCOMMAND);
                            
                            //remove separator
							::RemoveMenu(hMenu, posBase-1, MF_BYPOSITION);
                            ::RemoveMenu(hMenu, posBase-1, MF_BYPOSITION);
						}
					}
					return TRUE;
				}

				case IDD_BABYGRID_ID1: {
					if(HIWORD(wParam) == BGN_CELLDBCLICKED) //a cell was clicked in the properties grid
					{
						return ::SendMessage(_hSelf, WM_COMMAND, IDM_BABYGRID_MODIFY, LOWORD(lParam));
					}
					else if(HIWORD(wParam) == BGN_CELLRCLICKED) //a cell was clicked in the properties grid
					{
						POINT p;
						::GetCursorPos(&p);
						if (!_rightClickMenu.isCreated())
						{
							vector<MenuItemUnit> itemUnitArray;
							itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_MODIFY, TEXT("Modify")));
							itemUnitArray.push_back(MenuItemUnit(IDM_BABYGRID_DELETE, TEXT("Delete")));
							_rightClickMenu.create(_hSelf, itemUnitArray);
						}
						switch(_currentState) {
							case STATE_MACRO:
							case STATE_USER: {
								_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, true);
								break; }
							case STATE_MENU:
							case STATE_PLUGIN:
							case STATE_SCINTILLA: {
								_rightClickMenu.enableItem(IDM_BABYGRID_DELETE, false);
								break; }
						}
						
						_rightClickMenu.display(p);
						return TRUE;
					}
				}
			}
		}
		default:
			return FALSE;
	}
	return FALSE;
}
