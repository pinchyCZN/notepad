extern "C" {
int move_console();
void open_console();
void hide_console();
void print_msg(UINT msg,WPARAM wparam,LPARAM lparam);
void print_sci_msg(UINT msg,WPARAM wparam,LPARAM lparam);
int print_last_error();
void log_print_msg(const char *fname,UINT msg,WPARAM wparam,LPARAM lparam);
void log_msg(const char *fname,const char *fmt,...);
}
