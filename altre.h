// Uosk - Funzioni del programma

void bottoneDestra();
void caricaFont(char*);
void copiaNegliAppunti(wchar_t*);
wchar_t* elencaSeparatori();
wchar_t* escapaAnd(wchar_t*);
int lunghezzaBOM(char*);
_Bool preservaModifiche();
void scriviNomeFile();
void settaFontSistema(HWND);
void svuotaTastiera();
#define conta(array) sizeof array / sizeof(array[0])

// legge un file, scopre se è codificato ascii, ansi, utf8 o unicode, e ne restituisce il contenuto
wchar_t* decodificaFile(char *nome) {
	HANDLE file = CreateFile( nome, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
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
	// unicode
	if ( IsTextUnicode(test, pesoFile, NULL) ) {
		SetFilePointer(file, lunghezzaBOM(nome), NULL, FILE_BEGIN);
		DWORD caratteriLettiW;
		ReadFile(file, testoW, pesoFile, &caratteriLettiW, NULL);
		testoW[caratteriLettiW/2] = 0;
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

// crea i bottoni dal testo preso in editore
void creaBottoni() {
	svuotaTastiera();
	int num = GetWindowTextLength(hEditore);
	wchar_t testo[num+1];
	GetWindowTextW(hEditore, testo, num+1);
	SIZE dimensioniSnippet;
	unsigned int
		x = 0,
		largo,
		alto = 30,
		numSnippet = 0;
	HDC hDc = GetDC(hWindow);
	caricaFont("keyboard");
	HFONT hFontBottoni = CreateFontIndirect(&lf);
	SelectObject(hDc, hFontBottoni);
	// crea l'array con le righe
	wchar_t **righe;
	unsigned int r = 0;
	unsigned int c = 0;
	righe = calloc(1,sizeof(wchar_t*));
	righe[r] = calloc(1,sizeof(wchar_t));
	for(unsigned int i=0; i<wcslen(testo); i++) {
		righe[r][c] = testo[i];
		c++;
		righe[r] = realloc(righe[r], (c+1) * sizeof(wchar_t));
		if( testo[i]=='\r' && testo[i+1]=='\n' ) {
			righe[r][c-1] = 0;
			r++;
			c = 0;
			i++;
			righe = (wchar_t**)realloc(righe, (r+1) * sizeof(wchar_t*));
			righe[r] = calloc(1,sizeof(wchar_t));
		}
		
	}
	righe[r][c] = 0;
	// creazione bottoni
	unsigned int numRighe = 0;
	while ( numRighe <= r ) {
		wchar_t *snippet = wcstok( righe[numRighe], elencaSeparatori() );
		while (snippet) {
			GetTextExtentPoint32W(hDc, snippet, wcslen(snippet), &dimensioniSnippet);
			largo = dimensioniSnippet.cx + 12;	
			alto = dimensioniSnippet.cy + 10;
			if( x+largo>size.clienteLargo && x>0 ) {
				if( x > size.pulsaLarga )
					size.pulsaLarga = x;
				x = 0;
				si.nMax += alto;
			}
			HWND hBotton = CreateWindowW( L"Button", escapaAnd(snippet), WS_CHILD | WS_VISIBLE, x, si.nMax,
			               largo, alto, hTastiera, (HMENU)ID_BOTTONE_SNIPPET, GetModuleHandle(NULL), NULL);
			SendMessage(hBotton, WM_SETFONT, (WPARAM)hFontBottoni, 0);
			x += largo;
			numSnippet++;
			snippet = wcstok (NULL, elencaSeparatori() );
		}
		numRighe++;
		if( x > size.pulsaLarga )
			size.pulsaLarga = x;
		si.nMax += !x ? alto/2 : alto;
		x = 0;
	}
	ReleaseDC(hWindow, hDc);
	free(righe);
	si.nMax -= 1;
	SetScrollInfo (hTastiera, SB_VERT, &si, TRUE);
	// scrive nella barra di stato numero dei bottoni creati
	char snippets[6];
	sprintf( snippets, "\t%d", numSnippet);
	SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)&snippets);
}

void disponiBottoni() {
	if (!okEdita && nomeFile[0]) {
		ShowWindow(hTastiera, SW_HIDE);
		RECT rect;
		GetClientRect(hWindow,&rect);
		size.clienteLargo = rect.right;
		creaBottoni();
		if(si.nMax >= si.nPage)
			size.clienteLargo -= GetSystemMetrics(SM_CXVSCROLL);
		creaBottoni();
		GetScrollInfo( hTastiera, SB_VERT, &si );
		ScrollWindow( hTastiera, 0, -si.nPos, NULL, NULL );
		ShowWindow(hTastiera, SW_SHOW);
	}
}

// aggiorna lista file recenti nel menu e in config.ini
void aggiornaFileRecenti() {
	int i;
	char nomePath[8];
	char fileRecente[MAX_PATH];
	HMENU hSubMenu = GetSubMenu( GetSubMenu(GetMenu(hWindow),0), 2 );
	int posizioneFileAttuale = 5;
	for(i=0; i<5; i++) {
		sprintf(nomePath,"recent%d", i );
		GetPrivateProfileString( "file", nomePath, "", fileRecente, MAX_PATH, config_ini);
		if(strcmp(fileRecente, nomeFile)==0)
			posizioneFileAttuale = i;
		RemoveMenu( hSubMenu, 0, MF_BYPOSITION );
	}
	for(i=3; i>=0; i--) {
		if ( i < posizioneFileAttuale ) {
			sprintf(nomePath,"recent%d", i );
			GetPrivateProfileString( "file", nomePath, "", fileRecente, MAX_PATH, config_ini);
			sprintf(nomePath,"recent%d", i+1 );
			WritePrivateProfileString( "file", nomePath, fileRecente, config_ini);
		}
	}
	WritePrivateProfileString( "file", "recent0", nomeFile, config_ini);
	for(i=0; i<5; i++) {
		sprintf(nomePath,"recent%d", i );
		GetPrivateProfileString( "file", nomePath, "", fileRecente, MAX_PATH, config_ini);
		if( fileRecente[0] )
			InsertMenu( hSubMenu, -1, MF_BYPOSITION|MF_STRING, MENU_FILE_RECENTI+i, fileRecente);
	}
}

void apriFile() {
	// converte il nome del file da char a wchar_t
	wchar_t nomeFileW[MAX_PATH];
	if ( fileDaAprire[0] ) {
		MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, fileDaAprire, -1, nomeFileW, MAX_PATH);
	} else if ( nomeFile[0] ) {
		MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, nomeFile, -1, nomeFileW, MAX_PATH);
	}

	// se il file non esiste o non ha permesso di lettura messaggio di errore
	wchar_t messaggio[MAX_PATH+50];
	if ( !fileDaAprire[0] && !nomeFile[0] )
		return;
	if ( fileDaAprire[0] ) {
		if ( access(fileDaAprire, F_OK) == -1 ) {  // il file non esiste
			if (apriDaConfig_ini) {
				snwprintf(messaggio,conta(messaggio),L"Can't find the file opened last time %s",nomeFileW);
				MessageBoxW(hWindow, messaggio, L"File not found", MB_ICONWARNING);
				apriDaConfig_ini = 0;
				nomeFile[0] = 0;
			} else {	// non riesce ad aprirlo (caratteri Unicode)
				snwprintf(messaggio,conta(messaggio),L"Can't open the file %s",nomeFileW);
				MessageBoxW(hWindow, messaggio, L"Impossible to open", MB_ICONWARNING);
			}
			fileDaAprire[0] = 0;
			return;
		} else if (access( fileDaAprire, R_OK ) == -1 ) {	// il file non ha permesso di lettura
			snwprintf(messaggio,conta(messaggio),L"I can't read the file %s",nomeFileW);
			MessageBoxW(hWindow, messaggio, L"Failed to open", MB_ICONWARNING);
			fileDaAprire[0] = 0;
			return;
		} else {
			strcpy(nomeFile, fileDaAprire);
		}
	}
	fileDaAprire[0] = 0;

	SetWindowTextW( hEditore, decodificaFile(nomeFile) );
	if(okEdita) {
		contaCaratteri();
	} else {
		RECT rect;
		GetClientRect(hWindow,&rect);
		size.clienteLargo = rect.right;
		creaBottoni();
		if(si.nMax >= si.nPage)
			size.clienteLargo -= GetSystemMetrics(SM_CXVSCROLL);
		creaBottoni();
		ShowWindow(hTastiera, SW_SHOW);
		SetWindowPos(hWindow, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	}
	
	// modifica titolo finestra e nome file in statusbar
	char* nome = strrchr(nomeFile, '\\') + 1; 
	SetWindowText(hWindow, nome);
	scriviNomeFile();

	// abilita bottoni menu
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_EDITA, MF_ENABLED);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVACOME, MF_ENABLED);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_CHIUDI, MF_ENABLED);
	bottoneDestra();
	aggiornaFileRecenti();
}


