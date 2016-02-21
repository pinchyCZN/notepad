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

static std::vector<generic_string> *slist=0;
static int selection=0;
DLGPROC edit_entry(HWND hwnd,UINT msg,WPARAM wparam,LPARAM lparam)
{
	static int mode=0;
	switch(msg){
	case WM_INITDIALOG:
		mode=(int)lparam;
		if(slist!=0){
			if(selection>=0 && selection<(int)slist->size()){
				generic_string *str=&slist->at(selection);
				int pos=str->find(TEXT('|'));
				if(pos>0){
					
				}
			}
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDOK:
			if(mode==IDC_ADD){
				TCHAR a[80],b[80],s[160];
				GetDlgItemText(hwnd,IDC_FILEPATTERN,a,_countof(a));
				GetDlgItemText(hwnd,IDC_FILTERMASK,b,_countof(b));
				wnsprintfW(s,_countof(s),TEXT("%s|%s"),a,b);
				s[_countof(s)-1]=0;
				slist->push_back(s);
			}
			else if(mode==IDC_EDIT){
				if(selection>=0 && selection<(int)slist->size()){
					TCHAR a[80],b[80],s[160];
					generic_string *str=&slist->at(selection);
					GetDlgItemText(hwnd,IDC_FILEPATTERN,a,_countof(a));
					GetDlgItemText(hwnd,IDC_FILTERMASK,b,_countof(b));
					wnsprintfW(s,_countof(s),TEXT("%s|%s"),a,b);
					s[_countof(s)-1]=0;
					*str=s;
				}
			}
			EndDialog(hwnd,1);
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

	switch(msg){
	case WM_INITDIALOG:
		{
			slist=(std::vector<generic_string> *)lparam;
			HWND hlistview=GetDlgItem(hwnd,IDC_FILTERLIST);
			if(hlistview!=0){
				const TCHAR *cols[]={TEXT("If file matches ..."),TEXT("Set mask to ...")};
				int i,style=WS_TABSTOP|WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS;
				SetWindowLong(hlistview,GWL_STYLE,style);
				ListView_SetExtendedListViewStyle(hlistview,
					ListView_GetExtendedListViewStyle(hlistview)|LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES);
				for(i=0;i<sizeof(cols)/sizeof(TCHAR*);i++){
					lv_add_column(hlistview,cols[i],i);
				}
				populate_listview(hlistview,slist);
			}
			{
				HWND hgrippy=GetDlgItem(hwnd,IDC_GRIPPER);
				if(hgrippy){
					SetWindowLong(hlistview,GWL_STYLE,WS_CHILD|WS_VISIBLE|SBS_SIZEGRIP);
				}
			}
			AnchorInit(hwnd,FileMaskAnchors,sizeof(FileMaskAnchors)/sizeof(struct CONTROL_ANCHOR));
			RestoreWinRelPosition(GetParent(hwnd),hwnd,&WinRelPos);
		}
		break;
	case WM_SIZE:
		AnchorResize(hwnd,FileMaskAnchors,sizeof(FileMaskAnchors)/sizeof(struct CONTROL_ANCHOR));
		break;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
		case IDC_ADD:
		case IDC_EDIT:
			{
				int r=DialogBoxParam(ghinstance,MAKEINTRESOURCE(IDD_FILEMASK_ENTRY),hwnd,(DLGPROC)edit_entry,(LPARAM)LOWORD(wparam));
				if(r>0)
					populate_listview(GetDlgItem(hwnd,IDC_FILTERLIST),slist);
			}
			break;
		case IDC_DELETE:
			break;
		case IDCANCEL:
			SaveWinRelPosition(GetParent(hwnd),hwnd,&WinRelPos);
			EndDialog(hwnd,0);
			break;
		case IDOK:
			save_list(GetDlgItem(hwnd,IDC_FILTERLIST),slist);
			SaveWinRelPosition(GetParent(hwnd),hwnd,&WinRelPos);
			EndDialog(hwnd,0);
			break;
		}
		break;
	}
	return FALSE;
}