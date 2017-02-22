// Variabili globali

// manico della Tastiera e dell'Editore
HWND hWindow;
HWND hTastiera;         	
HWND hEditore;
int okEdita = MF_UNCHECKED;
_Bool fileModificato = 0;

// destinatario dello snippet
HWND ultimoProgrammaAttivo;

char config_ini[MAX_PATH];
_Bool apriDaConfig_ini = 0;
char fileDaAprire[MAX_PATH] = "";
char nomeFile[MAX_PATH] = "";

// dimensioni della finestra
struct {
	int pulsaLarga;
	int statusAlta;
	int statusSecondoLargo;
	int clienteLargo;
} size = { 0 };

// struttura per gestire la barra di scorrimento e altezza finestra
SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
_Bool veroResize = 0;
_Bool massimizzata = 0;

unsigned int okCopiaAppunti = 0;
unsigned int conBarraTitolo;
HWND hStatusBar;
unsigned int okMostraStatus;
LOGFONT lf = {0};
HWND hToolTip;
TOOLINFO ti = {0};

// MACRO
#define ID_STATICO       	-1
#define ID_ICONA           	1
#define ID_IMMAGINE			2
//#define ID_SEGNAPOSTO
#define ID_FINESTRA_CLIENTE	3
#define ID_BOTTONE_SNIPPET	4

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
#define MENU_FONT_TASTIERA	33
#define MENU_FONT_EDITORE	34
#define MENU_PREFERENZE 	35

#define MENU_INFORMAZIONI	40
#define MENU_PROBLEMA 		41
#define MENU_RIDUCI			42
#define MENU_INGRANDISCI	43
#define MENU_BOTTONE_DESTRA	44

#define DIALOGO_PREFERENZE	50
#define CHECKBOX_SPAZI	60
#define CHECKBOX_TAB	61
#define EDITTEXT_ALTRI	62

#define DIALOGO_RICONOSCIMENTI	70
#define ID_LINK					71

#define MENU_FILE_RECENTI	80