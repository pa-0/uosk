// Uosk - Funzioni del programma
#include "macro.h"

// legge un file, scopre se è codificato ascii, ansi, utf8 o unicode, e ne restituisce il contenuto
wchar_t* decodificaFile(wchar_t *nome) {
	HANDLE file = CreateFileW( nome, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
	DWORD pesoFile = GetFileSize(file, NULL);	// numero dei byte incluso il BOM
	char test[pesoFile+1];
	DWORD caratteriLetti;
	ReadFile(file, test, pesoFile, &caratteriLetti, NULL);
	test[caratteriLetti] = 0;
	char* testo = test + lunghezzaBOM(nome);
	caratteriLetti -= lunghezzaBOM(nome);
	static wchar_t* testoW;
	testoW = malloc( (pesoFile+1) * sizeof(wchar_t) );
	testoW[0] = 0;
	int caratteriConvertiti;
	// Utf-16BE
	if( (test[0]&0xFF) == 0xFE && (test[1]&0xFF) == 0xFF ) {
		SetFilePointer(file, lunghezzaBOM(nome), NULL, FILE_BEGIN);
		ReadFile(file, testoW, pesoFile, &caratteriLetti, NULL);
		for( int c=0; c<caratteriLetti/2; c++ )
			testoW[c] = testoW[c] << 8 | testoW[c] >> 8;
		testoW[caratteriLetti/2] = 0;
	// Utf-16LE
	} else if( IsTextUnicode(test, pesoFile, NULL) ) {
		SetFilePointer(file, lunghezzaBOM(nome), NULL, FILE_BEGIN);
		ReadFile(file, testoW, pesoFile, &caratteriLetti, NULL);
		testoW[caratteriLetti/2] = 0;
	// asci/utf8
	} else if((caratteriConvertiti = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, testo, caratteriLetti, testoW, caratteriLetti+1))) {
		testoW[caratteriConvertiti] = 0;
	// ansi
	} else if((caratteriConvertiti = MultiByteToWideChar(CP_ACP,MB_ERR_INVALID_CHARS,testo, caratteriLetti, testoW, caratteriLetti+1))) {
		testoW[caratteriConvertiti] = 0;
	}
	CloseHandle(file);
	return testoW;
}

// riceve il puntatore a un testo e restituisce man mano gli snippet
unsigned int carat;
_Bool aCapo;
wchar_t *spezzaTesto( wchar_t *text ) {
	if( aCapo ) {
		aCapo = 0;
		return L"\n";
	}
	wchar_t *separatori = elencaSeparatori();
	wchar_t *inizioSnippet = 0;
	verificaCarattere:
	while( text[carat] ) {
		// individua gli acapi
		if( text[carat]=='\r' && text[carat+1]=='\n' ) {
			text[carat] = 0;
			carat += 2;
			if( inizioSnippet ) {
				aCapo = 1;
				return inizioSnippet;
			} else
				return L"\n";
		}
		// restituisce il singolo carattere
		if( okMosaico ) {
			static wchar_t glifo[3];
			glifo[0] = text[carat];
			if( text[carat]>=0xD800 && text[carat]<=0xDBFF) {
				glifo[1] = text[carat+1];
				glifo[2] = 0;
				carat++;
			} else 
				glifo[1] = 0;
			carat++;
			return glifo;
		}
		// azzera i separatori
		for( int s=0; s<wcslen(separatori); s++ ) {
			if( text[carat] == separatori[s] ) {
				text[carat] = 0;
				carat++;
				if( inizioSnippet )
					return inizioSnippet;
				else
					goto verificaCarattere;
			}
		}
		if( !inizioSnippet )
			inizioSnippet = &text[carat];
		carat++;
	}
	if( inizioSnippet )
		return inizioSnippet;
	else {
		carat = 0;
		return NULL;
	}
}

