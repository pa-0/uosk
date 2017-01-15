// Uosk - Funzioni del programma

wchar_t* elencaSeparatori();
wchar_t* escapaAnd(wchar_t*);
void caricaFont(char*);
void copiaNegliAppunti(wchar_t*);
_Bool preservaModifiche(HWND);
void svuotaPulsantiera();
#define conta(array) sizeof array / sizeof(array[0])

// crea bottoni dal testo di un file
_Bool creaBottoni(HWND hWnd, wchar_t* nomeFileW) {
	svuotaPulsantiera();
	setlocale(LC_CTYPE,".1252");
	if (fileModificato)
		nomeFileW = Lbuffer_tmp;
	FILE *file = _wfopen( nomeFileW, L"r,ccs=UNICODE" );
	fseek(file, 0, SEEK_END);
	int pesoFile = ftell(file)+1;
	fseek(file, lunghezzaBOM(nomeFileW), SEEK_SET);
	wchar_t riga[pesoFile];

	wchar_t *scampolo;
	SIZE dimensioniScampolo;
	int x = 0,
		largo, 
		alto = 0,
		numrighe = 0,
		numscampoli = 0;
	HDC hDc = GetDC(hWnd);
	caricaFont("buttons");
	HFONT hFontBottoni = CreateFontIndirect(&lf);
	SelectObject(hDc, hFontBottoni);
		
	while( fgetws(riga, sizeof riga/sizeof(wchar_t), file) ) {
		if(!riga[0])
			goto saltaCreazione;
		numrighe++;
		// ricava le parole da ogni riga
		scampolo = wcstok( riga, elencaSeparatori() );
		while (scampolo) {
			// dimensioni del bottone
			GetTextExtentPoint32W(hDc, scampolo, wcslen(scampolo), &dimensioniScampolo);
			largo = dimensioniScampolo.cx + 12;
			alto = dimensioniScampolo.cy + 10;

			// se il pulsante sborda a destra della finestra viene messo a capo (tranne che se è l'unico bottone della riga)
			if( x+largo>size.clienteLargo && x>0 ) {
				if( x > size.pulsaLarga )
					size.pulsaLarga = x;
				x = 0;
				si.nMax += alto;
			}

			// crea bottone
			HWND botton = CreateWindowExW( 0, L"Button", escapaAnd(scampolo), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
					x, si.nMax, largo, alto, hPulsantiera, (HMENU)ID_BOTTONE_SCAMPOLO,	// id unico per tutti i bottoni
					GetModuleHandle(NULL), NULL);
			SendMessage(botton, WM_SETFONT, (WPARAM)hFontBottoni, 0);
			scampolo = wcstok (NULL, elencaSeparatori() );
			x += largo;
			numscampoli++;
		}
		if( x > size.pulsaLarga )
			size.pulsaLarga = x;
		si.nMax += !x ? alto/2 : alto;
		x = 0;
	}
	saltaCreazione:
	fclose (file);

	// inizializza in ScrollInfo l'altezza della pagina
	si.nMax -= 1;
	SetScrollInfo (hPulsantiera, SB_VERT, &si, TRUE);

	// mostra la pulsantiera (i bottoni si vedono di già)
	ShowWindow(hPulsantiera, SW_SHOW);
	
	GlobalFree(riga);
	
	// scrive nella barra di stato numero dei bottoni creati e nome del file aperto
	char scampoli[6];
	sprintf( scampoli, "\t%d", numscampoli);
	SendMessage( GetDlgItem(hWnd,ID_STATUSBAR), SB_SETTEXT, SBT_NOBORDERS, (LPARAM)&scampoli);

	return 1;
}

void disponiBottoni(HWND hWnd) {
	if (!okEdita && nomeFile[0]) {
		wchar_t nomeFileW[MAX_PATH];
		MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, nomeFile, -1, nomeFileW, MAX_PATH);
		RECT rect;
		GetClientRect(hWnd,&rect);
		size.clienteLargo = rect.right;
		creaBottoni(hWnd,nomeFileW);
		if(si.nMax >= si.nPage)
			size.clienteLargo -= GetSystemMetrics(SM_CXVSCROLL);
		creaBottoni(hWnd,nomeFileW);
	}
}