// scopre se il file ha il BOM e restituisce il numero di caratteri da eliminare all'inizio
int lunghezzaBOM(char* nomeFile) {
	FILE* file = fopen( nomeFile, "r" );	// apre il file come testo intiero, incluso il BOM
	fseek(file, 0, SEEK_END);
	wchar_t testo[ftell(file)+1];
	fseek(file, 0, SEEK_SET);
	fgetws(testo, sizeof testo/sizeof(char), file);
	int num = 0;
	if (testo[0]==L'ï' && testo[1]==L'»' && testo[2]==L'¿') {
		num = 3;
	} else if ( testo[0]==L'ÿ' && testo[1]==L'þ' ) {
		num = 2;
	}
	fclose(file);
	return num;
}

// prende i separatori da config.ini e li assembla in una stringa da restituire a creaBottoni()
wchar_t * elencaSeparatori() {
	char stringaSeparatori[20] = {0};
	static wchar_t stringaSeparatoriWide[20];
	GetPrivateProfileString("options","cutter3","",stringaSeparatori,sizeof(stringaSeparatori),config_ini);
	int tot = strlen(stringaSeparatori);
	if ( GetPrivateProfileInt("options","cutter1",0,config_ini) ) {
		stringaSeparatori[tot] = ' ';
		tot++;
	}
	if ( GetPrivateProfileInt("options","cutter2",0,config_ini) ) {
		stringaSeparatori[tot] = '	';
		tot++;
	}
	strcpy( stringaSeparatori+tot, "\n");
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, stringaSeparatori, -1, stringaSeparatoriWide, 20);
	return stringaSeparatoriWide;	
}

