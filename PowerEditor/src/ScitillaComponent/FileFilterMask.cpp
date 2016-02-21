#include "precompiledHeaders.h"
#include "Parameters.h"
#include "FileMask.h"
#include "AnchorSystem.h"

static HINSTANCE ghinstance=0;

static struct CONTROL_ANCHOR FileMaskAnchors[]={
	{IDC_FILTERLIST,ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP|ANCHOR_BOTTOM,0,0,0},
	{IDOK,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_ADD,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_EDIT,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_DELETE,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDCANCEL,ANCHOR_LEFT|ANCHOR_BOTTOM,0,0,0},
	{IDC_GRIPPER,ANCHOR_RIGHT|ANCHOR_BOTTOM,0,0,0}
};
static struct WIN_REL_POS WinRelPos={0};

int filter_set_hinstance(HINSTANCE hinst)
{
	ghinstance=hinst;
	return 0;
}
int get_first_line_len(const TCHAR *str)
{
	int i,len;
	len=0x10000;
	for(i=0;i<len;i++){
		if(str[i]==TEXT('\n') || str[i]==TEXT('\r') || str[i]==0)
			break;
	}
	return i;
}
int get_str_width(HWND hwnd,const TCHAR *str)
{
	if(hwnd!=0 && str!=0){
		SIZE size={0};
		HDC hdc;
		hdc=GetDC(hwnd);
		if(hdc!=0){
			HFONT hfont;
			int len=get_first_line_len(str);
			hfont=(HFONT)SendMessage(hwnd,WM_GETFONT,0,0);
			if(hfont!=0){
				HGDIOBJ hold=0;
				hold=SelectObject(hdc,hfont);
				GetTextExtentPoint32(hdc,str,len,&size);
				if(hold!=0)
					SelectObject(hdc,hold);
			}
			else{
				GetTextExtentPoint32(hdc,str,len,&size);
			}
			ReleaseDC(hwnd,hdc);
			return size.cx;
		}
	}
	return 0;

}

int lv_add_column(HWND hlistview,const TCHAR *str,int index)
{
	LV_COLUMN col;
	if(hlistview!=0 && str!=0){
		HWND header;
		int width=0;
		header=(HWND)SendMessage(hlistview,LVM_GETHEADER,0,0);
		width=get_str_width(header,str);
		width+=14;
		if(width<40)
			width=40;
		col.mask = LVCF_WIDTH|LVCF_TEXT;
		col.cx = width;
		col.pszText = (LPWSTR)str;
		if(ListView_InsertColumn(hlistview,index,&col)>=0)
			return width;
	}
	return 0;
}
int lv_insert_data(HWND hlistview,int row,int col,const TCHAR *str)
{
	if(hlistview!=0 && str!=0){
		LV_ITEM item;
		memset(&item,0,sizeof(item));
		if(col==0){
			item.mask=LVIF_TEXT|LVIF_PARAM;
			item.iItem=row;
			item.pszText=(LPWSTR)str;
			item.lParam=row;
			ListView_InsertItem(hlistview,&item);
		}
		else{
			item.mask=LVIF_TEXT;
			item.iItem=row;
			item.pszText=(LPWSTR)str;
			item.iSubItem=col;
			ListView_SetItem(hlistview,&item);
		}
		return TRUE;
	}
	return FALSE;
}
int populate_listview(HWND hlistview,std::vector<generic_string> *slist)
{
	ListView_DeleteAllItems(hlistview);
	if(slist!=0){
		int row=0;
		for(std::vector<generic_string>::iterator i = slist->begin();i!=slist->end();i++){
			int pos=i->find(TEXT('|'));
			if(pos>=0){
				lv_insert_data(hlistview,row,0,i->substr(0,pos).c_str());
				lv_insert_data(hlistview,row,1,i->substr(pos+1).c_str());
				row++;
			}
		}
		return 0;
	}
	return 0;
}
int save_list(HWND hlistview,std::vector<generic_string> *slist)
{
	if(hlistview!=0 && slist!=0){
		int i,count;
		slist->clear();
		count=ListView_GetItemCount(hlistview);
		for(i=0;i<count;i++){
			TCHAR tmp[160],a[80],b[80];
			a[0]=0;
			ListView_GetItemText(hlistview,i,0,a,_countof(a));
			ListView_GetItemText(hlistview,i,1,b,_countof(b));
			wnsprintfW(tmp,_countof(tmp),TEXT("%s|%s"),a,b);
			tmp[_countof(tmp)-1]=0;
			slist->push_back(tmp);
		}
	}
	return 0;
}