void apriFile(HWND hWnd) {
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
				MessageBoxW(hWnd, messaggio, L"File not found", MB_ICONWARNING);
				apriDaConfig_ini = 0;
				nomeFile[0] = 0;
			} else {	// non riesce ad aprirlo (caratteri Unicode)
				snwprintf(messaggio,conta(messaggio),L"Can't open the file %s",nomeFileW);
				MessageBoxW(hWnd, messaggio, L"Impossible to open", MB_ICONWARNING);
			}
			fileDaAprire[0] = 0;
			return;
		} else if (access( fileDaAprire, R_OK ) == -1 ) {	// il file non ha permesso di lettura
			snwprintf(messaggio,conta(messaggio),L"I can't read the file %s",nomeFileW);
			MessageBoxW(hWnd, messaggio, L"Failed to open", MB_ICONWARNING);
			fileDaAprire[0] = 0;
			return;
		} else {
			strcpy(nomeFile, fileDaAprire);
		}
	}
	fileDaAprire[0] = 0;

	apriFileEditore(hWnd);
	if(okEdita) {
		HWND hStatus = GetDlgItem(hWnd, ID_STATUSBAR);
		SendMessage(hStatus, SB_SETTEXT, 1+SBT_NOBORDERS, (LPARAM)nomeFile);
		contaCaratteri();
	} else {
		RECT rect;
		GetClientRect(hWnd,&rect);
		size.clienteLargo = rect.right;
		creaBottoni(hWnd,nomeFileW);
		if(si.nMax >= si.nPage)
			size.clienteLargo -= GetSystemMetrics(SM_CXVSCROLL);
		creaBottoni(hWnd,nomeFileW);
	}
	
	// modifica titolo finestra e nome file in menubar
	char* nome = strrchr(nomeFile, '\\') + 1; 
	SetWindowText(hWnd, nome);
	SendMessage( GetDlgItem(hWnd,ID_STATUSBAR), SB_SETTEXT, 1+SBT_NOBORDERS, (LPARAM)nomeFile);

	// abilita bottoni menu
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVACOME, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_CHIUDI, MF_ENABLED);
	EnableMenuItem(GetMenu(hWnd), ID_BOTTONE_AUTOSIZE, MF_ENABLED);
	ShowWindow(GetDlgItem(hWnd, ID_BOTTONE_AUTOSIZE), SW_SHOW);
}


// scopre se il file ha il BOM e restituisce il numero di caratteri da eliminare all'inizio
int lunghezzaBOM(wchar_t* nomeFile) {
	FILE* file = _wfopen( nomeFile, L"r" );	// apre il file come testo intiero, incluso il BOM
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
	strcpy( stringaSeparatori+tot, "\n\r");
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

LRESULT CALLBACK ProceduraPulsantiera(HWND hPulsantir, UINT Message, WPARAM wParam, LPARAM lParam) {
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
			SetScrollInfo (hPulsantir, SB_VERT, &si, TRUE);
			GetScrollInfo (hPulsantir, SB_VERT, &si);
			ScrollWindow (hPulsantir, 0, posVertIniz-si.nPos, NULL, NULL);
			break;
		}
		case WM_MOUSEWHEEL:	// rotellina mouse e touchpad sui bottoni
			if( si.nMax >= si.nPage ) {
				int posVertIniz = si.nPos;
				si.nPos -= GET_WHEEL_DELTA_WPARAM(wParam) / 3;
				SetScrollInfo (hPulsantir, SB_VERT, &si, TRUE);
				GetScrollInfo (hPulsantir, SB_VERT, &si);
				ScrollWindow(hPulsantir, 0, posVertIniz-si.nPos, NULL, NULL );
			}
			break;
		case WM_CONTEXTMENU: {
			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION, MENU_FILE_EDITA, "Edit");
			POINT pt;
			GetCursorPos(&pt);
			TrackPopupMenu(hPopupMenu, TPM_TOPALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, GetParent(hPulsantir), NULL);
			break;
		} 
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case ID_BOTTONE_SCAMPOLO: {
						
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
					HWND hStatus = GetDlgItem(GetParent(hPulsantir), ID_STATUSBAR);
					char quantiCaratteri[10];
					snprintf( quantiCaratteri, sizeof(quantiCaratteri), "\t%d", numCaratteriInviati);
					SendMessage( hStatus, SB_SETTEXT, 0, (LPARAM)&quantiCaratteri );
					// e stringa inserita
					wchar_t stringaInviata[numCaratteriInviati+1];
					for( int i=0; i<numCaratteriInviati; i++ )
						stringaInviata[i] = input[i*2].ki.wScan;
					stringaInviata[numCaratteriInviati]=0;
					SendMessage( hStatus, SB_SETTEXTW, 1, (LPARAM)&stringaInviata );
				}
			} break;
		default:
            return DefWindowProc(hPulsantir, Message, wParam, lParam); 
	}
	return 0;
}

