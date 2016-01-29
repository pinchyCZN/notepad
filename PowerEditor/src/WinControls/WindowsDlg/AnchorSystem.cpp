#include "precompiledHeaders.h"
#include "AnchorSystem.h"

int AnchorInit(HWND hparent,struct CONTROL_ANCHOR *clist,int clist_len)
{
	int i;
	RECT rparent={0};
	GetClientRect(hparent,&rparent);
	for(i=0;i<clist_len;i++){
		HWND hctrl;
		struct CONTROL_ANCHOR *anchor;
		anchor=&clist[i];
		anchor->rect_parent=rparent;
		hctrl=GetDlgItem(hparent,anchor->ctrl_id);
		if(hctrl){
			RECT rctrl={0};
			GetWindowRect(hctrl,&rctrl);
			MapWindowPoints(NULL,hparent,(LPPOINT)&rctrl,2);
			anchor->rect_ctrl=rctrl;
		}
		anchor->initialized=1;
	}
	return 0;
}

int AnchorResize(HWND hparent,struct CONTROL_ANCHOR *clist,int clist_len)
{
	int i;
	RECT rparent={0};
	GetClientRect(hparent,&rparent);
	for(i=0;i<clist_len;i++){
		HWND hctrl;
		struct CONTROL_ANCHOR *anchor;
		anchor=&clist[i];
		if(!anchor->initialized)
			continue;
		hctrl=GetDlgItem(hparent,anchor->ctrl_id);
		if(hctrl){
			int x=0,y=0,cx=0,cy=0,delta;
			int flags=0;
			switch(anchor->anchor_mask){
			case ANCHOR_RIGHT|ANCHOR_TOP:
			case ANCHOR_RIGHT:
				{
					delta=anchor->rect_parent.right-anchor->rect_ctrl.left;
					x=rparent.right-delta;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					flags=SWP_NOSIZE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_BOTTOM:
			case ANCHOR_BOTTOM:
				{
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					y=rparent.bottom-delta;
					flags=SWP_NOSIZE;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP|ANCHOR_BOTTOM:
				{
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					delta=anchor->rect_parent.right -  anchor->rect_ctrl.right;
					cx=(rparent.right-rparent.left) - delta - x;
					delta=anchor->rect_parent.bottom -  anchor->rect_ctrl.bottom;
					cy=(rparent.bottom-rparent.top) - delta - y;
					flags=SWP_SHOWWINDOW;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_TOP:
			case ANCHOR_LEFT|ANCHOR_RIGHT:
				{
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					y=anchor->rect_ctrl.top-anchor->rect_parent.top;
					delta=anchor->rect_parent.right -  anchor->rect_ctrl.right;
					cx=(rparent.right-rparent.left) - delta - x;
					cy=anchor->rect_ctrl.bottom-anchor->rect_ctrl.top;
					flags=SWP_SHOWWINDOW;
				}
				break;
			case ANCHOR_LEFT|ANCHOR_RIGHT|ANCHOR_BOTTOM:
				{
					x=anchor->rect_ctrl.left-anchor->rect_parent.left;
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					y=rparent.bottom-delta;
					delta=anchor->rect_parent.right -  anchor->rect_ctrl.right;
					cx=(rparent.right-rparent.left) - delta - x;
					cy=anchor->rect_ctrl.bottom-anchor->rect_ctrl.top;
					flags=SWP_SHOWWINDOW;
				}
				break;
			case ANCHOR_RIGHT|ANCHOR_BOTTOM:
				{
					delta=anchor->rect_parent.right-anchor->rect_ctrl.left;
					x=rparent.right-delta;
					delta=anchor->rect_parent.bottom-anchor->rect_ctrl.top;
					y=rparent.bottom-delta;
					flags=SWP_NOSIZE;
				}
				break;
			}
			if(flags){
				flags|=SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOREPOSITION;
				SetWindowPos(hctrl,NULL,x,y,cx,cy,flags);
			}
		}
	}
	return 0;
}

int SaveWinRelPosition(HWND hparent,HWND hwin,struct WIN_REL_POS *relpos)
{
	int result=FALSE;
	WINDOWPLACEMENT wp={0};
	wp.length=sizeof(WINDOWPLACEMENT);
	if(GetWindowPlacement(hparent,&wp)){
		relpos->rparent=wp.rcNormalPosition;
		if(GetWindowPlacement(hwin,&wp)){
			relpos->rwin=wp.rcNormalPosition;
			result=TRUE;
		}
	}
	relpos->initialized=result;
	return 0;
}
int RestoreWinRelPosition(HWND hparent,HWND hwin,struct WIN_REL_POS *relpos)
{
	//clamp window to nearest monitor
	if(relpos->initialized){
		RECT rparent;
		if(GetWindowRect(hparent,&rparent)){
			HMONITOR hmon;
			RECT rwin;
			int x,y,cx,cy;
			x=relpos->rwin.left-relpos->rparent.left;
			y=relpos->rwin.top-relpos->rparent.top;
			cx=relpos->rwin.right-relpos->rwin.left;
			cy=relpos->rwin.bottom-relpos->rwin.top;
			rwin.left=rparent.left+x;
			rwin.top=rparent.top+y;
			rwin.right=rwin.left+cx;
			rwin.bottom=rwin.top+cy;
			hmon=MonitorFromRect(&rwin,MONITOR_DEFAULTTONEAREST);
			if(hmon){
				MONITORINFO mi;
				mi.cbSize=sizeof(mi);
				if(GetMonitorInfo(hmon,&mi)){
					RECT rmon;
					rmon=mi.rcWork;
					x=rwin.left;
					y=rwin.top;
					if(x<rmon.left)
						x=rmon.left;
					if(y<rmon.top)
						y=rmon.top;
					if(cx>(rmon.right-rmon.left))
						cx=rmon.right-rmon.left;
					if(cy>(rmon.bottom-rmon.top))
						cy=rmon.bottom-rmon.top;
					if((x+cx)>rmon.right)
						x=rmon.right-cx;
					if((y+cy)>rmon.bottom)
						y=rmon.bottom-cy;
					SetWindowPos(hwin,NULL,x,y,cx,cy,
						SWP_SHOWWINDOW|SWP_NOZORDER);
				}
			}
			
		}
	}
	return 0;
}

int SnapWindow(HWND hwnd,RECT *rect)
{
	if(hwnd && rect){
		HMONITOR hmon;
		MONITORINFO mi;
		hmon=MonitorFromRect(rect,MONITOR_DEFAULTTONEAREST);
		mi.cbSize=sizeof(mi);
		if(GetMonitorInfo(hmon,&mi)){
			long d_top,d_bottom,d_left,d_right;
			d_right=mi.rcWork.right-rect->right;
			if(d_right<=8 && d_right>=-4){
				rect->right=mi.rcWork.right;
				rect->left+=d_right;
			}
			d_left=rect->left-mi.rcWork.left;
			if(d_left<=8 && d_left>=-4){
				rect->left=mi.rcWork.left;
				rect->right-=d_left;
			}
			d_top=rect->top-mi.rcWork.top;
			if(d_top<=8 && d_top>=-4){
				rect->top=mi.rcWork.top;
				rect->bottom-=d_top;
			}
			d_bottom=mi.rcWork.bottom-rect->bottom;
			if(d_bottom<=8 && d_bottom>=-4){
				rect->bottom=mi.rcWork.bottom;
				rect->top+=d_bottom;
			}
		}
	}
	return 0;
}

int SnapSizing(HWND hwnd,RECT *rect,int side)
{
	int result=FALSE;
	if(hwnd && rect){
		HMONITOR hmon;
		MONITORINFO mi;
		hmon=MonitorFromRect(rect,MONITOR_DEFAULTTONEAREST);
		mi.cbSize=sizeof(mi);
		if(GetMonitorInfo(hmon,&mi)){
			RECT *rwork=&mi.rcWork;
			const int snap_size=10;
			if(side==WMSZ_TOP || side==WMSZ_TOPLEFT || side==WMSZ_TOPRIGHT){
				if(abs(rect->top - rwork->top)<snap_size){
					rect->top=rwork->top;
					result=TRUE;
				}
			}
			if(side==WMSZ_BOTTOM || side==WMSZ_BOTTOMLEFT || side==WMSZ_BOTTOMRIGHT){
				if(abs(rect->bottom - rwork->bottom)<snap_size){
					rect->bottom=rwork->bottom;
					result=TRUE;
				}
			}
			if(side==WMSZ_LEFT || side==WMSZ_TOPLEFT || side==WMSZ_BOTTOMLEFT){
				if(abs(rect->left - rwork->left)<snap_size){
					rect->left=rwork->left;
					result=TRUE;
				}
			}
			if(side==WMSZ_RIGHT || side==WMSZ_TOPRIGHT || side==WMSZ_BOTTOMRIGHT){
				if(abs(rect->right - rwork->right)<snap_size){
					rect->right=rwork->right;
					result=TRUE;
				}
			}
		}
	}
	return result;
}