// raddoppia il carattere '&' che non compare nei bottoni
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

// riceve il puntatore a una stringa e la modifica dimezzando i doppi '&&'
wchar_t* disescapaAnd(wchar_t* snip) {
	for(int i=0,ip=0;i<=wcslen(snip);i++) {
		snip[i] = snip[ip];
		if (snip[i]=='&')
			ip++;
		ip++;
	}
	return snip;
}

// accorcia l'indirizzo lungo e lo scrive nella status bar
void scriviNomeFile() {
	HDC hDc = GetDC(hStatusBar);
	HFONT defaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SelectObject(hDc, defaultFont);
	SIZE dimensioniPath;
	GetTextExtentPoint32( hDc, nomeFile, strlen(nomeFile), &dimensioniPath );
	int larghezzaPath = dimensioniPath.cx * 1.13;
	ReleaseDC(hStatusBar, hDc);
	if( larghezzaPath > size.statusSecondoLargo) {
		float mezziCaratteri = (float)strlen(nomeFile) / larghezzaPath * size.statusSecondoLargo / 2 -1;
		if (mezziCaratteri < 0) mezziCaratteri = 0;
		char testa[MAX_PATH/2];
		strncpy(testa, nomeFile, mezziCaratteri);
		testa[(int)mezziCaratteri] = 0;
		char *coda = nomeFile + strlen(nomeFile)-(int)mezziCaratteri;
		char nomeTroncato[MAX_PATH];
		sprintf( nomeTroncato, "%s...%s", testa, coda );
		SendMessage( hStatusBar, SB_SETTEXT, 1, (LPARAM)nomeTroncato );
	} else
		SendMessage( hStatusBar, SB_SETTEXT, 1, (LPARAM)nomeFile );
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
			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION, MENU_FILE_EDITA, "Edit");
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
					HWND manicoBottone = (HWND)lParam;
					int lunga = GetWindowTextLength(manicoBottone);
					wchar_t capzione[lunga+1];
					GetWindowTextW(manicoBottone, capzione, lunga+1);
					disescapaAnd(capzione);
					lunga = wcslen(capzione);

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
					char quantiCaratteri[10];
					snprintf( quantiCaratteri, sizeof(quantiCaratteri), "\t%d", numCaratteriInviati);
					SendMessage( hStatusBar, SB_SETTEXT, 0, (LPARAM)&quantiCaratteri );
					// e stringa inserita
					wchar_t stringaInviata[numCaratteriInviati+1];
					for( int i=0; i<numCaratteriInviati; i++ )
						stringaInviata[i] = input[i*2].ki.wScan;
					stringaInviata[numCaratteriInviati]=0;
					SendMessage( hStatusBar, SB_SETTEXTW, 1, (LPARAM)&stringaInviata );
					if (!ultimoProgrammaAttivo)
						DialogBox( 0, "messaggioIstruttivo", hWindow, proceduraDialogoIstruzione );
				}
			} break;
		default:
            return DefWindowProc(hTastier, Message, wParam, lParam); 
	}
	return 0;
}