// avvia la finestra di dialogo Windows per aprire il file
void finestraDialogoApri(HWND hWnd) {
	if (!preservaModifiche(hWnd))
		return;
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Text Files (txt)\0*.txt\0All Files (*)\0*.*\0";
	ofn.lpstrFile = fileDaAprire;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "txt";
	if( GetOpenFileName(&ofn) )
		apriFile(hWnd);
}

// salva il file sovrascrivendolo
void salvaFile(HWND hWnd) {
	int num = GetWindowTextLength(hEditore);
	wchar_t buffer[num+1];
	GetWindowTextW(hEditore, buffer, num+1);
	FILE* file = fopen( nomeFile, "w,ccs=UTF-8" );
	for(int i=0, id=0; id<=num; i++, id++) {
		while (buffer[id]=='\r')
			id++;
		buffer[i] = buffer[id];
	}
	if( fwprintf( file, L"%s", buffer ) < 0 ) {
		MessageBox(hWnd,"I can't save the file","Unable to save",MB_ICONSTOP);
		return;
	}
	fclose(file);
	fileModificato = 0;
	char* nome = strrchr(nomeFile, '\\') + 1; 
	SetWindowText(hWnd, nome);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVA, MF_GRAYED);
}

// avvia la finestra di dialogo Windows per salvare il file
int finestraDialogoSalva(HWND hWnd) {
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFilter = "Text Files (txt)\0*.txt\0All Files (*)\0*.*\0";
	ofn.lpstrFile = nomeFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;	//OFN_HIDEREADONLY
	ofn.lpstrDefExt = "txt";
	int risposta;
	if( (risposta=GetSaveFileName(&ofn)) ) {
		salvaFile(hWnd);
		SendMessage( GetDlgItem(hWnd,ID_STATUSBAR), SB_SETTEXT, 1+SBT_NOBORDERS, (LPARAM)nomeFile);
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
void svuotaPulsantiera() {
	HWND scampolo_esistente;
	while ( (scampolo_esistente=GetDlgItem(hPulsantiera,ID_BOTTONE_SCAMPOLO)) ) {
		DestroyWindow(scampolo_esistente);
	}
	
	// resetta dimensioni e posizione globali della pagina
	si.nPos = 0;
	si.nMax = 0;
	size.pulsaLarga = 0;
	SetScrollInfo(hPulsantiera,SB_VERT,&si,1);
}

// chiede di salvare eventuali modifiche al testo
_Bool preservaModifiche(HWND hWnd) {
	_Bool continua = 1;
	if (fileModificato) {
		int risposta = MessageBox(hWnd, "Do you want to save last changes?", "File modified", 
			MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON1 );
		switch (risposta) {
			case IDYES:
				if (strcmp(nomeFile, "untitled") == 0) {
					if (finestraDialogoSalva(hWnd) == 0)	// anche questa finisce con Annulla
						continua=0;
				} else
					salvaFile(hWnd);
				break;
			case IDNO:
				EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVA, MF_GRAYED);
				fileModificato = 0;
				break;
			case IDCANCEL:
				continua=0;
		}
	}	
	return continua;
}

// svuota la finestra dei bottoni
_Bool chiudiFile(HWND hWnd) {
	if (!preservaModifiche(hWnd))
		return 0;
	svuotaPulsantiera();
	ShowWindow(hPulsantiera, SW_HIDE);
	ShowWindow(hEditore, SW_HIDE);
	okEdita = 0;

	// disabilita bottoni
	EnableMenuItem(GetMenu(hWnd), ID_BOTTONE_AUTOSIZE, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, MF_GRAYED);
	CheckMenuItem(GetMenu(hWnd), MENU_FILE_EDITA, 0);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVA, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_SALVACOME, MF_GRAYED);
	EnableMenuItem(GetMenu(hWnd), MENU_FILE_CHIUDI, MF_GRAYED);
	ShowWindow(GetDlgItem(hWnd, ID_BOTTONE_AUTOSIZE), SW_HIDE);	// non funziona
	
	// cancella testo in status bar
	HWND hStatus = GetDlgItem(hWnd,ID_STATUSBAR);
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)"");
	SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)"");
	
	// annulla nome del file aperto
	nomeFile[0] = 0;
	fileModificato = 0;
	SetWindowText(hWnd, "Uosk");
	return 1;
}

