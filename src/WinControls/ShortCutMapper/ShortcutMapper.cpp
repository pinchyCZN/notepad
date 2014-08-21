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
	HWND hTab = _hTabCtrl = ::GetDlgItem(_hSelf, IDC_SHORTCUT_TABBAR);
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
			int j,max=cshortcuts[i].getSize();
			for(j=0;j<max;j++){
				generic_string s=cshortcuts[i].toString(j);
				_sntprintf_s(str,len,_TRUNCATE,L"%s%s%c",str,s.c_str(),j<(max-1)?L';':L'');
			}
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
int ShortcutMapper::gettextwidth(HWND hwnd,const TCHAR *str)
{
	if(hwnd!=0 && str!=0){
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			SIZE size={0};
			HFONT hfont;
			hfont=(HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
			if(hfont!=0){
				HGDIOBJ hold=0;
				hold=SelectObject(hdc,hfont);
				GetTextExtentPoint32(hdc,str,_tcslen(str),&size);
				if(hold!=0)
					SelectObject(hdc,hold);
			}
			else{
				GetTextExtentPoint32(hdc,str,_tcslen(str),&size);
			}
			ReleaseDC(hwnd,hdc);
			return size.cx+LISTWIDTH_ADD;
		}
	}
	return 0;
}
void ShortcutMapper::initList()
{
	int i;
	LV_COLUMN col={0};
	TCHAR *col_names[3]={L"Index",L"Name",L"Shortcut"};
	hlistview=GetDlgItem(_hSelf,IDC_SHORTCUT_LIST);
	for(i=0;i<sizeof(col_names)/sizeof(TCHAR *);i++){
		col.mask=LVCF_FMT|LVCF_TEXT|LVCF_WIDTH;
		col.fmt=LVCFMT_LEFT;
		col.cx=gettextwidth(hlistview,col_names[i]);
		col.pszText=col_names[i];
		ListView_InsertColumn(hlistview,i,&col);
	}
	ListView_SetExtendedListViewStyle(hlistview,
		ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_FULLROWSELECT);
}
int ShortcutMapper::getselectedrow()
{
	int i,count,sel=-1;
	count=ListView_GetItemCount(hlistview);
	for(i=0;i<count;i++){
		if(ListView_GetItemState(hlistview,i,LVIS_SELECTED)==LVIS_SELECTED){
			sel=i;
			break;
		}
	}
	return sel;
}
int ShortcutMapper::update_col_width(const TCHAR *str,int col)
{
	int w,col_w;
	w=gettextwidth(hlistview,str);
	col_w=ListView_GetColumnWidth(hlistview,col);
	if(w>col_w)
		ListView_SetColumnWidth(hlistview,col,w);
	return TRUE;
}
int ShortcutMapper::getitemindex(int row)
{
	LV_ITEM lv={0};
	lv.mask=LVIF_PARAM;
	lv.iItem=row;
	ListView_GetItem(hlistview,&lv);
	return lv.lParam;
}
void ShortcutMapper::populateShortCuts()
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
	ListView_DeleteAllItems(hlistview);

	if(nrItems==0){
        ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), false);
        ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), false);
		return;
	}

	switch(_currentState) {
		case STATE_MENU: {
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), false);
			break; }
		case STATE_MACRO: {
            bool shouldBeEnabled = nrItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), shouldBeEnabled);
			break; }
		case STATE_USER: {
            bool shouldBeEnabled = nrItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), shouldBeEnabled);
			break; }
		case STATE_PLUGIN: {
            bool shouldBeEnabled = nrItems > 0;
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), shouldBeEnabled);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), false);
			break; }
		case STATE_SCINTILLA: {
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), true);
            ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), false);
			break; }
	}
	int index=0;
	int widths[3];
	widths[0]=gettextwidth(hlistview,L"Index");
	widths[1]=gettextwidth(hlistview,L"Name");
	widths[2]=gettextwidth(hlistview,L"Shortcut");
	GetWindowText(GetDlgItem(_hSelf,IDC_SHORTCUT_FILTER1),filter1,sizeof(filter2)/sizeof(TCHAR));
	GetWindowText(GetDlgItem(_hSelf,IDC_SHORTCUT_FILTER2),filter2,sizeof(filter2)/sizeof(TCHAR));
	_tcslwr(filter1);
	_tcslwr(filter2);

	for(size_t i = 0; i < nrItems; i++) {
		TCHAR keys[40]={0};
		const TCHAR *name=GetShortcutName(_currentState,i,nppParam);
		GetShortcutKeys(_currentState,i,nppParam,keys,sizeof(keys)/sizeof(TCHAR));
		if((filter1[0]==L'\0' && filter2[0]==L'\0') 
			|| (filter1[0]!=L'\0' && CheckFilter(name,filter1))
			|| (filter2[0]!=L'\0' && CheckFilter(keys,filter2))){
			TCHAR str[10]={0};
			LV_ITEM lvitem={0};
			int w;
			_sntprintf_s(str,sizeof(str)/sizeof(TCHAR),_TRUNCATE,L"%i",i);
			w=gettextwidth(hlistview,str);
			if(w>widths[0])
				widths[0]=w;
			w=gettextwidth(hlistview,name);
			if(w>widths[1])
				widths[1]=w;
			w=gettextwidth(hlistview,keys);
			if(w>widths[2])
				widths[2]=w;

			lvitem.mask=LVIF_TEXT|LVIF_PARAM;
			lvitem.iItem=index;
			lvitem.pszText=(LPWSTR)str;
			lvitem.lParam=i;
			ListView_InsertItem(hlistview,&lvitem);
			lvitem.mask=LVIF_TEXT;
			lvitem.iSubItem=1;
			lvitem.pszText=(LPWSTR)name;
			ListView_SetItem(hlistview,&lvitem);
			lvitem.iSubItem=2;
			lvitem.pszText=(LPWSTR)keys;
			ListView_SetItem(hlistview,&lvitem);
			index++;
		}
	}
	ListView_SetColumnWidth(hlistview,0,widths[0]);
	ListView_SetColumnWidth(hlistview,1,widths[1]);
	ListView_SetColumnWidth(hlistview,2,widths[2]);
	ListView_SetItemState(hlistview,0,LVIS_SELECTED,LVIS_SELECTED);
	if(index==0){
	    ::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_MODIFY), false);
		::EnableWindow(::GetDlgItem(_hSelf, IDC_SHORTCUT_DELETE), false);
	}
}
int ShortcutMapper::compare_keys(int indexa,int indexb,const TCHAR *name,const KeyCombo *ka,const KeyCombo *kb,TCHAR *str,int len)
{
	int count=0;
	if(indexa!=indexb
		&& ka->_isAlt==kb->_isAlt
		&& ka->_isCtrl==kb->_isCtrl
		&& ka->_isShift==kb->_isShift
		&& ka->_key==kb->_key){
			_sntprintf_s(str,len,_TRUNCATE,L"%s\r\n%s",str,name);
			count++;
	}
	return count;
}
int ShortcutMapper::check_in_use(int index,const KeyCombo *kc,NppParameters *nppParam)
{
	int i,count=0;
	TCHAR str[255]={0};
	if(kc->_key==0)
		return count;
	_sntprintf_s(str,sizeof(str)/sizeof(TCHAR),_TRUNCATE,L"%s",L"Duplicate shortcuts found:");
	{
		vector<CommandShortcut> &shortcuts=nppParam->getUserShortcuts();
		for(i=0;i<(int)shortcuts.size();i++){
			CommandShortcut sc=shortcuts[i];
			count+=compare_keys(_currentState==STATE_MENU?i:-1,index,sc.getName(),&sc.getKeyCombo(),kc,str,sizeof(str)/sizeof(TCHAR));
		}
	}
	{
		vector<MacroShortcut> & shortcuts = nppParam->getMacroList();
		for(i=0;i<(int)shortcuts.size();i++){
			MacroShortcut sc=shortcuts[i];
			count+=compare_keys(_currentState==STATE_MACRO?i:-1,index,sc.getName(),&sc.getKeyCombo(),kc,str,sizeof(str)/sizeof(TCHAR));
		}
	}
	{
		vector<UserCommand> & shortcuts = nppParam->getUserCommandList();
		for(i=0;i<(int)shortcuts.size();i++){
			UserCommand sc=shortcuts[i];
			count+=compare_keys(_currentState==STATE_USER?i:-1,index,sc.getName(),&sc.getKeyCombo(),kc,str,sizeof(str)/sizeof(TCHAR));
		}
	}
	{
		vector<PluginCmdShortcut> & shortcuts = nppParam->getPluginCommandList();
		for(i=0;i<(int)shortcuts.size();i++){
			PluginCmdShortcut sc=shortcuts[i];
			count+=compare_keys(_currentState==STATE_PLUGIN?i:-1,index,sc.getName(),&sc.getKeyCombo(),kc,str,sizeof(str)/sizeof(TCHAR));
		}
	}
	{
		vector<ScintillaKeyMap> & shortcuts = nppParam->getScintillaKeyList();
		for(i=0;i<(int)shortcuts.size();i++){
			ScintillaKeyMap sc=shortcuts[i];
			int j,max;
			max=sc.getSize();
			for(j=0;j<max;j++){
				count+=compare_keys(_currentState==STATE_SCINTILLA?i:-1,index,sc.getName(),&sc.getKeyComboByIndex(j),kc,str,sizeof(str)/sizeof(TCHAR));
			}
			if(_currentState==STATE_SCINTILLA && index<shortcuts.size()){
				ScintillaKeyMap current_sc=shortcuts[index];
				if(current_sc.getSize()>1){
					int k,current_max=current_sc.getSize();
					for(k=1;k<current_max;k++){
						for(j=0;j<max;j++){
							KeyCombo *current_kc=&current_sc.getKeyComboByIndex(k);
							count+=compare_keys(i,index,sc.getName(),&sc.getKeyComboByIndex(j),current_kc,str,sizeof(str)/sizeof(TCHAR));
						}
					}
				}
			}
		}
	}
	if(count>0)
		::MessageBox(_hSelf,str,L"Warning",MB_OK);
	return count;
}

