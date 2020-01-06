int read_zip_file(void *buf,int buf_len,void **out,int *out_len);

int read_zip_file_name(const char *name,void **out,int *out_len);

int extract_zip_file(const WCHAR *dest,const char *name);