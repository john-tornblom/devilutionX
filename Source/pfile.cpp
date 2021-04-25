/**
 * @file pfile.cpp
 *
 * Implementation of the save game encoding functionality.
 */
#include "pfile.h"

#include <string>

#include "codec.h"
#include "init.h"
#include "loadsave.h"
#include "mainmenu.h"
#include "mpqapi.h"
#include "pack.h"
#include "storm/storm.h"
#include "utils/file_util.h"
#include "utils/paths.h"

namespace devilution {

#define PASSWORD_SPAWN_SINGLE "adslhfb1"
#define PASSWORD_SPAWN_MULTI "lshbkfg1"
#define PASSWORD_SINGLE "xrgyrkj1"
#define PASSWORD_MULTI "szqnlsk1"

namespace {

std::string GetSavePath(DWORD save_num)
{
	std::string path = GetPrefPath();
	const char *ext = _(".sv");
	if (gbIsHellfire)
		ext = _(".hsv");

	if (gbIsSpawn) {
		if (!gbIsMultiplayer) {
			path.append(_("spawn"));
		} else {
			path.append(_("share_"));
		}
	} else {
		if (!gbIsMultiplayer) {
			path.append(_("single_"));
		} else {
			path.append(_("multi_"));
		}
	}

	char save_num_str[21];
	snprintf(save_num_str, sizeof(save_num_str) / sizeof(char), _("%d"), save_num);
	path.append(save_num_str);
	path.append(ext);
	return path;
}

bool GetPermSaveNames(uint8_t dwIndex, char *szPerm)
{
	const char *fmt;

	if (dwIndex < giNumberOfLevels)
		fmt = _("perml%02d");
	else if (dwIndex < giNumberOfLevels * 2) {
		dwIndex -= giNumberOfLevels;
		fmt = _("perms%02d");
	} else
		return false;

	sprintf(szPerm, fmt, dwIndex);
	return true;
}

bool GetTempSaveNames(uint8_t dwIndex, char *szTemp)
{
	const char *fmt;

	if (dwIndex < giNumberOfLevels)
		fmt = _("templ%02d");
	else if (dwIndex < giNumberOfLevels * 2) {
		dwIndex -= giNumberOfLevels;
		fmt = _("temps%02d");
	} else
		return false;

	sprintf(szTemp, fmt, dwIndex);
	return true;
}

void pfile_rename_temp_to_perm()
{
	DWORD dwIndex;
	char szTemp[MAX_PATH];
	char szPerm[MAX_PATH];

	dwIndex = 0;
	while (GetTempSaveNames(dwIndex, szTemp)) {
		bool result = GetPermSaveNames(dwIndex, szPerm); // DO NOT PUT DIRECTLY INTO ASSERT!
		assert(result);
		dwIndex++;
		if (mpqapi_has_file(szTemp)) {
			if (mpqapi_has_file(szPerm))
				mpqapi_remove_hash_entry(szPerm);
			mpqapi_rename(szTemp, szPerm);
		}
	}
	assert(!GetPermSaveNames(dwIndex, szPerm));
}

} // namespace

/** List of character names for the character selection screen. */
static char hero_names[MAX_CHARACTERS][PLR_NAME_LEN];
bool gbValidSaveFile;

const char *pfile_get_password()
{
	if (gbIsSpawn)
		return gbIsMultiplayer ? PASSWORD_SPAWN_MULTI : PASSWORD_SPAWN_SINGLE;
	return gbIsMultiplayer ? PASSWORD_MULTI : PASSWORD_SINGLE;
}

static DWORD pfile_get_save_num_from_name(const char *name)
{
	DWORD i;

	for (i = 0; i < MAX_CHARACTERS; i++) {
		if (strcasecmp(hero_names[i], name) == 0)
			break;
	}

	return i;
}

static BYTE *pfile_read_archive(HANDLE archive, const char *pszName, uint32_t *pdwLen)
{
	DWORD nread;
	HANDLE file;
	BYTE *buf;

	if (!SFileOpenFileEx(archive, pszName, 0, &file))
		return nullptr;

	*pdwLen = SFileGetFileSize(file, nullptr);
	if (*pdwLen == 0)
		return nullptr;

	buf = DiabloAllocPtr(*pdwLen);
	if (!SFileReadFile(file, buf, *pdwLen, &nread, nullptr))
		return nullptr;
	SFileCloseFile(file);

	*pdwLen = codec_decode(buf, *pdwLen, pfile_get_password());
	if (*pdwLen == 0)
		return nullptr;

	return buf;
}

static bool pfile_read_hero(HANDLE archive, PkPlayerStruct *pPack)
{
	DWORD read;
	BYTE *buf;

	buf = pfile_read_archive(archive, _("hero"), &read);
	if (buf == nullptr)
		return false;

	bool ret = false;
	if (read == sizeof(*pPack)) {
		memcpy(pPack, buf, sizeof(*pPack));
		ret = true;
	}

	mem_free_dbg(buf);
	return ret;
}

static void pfile_encode_hero(const PkPlayerStruct *pPack)
{
	BYTE *packed;
	DWORD packed_len;

	packed_len = codec_get_encoded_len(sizeof(*pPack));
	packed = (BYTE *)DiabloAllocPtr(packed_len);
	memcpy(packed, pPack, sizeof(*pPack));
	codec_encode(packed, sizeof(*pPack), packed_len, pfile_get_password());
	mpqapi_write_file(_("hero"), packed, packed_len);
	mem_free_dbg(packed);
}

static bool pfile_open_archive(DWORD save_num)
{
	if (OpenMPQ(GetSavePath(save_num).c_str()))
		return true;

	return false;
}

static HANDLE pfile_open_save_archive(DWORD save_num)
{
	HANDLE archive;

	if (SFileOpenArchive(GetSavePath(save_num).c_str(), 0, 0, &archive))
		return archive;
	return nullptr;
}

static void pfile_SFileCloseArchive(HANDLE *hsArchive)
{
	if (*hsArchive == nullptr)
		return;

	SFileCloseArchive(*hsArchive);
	*hsArchive = nullptr;
}

PFileScopedArchiveWriter::PFileScopedArchiveWriter(bool clear_tables)
    : save_num_(pfile_get_save_num_from_name(plr[myplr]._pName))
    , clear_tables_(clear_tables)
{
	if (!pfile_open_archive(save_num_))
		app_fatal(_("Failed to open player archive for writing."));
}

PFileScopedArchiveWriter::~PFileScopedArchiveWriter()
{
	mpqapi_flush_and_close(clear_tables_);
}

void pfile_write_hero(bool write_game_data, bool clear_tables)
{
	PFileScopedArchiveWriter scoped_writer(clear_tables);
	if (write_game_data) {
		SaveGameData();
		pfile_rename_temp_to_perm();
	}
	PkPlayerStruct pkplr;
	PackPlayer(&pkplr, myplr, !gbIsMultiplayer);
	pfile_encode_hero(&pkplr);
	if (!gbVanilla) {
		SaveHotkeys();
		SaveHeroItems(&plr[myplr]);
	}
}

static void game_2_ui_player(const PlayerStruct *p, _uiheroinfo *heroinfo, bool bHasSaveFile)
{
	memset(heroinfo, 0, sizeof(*heroinfo));
	strncpy(heroinfo->name, p->_pName, sizeof(heroinfo->name) - 1);
	heroinfo->name[sizeof(heroinfo->name) - 1] = '\0';
	heroinfo->level = p->_pLevel;
	heroinfo->heroclass = p->_pClass;
	heroinfo->strength = p->_pStrength;
	heroinfo->magic = p->_pMagic;
	heroinfo->dexterity = p->_pDexterity;
	heroinfo->vitality = p->_pVitality;
	heroinfo->hassaved = bHasSaveFile;
	heroinfo->herorank = p->pDiabloKillLevel;
	heroinfo->spawned = gbIsSpawn;
}

bool pfile_ui_set_hero_infos(bool (*ui_add_hero_info)(_uiheroinfo *))
{
	memset(hero_names, 0, sizeof(hero_names));

	for (DWORD i = 0; i < MAX_CHARACTERS; i++) {
		HANDLE archive = pfile_open_save_archive(i);
		if (archive != nullptr) {
			PkPlayerStruct pkplr;
			if (pfile_read_hero(archive, &pkplr)) {
				_uiheroinfo uihero;
				strcpy(hero_names[i], pkplr.pName);
				bool hasSaveGame = pfile_archive_contains_game(archive);
				if (hasSaveGame)
					pkplr.bIsHellfire = gbIsHellfireSaveGame;

				UnPackPlayer(&pkplr, 0, false);

				pfile_SFileCloseArchive(&archive);
				LoadHeroItems(&plr[0]);
				RemoveEmptyInventory(0);
				CalcPlrInv(0, false);

				game_2_ui_player(&plr[0], &uihero, hasSaveGame);
				ui_add_hero_info(&uihero);
			}
			pfile_SFileCloseArchive(&archive);
		}
	}

	return true;
}

bool pfile_archive_contains_game(HANDLE hsArchive)
{
	if (gbIsMultiplayer)
		return false;

	uint32_t dwLen;
	BYTE *gameData = pfile_read_archive(hsArchive, _("game"), &dwLen);
	if (gameData == nullptr)
		return false;

	uint32_t hdr = LOAD_LE32(gameData);
	mem_free_dbg(gameData);

	return IsHeaderValid(hdr);
}

void pfile_ui_set_class_stats(unsigned int player_class_nr, _uidefaultstats *class_stats)
{
	class_stats->strength = StrengthTbl[player_class_nr];
	class_stats->magic = MagicTbl[player_class_nr];
	class_stats->dexterity = DexterityTbl[player_class_nr];
	class_stats->vitality = VitalityTbl[player_class_nr];
}

bool pfile_ui_save_create(_uiheroinfo *heroinfo)
{
	DWORD save_num;
	PkPlayerStruct pkplr;

	save_num = pfile_get_save_num_from_name(heroinfo->name);
	if (save_num >= MAX_CHARACTERS) {
		for (save_num = 0; save_num < MAX_CHARACTERS; save_num++) {
			if (!hero_names[save_num][0])
				break;
		}
		if (save_num >= MAX_CHARACTERS)
			return false;
	}
	if (!pfile_open_archive(save_num))
		return false;
	mpqapi_remove_hash_entries(pfile_get_file_name);
	strncpy(hero_names[save_num], heroinfo->name, PLR_NAME_LEN);
	hero_names[save_num][PLR_NAME_LEN - 1] = '\0';
	CreatePlayer(0, heroinfo->heroclass);
	strncpy(plr[0]._pName, heroinfo->name, PLR_NAME_LEN);
	plr[0]._pName[PLR_NAME_LEN - 1] = '\0';
	PackPlayer(&pkplr, 0, true);
	pfile_encode_hero(&pkplr);
	game_2_ui_player(&plr[0], heroinfo, false);
	if (!gbVanilla) {
		SaveHotkeys();
		SaveHeroItems(&plr[0]);
	}
	mpqapi_flush_and_close(true);
	return true;
}

bool pfile_get_file_name(uint8_t lvl, char *dst)
{
	const char *fmt;

	if (gbIsMultiplayer) {
		if (lvl)
			return false;
		fmt = _("hero");
	} else {
		if (lvl < giNumberOfLevels)
			fmt = _("perml%02d");
		else if (lvl < giNumberOfLevels * 2) {
			lvl -= giNumberOfLevels;
			fmt = _("perms%02d");
		} else if (lvl == giNumberOfLevels * 2)
			fmt = _("game");
		else if (lvl == giNumberOfLevels * 2 + 1)
			fmt = _("hero");
		else
			return false;
	}
	sprintf(dst, fmt, lvl);
	return true;
}

bool pfile_delete_save(_uiheroinfo *hero_info)
{
	DWORD save_num;

	save_num = pfile_get_save_num_from_name(hero_info->name);
	if (save_num < MAX_CHARACTERS) {
		hero_names[save_num][0] = '\0';
		RemoveFile(GetSavePath(save_num).c_str());
	}
	return true;
}

void pfile_read_player_from_save(char name[16], int playerId)
{
	HANDLE archive;
	DWORD save_num;
	PkPlayerStruct pkplr;

	save_num = pfile_get_save_num_from_name(name);
	archive = pfile_open_save_archive(save_num);
	if (archive == nullptr)
		app_fatal(_("Unable to open archive"));
	if (!pfile_read_hero(archive, &pkplr))
		app_fatal(_("Unable to load character"));

	gbValidSaveFile = pfile_archive_contains_game(archive);
	if (gbValidSaveFile)
		pkplr.bIsHellfire = gbIsHellfireSaveGame;

	pfile_SFileCloseArchive(&archive);

	UnPackPlayer(&pkplr, playerId, false);

	LoadHeroItems(&plr[playerId]);
	RemoveEmptyInventory(playerId);
	CalcPlrInv(playerId, false);
}

bool LevelFileExists()
{
	char szName[MAX_PATH];

	GetPermLevelNames(szName);

	DWORD save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	if (!pfile_open_archive(save_num))
		app_fatal(_("Unable to read to save file archive"));

	bool has_file = mpqapi_has_file(szName);
	mpqapi_flush_and_close(true);
	return has_file;
}

void GetTempLevelNames(char *szTemp)
{
	if (setlevel)
		sprintf(szTemp, _("temps%02d"), setlvlnum);
	else
		sprintf(szTemp, _("templ%02d"), currlevel);
}

void GetPermLevelNames(char *szPerm)
{
	DWORD save_num;
	bool has_file;

	save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	GetTempLevelNames(szPerm);
	if (!pfile_open_archive(save_num))
		app_fatal(_("Unable to read to save file archive"));

	has_file = mpqapi_has_file(szPerm);
	mpqapi_flush_and_close(true);
	if (!has_file) {
		if (setlevel)
			sprintf(szPerm, _("perms%02d"), setlvlnum);
		else
			sprintf(szPerm, _("perml%02d"), currlevel);
	}
}

void pfile_remove_temp_files()
{
	if (gbIsMultiplayer)
		return;

	DWORD save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	if (!pfile_open_archive(save_num))
		app_fatal(_("Unable to write to save file archive"));
	mpqapi_remove_hash_entries(GetTempSaveNames);
	mpqapi_flush_and_close(true);
}

void pfile_write_save_file(const char *pszName, BYTE *pbData, DWORD dwLen, DWORD qwLen)
{
	DWORD save_num;

	save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	codec_encode(pbData, dwLen, qwLen, pfile_get_password());
	if (!pfile_open_archive(save_num))
		app_fatal(_("Unable to write to save file archive"));
	mpqapi_write_file(pszName, pbData, qwLen);
	mpqapi_flush_and_close(true);
}

BYTE *pfile_read(const char *pszName, DWORD *pdwLen)
{
	DWORD save_num;
	HANDLE archive;
	BYTE *buf;

	save_num = pfile_get_save_num_from_name(plr[myplr]._pName);
	archive = pfile_open_save_archive(save_num);
	if (archive == nullptr)
		return nullptr;

	buf = pfile_read_archive(archive, pszName, pdwLen);
	pfile_SFileCloseArchive(&archive);
	if (buf == nullptr)
		return nullptr;

	return buf;
}

void pfile_update(bool force_save)
{
	static Uint32 save_prev_tc;

	if (!gbIsMultiplayer)
		return;

	Uint32 tick = SDL_GetTicks();
	if (!force_save && tick - save_prev_tc <= 60000)
		return;

	save_prev_tc = tick;
	pfile_write_hero();
}

} // namespace devilution