// salva le impostazioni e chiude il programma
void chiudiProgramma(HWND hWnd) {
	if (!preservaModifiche(hWnd))
		return;
	// prima di chiudere salva le dimensioni finestra in config.ini
	if( !IsIconic(hWnd) ) {
		RECT wndRect;
		GetWindowRect( hWnd, &wndRect );
		char dimensioni[50] = {0};
		sprintf(dimensioni,"left=%d%ctop=%d%cright=%d%cbottom=%d", (int)wndRect.left,0, (int)wndRect.top,0, (int)wndRect.right,0, (int)wndRect.bottom );
		WritePrivateProfileSection("size", dimensioni, config_ini);
	}
	if (strcmp(nomeFile, "untitled") == 0)
		nomeFile[0] = 0;
	// scrive in confing.ini nome del file aperto 
	if( !WritePrivateProfileString( "file", "openfile", nomeFile, config_ini) )
		MessageBox(hWnd,"Unable to save the file name","Failed to save",MB_ICONWARNING);
	DeleteFile(buffer_tmp);
	DestroyWindow(hWnd);
}

// prende le impostazioni del font da config.ini
void caricaFont(char* utente) {
	lf.lfHeight = GetPrivateProfileInt(utente,"height",0,config_ini);
	lf.lfWeight = GetPrivateProfileInt(utente,"weight",0,config_ini);
	lf.lfItalic = GetPrivateProfileInt(utente,"italic",0,config_ini);
	lf.lfCharSet = GetPrivateProfileInt(utente,"charset",0,config_ini);
	GetPrivateProfileString( utente, "face", "", lf.lfFaceName, sizeof(lf.lfFaceName), config_ini);
}

// procedura della finestra di dialogo Preferenze
BOOL CALLBACK proceduraDialogoPreferenze(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
		case WM_INITDIALOG:	{ // procedure preliminari alla creazione della finestra
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
					disponiBottoni(GetParent(hWnd));
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
		case WM_COMMAND:
			switch(LOWORD(wParam)) {
				case ID_LINK:
					ShellExecute(0, 0, "https://sourceforge.net/projects/uosk/", 0, 0 , SW_SHOW);
					break;
				case IDCANCEL:
					EndDialog(hWnd, IDCANCEL);
			}
			break;
		default:
			return FALSE;
	}
	return TRUE;
}