// crea i bottoni prendendo il testo dall'Editore, restituisce il numero di bottoni creati
int creaBottoni() {
	// prima di creare svuota la tastiera
	svuotaTastiera();
	// prende il testo da Editore
	int num = GetWindowTextLength(hEditore);
	wchar_t *testo = malloc( (num+1) * sizeof(wchar_t) );
	GetWindowTextW( hEditore, testo, num+1 );
	// definizione di un po' di variabili
	wchar_t *snippet;
	RECT rectSnippet;
	POINT *punti;
	BYTE *tipo;
	int x = 0;
	if(das) x = size.clienteLargo;
	unsigned int quantiPunti,
		largo,
		alto = 30,
		numRighe = 0,
		numSnippet = 0;

	HDC hDc = GetDC( hWindow );
	caricaFont( L"keyboard" );
	HFONT hFontBottoni = CreateFontIndirectW( &lf );
	SelectObject( hDc, hFontBottoni );

	// passa tutto il testo
	riparti:
	snippet = spezzaTesto( testo );
	while( snippet ) {
		if( wcscmp(snippet,L"\n") == 0 ) {
			if( das ) {
				if( size.clienteLargo-x > size.pulsaLarga )
					size.pulsaLarga = size.clienteLargo-x;
				si.nMax += (x==size.clienteLargo) ? alto/2 : alto;
				x = size.clienteLargo;
			} else {
				if( x > size.pulsaLarga )
					size.pulsaLarga = x;
				si.nMax += !x ? alto/2 : alto;
				x = 0;
			}
			numRighe++;
			goto riparti;
		}
		// specifica dimensioni del bottone adattate al testo dello snippet
		DrawTextW( hDc, snippet, -1, &rectSnippet, DT_CALCRECT|DT_NOPREFIX );
		alto = rectSnippet.bottom-rectSnippet.top + 10;
		// larghezza del bottone
		if( rectSnippet.right-rectSnippet.left == 0 ) {
			int puntoMin = 100;
			int puntoMax = -100;
			BeginPath( hDc );
			SetBkMode( hDc, TRANSPARENT );
			TextOutW( hDc, 0, 0, snippet, wcslen(snippet) );
			EndPath( hDc );
			FlattenPath( hDc );
			quantiPunti = GetPath( hDc, NULL, NULL, 0 );
			if( quantiPunti > 0 ) {
				punti = malloc(sizeof(POINT) * quantiPunti);
				tipo = malloc(sizeof(BYTE) * quantiPunti);
				GetPath( hDc, punti, tipo, quantiPunti );
				for( int p=0; p<quantiPunti; p++ ) {
					if( punti[p].x < puntoMin )
						puntoMin = punti[p].x;
					if( punti[p].x > puntoMax )
						puntoMax = punti[p].x;
				}
			}
			AbortPath( hDc );
			largo = max( puntoMax - puntoMin, 10 ) + 12;
			if( snippi ) {
				snippi[numSnippet].minimo = puntoMin;
				snippi[numSnippet].massimo = puntoMax;
			}
		} else
			largo = max( rectSnippet.right-rectSnippet.left, 10 ) + 12;
		// calcola la posizione orizzontale x e verticale si.nMax
		if( das ) {
			x -= largo;
			if( x < 0 ) {	// a capo arabo: snippet sborda a sinistra
				if( size.clienteLargo-x > size.pulsaLarga )
					size.pulsaLarga = size.clienteLargo-x-largo;
				x = size.clienteLargo-largo;
				si.nMax += alto;
			}
		// se il pulsante sborda a destra della finestra cliente viene messo a capo
		// e se non si tratta di un unico bottone lungo
		} else if( x+largo>size.clienteLargo && x>0 )  {
			if( x > size.pulsaLarga )
				size.pulsaLarga = x;
			x = 0;
			si.nMax += alto;
		}
		// crea bottone
		HWND hBotto = CreateWindowW( L"Button", snippet, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
				x, si.nMax, largo, alto, hTastiera, (HMENU)ID_BOTTONE_SNIPPET, GetModuleHandle(NULL), NULL );
		if( snippi ) {
			snippi[numSnippet].manico = hBotto;
			snippi[numSnippet].caratteri = wcslen(snippet);
			snippi[numSnippet].larghezza = rectSnippet.right-rectSnippet.left;
		}
		if(!das) x += largo;
		numSnippet++;
		snippet = spezzaTesto( testo );
	}
	free( testo );
	ReleaseDC( hWindow, hDc );
	DeleteObject( hFontBottoni );
	si.nMax += alto - 1;
	SetScrollInfo( hTastiera, SB_VERT, &si, TRUE );
	// scrive nella barra di stato numero dei bottoni creati
	char snippets[6];
	sprintf( snippets, "\t%d", numSnippet);
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)&snippets);
	return numSnippet;
}

// crea due volte i bottoni per sapere se compare la barra e quindi mandarli a capo, e per allocare l'array snippi
void disponiBottoni() {
	if (!okEdita && nomeFile[0]) {
		ShowWindow(hTastiera, SW_HIDE);
		RECT rect;
		snippi = NULL;
		GetClientRect(hWindow,&rect);
		size.clienteLargo = rect.right;
		int quanti = creaBottoni();
		snippi = malloc( quanti * sizeof(struct unoSnippo) );
		if(si.nMax >= si.nPage)
			size.clienteLargo -= GetSystemMetrics(SM_CXVSCROLL);
		creaBottoni();
		si.fMask = SIF_POS;
		GetScrollInfo( hTastiera, SB_VERT, &si );
		si.fMask = SIF_ALL;
		ScrollWindow( hTastiera, 0, -si.nPos, NULL, NULL );
		ShowWindow(hTastiera, SW_SHOW);
	}
}

// aggiorna lista file recenti nel menu e in config.ini
void aggiornaFileRecenti() {
	int i;
	wchar_t nomePath[8];
	wchar_t fileRecente[MAX_PATH];
	HMENU hSubMenu = GetSubMenu( GetSubMenu(GetMenu(hWindow),0), 2 );
	int posizioneFileAttuale = 5;
	for(i=0; i<5; i++) {
		snwprintf( nomePath, conta(nomePath), L"recent%d", i );
		GetPrivateProfileStringW( L"file", nomePath, L"", fileRecente, MAX_PATH, config_ini);
		if( wcscmp(fileRecente, nomeFile)==0 )
			posizioneFileAttuale = i;
		RemoveMenu( hSubMenu, 0, MF_BYPOSITION );
	}
	for(i=3; i>=0; i--) {
		if ( i < posizioneFileAttuale ) {
			snwprintf( nomePath, conta(nomePath), L"recent%d", i );
			GetPrivateProfileStringW( L"file", nomePath, L"", fileRecente, MAX_PATH, config_ini);
			snwprintf( nomePath, conta(nomePath), L"recent%d", i+1 );
			WritePrivateProfileStringW( L"file", nomePath, fileRecente, config_ini);
		}
	}
	WritePrivateProfileStringW( L"file", L"recent0", nomeFile, config_ini);
	for(i=0; i<5; i++) {
		snwprintf( nomePath, conta(nomePath), L"recent%d", i );
		GetPrivateProfileStringW( L"file", nomePath, L"", fileRecente, MAX_PATH, config_ini );
		if( fileRecente[0] )
			InsertMenuW( hSubMenu, -1, MF_BYPOSITION|MF_STRING, MENU_FILE_RECENTI+i, fileRecente);
	}
}