static int selection=0;
static HWND ghlistview=0;
DLGPROC edit_entry(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int mode=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			int i,count;
			mode=(int)lparam;
			if(mode==IDC_ADD)
				SetWindowText(hwnd,TEXT("ADD"));
			else if(mode==IDC_EDIT)
				SetWindowText(hwnd,TEXT("EDIT"));
			count=ListView_GetItemCount(ghlistview);
			for(i=0;i<count;i++){
				int j,ctrl[]={IDC_FILEPATTERN,IDC_FILTERMASK};
				for(j=0;j<_countof(ctrl);j++){
					TCHAR tmp[80];
					tmp[0]=0;
					ListView_GetItemText(ghlistview,i,j,tmp,_countof(tmp));
					tmp[_countof(tmp)-1]=0;
					SendDlgItemMessage(hwnd,ctrl[j],CB_ADDSTRING,0,(LPARAM)tmp);
				}
			}
			if(mode==IDC_EDIT && selection>=0){
				int i,ctrl[]={IDC_FILEPATTERN,IDC_FILTERMASK};
				for(i=0;i<_countof(ctrl);i++){
					TCHAR tmp[80];
					tmp[0]=0;
					ListView_GetItemText(ghlistview,selection,i,tmp,_countof(tmp));
					tmp[_countof(tmp)-1]=0;
					SetDlgItemText(hwnd,ctrl[i],tmp);
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			{
			int end=FALSE;
			if(mode==IDC_ADD){
				int i,ctrl[]={IDC_FILEPATTERN,IDC_FILTERMASK};
				TCHAR tmp[2][80];
				end=TRUE;
				for(i=0;i<_countof(ctrl);i++){
					if(0==GetDlgItemText(hwnd,ctrl[i],tmp[i],_countof(tmp[i])))
						end=FALSE;
					tmp[i][_countof(tmp[i])-1]=0;
				}
				if(end){
					int index=ListView_GetItemCount(ghlistview);
					for(i=0;i<_countof(ctrl);i++)
						lv_insert_data(ghlistview,index,i,tmp[i]);
				}
			}
			else if(mode==IDC_EDIT){
				if(selection>=0){
					int i,ctrl[]={IDC_FILEPATTERN,IDC_FILTERMASK};
					TCHAR tmp[2][80];
					end=TRUE;
					for(i=0;i<_countof(ctrl);i++){
						if(0==GetDlgItemText(hwnd,ctrl[i],tmp[i],_countof(tmp[i])))
							end=FALSE;
						tmp[i][_countof(tmp[i])-1]=0;
					}
					if(end){
						for(i=0;i<_countof(ctrl);i++)
							ListView_SetItemText(ghlistview,selection,i,tmp[i]);
					}
				}
			}
			if(end)
				EndDialog(hwnd,1);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return FALSE;
}
DLGPROC FileFilterMask(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static std::vector<generic_string> *gslist=0;
	switch(msg){
	case WM_INITDIALOG:
		{
			gslist=(std::vector<generic_string> *)lparam;
			HWND hlistview=GetDlgItem(hwnd,IDC_FILTERLIST);
			if(hlistview!=0){
				const TCHAR *cols[]={TEXT("If file matches ..."),TEXT("Set mask to ...")};
				int i;
				ListView_SetExtendedListViewStyle(hlistview, 
					ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_GRIDLINES|LVS_EX_FULLROWSELECT);
				for(i=sizeof(cols)/sizeof(TCHAR*)-1;i>=0;i--){
					ListView_DeleteColumn(hlistview,i);
				}
				for(i=0;i<sizeof(cols)/sizeof(TCHAR*);i++){
					lv_add_column(hlistview,cols[i],i);
				}
				populate_listview(hlistview,gslist);
				//if(0)
				for(i=0;i<10;i++){
					for(int j=0;j<2;j++){
						TCHAR tmp[40]={0};
						wnsprintfW(tmp,_countof(tmp),TEXT("%04i"),i+j);
						lv_insert_data(hlistview,i,j,tmp);
					}
				}
				ghlistview=hlistview;
			}
			AnchorInit(hwnd,FileMaskAnchors,sizeof(FileMaskAnchors)/sizeof(struct CONTROL_ANCHOR));
			RestoreWinRelPosition(GetParent(hwnd),hwnd,&WinRelPos);
		}
		break;
	case WM_SIZE:
		AnchorResize(hwnd,FileMaskAnchors,sizeof(FileMaskAnchors)/sizeof(struct CONTROL_ANCHOR));
		break;
	case WM_NOTIFY:
		if(wparam==IDC_FILTERLIST){
			NMHDR *nmhdr=(NMHDR *)lparam;
			if(nmhdr!=0){
				switch(nmhdr->code){
				case NM_DBLCLK:
					{
						LV_HITTESTINFO ht={0};
						GetCursorPos(&ht.pt);
						MapWindowPoints(NULL,GetDlgItem(hwnd,IDC_FILTERLIST),&ht.pt,1);
						ListView_HitTest(GetDlgItem(hwnd,IDC_FILTERLIST),&ht);
						if(ht.flags==LVHT_NOWHERE)
							PostMessage(hwnd,WM_COMMAND,IDC_ADD,0);
						else
							PostMessage(hwnd,WM_COMMAND,IDC_EDIT,0);
					}
					break;
				case LVN_KEYDOWN:
					{
						LV_KEYDOWN *key=(LV_KEYDOWN *)lparam;
						switch(key->wVKey){
						case VK_F2:
							PostMessage(hwnd,WM_COMMAND,IDC_EDIT,0);
							break;
						case VK_INSERT:
							PostMessage(hwnd,WM_COMMAND,IDC_ADD,0);
							break;
						case VK_DELETE:
							PostMessage(hwnd,WM_COMMAND,IDC_DELETE,0);
							break;
						}
					}
					break;
				}
			}
		
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_ADD:
		case IDC_EDIT:
			{	
				selection=ListView_GetSelectionMark(GetDlgItem(hwnd,IDC_FILTERLIST));
				if(selection<0 && LOWORD(wparam)==IDC_EDIT)
					break;
				DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FILEMASK_ENTRY),hwnd,(DLGPROC)edit_entry,(LPARAM)LOWORD(wparam));
			}
			break;
		case IDC_DELETE:
			{
				HWND hlistview=GetDlgItem(hwnd,IDC_FILTERLIST);
				int i,count,selected;
				count=ListView_GetSelectedCount(hlistview);
				if(count>1){
					if(IDOK!=MessageBox(hwnd,TEXT("Are you sure you want to delete the selected items?"),TEXT("Warning"),MB_OKCANCEL|MB_SYSTEMMODAL))
						break;
				}
				selected=ListView_GetNextItem(hlistview,-1,LVIS_SELECTED);
				count=ListView_GetItemCount(hlistview);
				for(i=count-1;i>=0;i--){
					int state;
					state=ListView_GetItemState(hlistview,i,LVIS_SELECTED);
					if(state==LVIS_SELECTED)
						ListView_DeleteItem(hlistview,i);
				}
				if(selected>=0){
					count=ListView_GetItemCount(hlistview);
					if(selected>=count)
						selected=count-1;
					if(selected>=0)
						ListView_SetItemState(hlistview,selected,LVIS_SELECTED,LVIS_SELECTED);
				}
			}
			break;
		case IDCANCEL:
			SaveWinRelPosition(GetParent(hwnd),hwnd,&WinRelPos);
			EndDialog(hwnd,0);
			break;
		case IDOK:
			save_list(GetDlgItem(hwnd,IDC_FILTERLIST),gslist);
			SaveWinRelPosition(GetParent(hwnd),hwnd,&WinRelPos);
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return FALSE;
}