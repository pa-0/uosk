/* Uosk (Unicode On-Screen Keyboard)
 * Open-source virtual keyboard: insert text snippets into any Windows™ program	*/
#define WINVER 0x0501

#define RELEASE 0

#include <stdio.h>
#include <windows.h>
#include <winable.h>
#include <commctrl.h>
#include <windowsx.h>
#include <unistd.h>
#include <locale.h>
#include "macro.h"
#include "editore.h"
#include "altre.h"


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {	// Brook Miles l'ha chiamata 'WndProc' non so se è un nome usuale
    switch(message) {
		case WM_CREATE: {	// Creazione degli elementi nella finestra principale
				
			// creazione status bar
			HWND hStatus = CreateWindow( STATUSCLASSNAME, NULL, WS_CHILD,	// |WS_VISIBLE
				0,0,0,0, hWnd, (HMENU)ID_STATUSBAR, GetModuleHandle(NULL), NULL);
			int sezioni[] = {35, -1};
			SendMessage(hStatus, SB_SETPARTS, sizeof(sezioni)/sizeof(int), (LPARAM)sezioni);
			// calcola altezza barra di stato
			RECT rcStatus;
			GetWindowRect(hStatus, &rcStatus);
			size.statusAlta = rcStatus.bottom - rcStatus.top;
			// mostra la status bar secondo quanto memorizzato in congin.ini
			if( (okMostraStatus = GetPrivateProfileInt("options","statusbar",0,config_ini)) )
				ShowWindow(hStatus,SW_SHOW);
			okCopiaAppunti = GetPrivateProfileInt("options","clipboard",0,config_ini);
			
			// Classe finestra pulsantiera che ospita i bottoni
			WNDCLASS clsPls = {0};
			clsPls.lpfnWndProc = ProceduraPulsantiera;
			clsPls.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
			clsPls.lpszClassName = "ClassePulsantiera";
			clsPls.hCursor = LoadCursor(NULL, IDC_ARROW);
			// registrazione
			if (!RegisterClass(&clsPls)) {
				MessageBox(hWnd,"Registration of Pulsantiera class has failed","Error",MB_ICONERROR);
				break;
			}
			// creazione finestra pulsantiera
			hPulsantiera = CreateWindow ( clsPls.lpszClassName, "", WS_CHILD,	// | WS_BORDER
										  0, 0, 100, 100, hWnd, NULL, GetModuleHandle(NULL), NULL);
			if (hPulsantiera==NULL)
				MessageBox(hWnd,"Creation of Pulsantiera window has failed","Error",MB_ICONERROR);
			
			{	// creazione dell'Editore di testo
				hEditore = CreateWindowW( L"Edit", NULL, WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
					0, 0, 200, 100, hWnd, (HMENU)ID_EDITORE,  GetModuleHandle(NULL), NULL);
				if (hEditore)
					SetWindowSubclass(hEditore, ProceduraEditore, 0, 0);
				caricaFont("editor");
				HFONT hFontEditore = CreateFontIndirect(&lf);
				SendMessage(hEditore, WM_SETFONT, (WPARAM)hFontEditore, 0);

			}

			// spunta voce del menu 'Copia in appunti' 
			HMENU hmenu = GetMenu(hWnd);
			CheckMenuItem( hmenu, MENU_COPIA_APPUNTI, okCopiaAppunti );
			// 'Barra di stato'
			CheckMenuItem( hmenu, MENU_MOSTRA_STATUS, okMostraStatus );

			break;
		}
		case WM_SIZE:	// ridimensiona i comandi nella finestra principale
			if( wParam != SIZE_MINIMIZED ) {
				// ridimensiona barra di stato
				HWND hStatus = GetDlgItem(hWnd, ID_STATUSBAR);
				SendMessage(hStatus, WM_SIZE, 0, 0);
		
				// scopre dimensioni finestra client
				RECT rect;
				GetClientRect(hWnd,&rect);
				size.clienteLargo = rect.right;

				// aggiorna ScrollInfo
				int posVertIniz = si.nPos;
				si.nPage = rect.bottom;
				if (okMostraStatus)	
					si.nPage -= size.statusAlta;

				if( (si.nMax-si.nPos) < si.nPage ) 
					si.nPos = si.nMax - si.nPage + 1;
				if( si.nPos < 0 )
					si.nPos = 0;
				SetScrollInfo (hPulsantiera, SB_VERT, &si, TRUE);
				ScrollWindow (hPulsantiera, 0, posVertIniz-si.nPos, NULL, NULL);
				MoveWindow(hPulsantiera, 0, 0, rect.right, si.nPage, 1);
				MoveWindow(hEditore, 0, 0, rect.right, si.nPage, 1);
			}
			if (wParam==SIZE_MAXIMIZED)
				disponiBottoni(hWnd);
			veroResize = 1;
			break;
		case 799:
			veroResize = 0;
			break;
		case WM_EXITSIZEMOVE:
			if(veroResize) {
				disponiBottoni(hWnd);
				veroResize = 0;
			}
			break;
		case WM_SETCURSOR: // quando cursore va sopra uno dei bottoni dà alla finestra stile che non viene attivata
			if ( GetWindowLong((HWND)wParam, GWL_ID) == ID_BOTTONE_SCAMPOLO )
				SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_NOACTIVATE|WS_EX_ACCEPTFILES );
			else
				SetWindowLong(hWnd, GWL_EXSTYLE, WS_EX_ACCEPTFILES );
			int ritorno = DefWindowProc(hWnd, message, wParam, lParam);
			return ritorno;
		case WM_DROPFILES:
			if (!preservaModifiche(hWnd))
				break;
			DragQueryFile((HANDLE) wParam, 0, fileDaAprire, sizeof(fileDaAprire));
			apriFile(hWnd);
			DragFinish((HANDLE)wParam);
			break;
		case WM_COMMAND:	// i comandi alla finestra principale, in particolare dal menu principale
			switch (LOWORD(wParam)) {
				case MENU_FILE_NUOVO:
					if(chiudiFile(hWnd)==0)
						break;
					strcpy(nomeFile,"untitled");
					okEdita = 1;
					ShowWindow(hEditore,SW_SHOW);
					SetWindowText(hEditore, "\0");
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_ENABLED);
					CheckMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_CHECKED );
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVACOME, MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_CHIUDI, MF_ENABLED);
					ModifyMenuW(GetMenu(hWnd), 3, MF_BYPOSITION|MF_ENABLED|MF_HELP|MF_STRING, ID_BOTTONE_AUTOSIZE, L"✔");
					DrawMenuBar(hWnd);
					break;
				case MENU_FILE_APRI:
					finestraDialogoApri(hWnd);
					break;
				case MENU_FILE_EDITA:
					okEdita = !okEdita;
					if (okEdita) {
						ShowWindow(hPulsantiera,SW_HIDE);
						ShowWindow(hEditore,SW_SHOW);
						okEdita = MF_CHECKED;
						contaCaratteri();
						ModifyMenuW(GetMenu(hWnd), 3, MF_BYPOSITION|MF_ENABLED|MF_HELP|MF_STRING, ID_BOTTONE_AUTOSIZE, L"✔");
					} else {
						int caratteri = GetWindowTextLength(hEditore);
						wchar_t buffer[caratteri+1];
						GetWindowTextW(hEditore, buffer, caratteri+1);
						SetFileAttributes( buffer_tmp, FILE_ATTRIBUTE_NORMAL );
						FILE* file = _wfopen( Lbuffer_tmp, L"w,ccs=UTF-8" );
						SetFileAttributes( buffer_tmp, FILE_ATTRIBUTE_HIDDEN);
						fwprintf( file, L"%s", buffer );
						fclose(file);
						ShowWindow(hEditore,SW_HIDE);
						disponiBottoni(hWnd);
						ModifyMenu(GetMenu(hWnd), 3, MF_BYPOSITION|MF_ENABLED|MF_HELP|MF_STRING, ID_BOTTONE_AUTOSIZE, "[ ]");
					}
					DrawMenuBar(hWnd);
					CheckMenuItem( GetMenu(hWnd), MENU_FILE_EDITA, okEdita );
					break;
				case MENU_FILE_SALVA:
					salvaFile(hWnd);
					break;
				case MENU_FILE_SALVACOME:
					finestraDialogoSalva(hWnd);
					break;
				case MENU_FILE_CHIUDI:
					chiudiFile(hWnd);
					break;
				case MENU_EXIT:
					chiudiProgramma(hWnd);
					break;
				case MENU_COPIA_APPUNTI: { 	// interruttore scelta se copiare anche in appunti
					// inverte variabile globale
					okCopiaAppunti = okCopiaAppunti ? MF_UNCHECKED : MF_CHECKED;
					//setta menu checked o no
					HMENU hmenu = GetMenu(hWnd);
					CheckMenuItem( hmenu, MENU_COPIA_APPUNTI, okCopiaAppunti );
					char str[3];
					sprintf(str, "%d", okCopiaAppunti);
					WritePrivateProfileString( "options", "clipboard", str, config_ini);
					break;
				}
				case MENU_MOSTRA_STATUS: {
					HWND hStatus = GetDlgItem(hWnd, ID_STATUSBAR);
					if (okMostraStatus) {
						ShowWindow(hStatus, SW_HIDE);
						okMostraStatus = MF_UNCHECKED;
					} else {
						ShowWindow(hStatus, SW_SHOW);
						okMostraStatus = MF_CHECKED;
					}
					SendMessage(hWnd,WM_SIZE,0,0);
					HMENU hmenu = GetMenu(hWnd);
					CheckMenuItem( hmenu, MENU_MOSTRA_STATUS, okMostraStatus );
					char str[3];
					sprintf(str, "%d", okMostraStatus);
					WritePrivateProfileString( "options", "statusbar", str, config_ini);
					break;
				}
				case MENU_FONT_BOTTONI: {
					caricaFont("buttons");
					CHOOSEFONT cf = {0};
					cf.lStructSize = sizeof (cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					cf.Flags = CF_INITTOLOGFONTSTRUCT;
					if(ChooseFont(&cf)==TRUE) {
						printf("%s %d %d %d %d %d\n",lf.lfFaceName, lf.lfHeight, lf.lfWeight, lf.lfItalic, lf.lfCharSet, cf.iPointSize);
						char fontsettings[80] = {0};
						sprintf(fontsettings,"height=%d%cweight=%d%citalic=%d%ccharset=%d%cface=%s", 
						        lf.lfHeight,0, lf.lfWeight,0, lf.lfItalic,0, lf.lfCharSet,0, lf.lfFaceName);
						WritePrivateProfileSection("buttons", fontsettings, config_ini);
						disponiBottoni(hWnd);
					}
					break;
				}
				case MENU_FONT_EDITORE: {
					caricaFont("editor");
					CHOOSEFONT cf = {0};
					cf.lStructSize = sizeof (cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					cf.Flags = CF_INITTOLOGFONTSTRUCT;
					if(ChooseFont(&cf)==TRUE) {
						HFONT hfont = CreateFontIndirect(&lf);
						SendMessage(hEditore, WM_SETFONT, (WPARAM)hfont, 0);
						InvalidateRect (hWnd, NULL, TRUE);
						UpdateWindow(hWnd);
						char fontsettings[80] = {0};
						sprintf(fontsettings,"height=%d%cweight=%d%citalic=%d%ccharset=%d%cface=%s", 
						        lf.lfHeight,0, lf.lfWeight,0, lf.lfItalic,0, lf.lfCharSet,0, lf.lfFaceName);
						WritePrivateProfileSection("editor", fontsettings, config_ini);
					}
					break;
				}
				case MENU_PREFERENZE:	// finestra di dialogo Preferenze separatori dei bottoni
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(DIALOGO_PREFERENZE), hWnd, proceduraDialogoPreferenze);
					break;
				case MENU_INFORMAZIONI: // finestra di dialogo Informazioni
					DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(DIALOGO_RICONOSCIMENTI), hWnd, proceduraDialogoInformazioni);
					break;
				case MENU_PROBLEMA: {
					ShellExecute(0, 0, "https://sourceforge.net/projects/uosk/support", 0, 0 , SW_SHOW);
					break;
				}
				case ID_BOTTONE_AUTOSIZE: {
					if (okEdita)
						SendMessage(hWnd,WM_COMMAND,MAKEWPARAM(MENU_FILE_EDITA,0),0);
					else {
						// ridimensiona la finestra per coincidere col contenuto
						RECT dimPrima;
						GetWindowRect( hWnd, &dimPrima );
						RECT dimDopo = { dimPrima.left, dimPrima.top, max(size.pulsaLarga,150), si.nMax+1 };
						if (okMostraStatus)
							dimDopo.bottom += size.statusAlta;	
						AdjustWindowRectEx( &dimDopo, GetWindowStyle(hWnd),	1, GetWindowExStyle(hWnd));
						MoveWindow(hWnd, dimPrima.left, dimPrima.top, 
							dimDopo.right+dimPrima.left-dimDopo.left, dimDopo.bottom+dimPrima.top-dimDopo.top, 1);
					}
				}
			} break;
        case WM_CLOSE: {
			chiudiProgramma(hWnd);
			break;
		}
        case WM_DESTROY:
            PostQuitMessage(0);
		default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// funzione iniziale
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {

	if (RELEASE)
		FreeConsole();
		
	// percorso assoluto del file config.ini
	GetFullPathName(".\\config.ini", MAX_PATH, config_ini, NULL);
	GetFullPathName(".\\buffer.tmp", MAX_PATH, buffer_tmp, NULL);
	GetFullPathNameW(L".\\buffer.tmp", MAX_PATH, Lbuffer_tmp, NULL);

	// struttura della finestra
	WNDCLASSEX wc = {0};
	wc.cbSize   	= sizeof(WNDCLASSEX);
	wc.lpfnWndProc	= WndProc;
	wc.hInstance 	= hInstance;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
	wc.lpszMenuName  = MAKEINTRESOURCE(ID_MENUPRINCIPALE);
	wc.lpszClassName = "Uosk";
	wc.hIcon   = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICONA));
	wc.hIconSm = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICONA), IMAGE_ICON, 16, 16, 0);

	if (!RegisterClassEx(&wc)) {
		MessageBoxW(NULL, L"Registration of Uosk class is failed", L"Error", MB_ICONERROR);
			// "La registrazione della classe ClasseFinestra è fallita", L"Errore"
		return 0;
	}

	// prende dimensioni da config.ini
	int left = GetPrivateProfileInt("size", "left", 100, config_ini);
	int top = GetPrivateProfileInt("size", "top", 100, config_ini);
	int larga  = GetPrivateProfileInt("size", "right", 500, config_ini) - left;
	int alta = GetPrivateProfileInt("size", "bottom", 400, config_ini) - top;
	
	// creazione della finestra principale
	HWND hWnd = CreateWindowEx ( WS_EX_TOPMOST|WS_EX_ACCEPTFILES, wc.lpszClassName, "Uosk", 
		WS_OVERLAPPEDWINDOW|WS_VISIBLE, left, top, larga, alta, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		MessageBox(NULL,"Creation of window has failed", "Error", MB_ICONERROR);
	
	// apre il file iniziale							
	if (GetPrivateProfileString( "file", "openfile", "", fileDaAprire, sizeof(fileDaAprire), config_ini) && !lpCmdLine[0])
		apriDaConfig_ini = 1;
	// file che arriva da un drag & drop sull'icona
	if (lpCmdLine[0]) {
		if (lpCmdLine[0]=='"') {
			lpCmdLine++;
			lpCmdLine[strlen(lpCmdLine)-1] = 0;
		}
		strcpy(fileDaAprire, lpCmdLine);
	}
	apriFile(hWnd);

	MSG message;
	while ( GetMessage(&message,NULL,0,0) > 0 ) {
		TranslateMessage(&message);
		DispatchMessage(&message);
		
		// trova il manico dell'ultimo programma attivo in tutto Windows
		HWND programmaDavanti = GetForegroundWindow();
		if ( programmaDavanti != hWnd ) {
			ultimoProgrammaAttivo = programmaDavanti;
			DWORD idUltimoProgramma = GetWindowThreadProcessId(ultimoProgrammaAttivo,0);
			GUITHREADINFO gti;
			gti.cbSize = sizeof(GUITHREADINFO);
			GetGUIThreadInfo(idUltimoProgramma, &gti);
			finestraDestinazione = gti.hwndFocus;	
		}
	}
	return message.wParam;
}

