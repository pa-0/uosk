#ifndef HEADER_GUARD
#define HEADER_GUARD

#define WINVER 0x0501
#define _WIN32_IE 0x0300
#define _WIN32_WINNT WINVER
#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <windowsx.h>
#include <unistd.h>
#include <locale.h>
#include <wchar.h>
#include <shlwapi.h>
#include <psapi.h>
#include <tlhelp32.h>
//#include ".\..\winapi\utilita.c"
//#include "editore.h"
//#include "altre.h"

// Variabili globali

// manici
HWND hWindow;
HWND hTastiera;
HWND hEditore;
HWND hInfo;
HWND ultimoProgrammaAttivo;
HWND hSnippetFocus;

char config_ini[MAX_PATH];
wchar_t Lconfig_ini[MAX_PATH];
_Bool apriDaConfig_ini;
wchar_t *cartellaProgramma;
wchar_t fileDaAprire[MAX_PATH];
wchar_t nomeFile[MAX_PATH];
int okEdita;	// = MF_UNCHECKED
_Bool fileModificato;	// = 0

// dimensioni della finestra
struct {
	int pulsaLarga;
	int statusAlta;
	int statusSecondoLargo;
	int clienteLargo;
} size;	// = { 0 }

// struttura per gestire la barra di scorrimento e altezza finestra
SCROLLINFO si;
SCROLLINFO si2;
_Bool veroResize;	// = 0
_Bool massimizzata;	// = 0

_Bool okMosaico;
unsigned int okCopiaAppunti;
unsigned int conBarraTitolo;
HWND hStatusBar;
unsigned int okMostraStatus;
unsigned int das;
LOGFONT lf;	// = {0}
HWND hToolTip;
TOOLINFO ti;	// = {0}

// MACRO
#define ID_STATICO       	-1
#define ID_ICONA           	1
#define ID_IMMAGINE			2
#define ID_FINESTRA_CLIENTE	3
#define ID_BOTTONE_SNIPPET	4
#define ID_INFORMINO		5
#define ID_INFO_GLIFO		6

#define ID_MENUPRINCIPALE 	20
#define MENU_FILE_NUOVO		21
#define MENU_FILE_APRI  	22
#define MENU_FILE_EDITA		23
#define MENU_FILE_SALVA		24
#define MENU_FILE_SALVACOME	25
#define MENU_FILE_CHIUDI  	26
#define MENU_EXIT        	27

#define MENU_COPIA_APPUNTI 	30
#define MENU_MOSTRA_TITOLO 	31
#define MENU_MOSTRA_STATUS 	32
#define MENU_INFO_SNIPPET 	33
#define MENU_LETTURA_DAS	34
#define MENU_FONT_TASTIERA	35
#define MENU_FONT_EDITORE	36
#define MENU_PREFERENZE 	37

#define MENU_PROBLEMA 		40
#define MENU_INFORMAZIONI	41
#define MENU_RIDUCI			42
#define MENU_INGRANDISCI	43
#define MENU_BOTTONE_DESTRA	44

#define DIALOGO_PREFERENZE	50
#define CHECKBOX_SPAZI		60
#define CHECKBOX_TAB		61
#define EDITTEXT_ALTRI		62
#define CHECKBOX_MOSAICO	63

#define DIALOGO_RICONOSCIMENTI	70
#define ID_LINK					71

#define MENU_FILE_RECENTI	80


// dichiarazione funzioni
void bottoneDestra();
void caricaFont(char*);
void copiaNegliAppunti(wchar_t*);
wchar_t *elencaSeparatori();
wchar_t *escapaAnd(wchar_t*);
int lunghezzaBOM(wchar_t*);
_Bool preservaModifiche();
void scriviNomeFile();
void settaFontSistema(HWND);
void svuotaTastiera();
wchar_t *tesseraMosaico(wchar_t*);
#define conta(array) sizeof array / sizeof(array[0])

void apriFile();
_Bool chiudiFile();
void chiudiProgramma();
void contaCaratteri();
void creaBottoni();
int disescapaAnd( wchar_t* snip );
void disponiBottoni();
void finestraDialogoApri();
int finestraDialogoSalva();
void mostraNascondiBarraTitolo();
void ordineLetturaDestraSinistra();
LRESULT CALLBACK ProceduraEditore(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK proceduraTastiera(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraInfoSnippet(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraInformino(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraInfoTesto(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
BOOL CALLBACK proceduraDialogoInformazioni(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK proceduraDialogoPreferenze(HWND, UINT, WPARAM, LPARAM);
_Bool salvaFile();
void scriviNomeProgramma();

int errore();

#endif