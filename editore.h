// funzioni della finestra Editore

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
	InsertMenu( hMenu, 0, MF_BYPOSITION | MF_CHECKED, MENU_FILE_EDITA, "Edit");
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
			if ( wParam == VK_ESCAPE ) {
				DefSubclassProc(hEditor, message, wParam, lParam);
				break;
			}
		case WM_UNDO:
		case WM_CUT:
		case WM_PASTE:
		case WM_CLEAR:
			fileModificato = 1;
			if( strcmp(nomeFile,"untitled") ) {
				char nome[MAX_PATH]; 
				sprintf( nome, "*%s", strrchr(nomeFile,'\\')+1 );
				SetWindowText(GetParent(hEditor), nome);
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
			break;
		}
		case MENU_FILE_EDITA:
			SendMessage(GetParent(hEditor), WM_COMMAND, MENU_FILE_EDITA, 0);
			break;
		default:
			return DefSubclassProc(hEditor, message, wParam, lParam);	// editore subclassato con SetWindowSubclass()
	}
	return 0;
}
