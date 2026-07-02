#ifndef MODAL_OVERLAY_DLG_H
#define MODAL_OVERLAY_DLG_H

#include <vector>

#define MODAL_OVERLAY_DLG_FILE_DELETED  1

// Returns 1 for Yes, 0 for No or Escape.
// type: e.g. MODAL_OVERLAY_DLG_FILE_DELETED
// pszContext: for type 1, full path of the deleted file
int modalDlg(HWND hParent, int type, const TCHAR* pszContext);

// Files dropped on the overlay/panel during the last modalDlg call.
void modalOverlayDlg_clearPendingDrops();
size_t modalOverlayDlg_getPendingDropCount();
const TCHAR* modalOverlayDlg_getPendingDropPath(size_t index);

// Send NPPM_DOOPEN to hParent for each queued path, then clear the queue.
void modalOverlayDlg_dispatchPendingDrops(HWND hParent);

#endif // MODAL_OVERLAY_DLG_H