void apriFile() {
	// se il file non esiste o non ha permesso di lettura messaggio di errore
	wchar_t messaggio[MAX_PATH+50];
	_Bool continua = 0;
	if( fileDaAprire[0] ) {
		if( _waccess(fileDaAprire,F_OK) == -1 ) {
			if( apriDaConfig_ini ) {
				snwprintf(messaggio,conta(messaggio),L"Can't find the file opened last time %s",fileDaAprire);
				MessageBoxW(hWindow, messaggio, L"Uosk", MB_ICONWARNING);
				nomeFile[0] = 0;
			} else {
				snwprintf(messaggio,conta(messaggio),L"Can't find the file %s",fileDaAprire);
				MessageBoxW(hWindow, messaggio, L"Uosk", MB_ICONWARNING);
			}
		} else if( _waccess(fileDaAprire,R_OK) == -1 ) {	// il file non ha permesso di lettura
			snwprintf(messaggio,conta(messaggio),L"I can't read the file %s",fileDaAprire);
			MessageBoxW(hWindow, messaggio, L"Uosk", MB_ICONWARNING);
		} else if( GetFileAttributesW(fileDaAprire) != FILE_ATTRIBUTE_DIRECTORY ) {
			wcscpy(nomeFile, fileDaAprire);
			continua = 1;
		}
	}
	apriDaConfig_ini = 0;
	fileDaAprire[0] = 0;
	if(!continua) return;
	
	SetWindowTextW( hEditore, decodificaFile(nomeFile) );
	if(okEdita) {
		contaCaratteri();
	} else {
		snippi = NULL;
		si.nPos = 0;
		RECT rect;
		GetClientRect(hWindow,&rect);
		size.clienteLargo = rect.right;
		int quanti = creaBottoni();
		snippi = malloc( quanti * sizeof(struct unoSnippo) );
		if(si.nMax >= si.nPage)
			size.clienteLargo -= GetSystemMetrics(SM_CXVSCROLL);
		creaBottoni();
		ShowWindow(hTastiera, SW_SHOW);
		SetWindowPos(hWindow, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	}
	ShowWindow( hFrontalino, SW_HIDE );
	SetFocus( hTastiera );

	// modifica titolo finestra e nome file in statusbar
	wchar_t* nome = wcsrchr(nomeFile, '\\') + 1; 
	SetWindowTextW(hWindow, nome);
	scriviNomeFile();

	// abilita bottoni menu
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_EDITA, MF_ENABLED);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVACOME, MF_ENABLED);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_CHIUDI, MF_ENABLED);
	bottoniDestra();
	aggiornaFileRecenti();
}

// scopre se il file ha il BOM e ne restituisce il numero di caratteri
int lunghezzaBOM(wchar_t *nomeFile) {
	FILE* file = _wfopen( nomeFile, L"r" );
	char testo[4];
	fgets(testo, 4, file);
	fclose(file);
	int num = 0;
	if( testo[0]==(char)0xEF && testo[1]==(char)0xBB && testo[2]==(char)0xBF )
		num = 3;
	else if( testo[0]==(char)0xFF && testo[1]==(char)0xFE )
		num = 2;
	else if( testo[0]==(char)0xFE && testo[1]==(char)0xFF )
		num = 2;
	return num;
}

// prende i separatori da config.ini e li assembla in una stringa da restituire a creaBottoni()
wchar_t * elencaSeparatori() {
	static wchar_t stringaSeparatori[50];
	GetPrivateProfileStringW( L"options", L"cutter3", L"", stringaSeparatori, sizeof(stringaSeparatori), config_ini );
	int tot = wcslen(stringaSeparatori);
	if ( GetPrivateProfileIntW( L"options", L"cutter1", 0, config_ini ) ) {
		stringaSeparatori[tot] = ' ';
		tot++;
	}
	if ( GetPrivateProfileIntW( L"options", L"cutter2", 0, config_ini ) ) {
		stringaSeparatori[tot] = '	';
		tot++;
	}
	stringaSeparatori[tot] = 0;
	return stringaSeparatori;
}

/*// raddoppia il carattere '&' che non compare nei bottoni
wchar_t* escapaAnd(wchar_t* snip) {
	int lunga = wcslen(snip);
	static wchar_t bis[1];	// NON HA SENSO: come può il testo lungo stare nello spazio previsto di un carattere?? 
	for(int i=0,ib=0;i<=lunga;i++) {
		bis[ib] = snip[i];
		if (snip[i]=='&') {
			ib++;
			bis[ib] = '&';
		}
		ib++;
	}
	return bis;
}

// Riceve il puntatore a una stringa e la modifica dimezzando i doppi '&&'
// Restituisce il numero di code points (singoli glifi)
int disescapaAnd( wchar_t *snip ) {
	int glifi = -1;
	for(int i=0,ip=0;i<=wcslen(snip);i++) {
		snip[i] = snip[ip];
		if (snip[i]=='&')
			ip++;
		ip++;
		if (snip[i]<0xDC00 || snip[i]>0xDFFF)
			glifi++;
	}
	return glifi;
}*/

// Riceve il puntatore a una stringa e restituisce il numero di code points (singoli glifi)
int contaGlifi( wchar_t *snip ) {
	int numGlifi = 0;
	for( int i=0; i<wcslen(snip); i++ ) {
		if( snip[i]<0xDC00 || snip[i]>0xDFFF )
			numGlifi++;
	}
	return numGlifi;
}

// accorcia l'indirizzo lungo e lo scrive nella status bar
void scriviNomeFile() {
	HDC hDc = GetDC(hStatusBar);
	HFONT defaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(hDc, defaultFont);
	SIZE dimensioniPath;
	GetTextExtentPoint32W( hDc, nomeFile, wcslen(nomeFile), &dimensioniPath );
	int larghezzaPath = dimensioniPath.cx * 1.13;
	ReleaseDC(hStatusBar, hDc);
	if( larghezzaPath > size.statusSecondoLargo) {
		float mezziCaratteri = (float)wcslen(nomeFile) / larghezzaPath * size.statusSecondoLargo / 2 -1;
		if (mezziCaratteri < 0) mezziCaratteri = 0;
		wchar_t testa[MAX_PATH/2];
		wcsncpy(testa, nomeFile, mezziCaratteri);
		testa[(int)mezziCaratteri] = 0;
		wchar_t *coda = nomeFile + wcslen(nomeFile)-(int)mezziCaratteri;
		wchar_t nomeTroncato[MAX_PATH];
		snwprintf( nomeTroncato, conta(nomeTroncato), L"%s...%s", testa, coda );
		SendMessage( hStatusBar, SB_SETTEXTW, 1, (LPARAM)nomeTroncato );
	} else
		SendMessage( hStatusBar, SB_SETTEXTW, 1, (LPARAM)nomeFile );
}

