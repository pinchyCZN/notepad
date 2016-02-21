DLGPROC FileFilterMask(HWND,UINT,WPARAM,LPARAM);
int filter_set_hinstance(HINSTANCE);
int get_filemask_match(const TCHAR *,std::vector<generic_string> *,generic_string *);