// avvia la finestra di dialogo Windows per aprire il file
void finestraDialogoApri() {
	if (!preservaModifiche())
		return;
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = "Text Files (txt)\0*.txt\0All Files (*)\0*.*\0";
	ofn.lpstrFile = fileDaAprire;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "txt";
	if( GetOpenFileName(&ofn) )
		apriFile();
}

// salva il file sovrascrivendolo
_Bool salvaFile() {
	int num = GetWindowTextLength(hEditore);
	wchar_t buffer[num+1];
	GetWindowTextW(hEditore, buffer, num+1);
	HANDLE hFile = CreateFile(nomeFile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	char bufferUTF8[ WideCharToMultiByte( CP_UTF8, 0, buffer, -1, 0, 0, NULL, NULL ) ];
	WideCharToMultiByte( CP_UTF8, 0, buffer, -1, bufferUTF8, sizeof(bufferUTF8), NULL, NULL );
	DWORD byteScritti;
	if( !WriteFile(hFile, bufferUTF8, sizeof(bufferUTF8)-1, &byteScritti, NULL) ) {
		MessageBox(hWindow,"I can't save the file","Unable to save",MB_ICONSTOP);
		CloseHandle(hFile);
		return 0;
	}
	CloseHandle(hFile);
	fileModificato = 0;
	char* nome = strrchr(nomeFile, '\\') + 1; 
	SetWindowText(hWindow, nome);
	EnableMenuItem(GetMenu(hWindow), MENU_FILE_SALVA, MF_GRAYED);
	return 1;
}

// avvia la finestra di dialogo Windows per salvare il file
int finestraDialogoSalva() {
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWindow;
	ofn.lpstrFilter = "Text Files (txt)\0*.txt\0All Files (*)\0*.*\0";
	ofn.lpstrFile = nomeFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;	//OFN_HIDEREADONLY
	ofn.lpstrDefExt = "txt";
	int risposta;
	if( (risposta=GetSaveFileName(&ofn)) ) {
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
	if (fileModificato) {
		int risposta = MessageBox(hWindow, "Do you want to save last changes?", "File modified", 
			MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON1 );
		switch (risposta) {
			case IDYES:
				if (strcmp(nomeFile, "untitled") == 0)
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

void bottoneDestra() {
	int num = 2;
	if (!conBarraTitolo)
		num = 0;
	else if (!nomeFile[0])
		num = 1;
	else if  (okEdita)
		num = 3;
	else if (massimizzata)
		num = 4;
	wchar_t *icona[5] = { L"X", L"🗋", L"⊞", L"✔", L"🖉" };
	ModifyMenuW(GetMenu(hWindow), MENU_BOTTONE_DESTRA, MF_BYCOMMAND|MF_ENABLED|MF_HELP|MF_STRING, MENU_BOTTONE_DESTRA, icona[num]);
	DrawMenuBar(hWindow);
	char* tip[5] = { "", "New Document", "Adapt to Keyboard", "Close Editor", "Edit Keyboard" };
	ti.lpszText = tip[num];
	SendMessage( hToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti );
}

void mostraNascondiBarraTitolo() {
	if (conBarraTitolo) {
		SetWindowLong( hWindow, GWL_STYLE, WS_OVERLAPPEDWINDOW|WS_VISIBLE );
		RemoveMenu (GetMenu(hWindow), MENU_RIDUCI, 0);	// rimuove i due pulsanti di ridimensione
		RemoveMenu (GetMenu(hWindow), MENU_INGRANDISCI, 0);
		bottoneDestra();
	} else {
		SetWindowLong( hWindow, GWL_STYLE, WS_SIZEBOX|WS_VISIBLE );
		InsertMenu( GetMenu(hWindow), 3, MF_BYPOSITION|MF_STRING|MF_HELP, MENU_RIDUCI, "_");
		InsertMenuW( GetMenu(hWindow), 4, MF_BYPOSITION|MF_STRING|MF_HELP, MENU_INGRANDISCI, L"⬜");
		bottoneDestra();
	}
	SetWindowPos( hWindow, 0,0,0,0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
	veroResize = 0;
	CheckMenuItem( GetMenu(hWindow), MENU_MOSTRA_TITOLO, conBarraTitolo );
}

// svuota la finestra dei bottoni
_Bool chiudiFile() {
	if (!preservaModifiche())
		return 0;
	svuotaTastiera();
	ShowWindow(hTastiera, SW_HIDE);
	ShowWindow(hEditore, SW_HIDE);
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
	bottoneDestra();
	
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
		char dimensioni[50] = {0};
		sprintf(dimensioni,"left=%d%ctop=%d%cright=%d%cbottom=%d", (int)wndRect.left,0, (int)wndRect.top,0, (int)wndRect.right,0, (int)wndRect.bottom );
		WritePrivateProfileSection("size", dimensioni, config_ini);
	}
	if (strcmp(nomeFile, "untitled") == 0)
		nomeFile[0] = 0;
	// scrive in confing.ini nome del file aperto 
	if( !WritePrivateProfileString( "file", "openfile", nomeFile, config_ini) ) {
		char msg[MAX_PATH+35];
		sprintf (msg, "I can't save the program settings in %s", config_ini);
		MessageBox (hWindow, msg,"Failed to save", MB_ICONERROR);
	}
	DestroyWindow(hWindow);
}

// prende le impostazioni del font da config.ini
void caricaFont(char* utente) {
	lf.lfHeight = GetPrivateProfileInt(utente,"height",0,config_ini);
	lf.lfWeight = GetPrivateProfileInt(utente,"weight",0,config_ini);
	lf.lfItalic = GetPrivateProfileInt(utente,"italic",0,config_ini);
	lf.lfCharSet = GetPrivateProfileInt(utente,"charset",0,config_ini);
	GetPrivateProfileString( utente, "face", "", lf.lfFaceName, sizeof(lf.lfFaceName), config_ini);
}

// prende il font di sistema dei Message Box e lo impone a una finestra e a tutti i suoi figli
BOOL CALLBACK proceduraEnumChild( HWND hWnd, LPARAM lParam) {
	SendMessage( hWnd, WM_SETFONT, (WPARAM)lParam, TRUE );
	return TRUE;
}
void settaFontSistema(HWND hWnd) {
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo( SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0 );
	HFONT hFont = CreateFontIndirect( &(ncm.lfMessageFont) );
	SendMessage( hWnd, WM_SETFONT, (WPARAM)hFont, TRUE );
	EnumChildWindows( hWnd, proceduraEnumChild, (WPARAM)hFont);
}

// procedura della finestra di dialogo Preferenze
BOOL CALLBACK proceduraDialogoPreferenze(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:	{
			settaFontSistema(hWnd);
			// prendende i valori da config.ini e li setta nella finestra dialogo 
			PostMessage( GetDlgItem(hWnd,CHECKBOX_SPAZI), BM_SETCHECK, 
						 GetPrivateProfileInt("options","cutter1",0,config_ini), 0);
			PostMessage ( GetDlgItem(hWnd,CHECKBOX_TAB), BM_SETCHECK,
						  GetPrivateProfileInt("options","cutter2",0,config_ini), 0);
			char separatori[15];
			GetPrivateProfileString("options", "cutter3", "", separatori, sizeof(separatori), config_ini);
			SetWindowText( GetDlgItem(hWnd,EDITTEXT_ALTRI), separatori );
			break;
		}
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case IDOK: {	// chiude la finestra di dialogo e salva le preferenze
					char separatore[15];
					sprintf( separatore, "%d", SendDlgItemMessage(hWnd,CHECKBOX_SPAZI,BM_GETCHECK,0,0) );
					WritePrivateProfileString( "options", "cutter1", separatore, config_ini);
					sprintf( separatore, "%d", SendDlgItemMessage(hWnd,CHECKBOX_TAB,BM_GETCHECK,0,0) );
					WritePrivateProfileString( "options", "cutter2", separatore, config_ini);
					GetWindowText( GetDlgItem(hWnd,EDITTEXT_ALTRI), separatore, 15);
					WritePrivateProfileString( "options", "cutter3", separatore, config_ini);
					EndDialog(hWnd, IDOK);
					disponiBottoni();
					break;
				}
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
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