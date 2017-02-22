/* Uosk (Unicode On-Screen Keyboard)
 * Open-source virtual keyboard: insert text snippets into any Windows™ program	*/
#define WINVER 0x0501
#define _WIN32_IE 0x0300

#define RELEASE 1

#include <stdio.h>
#include <windows.h>
#include <winable.h>
#include <commctrl.h>
#include <windowsx.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <shlwapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include ".\..\winapi\utilita.h"
#include "macro.h"
#include "editore.h"
#include "altre.h"


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch(message) {
		case WM_CREATE: {	// Creazione degli elementi nella finestra principale
			hWindow = hWnd;

			// creazione status bar
			hStatusBar = CreateWindow( STATUSCLASSNAME, NULL, WS_CHILD, 0,0,0,0, hWnd, 0, GetModuleHandle(0), 0 );
			// calcola altezza barra di stato
			RECT rcStatus;
			GetWindowRect( hStatusBar, &rcStatus );
			size.statusAlta = rcStatus.bottom - rcStatus.top;
			// mostra la status bar secondo quanto memorizzato in congin.ini
			if( (okMostraStatus = GetPrivateProfileInt("options","statusbar",0,config_ini)) )
				ShowWindow(hStatusBar,SW_SHOW);

			// Classe finestra Tastiera che ospita i bottoni
			WNDCLASS clsPls = {0};
			clsPls.lpfnWndProc = proceduraTastiera;
			clsPls.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
			clsPls.lpszClassName = "ClasseTastiera";
			clsPls.hCursor = LoadCursor(NULL, IDC_ARROW);
			// registrazione
			if(!RegisterClass(&clsPls)) {
				MessageBox(hWnd,"Registration of Keyboard class has failed","Error",MB_ICONERROR);
				break;
			}
			// creazione finestra Tastiera
			hTastiera = CreateWindow ( clsPls.lpszClassName, "", WS_CHILD, 0, 0, 100, 100, hWnd, 0, GetModuleHandle(0), 0 );
			if (hTastiera==NULL)
				MessageBox(hWnd,"Creation of Keyboard window has failed","Error",MB_ICONERROR);
			
			{	// creazione dell'Editore di testo
				hEditore = CreateWindowW( L"Edit", NULL, WS_CHILD|WS_VSCROLL|ES_MULTILINE|ES_AUTOVSCROLL, 
				                          0, 0, 200, 100, hWnd, 0, GetModuleHandle(0), 0 );
				if (hEditore)
					SetWindowSubclass(hEditore, ProceduraEditore, 0, 0);
				caricaFont("editor");
				HFONT hFontEditore = CreateFontIndirect(&lf);
				SendMessage(hEditore, WM_SETFONT, (WPARAM)hFontEditore, 0);
			}

			{ // ToolTip della finestra principale
			hToolTip = CreateWindow( TOOLTIPS_CLASS, 0, WS_POPUP | TTS_ALWAYSTIP, 0, 0, 0, 0, hWnd, 0, 0, 0);
			ti.cbSize = sizeof(TOOLINFO);
			ti.uFlags = TTF_SUBCLASS;
			ti.hwnd = hWnd;
			ti.uId = (UINT)ID_FINESTRA_CLIENTE;
			ti.lpszText = "Double click to open a file";
			SendMessage( hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti );

			// ToolTip del bottone a destra del menu
			ti.uId = (UINT)MENU_BOTTONE_DESTRA;
			SendMessage( hToolTip, TTM_ADDTOOL, 0, (LPARAM)&ti );
			}

			// modifica il menu
			okCopiaAppunti = GetPrivateProfileInt("options","clipboard",0,config_ini);
			CheckMenuItem( GetMenu(hWnd), MENU_COPIA_APPUNTI, okCopiaAppunti );
			CheckMenuItem( GetMenu(hWnd), MENU_MOSTRA_STATUS, okMostraStatus );
			bottoneDestra();
			char nomePath[8];
			char fileRecente[MAX_PATH];
			HMENU hSubMenu = GetSubMenu( GetSubMenu(GetMenu(hWnd),0), 2 );
			for( int i=0; i<5; i++ ) {
				sprintf( nomePath, "recent%d", i );
				GetPrivateProfileString( "file", nomePath, "", fileRecente, MAX_PATH, config_ini);
				if( fileRecente[0] )
					InsertMenu( hSubMenu, -1, MF_BYPOSITION|MF_STRING, MENU_FILE_RECENTI+i, fileRecente);
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
				sezioni[0] = 35;
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
				MoveWindow( hTastiera, 0, 0, rect.right, si.nPage, 1 );
				MoveWindow( hEditore, 0, 0, rect.right, si.nPage, 1 );

				// aggiorna le coordiante dei tooltip
				ti.uId = (UINT)ID_FINESTRA_CLIENTE;
				GetClientRect (hWnd, &ti.rect);
				ti.rect.bottom -= size.statusAlta;
				SendMessage( hToolTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti );
				ti.uId = (UINT)MENU_BOTTONE_DESTRA;
				GetMenuItemRect( hWnd, GetMenu(hWnd), 3, &ti.rect );
				MapWindowPoints( NULL, hWnd, (POINT*)&ti.rect, 2 );
				SendMessage( hToolTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti );
			}
			veroResize = 1;
			if (wParam==SIZE_MAXIMIZED)	{
				disponiBottoni();
				massimizzata = 1;
				bottoneDestra();
			}
			if ( wParam==SIZE_RESTORED && massimizzata ) {
				disponiBottoni();
				massimizzata = 0;
				veroResize = 0;
				bottoneDestra();
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
			DragQueryFile((HANDLE) wParam, 0, fileDaAprire, sizeof(fileDaAprire));
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
					strcpy(nomeFile,"untitled");
					okEdita = 1;
					ShowWindow(hEditore,SW_SHOW);
					SetWindowText(hEditore, "");
					SetFocus(hEditore);
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_ENABLED);
					CheckMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_CHECKED );
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVACOME, MF_ENABLED);
					EnableMenuItem(GetMenu(hWnd), MENU_FILE_CHIUDI, MF_ENABLED);
					bottoneDestra();
					SetWindowPos(hWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
					contaCaratteri();
					SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)nomeFile);
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
					GetMenuString( GetMenu(hWnd), LOWORD(wParam), fileDaAprire, MAX_PATH, 0 );
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
						bottoneDestra();
						SetWindowPos( hWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
						scriviNomeFile();
					} else {
						ShowWindow(hEditore,SW_HIDE);
						disponiBottoni();
						bottoneDestra();
						SetWindowPos( hWnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE );
					}
					DrawMenuBar(hWnd);
					CheckMenuItem( GetMenu(hWnd), MENU_FILE_EDITA, okEdita );
					break;
				case MENU_FILE_SALVA:
					if( strcmp(nomeFile,"untitled") )
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
					char str[2];
					sprintf(str, "%d", okCopiaAppunti);
					WritePrivateProfileString( "options", "clipboard", str, config_ini);
					break;
				}
				case MENU_MOSTRA_TITOLO:
					conBarraTitolo = conBarraTitolo ? MF_UNCHECKED : MF_CHECKED;
					mostraNascondiBarraTitolo();
					char str[2];
					sprintf(str, "%d", conBarraTitolo);
					WritePrivateProfileString( "options", "titlebar", str, config_ini);
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
					char str[2];
					sprintf(str, "%d", okMostraStatus);
					WritePrivateProfileString( "options", "statusbar", str, config_ini);
					break;
				}
				case MENU_FONT_TASTIERA: {
					caricaFont("keyboard");
					CHOOSEFONT cf = {0};
					cf.lStructSize = sizeof (cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
					if(ChooseFont(&cf)==TRUE) {
						char fontsettings[80] = {0};
						sprintf(fontsettings,"height=%d%cweight=%d%citalic=%d%ccharset=%d%cface=%s", 
						        lf.lfHeight,0, lf.lfWeight,0, lf.lfItalic,0, lf.lfCharSet,0, lf.lfFaceName);
						WritePrivateProfileSection("keyboard", fontsettings, config_ini);
						disponiBottoni();
					}
					break;
				}
				case MENU_FONT_EDITORE: {
					caricaFont("editor");
					CHOOSEFONT cf = {0};
					cf.lStructSize = sizeof (cf);
					cf.hwndOwner = hWnd;
					cf.lpLogFont = &lf;
					cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;
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
				case MENU_RIDUCI:
					ShowWindow(hWnd, SW_MINIMIZE);
					break;
				case MENU_INGRANDISCI:
					if (massimizzata)
						ShowWindow(hWnd, SW_RESTORE);
					else
						ShowWindow(hWnd, SW_MAXIMIZE);
					break;
				case MENU_BOTTONE_DESTRA:
					if (!conBarraTitolo)
						SendMessage(hWnd, WM_CLOSE, 0, 0);
					else if (!nomeFile[0])
						SendMessage(hWnd, WM_COMMAND, MENU_FILE_NUOVO, 0);
					else if (okEdita || massimizzata)
						SendMessage(hWnd, WM_COMMAND, MENU_FILE_EDITA, 0);
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
						veroResize = 0;
					}
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
			return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// funzione iniziale
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow ) {

	if (RELEASE)
		FreeConsole();

	// se AppData\Uosk non esiste la crea
	char appdata[MAX_PATH];
	SHGetFolderPath( 0, CSIDL_APPDATA, NULL, 0, appdata );
	PathAppend (appdata, "Uosk");
	if (!PathIsDirectory(appdata)) {
		char msg[MAX_PATH+30];
		if (CreateDirectory(appdata ,NULL)) {
			sprintf(msg,"I have created the folder %s", appdata);
			MessageBox(NULL, msg, "New settings directory", MB_ICONINFORMATION);
		} else {
			sprintf (msg, "I can't create the folder %s", appdata);
			MessageBox (NULL, msg, "No settings directory", MB_ICONERROR);
		}
	}

	// percorso assoluto del file config.ini in UserName\AppData\Roaming
	SHGetFolderPath( 0, CSIDL_APPDATA, NULL, 0, config_ini );
	PathAppend(config_ini, "Uosk\\config.ini");

	// struttura della finestra
	WNDCLASSEX wc = {0};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_DBLCLKS;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
	wc.lpszMenuName = MAKEINTRESOURCE(ID_MENUPRINCIPALE);
	wc.lpszClassName = "Uosk";
	wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_ICONA));
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
	HWND hWnd = CreateWindowEx ( WS_EX_ACCEPTFILES, wc.lpszClassName, "Uosk", 
		WS_OVERLAPPEDWINDOW|WS_VISIBLE, left, top, larga, alta, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		MessageBox(NULL,"Creation of window has failed", "Error", MB_ICONERROR);

	HACCEL hAccelleratori = LoadAccelerators(hInstance, "Acceleri");	// MAKEINTRESOURCE(ID_ACCELERATORI)

	if( !(conBarraTitolo = GetPrivateProfileInt("options","titlebar",0,config_ini)) )
		mostraNascondiBarraTitolo();

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
	apriFile();

	MSG message;
	while ( GetMessage(&message,NULL,0,0) ) {
		if (!TranslateAccelerator (hWnd, hAccelleratori, &message)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
			
			// trova il manico dell'ultimo programma attivo in tutto Windows
			HWND programmaDavanti = GetForegroundWindow();
			if ( programmaDavanti != hWnd )
				ultimoProgrammaAttivo = programmaDavanti;
		}
	}
	return message.wParam;
}