BOOL CALLBACK ShortcutMapper::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_INITDIALOG :
		{
			initTabs();
			initList();
			populateShortCuts();
			goToCenter();
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
					populateShortCuts();
				}
			}
			else if(nmh.hwndFrom==hlistview){
				switch(nmh.code){
					case NM_DBLCLK:
						return ::SendMessage(_hSelf, WM_COMMAND, IDC_SHORTCUT_MODIFY, LOWORD(lParam));
					case NM_RCLICK:
					{
						POINT p;
						::GetCursorPos(&p);
						if (!_rightClickMenu.isCreated())
						{
							vector<MenuItemUnit> itemUnitArray;
							itemUnitArray.push_back(MenuItemUnit(IDC_SHORTCUT_MODIFY, TEXT("Modify")));
							itemUnitArray.push_back(MenuItemUnit(IDC_SHORTCUT_DELETE, TEXT("Delete")));
							_rightClickMenu.create(_hSelf, itemUnitArray);
						}
						switch(_currentState) {
							case STATE_MACRO:
							case STATE_USER: {
								_rightClickMenu.enableItem(IDC_SHORTCUT_DELETE, true);
								break; }
							case STATE_MENU:
							case STATE_PLUGIN:
							case STATE_SCINTILLA: {
								_rightClickMenu.enableItem(IDC_SHORTCUT_DELETE, false);
								break; }
						}
						
						_rightClickMenu.display(p);
						return TRUE;
					}
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
					if(GetFocus()==hlistview)
						return ::SendMessage(_hSelf, WM_COMMAND, IDC_SHORTCUT_MODIFY, LOWORD(lParam));
					else
						::EndDialog(_hSelf, 0);
					return TRUE;
				}
				case IDC_SHORTCUT_FILTER2:
				case IDC_SHORTCUT_FILTER1:
					if(HIWORD(wParam)==EN_CHANGE)
						populateShortCuts();
					break;
				case IDC_SHORTCUT_MODIFY :
				{
					NppParameters *nppParam = NppParameters::getInstance();
					int index;
					int selected_row=getselectedrow();
					if(selected_row<0)
						break;
					index=getitemindex(selected_row);

					switch(_currentState) {
						case STATE_MENU: {
							//Get CommandShortcut corresponding to index
							vector<CommandShortcut> & shortcuts = nppParam->getUserShortcuts();
							CommandShortcut csc = shortcuts[index], prevcsc = shortcuts[index];
							csc.init(_hInst, _hSelf);
							if (csc.doDialog() != -1 && prevcsc != csc) {	//shortcut was altered
								generic_string keys=csc.toString();
								nppParam->addUserModifiedIndex(index);
								shortcuts[index] = csc;
								ListView_SetItemText(hlistview,selected_row,2,(LPWSTR)keys.c_str());
								update_col_width(keys.c_str(),2);
								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								check_in_use(index,&csc.getKeyCombo(),nppParam);								
							}
							break; 
						}
						case STATE_MACRO: {
							//Get MacroShortcut corresponding to index
							vector<MacroShortcut> & shortcuts = nppParam->getMacroList();
							MacroShortcut msc = shortcuts[index], prevmsc = shortcuts[index];
							msc.init(_hInst, _hSelf);
							if (msc.doDialog() != -1 && prevmsc != msc) {	//shortcut was altered
								generic_string name=msc.getName();
								generic_string keys=msc.toString();
								shortcuts[index] = msc;
								ListView_SetItemText(hlistview,selected_row,1,(LPWSTR)name.c_str());
								ListView_SetItemText(hlistview,selected_row,2,(LPWSTR)keys.c_str());
								update_col_width(keys.c_str(),2);
								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								check_in_use(index,&msc.getKeyCombo(),nppParam);
							}
							break; 
						}
						case STATE_USER: {
							//Get UserCommand corresponding to index
							vector<UserCommand> & shortcuts = nppParam->getUserCommandList();
							UserCommand ucmd = shortcuts[index], prevucmd = shortcuts[index];
							ucmd.init(_hInst, _hSelf);
							prevucmd = ucmd;
							if (ucmd.doDialog() != -1 && prevucmd != ucmd) {	//shortcut was altered
								generic_string name=ucmd.getName();
								generic_string keys=ucmd.toString();
								shortcuts[index] = ucmd;
								ListView_SetItemText(hlistview,selected_row,1,(LPWSTR)name.c_str());
								ListView_SetItemText(hlistview,selected_row,2,(LPWSTR)keys.c_str());
								update_col_width(name.c_str(),1);
								update_col_width(keys.c_str(),2);
								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								check_in_use(index,&ucmd.getKeyCombo(),nppParam);
							}
							break; 
						}
						case STATE_PLUGIN: {
							//Get PluginCmdShortcut corresponding to index
							vector<PluginCmdShortcut> & shortcuts = nppParam->getPluginCommandList();
							if(shortcuts.empty())
								break;
							PluginCmdShortcut pcsc = shortcuts[index], prevpcsc = shortcuts[index];
							pcsc.init(_hInst, _hSelf);
							prevpcsc = pcsc;
							if (pcsc.doDialog() != -1 && prevpcsc != pcsc) {	//shortcut was altered
								nppParam->addPluginModifiedIndex(index);
								generic_string keys=pcsc.toString();
								shortcuts[index] = pcsc;
								ListView_SetItemText(hlistview,selected_row,2,(LPWSTR)keys.c_str());
								update_col_width(keys.c_str(),2);
								//Notify current Accelerator class to update everything
								nppParam->getAccelerator()->updateShortcuts();
								check_in_use(index,&pcsc.getKeyCombo(),nppParam);
								unsigned long cmdID = pcsc.getID();
								ShortcutKey shortcut;
								shortcut._isAlt = pcsc.getKeyCombo()._isAlt;
								shortcut._isCtrl = pcsc.getKeyCombo()._isCtrl;
								shortcut._isShift = pcsc.getKeyCombo()._isShift;
								shortcut._key = pcsc.getKeyCombo()._key;

								::SendMessage(_hParent, NPPM_INTERNAL_PLUGINSHORTCUTMOTIFIED, cmdID, (LPARAM)&shortcut);
							}
							break; 
						}
						case STATE_SCINTILLA: {
							//Get ScintillaKeyMap corresponding to index
							vector<ScintillaKeyMap> & shortcuts = nppParam->getScintillaKeyList();
							ScintillaKeyMap skm = shortcuts[index], prevskm = shortcuts[index];
							skm.init(_hInst, _hSelf);
							if (skm.doDialog() != -1 && prevskm != skm) 
							{
								//shortcut was altered
								nppParam->addScintillaModifiedIndex(index);
								shortcuts[index] = skm;
								int i,max;
								TCHAR str[40]={0};
								max=skm.getSize();
								for(i=0;i<max;i++){
									generic_string keys=skm.toString(i);
									_sntprintf_s(str,sizeof(str)/sizeof(TCHAR),_TRUNCATE,L"%s%s%c",str,keys.c_str(),i<(max-1)?L';':L'');
								}
								ListView_SetItemText(hlistview,selected_row,2,(LPWSTR)str);
								update_col_width(str,2);
								if(max>0)
									check_in_use(index,&skm.getKeyComboByIndex(0),nppParam);
								//Notify current Accelerator class to update key
								nppParam->getScintillaAccelerator()->updateKeys();
							}
							break; 
						}
					}
					return TRUE;
				}
				case IDC_SHORTCUT_DELETE :
				{
					NppParameters *nppParam = NppParameters::getInstance();
					int index,selected_row=getselectedrow();
					TCHAR str[255]={0},msg[255]={0};
					if(selected_row<0)
						break;
					index=getitemindex(selected_row);
					ListView_GetItemText(hlistview,selected_row,1,str,sizeof(str)/sizeof(TCHAR));
					_sntprintf_s(msg,sizeof(msg)/sizeof(TCHAR),_TRUNCATE,
						TEXT("%s\r\n%s"),
						TEXT("Are you sure you want to delete this shortcut?"),
						str);
					if (::MessageBox(_hSelf, msg, TEXT("Are you sure?"), MB_OKCANCEL) == IDOK)
					{
						int shortcutIndex = index;
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
								populateShortCuts();
								
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
								populateShortCuts();
							
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
			}
		}
		default:
			return FALSE;
	}
	return FALSE;
}
