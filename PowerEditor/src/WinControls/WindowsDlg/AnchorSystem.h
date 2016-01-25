#define ANCHOR_LEFT 1
#define ANCHOR_RIGHT 2
#define ANCHOR_TOP 4
#define ANCHOR_BOTTOM 8

struct CONTROL_ANCHOR{
	int ctrl_id;
	int anchor_mask;
	RECT rect_ctrl,rect_parent;
	int initialized;
};

int AnchorInit(HWND hparent,struct CONTROL_ANCHOR *clist,int clist_len);
int AnchorResize(HWND hparent,struct CONTROL_ANCHOR *clist,int clist_len);

struct WIN_REL_POS{
	RECT rparent,rwin;
	int initialized;
};

int SaveWinRelPosition(HWND hparent,HWND hwin,struct WIN_REL_POS *relpos);
int RestoreWinRelPosition(HWND hparent,HWND hwin,struct WIN_REL_POS *relpos);