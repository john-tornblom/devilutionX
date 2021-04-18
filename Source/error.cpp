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
	"Autokarta ej tillgänglig i staden",
	"No multiplayer functions in demo",
	"Direct Sound Creation Failed",
	"Not available in shareware version",
	"Not enough space to save",
	"Paus ej tillgängligt i staden",
	"Copying to a hard disk is recommended",
	"Multiplayer sync problem",
	"No pause in multiplayer",
	"Laddar...",
	"Sparar...",
	"Några veknar när andra växer sig starka",
	"Ny styrka formas genom förstörelse",
	"De som försvarar sig attackerar sällan",
	"Rättvisans svärd är rappt och vasst",
	"När själen är vaksam utvecklas kroppen",
	"Kraften från mana fokuseras och förnyas",
	"Tiden kan inte tära på stålets kraft",
	"Magi är inte alltid vad det verkar vara",
	"Det som en gång var öppet är nu stängt",
	"Visdom är Intensitetens pris",
	"Dold kraft bringar förstörelse",
	"Det som icke kan hållas kan icke skadas",
	"Crimson och Azure blir som solen",
	"Kunskap och visdom till priset av ditt jag",
	"Drick och återfå dina krafter",
	"Varthän du än går, där är du",
	"Visdom är Energins pris",
	"Rikedomar finns där du minst anar dem",
	"Där girigheten står sig slätt, prisas tålamodet",
	"Välsignad av en välvillig frände!",
	"Människans hand kan styras av ödet",
	"Styrka stöds av himmelskt öde",
	"Livets essens flödar inifrån",
	"Vägen blir tydlig när den ses från ovan",
	"Visdom är Frälsningens pris",
	"Mysterierna avslöjas i förnuftets ljus",
	"De som är sist kan än bli först",
	"Givmildhet för med sig sin egen belöning",
	"Du måste uppnå minst nivå 8 för att använda detta.",
	"Du måste uppnå minst nivå 13 för att använda detta.",
	"Du måste uppnå minst nivå 17 för att använda detta.",
	"Uråldrig kunskap förvärvad!",
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
