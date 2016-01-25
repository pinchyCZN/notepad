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
			int x,y,cx,cy,delta;
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
	if(GetWindowRect(hparent,&relpos->rparent)){
		if(GetWindowRect(hwin,&relpos->rwin)){
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