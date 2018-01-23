// funzioni del Frontalino

#include "macro.h"

// creazione Frontalino
void creaFrontalino() {
	hFrontalino = CreateWindow( "Static", NULL, WS_CHILD|WS_VISIBLE|WS_BORDER,
								0, 0, 100, 100, hWindow, (HMENU)ID_STATICO, GetModuleHandle(NULL), NULL );
	SubclassWindow( hFrontalino, proceduraFrontalino );
	CreateWindow( "Button", "Create a new keyboard", WS_VISIBLE | WS_CHILD | WS_TABSTOP, 
				70, 20, 160, 35, hFrontalino, (HMENU)ID_FRONTE_NUOVO, GetModuleHandle(NULL), NULL );
	CreateWindow( "Static", "Open preset keyboard", WS_CHILD|WS_VISIBLE,
				20, 70, 200, 20, hFrontalino, (HMENU)ID_STATICO, GetModuleHandle(NULL), NULL );
	HWND hNienteCartella = CreateWindowW( L"Edit", L"", WS_CHILD | ES_MULTILINE, 20, 70, 260, 80,
										hFrontalino, (HMENU)ID_STATICO, GetModuleHandle(NULL), NULL );
	// listbox per ospitare gli esempi di tastiere
	HWND hListaTastiere = CreateWindowW( L"COMBOBOX", L"", 
						WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT | CBS_OWNERDRAWFIXED |	WS_VSCROLL,
						20, 90, 260, 400, hFrontalino, (HMENU)ID_FRONTE_ESEMPI, GetModuleHandle(NULL), NULL );
	settaFontSistema(hFrontalino);
	wchar_t percorsoTastiere[MAX_PATH];
	snwprintf( percorsoTastiere, MAX_PATH, L"%s\\keyboards\\*", appData );
	// apre i file presenti in AppData\Roaming\Uosk\keyboards e popola l'array di esemplari
	WIN32_FIND_DATAW ffd;
	HANDLE hFind;
	esemplari = malloc( 0 );
	FILE *file;
	char esempio[84];
	int i = 0;
	if( (hFind=FindFirstFileW(percorsoTastiere,&ffd)) != INVALID_HANDLE_VALUE ) {
		do {
			if( ~ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
				esemplari = realloc( esemplari, sizeof *esemplari * (i+1) );
				snwprintf( esemplari[i].indirizzo, MAX_PATH, L"%s\\keyboards\\%s", appData, ffd.cFileName );
				file = _wfopen( esemplari[i].indirizzo, L"rb" );
				fseek(file, lunghezzaBOM(esemplari[i].indirizzo), SEEK_SET);
				int caratteriLetti = fread( esempio, sizeof(char), conta(esempio), file );
				esempio[caratteriLetti] = 0;
				for( int c=0; c<caratteriLetti; c++ ) {
					if( esempio[c] == '\r' )
						esempio[c] = ' ';
				}
				MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, esempio, -1, esemplari[i].testo, conta(esemplari[i].testo) );
				wcscpy( esemplari[i].nome, ffd.cFileName);
				*wcsrchr( esemplari[i].nome, L'.' ) = 0;
				SendMessageW( hListaTastiere, CB_ADDSTRING, 0, (LPARAM)esemplari[i].nome );
				fclose(file);
				i++;
			}
		} while( FindNextFileW(hFind,&ffd) );
		if( i )
			SendMessage( hListaTastiere, CB_SETCURSEL, 0, 0 );
		else {	// la cartella keyboards è vuota
			ShowWindow( hListaTastiere, 0 );
			ShowWindow( hNienteCartella, 1 );
			wchar_t disdetta[MAX_PATH+25];
			snwprintf( disdetta, conta(disdetta), L"Can't find any file in\r\n%s\\keyboards", appData );
			SetWindowTextW( hNienteCartella, disdetta );
		}
	} else {	// la cartella keyboards non esiste
		ShowWindow( hListaTastiere, 0 );
		ShowWindow( hNienteCartella, 1 );
		wchar_t purtroppo[MAX_PATH+20];
		snwprintf( purtroppo, conta(purtroppo), L"Can't find the directory\r\n%s\\keyboards", appData );
		SetWindowTextW( hNienteCartella, purtroppo );
	}
	FindClose(hFind);
}

// gestione messaggi del Frontalino
LRESULT CALLBACK proceduraFrontalino( HWND hFront, UINT message, WPARAM wParam, LPARAM lParam ) {
	switch( message ) {
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hFront, &ps );
			FillRect( hdc, &ps.rcPaint, (HBRUSH)(COLOR_3DFACE+1) );
			EndPaint( hFront, &ps );
			break;
		}
		case WM_MEASUREITEM: {	// modifica le dimensioni della listbox e degli elementi della lista
			MEASUREITEMSTRUCT *mis = (MEASUREITEMSTRUCT *)lParam;
			mis->itemHeight = 50;
			break;
		}
		case WM_DRAWITEM: {	// scrive il contenuto di ciascun elemento della lista di esempi
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			SetTextColor( dis->hDC, GetSysColor(dis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT) );
			SetBkColor( dis->hDC, GetSysColor(dis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_WINDOW) );
			// linea separatoria tra elementi della lista
			if( ~dis->itemState & ODS_COMBOBOXEDIT ) {
				HPEN hPenna = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW) );
				SelectObject( dis->hDC, hPenna );
				dis->rcItem.bottom--;
				MoveToEx( dis->hDC, dis->rcItem.left, dis->rcItem.bottom, NULL );
				LineTo( dis->hDC, dis->rcItem.right, dis->rcItem.bottom );
				DeleteObject( hPenna );
			}
			// scrive le due linee di testo
			HFONT hFont = settaFontSistema(0);
			SelectObject( dis->hDC, hFont );
			int x = LOWORD( GetDialogBaseUnits() );
			int y = dis->rcItem.top + LOWORD(GetDialogBaseUnits())/2;
			ExtTextOutW( dis->hDC, x, y, ETO_CLIPPED | ETO_OPAQUE, &dis->rcItem,
						 esemplari[dis->itemID].nome, wcslen(esemplari[dis->itemID].nome), NULL );
			caricaFont(L"keyboard");
			lf.lfHeight = -20;
			hFont = CreateFontIndirectW(&lf);
			SelectObject( dis->hDC, hFont );
			dis->rcItem.left += LOWORD( GetDialogBaseUnits() );
			dis->rcItem.top += 20;
			DrawTextW( dis->hDC, esemplari[dis->itemID].testo, wcslen(esemplari[dis->itemID].testo), 
						&(dis->rcItem), DT_SINGLELINE | DT_NOPREFIX );
			DeleteObject( hFont );
			return TRUE;
		}
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case ID_FRONTE_NUOVO:
					SendMessage( hWindow, WM_COMMAND, MENU_FILE_NUOVO, 0 );
					break;
				case ID_FRONTE_ESEMPI:
					if( HIWORD(wParam) == CBN_SELCHANGE ) {
						int idElemento = SendMessage( (HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0 );
						wcscpy( fileDaAprire, esemplari[idElemento].indirizzo );
						apriFile();
					}
					break;
			}
		default: return DefWindowProc( hFront, message, wParam, lParam );
	}
	return 0;
}