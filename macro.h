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
#include <usp10.h>
//#include ".\..\winapi\utilita.c"

// Variabili globali

// manici
HWND hWindow;
HWND hFrontalino;
HWND hTastiera;
HWND hEditore;
HWND hInfo;
HWND ultimoProgrammaAttivo;
HWND hSnippetFocus;

wchar_t appData[MAX_PATH];
wchar_t config_ini[MAX_PATH];
_Bool apriDaConfig_ini;
wchar_t *cartellaProgramma;
wchar_t fileDaAprire[MAX_PATH];
wchar_t nomeFile[MAX_PATH];
int okEdita;
_Bool fileModificato;
// esempi di tastiere in Frontalino
struct {
	wchar_t nome[MAX_PATH-10];
	wchar_t testo[50];
	wchar_t indirizzo[MAX_PATH];
} *esemplari;
// dimensioni della finestra
struct {
	int pulsaLarga;
	int statusAlta;
	int statusSecondoLargo;
	int clienteLargo;
} size;
SCROLLINFO si;
SCROLLINFO si2;
_Bool veroResize;
_Bool massimizzata;
// struttura per ospitare info su uno snippet
struct unoSnippo {
	HWND manico;
	int caratteri;
	int larghezza;
	int minimo;
	int massimo;
} *snippi;
_Bool okMosaico;
unsigned int okCopiaAppunti;
unsigned int conBarraTitolo;
HWND hStatusBar;
unsigned int okMostraStatus;
unsigned int das;
LOGFONTW lf;
HWND hToolTip;
TOOLINFO ti;

// MACRO
#define ID_STATICO       	-1
#define ID_ICONA           	1
#define ID_IMMAGINE			2
#define ID_FINESTRA_CLIENTE	3
#define ID_BOTTONE_SNIPPET	4
#define ID_INFORMINO		5
#define ID_INFO_GLIFO		6

#define ID_FRONTE_NUOVO		10
#define ID_FRONTE_ESEMPI	11

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
#define MENU_BOTTONE_CHIUDI	42
#define MENU_BOTTONE_EDITA	43
#define MENU_BOTTONE_RIDUCI	44

#define DIALOGO_PREFERENZE	50
#define CHECKBOX_SPAZI		60
#define CHECKBOX_TAB		61
#define EDITTEXT_ALTRI		62
#define CHECKBOX_MOSAICO	63

#define DIALOGO_RICONOSCIMENTI	70
#define ID_LINK					71

#define MENU_FILE_RECENTI	80


// dichiarazione funzioni
void bottoniDestra();
void caricaFont(wchar_t*);
int contaGlifi(wchar_t*);
void copiaNegliAppunti(wchar_t*);
void creaFrontalino();
wchar_t *elencaSeparatori();
//wchar_t *escapaAnd(wchar_t*);
int lunghezzaBOM(wchar_t*);
_Bool preservaModifiche();
void scriviNomeFile();
HFONT settaFontSistema(HWND);
void svuotaTastiera();

#define conta(array) sizeof array / sizeof(array[0])

void apriFile();
wchar_t* checkGlyphExist( wchar_t*, wchar_t* );
_Bool chiudiFile();
void chiudiProgramma();
void contaCaratteri();
int creaBottoni();
//int disescapaAnd( wchar_t* snip );
void disponiBottoni();
void finestraDialogoApri();
int finestraDialogoSalva();
int CALLBACK metaFileEnumProc( HDC, HANDLETABLE*, const ENHMETARECORD*, int, LPARAM );
void mostraNascondiBarraTitolo();
void ordineLetturaDestraSinistra();
LRESULT CALLBACK ProceduraEditore(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
LRESULT CALLBACK proceduraFrontalino(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraTastiera(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraInfoSnippet(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraInformino(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK proceduraInfoTesto(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
BOOL CALLBACK proceduraDialogoInformazioni(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK proceduraDialogoPreferenze(HWND, UINT, WPARAM, LPARAM);
_Bool salvaFile();
void scriviNomeProgramma();

FILE * _wfopen (const wchar_t *, const wchar_t *);

int errore();

#endif