// ricava il nome del programma da ultimoProgrammaAttivo e lo scrive nella barra di stato
void scriviNomeProgramma() {
	char nomeProgramma[50] = {0};
	long unsigned int idProcesso;
	GetWindowThreadProcessId( ultimoProgrammaAttivo, &idProcesso);
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, idProcesso );
	// percorso completo del modulo di un processo
	char percorsoCompleto[MAX_PATH] = "";
	GetModuleFileNameEx( hProcess, NULL, percorsoCompleto, sizeof(percorsoCompleto) );
	// nome del programma da File Version Info
	DWORD versz = GetFileVersionInfoSize(percorsoCompleto, NULL);
	if (versz) {
		void *fileVersionInfo = malloc(versz);
		WORD *translations;
		unsigned int puLen;
		void *lplpBuffer;
		if (GetFileVersionInfo( percorsoCompleto, 0, versz, fileVersionInfo)) {
			VerQueryValue( fileVersionInfo, "\\VarFileInfo\\Translation", (void**)&translations, &puLen);
			char stringa[60] = {0};
			sprintf(stringa, "\\StringFileInfo\\%04x%04x\\InternalName", translations[0], translations[1]);
			if( VerQueryValue( fileVersionInfo, stringa, &lplpBuffer, &puLen) ) {
				sprintf(nomeProgramma,"\t%s",lplpBuffer);
			}
		}
		free(fileVersionInfo);
	} else {	// nome alternativo dal modulo eseguibile
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
		Process32First(snapshot,&pe32);
        while ( Process32Next(snapshot,&pe32) ) {
			if( pe32.th32ProcessID == idProcesso ) {
				// elimina l'estensione del nome del file
				char *estensione = strrchr(pe32.szExeFile, '.');
				estensione[0] = 0;
				sprintf(nomeProgramma, "\t%s", pe32.szExeFile);
				break;
			}
		}
	}
	CloseHandle( hProcess );
	SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)nomeProgramma);
}

BOOL CALLBACK proceduraDialogoIstruzione(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:
			settaFontSistema(hWnd);
			break;
		case WM_CTLCOLORSTATIC: {
			// immagine istruttiva
			HBITMAP hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(ID_IMMAGINE));
			HDC hDc = GetDC(hWnd);
			HDC hdcMem = CreateCompatibleDC(hDc);
			BITMAP bm;
			GetObject(hBitmap, sizeof(bm), &bm);
			SelectObject(hdcMem, hBitmap);
			TransparentBlt(hDc, 30,55, bm.bmWidth,bm.bmHeight, hdcMem, 0,0, bm.bmWidth,bm.bmHeight, RGB(0,255,0));
			// sfondo trasparente del testo
			HDC hDcTesto = (HDC)wParam;
			SetBkMode( hDcTesto, TRANSPARENT );
			return (LRESULT)GetStockObject(HOLLOW_BRUSH);
		  }
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK proceduraTastiera(HWND hTastier, UINT Message, WPARAM wParam, LPARAM lParam) {
	switch (Message) {
		case WM_DRAWITEM: {	// disegno personalizzato dei bottoni
			DRAWITEMSTRUCT *dis = (DRAWITEMSTRUCT *)lParam;
			// disegna rettangolo di sfondo
			HPEN hPenna = CreatePen( PS_SOLID, 1, GetSysColor(dis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_BTNSHADOW) );
			HBRUSH hPennello = CreateSolidBrush( dis->itemState & ODS_SELECTED ? RGB(225,225,225) : GetSysColor(COLOR_3DFACE) );
			SelectObject( dis->hDC, hPenna );
			SelectObject( dis->hDC, hPennello );
			RECT rc = dis->rcItem;
			rc.top++;
			rc.left++;
			rc.bottom--;
			rc.right--;
			Rectangle( dis->hDC, rc.left, rc.top, rc.right, rc.bottom );
			int s = 0;
			while( snippi[s].manico != NULL ) {
				if( snippi[s].manico == dis->hwndItem )
					break;
				s++;
			}
			// scrive il testo
			wchar_t snippet[snippi[s].caratteri + 1];
			GetWindowTextW( dis->hwndItem, snippet, conta(snippet) );
			caricaFont(L"keyboard");
			HFONT hFontBottone = CreateFontIndirectW(&lf);
			SelectObject( dis->hDC, hFontBottone );
			int spostino = dis->itemState & ODS_SELECTED ? 1 : 0;
			rc = dis->rcItem;
			if( snippi[s].larghezza == 0 )
				rc.left += - snippi[s].massimo - snippi[s].minimo + spostino*2;
			else
				rc.left += spostino*2;
			rc.top += 5 + spostino;
			SetBkMode( dis->hDC,TRANSPARENT );
			DrawTextW( dis->hDC, snippet, wcslen(snippet), &rc, DT_CENTER|DT_NOPREFIX );
			// rettangolo intorno al bottone
			hPennello = CreateSolidBrush( GetSysColor(dis->itemState & ODS_SELECTED ? COLOR_HIGHLIGHT : COLOR_3DFACE) );
			FrameRect( dis->hDC, &dis->rcItem, hPennello );
			DeleteObject(hFontBottone);
			DeleteObject(hPenna);
			DeleteObject(hPennello);
			return TRUE;
		}
		case WM_VSCROLL: {	// utente interagisce con barra di scorrimento verticale
			int posVertIniz = si.nPos;
			switch (LOWORD (wParam)) {
				case SB_LINEUP:
					si.nPos -= 10;
					break;
				case SB_LINEDOWN:
					si.nPos += 10;
					break;
				case SB_PAGEUP:
					si.nPos -= si.nPage;
					break;
				case SB_PAGEDOWN:
					si.nPos += si.nPage;
					break;
				case SB_THUMBPOSITION:
					si.nPos = HIWORD(wParam);
					break;
				case SB_THUMBTRACK:
					si.nPos = HIWORD(wParam);
					break;
			}
			// aggiorna ScrollInfo e fa scorrere la pagina
			SetScrollInfo (hTastier, SB_VERT, &si, TRUE);
			GetScrollInfo (hTastier, SB_VERT, &si);
			ScrollWindow (hTastier, 0, posVertIniz-si.nPos, NULL, NULL);
			break;
		}
		case WM_CONTEXTMENU: {
			hSnippetFocus = (HWND)wParam;	// snippet su cui aprire la finestra Informazioni
			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION, MENU_INFO_SNIPPET, "Snippet Info");
			InsertMenu(hPopupMenu, 1, MF_BYPOSITION, MENU_FILE_EDITA, "Edit");
			InsertMenu(hPopupMenu, 2, MF_BYPOSITION | das, MENU_LETTURA_DAS, "Right to left Reading order");
			POINT pt;
			GetCursorPos(&pt);
			TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWindow, NULL);
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case ID_BOTTONE_SNIPPET: {
						
					// attiva la finestra di destinazione
					SetForegroundWindow(ultimoProgrammaAttivo);
					
					// recuperare il testo del bottone
					hSnippetFocus = (HWND)lParam;
					int lunga = GetWindowTextLength(hSnippetFocus);
					wchar_t capzione[lunga+1];
					GetWindowTextW(hSnippetFocus, capzione, lunga+1);

					if (okCopiaAppunti)
						copiaNegliAppunti(capzione);

					// Inserisce snippet nell'editore di testo
					INPUT input[lunga*2];
					for( int i=0; i<lunga*2; i++) {
						input[i].type = INPUT_KEYBOARD;
						input[i].ki.wScan = capzione[i/2];
						input[i].ki.wVk = 0;
						input[i].ki.time = 0;
						input[i].ki.dwExtraInfo = 0;
						if (i%2)
							input[i].ki.dwFlags = 0x0004 | KEYEVENTF_KEYUP;
						else
							input[i].ki.dwFlags = 0x0004;
					}
					int numCaratteriInviati = SendInput(lunga*2, input, sizeof(INPUT)) / 2;

					// scrive nella barra di stato numero di caratteri
					char quantiCaratteri[6];
					snprintf( quantiCaratteri, conta(quantiCaratteri), "\t%d", contaGlifi(capzione) );
					SendMessage( hStatusBar, SB_SETTEXT, 0, (LPARAM)&quantiCaratteri );
					// e stringa inserita
					wchar_t stringaInviata[numCaratteriInviati+1];
					for( int i=0; i<numCaratteriInviati; i++ )
						stringaInviata[i] = input[i*2].ki.wScan;
					stringaInviata[numCaratteriInviati]=0;
					SendMessage( hStatusBar, SB_SETTEXTW, 1, (LPARAM)&stringaInviata );
					if (!ultimoProgrammaAttivo)
						DialogBox( 0, "messaggioIstruttivo", hWindow, proceduraDialogoIstruzione );
					if (hInfo)
						SendMessage (hWindow, WM_COMMAND, MENU_INFO_SNIPPET, 0);
				}
			} break;
		default:
            return DefWindowProc(hTastier, Message, wParam, lParam); 
	}
	return 0;
}

