/**
 * @file error.cpp
 *
 * Implementation of in-game message functions.
 */
#include "all.h"

DEVILUTION_BEGIN_NAMESPACE

char msgtable[MAX_SEND_STR_LEN];
DWORD msgdelay;
char msgflag;
char msgcnt;

/** Maps from error_id to error message. */
const char *const MsgStrings[] = {
	"",
	"Autokarta ej tillg�nglig i staden",
	"No multiplayer functions in demo",
	"Direct Sound Creation Failed",
	"Not available in shareware version",
	"Not enough space to save",
	"Paus ej tillg�ngligt i staden",
	"Copying to a hard disk is recommended",
	"Multiplayer sync problem",
	"No pause in multiplayer",
	"Laddar...",
	"Sparar...",
	"N�gra veknar n�r andra v�xer sig starka",
	"Ny styrka formas genom f�rst�relse",
	"De som f�rsvarar sig attackerar s�llan",
	"R�ttvisans sv�rd �r rappt och vasst",
	"N�r sj�len �r vaksam utvecklas kroppen",
	"Kraften fr�n mana fokuseras och f�rnyas",
	"Tiden kan inte t�ra p� st�lets kraft",
	"Magi �r inte alltid vad det verkar vara",
	"Det som en g�ng var �ppet �r nu st�ngt",
	"Visdom �r Intensitetens pris",
	"Dold kraft bringar f�rst�relse",
	"Det som icke kan h�llas kan icke skadas",
	"Crimson och Azure blir som solen",
	"Kunskap och visdom till priset av ditt jag",
	"Drick och �terf� dina krafter",
	"Varth�n du �n g�r, d�r �r du",
	"Visdom �r Energins pris",
	"Rikedomar finns d�r du minst anar dem",
	"D�r girigheten st�r sig sl�tt, prisas t�lamodet",
	"V�lsignad av en v�lvillig fr�nde!",
	"M�nniskans hand kan styras av �det",
	"Styrka st�ds av himmelskt �de",
	"Livets essens fl�dar inifr�n",
	"V�gen blir tydlig n�r den ses fr�n ovan",
	"Visdom �r Fr�lsningens pris",
	"Mysterierna avsl�jas i f�rnuftets ljus",
	"De som �r sist kan �n bli f�rst",
	"Givmildhet f�r med sig sin egen bel�ning",
	"Du m�ste uppn� minst niv� 8 f�r att anv�nda detta.",
	"Du m�ste uppn� minst niv� 13 f�r att anv�nda detta.",
	"Du m�ste uppn� minst niv� 17 f�r att anv�nda detta.",
	"Ur�ldrig kunskap f�rv�rvad!",
	"That which does not kill you...",
	"Knowledge is power.",
	"Give and you shall receive.",
	"Some experience is gained by touch.",
	"There's no place like home.",
	"Spirtual energy is restored.",
	"You feel more agile.",
	"You feel stronger.",
	"You feel wiser.",
	"You feel refreshed.",
	"That which can break will.",
};

void InitDiabloMsg(char e)
{
	int i;

	if (msgcnt >= sizeof(msgtable))
		return;

	for (i = 0; i < msgcnt; i++) {
		if (msgtable[i] == e)
			return;
	}

	msgtable[msgcnt] = e; // BUGFIX: missing out-of-bounds check (fixed)
	msgcnt++;

	msgflag = msgtable[0];
	msgdelay = SDL_GetTicks();
}

void ClrDiabloMsg()
{
	int i;

	for (i = 0; i < sizeof(msgtable); i++)
		msgtable[i] = 0;

	msgflag = 0;
	msgcnt = 0;
}

void DrawDiabloMsg(CelOutputBuffer out)
{
	int i, len, width, sx, sy;
	BYTE c;

	CelDrawTo(out, PANEL_X + 101, DIALOG_Y, pSTextSlidCels, 1, 12);
	CelDrawTo(out, PANEL_X + 527, DIALOG_Y, pSTextSlidCels, 4, 12);
	CelDrawTo(out, PANEL_X + 101, DIALOG_Y + 48, pSTextSlidCels, 2, 12);
	CelDrawTo(out, PANEL_X + 527, DIALOG_Y + 48, pSTextSlidCels, 3, 12);

	sx = PANEL_X + 109;
	for (i = 0; i < 35; i++) {
		CelDrawTo(out, sx, DIALOG_Y, pSTextSlidCels, 5, 12);
		CelDrawTo(out, sx, DIALOG_Y + 48, pSTextSlidCels, 7, 12);
		sx += 12;
	}
	sy = DIALOG_Y + 12;
	for (i = 0; i < 3; i++) {
		CelDrawTo(out, PANEL_X + 101, sy, pSTextSlidCels, 6, 12);
		CelDrawTo(out, PANEL_X + 527, sy, pSTextSlidCels, 8, 12);
		sy += 12;
	}

	DrawHalfTransparentRectTo(out, PANEL_X + 104, DIALOG_Y - 8, 432, 54);

	strcpy(tempstr, MsgStrings[msgflag]);
	sx = PANEL_X + 101;
	sy = DIALOG_Y + 24;
	len = strlen(tempstr);
	width = 0;

	for (i = 0; i < len; i++) {
		width += fontkern[fontframe[gbFontTransTbl[(BYTE)tempstr[i]]]] + 1;
	}

	if (width < 442) {
		sx += (442 - width) >> 1;
	}

	for (i = 0; i < len; i++) {
		c = fontframe[gbFontTransTbl[(BYTE)tempstr[i]]];
		if (c != '\0') {
			PrintChar(out, sx, sy, c, COL_GOLD);
		}
		sx += fontkern[c] + 1;
	}

	if (msgdelay > 0 && msgdelay <= SDL_GetTicks() - 3500) {
		msgdelay = 0;
	}
	if (msgdelay == 0) {
		msgcnt--;
		if (msgcnt == 0) {
			msgflag = 0;
		} else {
			msgflag = msgtable[msgcnt];
			msgdelay = SDL_GetTicks();
		}
	}
}

DEVILUTION_END_NAMESPACE
