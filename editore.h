// funzioni della finestra Editore
int lunghezzaBOM(wchar_t*);

// legge tutto il file, scopre se è codificato ascii, ansi, utf8 o unicode, e lo mette nell'Editore
void apriFileEditore(HWND hWnd) {
	if (!fileModificato) {
		HANDLE file = CreateFile( nomeFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		DWORD pesoFile = GetFileSize(file, NULL);
		char test[pesoFile+1];
		DWORD caratteriLetti;
		ReadFile(file, test, pesoFile, &caratteriLetti, NULL);
		test[caratteriLetti] = 0;
		CloseHandle(file);
		wchar_t nomeFileW[MAX_PATH];
		MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, nomeFile, -1, nomeFileW, MAX_PATH);
		char* testo = test + lunghezzaBOM(nomeFileW);
		caratteriLetti -= lunghezzaBOM(nomeFileW);

		wchar_t testoW[caratteriLetti+1];
		testoW[0] = 0;
		int caratteriConvertiti;
		// unicode
		if ( IsTextUnicode(test, pesoFile, NULL) ) {
			HANDLE fileW = CreateFileW( nomeFileW, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			wchar_t testone[pesoFile+1];
			DWORD caratteriLettiW;
			ReadFile(fileW, testone, pesoFile, &caratteriLettiW, NULL);
			CloseHandle(fileW);
			testone[caratteriLettiW/2] = 0;
			wchar_t* testino = testone + lunghezzaBOM(nomeFileW)/2;
			SetWindowTextW( hEditore, testino );
			GlobalFree(testone);
			return;
		// asci/utf8
		} else if((caratteriConvertiti = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, testo, caratteriLetti, testoW, caratteriLetti+1))) {
			testoW[caratteriConvertiti] = 0;
		// ansi
		} else if((caratteriConvertiti = MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,testo, caratteriLetti, testoW, caratteriLetti+1))) {
			testoW[caratteriConvertiti] = 0;
		} else if(testo[0])
			MessageBox(hWnd, "Can't understand the file encoding.", "Encoding unknown", MB_ICONWARNING);
		
		SetWindowTextW( hEditore, testoW );
		GlobalFree(testo);
		GlobalFree(testoW);
	}
}

// scrive nella status bar il numero dei caratteri dell'Editore
void contaCaratteri() {
	int numCaratteri = GetWindowTextLength(hEditore);
	char caratteri[7];
	sprintf( caratteri, "\t%d", numCaratteri);
	HWND hStatus = GetDlgItem(GetParent(hEditore), ID_STATUSBAR);
	SendMessage(hStatus, SB_SETTEXT, SBT_NOBORDERS, (LPARAM)&caratteri);
}

#define MN_GETHMENU 0x01E1
void WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
	HMENU hMenu = SendMessage(hwnd, MN_GETHMENU, 0, 0);
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
			char nome[MAX_PATH]; 
			sprintf( nome, "*%s", strrchr(nomeFile,'\\')+1 );
			SetWindowText(GetParent(hEditor), nome);
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