// procedura finestra Informazioni sullo Snippet
LRESULT CALLBACK proceduraInfoSnippet(HWND hInf, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_SIZE: {
			si2.nPage = HIWORD(lParam);	// altezza finestra
			break;
		}
		case WM_VSCROLL : {
			int posVertIniz = si2.nPos;
			switch (LOWORD (wParam)) {
				case SB_LINEUP:
					si2.nPos -= 10;
					break;
				case SB_LINEDOWN:
					si2.nPos += 10;
					break;
				case SB_PAGEUP:
					si2.nPos -= si2.nPage;
					break;
				case SB_PAGEDOWN:
					si2.nPos += si2.nPage;
					break;
				case SB_THUMBPOSITION:
					si2.nPos = HIWORD(wParam);
					break;
				case SB_THUMBTRACK:
					si2.nPos = HIWORD(wParam);
					break;
			}
			SetScrollInfo (hInf, SB_VERT, &si2, TRUE);
			GetScrollInfo (hInf, SB_VERT, &si2);
			ScrollWindow (hInf, 0, posVertIniz-si2.nPos, NULL, NULL);
			break;
		}
		case WM_MOUSEWHEEL:
			if( si2.nMax >= si2.nPage ) {
				int posVertIniz = si2.nPos;
				si2.nPos -= GET_WHEEL_DELTA_WPARAM(wParam) / 3;
				SetScrollInfo (hInf, SB_VERT, &si2, TRUE);
				GetScrollInfo (hInf, SB_VERT, &si2);
				ScrollWindow (hInf, 0, posVertIniz-si2.nPos, NULL, NULL);
			}
			break;
		case WM_CLOSE:
			DestroyWindow(hInf);
			hInfo = 0;
			break;
		default:
			return DefWindowProc(hInf, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK proceduraInformino(HWND hInformin, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_CTLCOLORSTATIC:	// sfondo bianco del testo solo lettura
			if( (HWND)lParam == GetDlgItem(hInformin,ID_INFO_GLIFO) ) {
				return (LRESULT)( (HBRUSH)GetStockObject(WHITE_BRUSH) );
			}
		default:
			return DefWindowProc(hInformin, message, wParam, lParam);
	}
	return 0;
}

// procedura Testi in finestra Informazioni
LRESULT CALLBACK proceduraInfoTesto(HWND hTest, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	switch (message) {
		case WM_MOUSEWHEEL:
			SendMessage(hInfo, WM_MOUSEWHEEL, wParam, lParam);
			break;
		default:
			return DefSubclassProc(hTest, message, wParam, lParam);
	}
	return 0;
}

// avvia la finestra di dialogo Windows per aprire il file
void finestraDialogoApri() {
	if (!preservaModifiche())
		return;
	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = L"Text Files (txt)\0*.txt\0All Files (*)\0*.*\0";
	ofn.lpstrFile = fileDaAprire;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"txt";
	if( GetOpenFileNameW(&ofn) )
		apriFile();
}

// salva il file sovrascrivendolo
_Bool salvaFile() {
	int num = GetWindowTextLength(hEditore);
	wchar_t buffer[num+1];
	GetWindowTextW(hEditore, buffer, num+1);
	HANDLE hFile = CreateFileW( nomeFile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	char bufferUTF8[ WideCharToMultiByte( CP_UTF8, 0, buffer, -1, 0, 0, NULL, NULL ) + 3 ];
	WideCharToMultiByte( CP_UTF8, 0, buffer, -1, bufferUTF8, sizeof(bufferUTF8), NULL, NULL );
	// aggiunge il BOM Utf-8
	for( int i=sizeof(bufferUTF8)-4; i>=0; i-- )
		bufferUTF8[i+3] = bufferUTF8[i];
	bufferUTF8[0] = 0xEF;
	bufferUTF8[1] = 0xBB;
	bufferUTF8[2] = 0xBF;
	// salva il file
	DWORD byteScritti;
	if( !WriteFile(hFile, bufferUTF8, sizeof(bufferUTF8)-1, &byteScritti, NULL) ) {
		MessageBox( hWindow, "I can't save the file", "Uosk", MB_ICONSTOP );
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hFile);
	fileModificato = 0;
	wchar_t *nome = wcsrchr(nomeFile, '\\') + 1; 
	SetWindowTextW(hWindow, nome);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVA, MF_GRAYED);
	return 1;
}

// avvia la finestra di dialogo Windows per salvare il file
int finestraDialogoSalva() {
	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = L"Text Files (txt)\0*.txt\0All Files (*)\0*.*\0";
	ofn.lpstrFile = nomeFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;	//OFN_HIDEREADONLY
	ofn.lpstrDefExt = L"txt";
	int risposta;
	if( (risposta=GetSaveFileNameW(&ofn)) ) {
		if( (risposta=salvaFile()) ) {
			scriviNomeFile();
			aggiornaFileRecenti();
		}
	}
	return risposta;
}

// riceve una stringa widechar e la copia negli appunti di Windows 
void copiaNegliAppunti(wchar_t *copiando) {
	size_t sizeInBytes = wcslen(copiando) * sizeof(wchar_t);
	HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE, sizeInBytes + sizeof(wchar_t) );	
	wchar_t* testoCopiato = (wchar_t*)GlobalLock( hMem );
	memcpy( testoCopiato, copiando, sizeInBytes );
	GlobalUnlock(hMem);
	OpenClipboard(NULL);
	EmptyClipboard();
	SetClipboardData( CF_UNICODETEXT, hMem );
	CloseClipboard();
}	

