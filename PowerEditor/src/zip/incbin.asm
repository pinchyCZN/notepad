;vs2008:
;nasm -I "$(InputDir)\" -t -f  win32 -o "$(TargetDir)$(InputName).obj" -Xvc "$(InputDir)$(InputName).asm"
;vs2017:
;nasm -I "$(SolutionDir)..\src\zip\\" -t -f win32 -o "$(SolutionDir)$(Configuration)\incbin.obj" -Xvc "$(SolutionDir)..\src\zip\incbin.asm"
;need trailing slash on -I parameter to escape "\"
%warning make sure to zip the following files:
%warning config.xml    -> config.zip
%warning langs.xml     -> langs.zip
%warning shortcuts.xml -> shortcuts.zip
%warning stylers.xml   -> stylers.zip

SECTION .data


_zip_lang:
incbin "langs.zip"
_zip_lang_string:
db "langs",0
 
_zip_shortcuts:
incbin "shortcuts.zip"
_zip_shortcuts_string:
db "shortcuts",0

_zip_stylers:
incbin "stylers.zip"
_zip_stylers_string:
db "stylers",0
 
_zip_config:
incbin "config.zip"
_zip_config_string:
db "config",0


 global _zip_file_list
_zip_file_list:


dd _zip_lang
dd _zip_lang_string - _zip_lang
dd _zip_lang_string

dd _zip_shortcuts
dd _zip_shortcuts_string - _zip_shortcuts
dd _zip_shortcuts_string

dd _zip_stylers
dd _zip_stylers_string - _zip_stylers
dd _zip_stylers_string

dd _zip_config
dd _zip_config_string - _zip_config
dd _zip_config_string

dd 0,0,0,0,0,0