// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef MODAL_OVERLAY_DLG_H
#define MODAL_OVERLAY_DLG_H

// Returns 1 for Yes, 0 for No or Escape.
// type: reserved for future dialog kinds (e.g. file deleted).
int modalDlg(HWND hParent, int type);

#endif // MODAL_OVERLAY_DLG_H