// distugge i bottoni esistenti per crearne nuovi o per chiudere il file
void svuotaTastiera() {
	HWND snippet_esistente;
	while ( (snippet_esistente=GetDlgItem(hTastiera,ID_BOTTONE_SNIPPET)) ) {
		DestroyWindow(snippet_esistente);
	}
	
	// resetta dimensioni e posizione globali della pagina
	si.nMax = 0;
	size.pulsaLarga = 0;
	SetScrollInfo(hTastiera,SB_VERT,&si,1);
}

// chiede di salvare eventuali modifiche al testo
_Bool preservaModifiche() {
	_Bool continua = 1;
	if( fileModificato ) {
		wchar_t msg[MAX_PATH+20];
		if( wcscmp(nomeFile,L"untitled") )
			snwprintf( msg, conta(msg), L"Do you want to save %s?", wcsrchr(nomeFile,'\\')+1 );
		else
			wcscpy( msg, L"Do you want to save this new keyboard?" );
		int risposta = MessageBoxW( hWindow, msg, L"Uosk", MB_ICONWARNING | MB_YESNOCANCEL );
		switch (risposta) {
			case IDYES:
				if( wcscmp(nomeFile,L"untitled") == 0 )
					continua = finestraDialogoSalva();
				else
					continua = salvaFile();
				break;
			case IDNO:
				EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVA, MF_GRAYED);
				fileModificato = 0;
				break;
			case IDCANCEL:
				continua = 0;
		}
	}	
	return continua;
}

// aggiorna i bottoni a destra nel menu
void bottoniDestra() {
	RemoveMenu( GetMenu(hWindow), MENU_BOTTONE_RIDUCI, MF_BYCOMMAND );
	RemoveMenu( GetMenu(hWindow), MENU_BOTTONE_EDITA, MF_BYCOMMAND );
	RemoveMenu( GetMenu(hWindow), MENU_BOTTONE_CHIUDI, MF_BYCOMMAND );
	// crea gli array con le icone e coi tooltip 
	wchar_t ico[2][4][3] = {{ L"🗋" }, { L"☐" }};
	char tip[3][20] = { "New Keyboard" };
	if( nomeFile[0] ) {
		wcscpy( ico[0][0], L"✖" );
		wcscpy( ico[0][1], L"✔" );
		wcscpy( ico[1][0], L"X" );
		wcscpy( ico[1][1], L"V" );
		strcpy( tip[0], "Close Keyboard" );
		strcpy( tip[1], "Finish Editing" );
		if( !okEdita ) {
			wcscpy( ico[0][1], L"🖉" );
			wcscpy( ico[1][1], L"/" );
			strcpy( tip[1], "Edit Keyboard" );
			if( !massimizzata ) {
				wcscpy( ico[0][2], L"⊞" );
				wcscpy( ico[1][2], L"#" );
				strcpy( tip[2], "Adapt to Keyboard" );
		   }
		}
	}
	if( !conBarraTitolo ) {
		wcscpy( ico[0][0], L"✖" );
		wcscpy( ico[0][1], L"☐" );
		wcscpy( ico[0][2], L"—" );
		wcscpy( ico[1][0], L"X" );
		wcscpy( ico[1][1], L"☐" );
		wcscpy( ico[1][2], L"_" );
		strcpy( tip[0], "Close" );
		strcpy( tip[1], "Maximize" );
		strcpy( tip[2], "Minimize" );
	}
	// inserisce i bottoni e i tooltip
	int quanti = 0;
	while( ico[0][quanti][0] != 0 ) {
		quanti++;
	}
	for( int i=quanti-1; i>=0; i-- ) {
		InsertMenuW( GetMenu(hWindow), -1, MF_BYPOSITION|MF_HELP|MF_STRING, MENU_BOTTONE_CHIUDI+i, checkGlyphExist(ico[0][i],ico[1][i]) );
	}
	for( int i=0; i<3; i++ ) {
		ti.uId = MENU_BOTTONE_CHIUDI+i;
		ti.lpszText = tip[i];
		SendMessage( hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti );
		int pos = 0;
		while( GetMenuItemID(GetMenu(hWindow),pos)!=MENU_BOTTONE_CHIUDI+i && pos<6 )
			pos++;
		GetMenuItemRect( hWindow, GetMenu(hWindow), pos, &ti.rect );
		MapWindowPoints( NULL, hWindow, (POINT*)&ti.rect, 2 );
		SendMessage( hToolTip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti );
	}
	DrawMenuBar(hWindow);
}

