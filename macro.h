// Variabili globali

char config_ini[MAX_PATH];
_Bool apriDaConfig_ini = 0;
char buffer_tmp[MAX_PATH];
wchar_t Lbuffer_tmp[MAX_PATH];
char fileDaAprire[MAX_PATH] = "";
char nomeFile[MAX_PATH] = "";

// destinatari dello scampolo
HWND ultimoProgrammaAttivo;
HWND finestraDestinazione;

// manico della pulsantiera e dell'editore
HWND hPulsantiera;         	
HWND hEditore;
int okEdita = MF_UNCHECKED;
_Bool fileModificato = 0;

// dimensioni della finestra
int larghezzaPulsantiera;
int posizioneVerticalePulsantiera = 0;
struct {
	int pulsaLarga;
	int statusAlta;
	int clienteLargo;
} size = { 0 };

// struttura per gestire la barra di scorrimento e altezza finestra
SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
int veroResize = 0;

unsigned int okCopiaAppunti = 0;
int okMostraStatus;
LOGFONT lf = {0};

// MACRO
// gli int dei messaggi sono del tutto a libera scelta
// arrivano in ProceduraFinestra() dentro wParam
#define ID_STATICO       	-1	// risorsa a cui non è mai necessario accedere, ad es. testo fisso
#define ID_ICONA           	1
#define ID_STATUSBAR		2
#define ID_PULSANTIERA   	3
#define ID_BOTTONE_SCAMPOLO	4
#define ID_EDITORE      	5  	// editore testo

#define ID_MENUPRINCIPALE 	20
#define MENU_FILE_NUOVO		21
#define MENU_FILE_APRI  	22
#define MENU_FILE_EDITA		23
#define MENU_FILE_SALVA		24
#define MENU_FILE_SALVACOME	25
#define MENU_FILE_CHIUDI  	26
#define MENU_EXIT        	27

#define MENU_COPIA_APPUNTI 	30
#define MENU_MOSTRA_STATUS 	31
#define MENU_FONT_BOTTONI	32
#define MENU_FONT_EDITORE	33
#define MENU_PREFERENZE 	34

#define MENU_INFORMAZIONI	40
#define MENU_PROBLEMA 		41
#define ID_BOTTONE_AUTOSIZE	42

#define DIALOGO_PREFERENZE	50
#define CHECKBOX_SPAZI	51
#define CHECKBOX_TAB	52
#define EDITTEXT_ALTRI	53

#define DIALOGO_RICONOSCIMENTI	60
#define ID_LINK					61
