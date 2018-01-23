/* Uosk (Unicode On-Screen Keyboard)
 * Open-source virtual keyboard: insert text snippets into any Windows™ program	*/

#include "macro.h"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
		case WM_CREATE: {	// Creazione degli elementi nella finestra principale
			hWindow = hWnd;

			// inizializza le scrollinfo
			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si2 = si;

			// creazione status bar
			hStatusBar = CreateWindow( STATUSCLASSNAME, NULL, WS_CHILD, 0,0,0,0, hWnd, 0, GetModuleHandle(0), 0 );
			// calcola altezza barra di stato
			RECT rcStatus;
			GetWindowRect( hStatusBar, &rcStatus );
			size.statusAlta = rcStatus.bottom - rcStatus.top;
			// mostra la status bar secondo quanto memorizzato in congin.ini
			if( (okMostraStatus = GetPrivateProfileIntW(L"options",L"statusbar",0,config_ini)) )
				ShowWindow( hStatusBar, SW_SHOW );

			creaFrontalino();

			// Classe finestra Tastiera che ospita i bottoni
			WNDCLASS clsPls = {0};
			clsPls.lpfnWndProc = proceduraTastiera;
			clsPls.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
			clsPls.lpszClassName = "ClasseTastiera";
			clsPls.hCursor = LoadCursor(NULL, IDC_ARROW);
			RegisterClass(&clsPls);
			// creazione finestra Tastiera
			hTastiera = CreateWindow( clsPls.lpszClassName, "", WS_CHILD, 0, 0, 100, 100, hWnd, 0, GetModuleHandle(0), 0 );

			// creazione dell'Editore di testo
			hEditore = CreateWindowW( L"Edit", NULL, WS_CHILD|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL, 
									  0, 0, 200, 100, hWnd, 0, GetModuleHandle(0), 0 );
			if (hEditore)
				SetWindowSubclass(hEditore, ProceduraEditore, 0, 0);
			caricaFont(L"editor");
			HFONT hFontEditore = CreateFontIndirectW(&lf);
			SendMessage(hEditore, WM_SETFONT, (WPARAM)hFontEditore, 0);

			// ToolTip della finestra principale
			hToolTip = CreateWindow( TOOLTIPS_CLASS, 0, WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, 0, 0, 0);
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = hWnd;
			ti.uId = (UINT)ID_FINESTRA_CLIENTE;
			ti.lpszText = "Double click to open a file";
			SendMessage( hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti );

			// ToolTip dei bottoni a destra del menu
			ti.uId = MENU_BOTTONE_CHIUDI;
			SendMessage( hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti );
			ti.uId = MENU_BOTTONE_EDITA;
			SendMessage( hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti );
			ti.uId = MENU_BOTTONE_RIDUCI;
			SendMessage( hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti );

			// modifica il menu
			okCopiaAppunti = GetPrivateProfileIntW( L"options", L"clipboard", 0, config_ini );
			CheckMenuItem( GetMenu(hWnd), MENU_COPIA_APPUNTI, okCopiaAppunti );
			CheckMenuItem( GetMenu(hWnd), MENU_MOSTRA_STATUS, okMostraStatus );
			okMosaico = GetPrivateProfileIntW( L"options", L"nocutter", 0, config_ini );
			wchar_t nomePath[8];
			wchar_t fileRecente[MAX_PATH];
			HMENU hSubMenu = GetSubMenu( GetSubMenu(GetMenu(hWnd),0), 2 );
			for( int i=0; i<5; i++ ) {
				snwprintf( nomePath, conta(nomePath), L"recent%d", i );
				GetPrivateProfileStringW( L"file", nomePath, L"", fileRecente, MAX_PATH, config_ini);
				if( fileRecente[0] )
					InsertMenuW( hSubMenu, -1, MF_BYPOSITION|MF_STRING, MENU_FILE_RECENTI+i, fileRecente);
			}
			RemoveMenu( hSubMenu, 0, MF_BYPOSITION );
			break;
		}
		case WM_SIZE:	// ridimensiona i comandi nella finestra principale
			if( wParam != SIZE_MINIMIZED ) {

				// scopre dimensioni finestra client
				RECT rect;
				GetClientRect(hWnd,&rect);
				size.clienteLargo = rect.right;

				// ridimensiona barra di stato
				int sezioni[3];
				sezioni[0] = 45;
				sezioni[1] = rect.right/3*2;
				size.statusSecondoLargo = sezioni[1] - sezioni[0];
				sezioni[2] = -1;
				SendMessage(hStatusBar, SB_SETPARTS, sizeof(sezioni)/sizeof(int), (LPARAM)sezioni);
				SendMessage(hStatusBar, WM_SIZE, 0, 0);
				scriviNomeFile();

				// aggiorna ScrollInfo
				int posVertIniz = si.nPos;
				si.nPage = rect.bottom;
				if(okMostraStatus)
					si.nPage -= size.statusAlta;
				if( (si.nMax-si.nPos) < si.nPage ) 
					si.nPos = si.nMax - si.nPage + 1;
				if( si.nPos < 0 )
					si.nPos = 0;
				SetScrollInfo( hTastiera, SB_VERT, &si, TRUE );
				ScrollWindow( hTastiera, 0, posVertIniz-si.nPos, NULL, NULL );

				// ridimensiona le finestre
				int yFronte = si.nPage>170 ? (si.nPage-170)/2 : 0;
				MoveWindow( hFrontalino, (rect.right-300)/2, yFronte, 300, 170, 1 );
				MoveWindow( hTastiera, 0, 0, rect.right, si.nPage, 1 );
				MoveWindow( hEditore, 0, 0, rect.right, si.nPage, 1 );

				// aggiorna le coordiante dei tooltip
				ti.uId = ID_FINESTRA_CLIENTE;
				GetClientRect (hWnd, &ti.rect);
				ti.rect.bottom -= size.statusAlta;
				SendMessage( hToolTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti );
				for( int i=0; i<3; i++ ) {
					ti.uId = MENU_BOTTONE_CHIUDI + i;
					int pos = 0;
					while( GetMenuItemID(GetMenu(hWindow),pos)!=MENU_BOTTONE_CHIUDI+i && pos<6 )
						pos++;
					GetMenuItemRect( hWnd, GetMenu(hWnd), pos, &ti.rect );
					MapWindowPoints( NULL, hWnd, (POINT*)&ti.rect, 2 );
					SendMessage( hToolTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti );
				}
			}
			veroResize = 1;
			if (wParam==SIZE_MAXIMIZED)	{
				disponiBottoni();
				massimizzata = 1;
				bottoniDestra();
			}
			if ( wParam==SIZE_RESTORED && massimizzata ) {
				disponiBottoni();
				massimizzata = 0;
				veroResize = 0;
				bottoniDestra();
			}
			break;
		case WM_GETICON:	// solo per azzerare 'veroResize' dopo il primo WM_SIZE
			veroResize = 0;
			break;
		case WM_EXITSIZEMOVE:
			if(veroResize) {
				disponiBottoni();
				veroResize = 0;
			}
			break;
		case WM_LBUTTONDBLCLK:
			finestraDialogoApri();
			break;
		case WM_MOUSEWHEEL:	// rotellina mouse e touchpad sui bottoni
			if( si.nMax >= si.nPage ) {
				int posVertIniz = si.nPos;
				si.nPos -= GET_WHEEL_DELTA_WPARAM(wParam) / 3;
				SetScrollInfo (hTastiera, SB_VERT, &si, TRUE);
				GetScrollInfo (hTastiera, SB_VERT, &si);
				ScrollWindow(hTastiera, 0, posVertIniz-si.nPos, NULL, NULL );
			}
			break;
		case WM_SETCURSOR: // quando cursore va sopra uno dei bottoni dà alla finestra stile che non viene attivata
			if ( GetWindowLong((HWND)wParam, GWL_ID) == ID_BOTTONE_SNIPPET )
				SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_NOACTIVATE|WS_EX_ACCEPTFILES );
			else
				SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES );
			int ritorno = DefWindowProc(hWnd, message, wParam, lParam);
			return ritorno;
		case WM_DROPFILES:
			if (!preservaModifiche())
				break;
			DragQueryFileW((HANDLE) wParam, 0, fileDaAprire, sizeof(fileDaAprire));
			apriFile();
			DragFinish((HANDLE)wParam);
			break;
		case WM_NCHITTEST: {	// finestra draggabile dal menu
			LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
			RECT rect1;
			GetMenuItemRect(hWnd, GetMenu(hWnd), 2, &rect1);
			RECT rect2;
			GetMenuItemRect(hWnd, GetMenu(hWnd), 3, &rect2);
			if ( hit==HTMENU && GET_X_LPARAM(lParam)>rect1.right && GET_X_LPARAM(lParam)<rect2.left )
				hit = HTCAPTION;
			return hit;
		}
		case WM_COMMAND:	// i comandi alla finestra principale, in particolare dal menu principale
			switch (LOWORD(wParam)) {
				case MENU_FILE_NUOVO:
					if(chiudiFile()==0)
						break;
					wcscpy(nomeFile,L"untitled");
					okEdita = 1;
					ShowWindow( hFrontalino, SW_HIDE );
					ShowWindow( hEditore,SW_SHOW );
					SetWindowText(hEditore, "");
					SetFocus(hEditore);
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_ENABLED);
					CheckMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_CHECKED );
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVACOME, MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_CHIUDI, MF_ENABLED);
					bottoniDestra();
					SetWindowPos(hWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
					contaCaratteri();
					SendMessage(hStatusBar, SB_SETTEXTW, 1, (LPARAM)nomeFile);
					break;
				case MENU_FILE_APRI:
					finestraDialogoApri();
					break;
				case MENU_FILE_RECENTI:
				case MENU_FILE_RECENTI+1:
				case MENU_FILE_RECENTI+2:
				case MENU_FILE_RECENTI+3:
				case MENU_FILE_RECENTI+4: {
					if (!preservaModifiche())
						break;
					GetMenuStringW( GetMenu(hWnd), LOWORD(wParam), fileDaAprire, MAX_PATH, 0 );
					apriFile();
					break;
				}
				case MENU_FILE_EDITA:
					okEdita = !okEdita;
					if (okEdita) {
						ShowWindow(hTastiera,SW_HIDE);
						ShowWindow(hEditore,SW_SHOW);
						okEdita = MF_CHECKED;
						contaCaratteri();
						SetWindowPos( hWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
						scriviNomeFile();
					} else {
						ShowWindow(hEditore,SW_HIDE);
						disponiBottoni();
						SetWindowPos( hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
					}
					bottoniDestra();
					CheckMenuItem( GetMenu(hWnd), MENU_FILE_EDITA, okEdita );
					break;
				case MENU_FILE_SALVA:
					if( wcscmp(nomeFile,L"untitled") )
						salvaFile();
					else
						finestraDialogoSalva();
					break;
				case MENU_FILE_SALVACOME:
					finestraDialogoSalva();
					break;
				case MENU_FILE_CHIUDI:
					chiudiFile();
					break;
				case MENU_EXIT:
					chiudiProgramma();
					break;
				case MENU_COPIA_APPUNTI: { 	// interruttore scelta se copiare anche in appunti
					// inverte variabile globale
					okCopiaAppunti = okCopiaAppunti ? MF_UNCHECKED : MF_CHECKED;
					//setta menu checked o no
					CheckMenuItem( GetMenu(hWnd), MENU_COPIA_APPUNTI, okCopiaAppunti );
					wchar_t str[2];
					snwprintf( str, conta(str), L"%d", okCopiaAppunti );
					WritePrivateProfileStringW( L"options", L"clipboard", str, config_ini );
					break;
				}
				case MENU_MOSTRA_TITOLO:
					conBarraTitolo = conBarraTitolo ? MF_UNCHECKED : MF_CHECKED;
					mostraNascondiBarraTitolo();
					wchar_t str[2];
					snwprintf( str, conta(str), L"%d", conBarraTitolo );
					WritePrivateProfileStringW( L"options", L"titlebar", str, config_ini );
					break;
				case MENU_MOSTRA_STATUS: {
					if (okMostraStatus) {
						ShowWindow(hStatusBar, SW_HIDE);
						okMostraStatus = MF_UNCHECKED;
					} else {
						ShowWindow(hStatusBar, SW_SHOW);
						okMostraStatus = MF_CHECKED;
					}
					SendMessage(hWnd,WM_SIZE,0,0);
					CheckMenuItem( GetMenu(hWnd), MENU_MOSTRA_STATUS, okMostraStatus );
					wchar_t str[2];
					snwprintf( str, conta(str), L"%d", okMostraStatus );
					WritePrivateProfileStringW( L"options", L"statusbar", str, config_ini );
					break;
				}
				case MENU_INFO_SNIPPET: {
					if (hInfo) {
						HWND informino_esistente;
						while ( (informino_esistente = GetDlgItem(hInfo,ID_INFORMINO)) )
							DestroyWindow(informino_esistente);
					} else {
						WNDCLASS clsInfo = {0};
						clsInfo.lpfnWndProc = proceduraInfoSnippet;
						clsInfo.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
						clsInfo.lpszClassName = "ClasseInfoSnippet";
						RegisterClass(&clsInfo);
						hInfo = CreateWindowEx (WS_EX_TOOLWINDOW|WS_EX_TOPMOST, clsInfo.lpszClassName, "Snippet Info",	
							WS_OVERLAPPED|WS_SYSMENU|WS_VSCROLL|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 350, 300, hWnd, NULL, GetModuleHandle(0), NULL);
					}
					unsigned int lungo;
					wchar_t *snippet = malloc( sizeof(wchar_t) );
					if (okEdita) {	// ricava il testo selezionato nell'editore
						DWORD inizioSelez, fineSelez;
						SendMessage (hEditore, EM_GETSEL, (WPARAM)&inizioSelez, (LPARAM)&fineSelez);
						lungo = GetWindowTextLength(hEditore);
						wchar_t tuttoTesto[lungo+1];
						GetWindowTextW(hEditore, tuttoTesto, lungo+1);
						snippet = realloc(snippet, (fineSelez-inizioSelez+1)*sizeof(wchar_t) );
						tuttoTesto[fineSelez] = 0;
						wcscpy(snippet, &tuttoTesto[inizioSelez]);
						lungo = wcslen(snippet);
					} else {	// ricava lo snippet dal bottone
						if( !hSnippetFocus || hSnippetFocus==hTastiera )
							hSnippetFocus = GetDlgItem(hTastiera,ID_BOTTONE_SNIPPET);
						lungo = GetWindowTextLength(hSnippetFocus);
						snippet = realloc(snippet, (lungo+1)*sizeof(wchar_t));
						GetWindowTextW(hSnippetFocus, snippet, lungo+1);
					}
					_Bool surrogato = 0;
					si2.nMax = 10;
					si2.nPos = 0;
					int numInformino = 1;
					char numTesto[4];
					wchar_t glifo[3];
					// font degli informini
					caricaFont(L"keyboard");	
					lf.lfHeight = -45;
					HFONT hFontGlifo = CreateFontIndirectW(&lf);
					HDC hDc = GetDC(hInfo);
					SelectObject(hDc, hFontGlifo);
					wchar_t unicodePath[MAX_PATH];
					snwprintf( unicodePath, MAX_PATH, L"%s\\unicode.txt", cartellaProgramma );
					char testino[150];
					char riga[100];
					char *unicode;
					char esadec[7];
					for (int i=0; i<lungo; i++) {
						if (surrogato)
							surrogato = 0;
						else {
							HWND hInformino = CreateWindow ("Static", 0, WS_CHILD|WS_VISIBLE, 0,si2.nMax, 350,80, hInfo, (HMENU)ID_INFORMINO, GetModuleHandle(0), NULL);
							SubclassWindow( hInformino, proceduraInformino );
							sprintf(numTesto, "%d", numInformino++);
							HWND hNum = CreateWindow ("Static", numTesto, ES_RIGHT|WS_CHILD|WS_VISIBLE, 0,0, 25, 20, hInformino, NULL, GetModuleHandle(0), NULL);
							settaFontSistema(hNum);
							// definizione unicode del glifo
							if (snippet[i]>=0xD800 && snippet[i]<0xDC00) {	// coppie surrogate
								snwprintf (glifo, 3, L"%c%c", snippet[i], snippet[i+1]);
								sprintf(esadec,"%X",0x10000 + (snippet[i]-0xD800)*0x400 + (snippet[i+1]-0xDC00) );
								surrogato = 1;
							} else {	// utf16 regolare
								snwprintf (glifo, 2, L"%c", snippet[i]);
								sprintf(esadec,"%04X",snippet[i]);
							}
							HWND hGlifo = CreateWindowW (L"Edit", glifo, ES_CENTER|ES_READONLY|/*WS_BORDER|*/WS_CHILD|WS_VISIBLE, 30,0, 60, 60, hInformino, (HMENU)ID_INFO_GLIFO, GetModuleHandle(0), NULL);
							SendMessage(hGlifo, WM_SETFONT, (WPARAM)hFontGlifo, 0);
							// cerca la definizione in unicode.txt
							FILE *file = _wfopen( unicodePath, L"rb" );
							if (file) {
								while( fgets( riga, sizeof riga/sizeof(char), file ) ) {
									unicode = strtok( riga, ";");
									if ( strcmp(unicode,esadec) == 0 ) {
										unicode = strtok (NULL, ";");
										break;
									} else
										unicode = "";
								}
							} else unicode = "";
							fclose(file);
							sprintf(testino, "U+%s\t\t%d", esadec, strtol(esadec, NULL, 16));
							sprintf(testino, "%s\r\n%s", testino, unicode);
							HWND hTesto = CreateWindow ("Edit", testino, ES_MULTILINE|ES_READONLY|WS_CHILD|WS_VISIBLE, 100,0, 220,80, hInformino, NULL, GetModuleHandle(0), NULL);
							SetWindowSubclass(hTesto, proceduraInfoTesto, 0, 0);
							settaFontSistema(hTesto);
							si2.nMax += 80;
						}
					}
					free(snippet);
					SetScrollInfo (hInfo, SB_VERT, &si2, TRUE);
					break;
				}
				case MENU_LETTURA_DAS: {
					das = das ? MF_UNCHECKED : MF_CHECKED;
					ordineLetturaDestraSinistra();
					wchar_t str[2];
					snwprintf( str, conta(str), L"%d", das );
					WritePrivateProfileStringW( L"options", L"rtlreading", str, config_ini );
					disponiBottoni();
					break;
				}
				case MENU_FONT_TASTIERA: {
					caricaFont(L"keyboard");
					CHOOSEFONTW cf = {0};
					cf.lStructSize = sizeof(cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					cf.Flags = CF_SCREENFONTS | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
					if(ChooseFontW(&cf)==TRUE) {
						wchar_t fontsettings[80] = {0};
						snwprintf( fontsettings, conta(fontsettings), L"height=%d%cweight=%d%citalic=%d%ccharset=%d%cface=%s", 
						        lf.lfHeight,0, lf.lfWeight,0, lf.lfItalic,0, lf.lfCharSet,0, lf.lfFaceName);
						WritePrivateProfileSectionW( L"keyboard", fontsettings, config_ini );
						disponiBottoni();
					}
					break;
				}
				case MENU_FONT_EDITORE: {
					caricaFont(L"editor");
					CHOOSEFONTW cf = {0};
					cf.lStructSize = sizeof (cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					cf.Flags = CF_SCREENFONTS | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT;
					if(ChooseFontW(&cf)==TRUE) {
						HFONT hfont = CreateFontIndirectW(&lf);
						SendMessage(hEditore, WM_SETFONT, (WPARAM)hfont, 0);
						InvalidateRect (hWnd, NULL, TRUE);
						UpdateWindow(hWnd);
						wchar_t fontsettings[80] = {0};
						snwprintf( fontsettings, conta(fontsettings), L"height=%d%cweight=%d%citalic=%d%ccharset=%d%cface=%s", 
						        lf.lfHeight,0, lf.lfWeight,0, lf.lfItalic,0, lf.lfCharSet,0, lf.lfFaceName);
						WritePrivateProfileSectionW( L"editor", fontsettings, config_ini );
					}
					break;
				}
				case MENU_PREFERENZE:	// finestra di dialogo Preferenze separatori dei bottoni
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(DIALOGO_PREFERENZE), hWnd, proceduraDialogoPreferenze);
					break;
				case MENU_PROBLEMA:
					ShellExecute(0, 0, "https://sourceforge.net/projects/uosk/support", 0, 0 , SW_SHOW);
					break;
				case MENU_INFORMAZIONI: // finestra di dialogo Informazioni
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(DIALOGO_RICONOSCIMENTI), hWnd, proceduraDialogoInformazioni);
					break;
				case MENU_BOTTONE_RIDUCI:
					if( conBarraTitolo ) {	// ridimensiona la finestra per coincidere col contenuto
						RECT dimPrima;
						GetWindowRect( hWnd, &dimPrima );
						RECT dimDopo = { dimPrima.left, dimPrima.top, max(size.pulsaLarga,150), si.nMax+1 };
						if (okMostraStatus)
							dimDopo.bottom += size.statusAlta;	
						AdjustWindowRectEx( &dimDopo, GetWindowStyle(hWnd),	1, GetWindowExStyle(hWnd));
						MoveWindow(hWnd, dimPrima.left, dimPrima.top, 
							dimDopo.right+dimPrima.left-dimDopo.left, dimDopo.bottom+dimPrima.top-dimDopo.top, 1);
						if (das) creaBottoni();
						veroResize = 0;
					} else
						ShowWindow(hWnd, SW_MINIMIZE);
					break;
				case MENU_BOTTONE_EDITA:
					if( conBarraTitolo )
						SendMessage( hWnd, WM_COMMAND, MENU_FILE_EDITA, 0 );
					else if( massimizzata )
						ShowWindow( hWnd, SW_RESTORE );
					else
						ShowWindow( hWnd, SW_MAXIMIZE );
					break;
				case MENU_BOTTONE_CHIUDI:
					if( conBarraTitolo ) {
						if( nomeFile[0] )	
							chiudiFile();
						else
							SendMessage( hWnd, WM_COMMAND, MENU_FILE_NUOVO, 0 );
					} else
						SendMessage( hWnd, WM_CLOSE, 0, 0 );
			} break;
		case WM_SETFOCUS:
			scriviNomeProgramma();
			break;
		case WM_CLOSE:
			chiudiProgramma();
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

// funzione iniziale
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {

	// se AppData\Uosk non esiste la crea
	SHGetFolderPathW( 0, CSIDL_APPDATA, NULL, 0, appData );
	PathAppendW( appData, L"Uosk");
	if ( !PathIsDirectoryW(appData) ) {	// se non esiste
		wchar_t msg[MAX_PATH+30];
		if( CreateDirectoryW(appData,NULL) ) {	// la crea
			snwprintf( msg, conta(msg), L"I have created the settings directory %s", appData );
			MessageBoxW( NULL, msg, L"Uosk", MB_ICONINFORMATION );
		} else {
			snwprintf( msg, conta(msg), L"I can't create the settings directory %s", appData );
			MessageBoxW( NULL, msg, L"Uosk", MB_ICONERROR );
		}
	}

	// percorso assoluto del file config.ini in UserName\AppData\Roaming
	snwprintf( config_ini, conta(config_ini), L"%s\\config.ini", appData );
	// se non esiste config.ini lo crea utf-16 little endian
	if( GetFileAttributesW(config_ini) == INVALID_FILE_ATTRIBUTES ) {
		FILE *hFile = _wfopen( config_ini, L"w" );
		char bom[] = { 0xFF, 0xFE, 0 };
		fprintf( hFile, bom );
		fclose(hFile);
	}

	// struttura della finestra
	WNDCLASSEXW wc = {0};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
	wc.lpszMenuName = MAKEINTRESOURCEW(ID_MENUPRINCIPALE);
	wc.lpszClassName = L"Uosk";
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICONA));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICONA), IMAGE_ICON, 16, 16, 0);
	RegisterClassExW(&wc);

	// prende dimensioni da config.ini
	int left = GetPrivateProfileIntW( L"size", L"left", 100, config_ini );
	int top = GetPrivateProfileIntW( L"size", L"top", 100, config_ini );
	int larga  = GetPrivateProfileIntW( L"size", L"right", 500, config_ini ) - left;
	int alta = GetPrivateProfileIntW( L"size", L"bottom", 400, config_ini ) - top;

	// creazione della finestra principale
	HWND hWnd = CreateWindowExW ( WS_EX_ACCEPTFILES, wc.lpszClassName, L"Uosk", 
		WS_OVERLAPPEDWINDOW|WS_VISIBLE, left, top, larga, alta, NULL, NULL, hInstance, NULL);

	HACCEL hAccelleratori = LoadAccelerators(hInstance, "Acceleri");	// MAKEINTRESOURCE(ID_ACCELERATORI)

	if( !(conBarraTitolo = GetPrivateProfileIntW(L"options",L"titlebar",0,config_ini)) )
		mostraNascondiBarraTitolo();
	bottoniDestra();

	// opzione lettura da destra a sinistra
	das = GetPrivateProfileIntW( L"options", L"rtlreading", 0, config_ini );
	ordineLetturaDestraSinistra();

	// apre il file iniziale
	wchar_t fileDaConfig[MAX_PATH];
	if( GetPrivateProfileStringW( L"file", L"openfile", L"", fileDaConfig, sizeof(fileDaConfig), config_ini) ) {
		apriDaConfig_ini = 1;
		GetFullPathNameW( fileDaConfig, MAX_PATH, fileDaAprire, NULL );
	}

	// directory del programma
	wchar_t *lineaComando = GetCommandLineW();
	int quantiArgomenti;
	wchar_t **argomenti = CommandLineToArgvW( lineaComando, &quantiArgomenti );
	if( wcsrchr(argomenti[0], '\\') )	// solo per non far inchiodare CodeLite
		wcsrchr(argomenti[0], '\\')[0] = 0;
	cartellaProgramma = argomenti[0];
	// file che arriva da un drag & drop sull'icona
	if( quantiArgomenti > 1 )
		wcscpy( fileDaAprire, argomenti[1] );

	apriFile();

	MSG message;
	while ( GetMessageW(&message,NULL,0,0) ) {
		if (!TranslateAccelerator (hWnd, hAccelleratori, &message)) {
			TranslateMessage(&message);
			DispatchMessageW(&message);
			
			// trova il manico dell'ultimo programma attivo in tutto Windows
			HWND programmaDavanti = GetForegroundWindow();
			if ( programmaDavanti != hWnd )
				ultimoProgrammaAttivo = programmaDavanti;
		}
	}
	return message.wParam;
}