void mostraNascondiBarraTitolo() {
	if (conBarraTitolo)
		SetWindowLong( hWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW|WS_VISIBLE );
	else
		SetWindowLong( hWindow, GWL_STYLE, WS_SIZEBOX|WS_VISIBLE );
	SetWindowPos( hWindow, 0,0,0,0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
	veroResize = 0;
	CheckMenuItem( GetMenu(hWindow), MENU_MOSTRA_TITOLO, conBarraTitolo );
	bottoniDestra();
}

void ordineLetturaDestraSinistra() {
	if (das) {
		SetWindowLong(hTastiera, GWL_EXSTYLE, WS_EX_LEFTSCROLLBAR);	// barra a sinistra per lettura da destra a sinistra
		SetWindowLong(hEditore, GWL_EXSTYLE, WS_EX_RIGHT|WS_EX_LEFTSCROLLBAR|WS_EX_RTLREADING);	// set completo per lettura da destra a sinistra
	} else {	
		SetWindowLong(hTastiera, GWL_EXSTYLE, 0);
		SetWindowLong(hEditore, GWL_EXSTYLE, 0);
	}
	CheckMenuItem (GetMenu(hWindow), MENU_LETTURA_DAS, das);
}

// chiude baracca e burattini
_Bool chiudiFile() {
	if (!preservaModifiche())
		return 0;
	svuotaTastiera();
	ShowWindow( hTastiera, SW_HIDE );
	ShowWindow( hEditore, SW_HIDE );
	ShowWindow( hFrontalino, SW_SHOW );
	okEdita = 0;
	// annulla nome del file aperto
	nomeFile[0] = 0;
	fileModificato = 0;

	// disabilita bottoni
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_EDITA, MF_GRAYED);
	CheckMenuItem(GetMenu(hWindow), MENU_FILE_EDITA, 0);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVA, MF_GRAYED);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVACOME, MF_GRAYED);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_CHIUDI, MF_GRAYED);
	bottoniDestra();
	
	// cancella testo in status bar
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)"");
	SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)"");
	
	SetWindowText(hWindow, "Uosk");
	SetWindowPos(hWindow, HWND_NOTOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	return 1;
}

// salva le impostazioni e chiude il programma
void chiudiProgramma() {
	if (!preservaModifiche())
		return;
	// prima di chiudere salva le dimensioni finestra in config.ini
	if( !IsIconic(hWindow) ) {
		RECT wndRect;
		GetWindowRect( hWindow, &wndRect );
		wchar_t dimensioni[50] = {0};
		snwprintf( dimensioni, conta(dimensioni), L"left=%d%ctop=%d%cright=%d%cbottom=%d", 
				(int)wndRect.left,0, (int)wndRect.top,0, (int)wndRect.right,0, (int)wndRect.bottom );
		WritePrivateProfileSectionW( L"size", dimensioni, config_ini );
	}
	if( wcscmp(nomeFile, L"untitled") == 0 )
		nomeFile[0] = 0;
	// scrive in confing.ini nome del file aperto 
	if( !WritePrivateProfileStringW( L"file", L"openfile", nomeFile, config_ini ) ) {
		wchar_t msg[MAX_PATH+35];
		snwprintf( msg, conta(msg), L"I can't save the program settings in %s", config_ini);
		MessageBoxW( hWindow, msg, L"Uosk", MB_ICONERROR );
	}
	DestroyWindow(hWindow);
}

// prende le impostazioni del font da config.ini
void caricaFont(wchar_t *utente) {
	lf.lfHeight = GetPrivateProfileIntW( utente, L"height", 0, config_ini );
	lf.lfWeight = GetPrivateProfileIntW( utente, L"weight", 0, config_ini );
	lf.lfItalic = GetPrivateProfileIntW( utente, L"italic", 0, config_ini );
	lf.lfCharSet = GetPrivateProfileIntW( utente, L"charset", 0, config_ini );
	GetPrivateProfileStringW( utente, L"face", L"", lf.lfFaceName, sizeof(lf.lfFaceName), config_ini );
}

// prende il font di sistema dei Message Box e lo impone a una finestra e a tutti i suoi figli
BOOL CALLBACK proceduraEnumChild( HWND hWnd, LPARAM lParam) {
	SendMessage( hWnd, WM_SETFONT, (WPARAM)lParam, TRUE );
	return TRUE;
}
HFONT settaFontSistema(HWND hWnd) {
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0 );
	static HFONT hFont;
	hFont = CreateFontIndirect( &(ncm.lfMessageFont) );
	if( hWnd != 0 ) {
		SendMessage( hWnd, WM_SETFONT, (WPARAM)hFont, TRUE );
		EnumChildWindows( hWnd, proceduraEnumChild, (WPARAM)hFont);
	}
	return hFont;
}

