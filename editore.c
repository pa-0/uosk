// funzioni della finestra Editore
#include "macro.h"

// scrive nella status bar il numero dei caratteri dell'Editore
void contaCaratteri() {
	int numCaratteri = GetWindowTextLength(hEditore);
	char caratteri[7];
	sprintf( caratteri, "\t%d", numCaratteri);
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)&caratteri);
}

#define MN_GETHMENU 0x01E1
void WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	HMENU hMenu = (HMENU)SendMessage(hwnd, MN_GETHMENU, 0, 0);
	InsertMenu (hMenu, 0, MF_BYPOSITION, WM_USER, "Snippet Info");
	InsertMenu (hMenu, 1, MF_BYPOSITION|MF_CHECKED, WM_USER+1, "Edit");
	return;
}

// procedura Editore
LRESULT CALLBACK ProceduraEditore(HWND hEditor, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (message) {
		case WM_KEYDOWN:
			if ( wParam != VK_DELETE ) {
				DefSubclassProc(hEditor, message, wParam, lParam);
				break;
			}
		case WM_CHAR:
			if ( wParam == VK_ESCAPE  || wParam == VK_CANCEL) {
				DefSubclassProc(hEditor, message, wParam, lParam);
				break;
			}
			if( wParam == 1 ) {	// selezione tutto il testo con Ctrl+A
				SendMessage( hEditor, EM_SETSEL, 0, -1 ); 
				break;
			}
		case WM_UNDO:
		case WM_CUT:
		case WM_PASTE:
		case WM_CLEAR:
			fileModificato = 1;
			if( wcscmp(nomeFile,L"untitled") ) {
				wchar_t nome[MAX_PATH];
				snwprintf( nome, conta(nome), L"*%s", wcsrchr(nomeFile,'\\')+1 );
				SetWindowTextW( GetParent(hEditor), nome );
			}
			EnableMenuItem(GetMenu(GetParent(hEditor)), MENU_FILE_SALVA, MF_ENABLED);
			DefSubclassProc(hEditor, message, wParam, lParam);
			contaCaratteri();
			break;
		case WM_CONTEXTMENU: {	// apre il menu contestuale
			HWINEVENTHOOK hEventHook = SetWinEventHook( EVENT_SYSTEM_MENUPOPUPSTART, EVENT_SYSTEM_MENUPOPUPSTART, 0,
			                                            WinEventProc, GetCurrentProcessId(), GetCurrentThreadId(), 0);
			DefSubclassProc(hEditor, WM_CONTEXTMENU, wParam, lParam);
			UnhookWinEvent(hEventHook);
			if( GetWindowLong(hEditor,GWL_EXSTYLE) & WS_EX_RTLREADING )
				das = MF_UNCHECKED;
			else das = MF_CHECKED;
			SendMessage (hWindow, WM_COMMAND, MENU_LETTURA_DAS, 0);
			break;
		}
		case WM_USER:
			SendMessage (hWindow, WM_COMMAND, MENU_INFO_SNIPPET, 0);
			break;
		case WM_USER+1:
			SendMessage (hWindow, WM_COMMAND, MENU_FILE_EDITA, 0);
			break;

		default:
			return DefSubclassProc(hEditor, message, wParam, lParam);	// editore subclassato con SetWindowSubclass()
	}
	return 0;
}
