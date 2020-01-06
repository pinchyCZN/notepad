;nasm -I "$(InputDir)\" -t -f  win32 -o "$(TargetDir)$(InputName).obj" -Xvc "$(InputDir)$(InputName).asm"
;need trailing slash on -I parameter to escape "\"

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
 

 



global _zip_file_list
_zip_font_list:

dd _zip_lang
dd _zip_lang_string - _zip_lang
dd _zip_lang_string

dd _zip_shortcuts
dd _zip_shortcuts_string - _zip_shortcuts
dd _zip_shortcuts_string

dd _zip_stylers
dd _zip_stylers_string - _zip_stylers
dd _zip_stylers_string

dd 0,0,0,0,0,0