// procedura della finestra di dialogo Preferenze
BOOL CALLBACK proceduraDialogoPreferenze(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:	{
			settaFontSistema(hWnd);
			// prendende i valori da config.ini e li setta nella finestra dialogo 
			PostMessage( GetDlgItem(hWnd,CHECKBOX_SPAZI), BM_SETCHECK, 
						 GetPrivateProfileIntW(L"options",L"cutter1",0,config_ini), 0);
			PostMessage ( GetDlgItem(hWnd,CHECKBOX_TAB), BM_SETCHECK,
						  GetPrivateProfileIntW(L"options",L"cutter2",0,config_ini), 0);
			wchar_t separatori[50];
			GetPrivateProfileStringW( L"options", L"cutter3", L"", separatori, sizeof(separatori), config_ini );
			SetWindowTextW( GetDlgItem(hWnd,EDITTEXT_ALTRI), separatori );
			PostMessage ( GetDlgItem(hWnd,CHECKBOX_MOSAICO), BM_SETCHECK,
						  GetPrivateProfileIntW(L"options",L"nocutter",0,config_ini), 0);
			PostMessage(hWnd,WM_COMMAND,CHECKBOX_MOSAICO,0);
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case CHECKBOX_MOSAICO:	// disabilita gli altri comandi
					if ( SendDlgItemMessage(hWnd,CHECKBOX_MOSAICO,BM_GETCHECK,0,0) == BST_CHECKED) {
						okMosaico = 1;
						EnableWindow( GetDlgItem(hWnd,CHECKBOX_SPAZI), FALSE);
						EnableWindow( GetDlgItem(hWnd,CHECKBOX_TAB), FALSE);
						EnableWindow( GetDlgItem(hWnd,EDITTEXT_ALTRI), FALSE);
					} else {
						okMosaico = 0;
						EnableWindow( GetDlgItem(hWnd,CHECKBOX_SPAZI), TRUE);
						EnableWindow( GetDlgItem(hWnd,CHECKBOX_TAB), TRUE);
						EnableWindow( GetDlgItem(hWnd,EDITTEXT_ALTRI), TRUE);
					}
					break;
				case IDOK: {	// chiude la finestra di dialogo e salva le preferenze
					wchar_t separatore[50];
					snwprintf( separatore, conta(separatore), L"%d", SendDlgItemMessage(hWnd,CHECKBOX_SPAZI,BM_GETCHECK,0,0) );
					WritePrivateProfileStringW( L"options", L"cutter1", separatore, config_ini );
					snwprintf( separatore, conta(separatore), L"%d", SendDlgItemMessage(hWnd,CHECKBOX_TAB,BM_GETCHECK,0,0) );
					WritePrivateProfileStringW( L"options", L"cutter2", separatore, config_ini );
					GetWindowTextW( GetDlgItem(hWnd,EDITTEXT_ALTRI), separatore, conta(separatore) );
					WritePrivateProfileStringW( L"options", L"cutter3", separatore, config_ini );
					snwprintf( separatore, conta(separatore), L"%d", SendDlgItemMessage(hWnd,CHECKBOX_MOSAICO,BM_GETCHECK,0,0) );
					WritePrivateProfileStringW( L"options", L"nocutter", separatore, config_ini );
					EndDialog( hWnd, IDOK );
					disponiBottoni();
					break;
				}
				case IDCANCEL:
					EndDialog( hWnd, IDCANCEL );
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}


// procedura di elaborazione dei messaggi che riguardano la finestra 'Informazioni'
BOOL CALLBACK proceduraDialogoInformazioni(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:
			settaFontSistema(hWnd);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				/*case ID_LINK:
					ShellExecute(0, 0, "https://sourceforge.net/projects/uosk/", 0, 0 , SW_SHOW);
					break;*/
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
			}
			break;
		case WM_NOTIFY:
			switch (((LPNMHDR)lParam)->code) {
				case NM_CLICK:
				case NM_RETURN: {
					LITEM item = ((PNMLINK)lParam)->item;
					ShellExecuteW(0, L"open", item.szUrl, 0, 0, SW_SHOW);
					break;
				}
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

// controlla se il primo carattere della stringa esiste nei font di sistema se no restituisce il secondo
wchar_t* checkGlyphExist( wchar_t *sUnicode, wchar_t *sLimited ) {
	HDC hdc = GetDC( hWindow );
	HDC metaFileDC = CreateEnhMetaFile( hdc, NULL, NULL, NULL );
	NONCLIENTMETRICSW ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfoW( SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0 );
	HFONT hFont = CreateFontIndirectW( &(ncm.lfMenuFont) );
	SelectObject( metaFileDC, hFont );
	SCRIPT_STRING_ANALYSIS ssa;
	ScriptStringAnalyse( metaFileDC, sUnicode, wcslen(sUnicode), 0, -1,
					  SSA_METAFILE | SSA_FALLBACK | SSA_GLYPHS | SSA_LINK,	
					  0, NULL, NULL, NULL, NULL, NULL, &ssa );
	ScriptStringFree( &ssa );
	HENHMETAFILE metaFile = CloseEnhMetaFile(metaFileDC);
	LOGFONTW logFont = {0};
	logFont.lfFaceName[0] = 0;
	EnumEnhMetaFile( 0, metaFile, metaFileEnumProc, &logFont, NULL );
	DeleteEnhMetaFile( metaFile );
	hFont = CreateFontIndirectW( &logFont );
	SelectObject( hdc, hFont );
	GCP_RESULTSW infoStr = {0};
	infoStr.lStructSize = sizeof(GCP_RESULTSW);
	wchar_t tempStr[wcslen(sUnicode)];	
	wcscpy( tempStr, sUnicode );
	infoStr.lpGlyphs = tempStr;
	infoStr.nGlyphs = wcslen(tempStr);
	GetCharacterPlacementW( hdc, tempStr, wcslen(tempStr), 0, &infoStr, GCP_GLYPHSHAPE );
	DeleteObject(hFont);
	ReleaseDC( hWindow, hdc );
	if( infoStr.lpGlyphs[0] == 3 ||	infoStr.lpGlyphs[0] == 0 ) {
		return sLimited;
	}
	return sUnicode;
}
// Callback function to intercept font creation
int CALLBACK metaFileEnumProc( HDC hdc, HANDLETABLE *table, const ENHMETARECORD *record, int tableEntries, LPARAM logFont ) {
	if( record->iType == EMR_EXTCREATEFONTINDIRECTW ) {
		const EMREXTCREATEFONTINDIRECTW* fontRecord = (const EMREXTCREATEFONTINDIRECTW *)record;
		*(LOGFONTW *)logFont = fontRecord->elfw.elfLogFont;
	}
	return 1;
}