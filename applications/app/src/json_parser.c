#include "json_parser.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/data/json.h>
#include <zephyr/fs/fs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(json_parser, LOG_LEVEL_DBG);

static const struct json_obj_descr zth_coc_magic_points_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct zth_coc_magic_points, maximum, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zth_coc_magic_points, current, JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_hit_points_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct zth_coc_hit_points, maximum, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zth_coc_hit_points, current, JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_sanity_desc[] = {
	JSON_OBJ_DESCR_PRIM(struct zth_coc_sanity, starting, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zth_coc_sanity, current, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zth_coc_sanity, insane, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct zth_coc_sanity, maximum, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_sanity, "Temporary Insanity",
				  temporary_insanity, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_sanity, "Indefinite Insanity",
				  indefinite_insanity, JSON_TOK_TRUE),
};

static const struct json_obj_descr zth_coc_characteristics_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "STR", str,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "CON", con,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "DEX", dex,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "INT", int_stat,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "SIZ", siz,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "POW", pow,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "APP", app,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "EDU", edu,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_characteristics, "Magic Points",
				    magic_points, zth_coc_magic_points_desc),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_characteristics, "Hit Points",
				    hit_points, zth_coc_hit_points_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "Luck", luck,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_characteristics, "Sanity",
				    sanity, zth_coc_sanity_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "Major Wound",
				  major_wound, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "Unconscious",
				  unconscious, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_characteristics, "Dying", dying,
				  JSON_TOK_TRUE),
};

static const struct json_obj_descr zth_coc_skill_firearms_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_firearms, "Handgun",
				  handgun, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_firearms, "Rifle/Shotgun",
				  rifle_shotgun, JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_skill_language_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_language, "English",
				  english, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_language, "Latin", latin,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_language, "Own", own,
				  JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_skill_pilot_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_pilot, "Vehicle", vehicle,
				  JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_skill_science_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_science, "Disciple",
				  disciple, JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_skill_survival_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_survival, "Type", type,
				  JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_skill_other_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_other, "Specialty 1",
				  specialty_1, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_other, "Specialty 2",
				  specialty_2, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skill_other, "Specialty 3",
				  specialty_3, JSON_TOK_NUMBER),
};

static const struct json_obj_descr zth_coc_skills_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Accounting", accounting,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Appraise", appraise,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Archaeology",
				  archaeology, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Art/Craft", art_craft,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Charm", charm,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Climb", climb,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Computer Use",
				  computer_use, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Credit Rating",
				  credit_rating, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Cthulhu Mythos",
				  cthulhu_mythos, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Demolitions",
				  demolitions, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Disguise", disguise,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Diving", diving,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Dodge", dodge,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Drive Auto", drive_auto,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Elec. Repair",
				  elec_repair, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Fast Talk", fast_talk,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Fighting (Brawl)",
				  fighting_brawl, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_skills, "Firearms", firearms,
				    zth_coc_skill_firearms_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "First Aid", first_aid,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "History", history,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Intimidate", intimidate,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Jump", jump,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_skills, "Language", language,
				    zth_coc_skill_language_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Law", law,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Library Use",
				  library_use, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Listen", listen,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Locksmith", locksmith,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Mech. Repair",
				  mech_repair, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Medicine", medicine,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Natural World",
				  natural_world, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Navigate", navigate,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Occult", occult,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Persuade", persuade,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_skills, "Pilot", pilot,
				    zth_coc_skill_pilot_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Psychoanalysis",
				  psychoanalysis, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Psychology", psychology,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Read Lips", read_lips,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Ride", ride,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_skills, "Science", science,
				    zth_coc_skill_science_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Sleight of Hand",
				  sleight_of_hand, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Spot Hidden",
				  spot_hidden, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Stealth", stealth,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_skills, "Survival", survival,
				    zth_coc_skill_survival_desc),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Swim", swim,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Throw", throw_skill,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_skills, "Track", track,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_skills, "Other", other,
				    zth_coc_skill_other_desc),
};

static const struct json_obj_descr zth_coc_weapon_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_weapon, "Name", name,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_weapon, "Skill", skill,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_weapon, "Damage", damage,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_weapon, "num of attacks",
				  num_attacks, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_weapon, "Range", range,
				  JSON_TOK_STRING_BUF),
};

static const struct json_obj_descr zth_coc_weapons_desc[] = {
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_weapons, "Weapon 1", weapon_1,
				    zth_coc_weapon_desc),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_weapons, "Weapon 2", weapon_2,
				    zth_coc_weapon_desc),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_weapons, "Weapon 3", weapon_3,
				    zth_coc_weapon_desc),
};

static const struct json_obj_descr zth_coc_phobias_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_phobias, "Phobia 1", phobia_1,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_phobias, "Phobia 2", phobia_2,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_phobias, "Phobia 3", phobia_3,
				  JSON_TOK_STRING_BUF),
};

static const struct json_obj_descr zth_coc_character_desc[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Name", name,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Pronoun", pronoun,
				  JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Occupation",
				  occupation, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Archetype",
				  archetype, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Residence",
				  residence, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Birthplace",
				  birthplace, JSON_TOK_STRING_BUF),
	JSON_OBJ_DESCR_PRIM_NAMED(struct zth_coc_character, "Age", age,
				  JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_character, "Characteristics",
				    characteristics, zth_coc_characteristics_desc),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_character, "Skills", skills,
				    zth_coc_skills_desc),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_character, "Weapons", weapons,
				    zth_coc_weapons_desc),
	JSON_OBJ_DESCR_OBJECT_NAMED(struct zth_coc_character, "Phobias", phobias,
				    zth_coc_phobias_desc),
};

static int zth_json_encode_coc_character(const struct zth_coc_character *sheet,
					 char **out_buf, size_t *out_len)
{
	ssize_t calc_len;
	char *buffer;
	int ret;

	if (sheet == NULL || out_buf == NULL || out_len == NULL) {
		return -EINVAL;
	}

	calc_len = json_calc_encoded_len(zth_coc_character_desc,
					 ARRAY_SIZE(zth_coc_character_desc), sheet);
	if (calc_len < 0) {
		return (int)calc_len;
	}

	buffer = k_malloc((size_t)calc_len + 1);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	ret = json_obj_encode_buf(zth_coc_character_desc,
				  ARRAY_SIZE(zth_coc_character_desc), sheet,
				  buffer, (size_t)calc_len + 1);
	if (ret < 0) {
		k_free(buffer);
		return ret;
	}

	*out_buf = buffer;
	*out_len = (size_t)calc_len;
	return 0;
}

static int zth_json_copy_file(const char *src_path, const char *dst_path)
{
	struct fs_file_t src;
	struct fs_file_t dst;
	char *buffer = NULL;
	struct fs_dirent entry;
	ssize_t bytes;
	int ret;

	ret = fs_stat(src_path, &entry);
	if (ret) {
		return ret;
	}

	if (entry.size == 0) {
		fs_file_t_init(&dst);
		ret = fs_open(&dst, dst_path, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE);
		if (ret) {
			return ret;
		}
		fs_close(&dst);
		return 0;
	}

	buffer = k_malloc(entry.size);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	fs_file_t_init(&src);
	fs_file_t_init(&dst);
	ret = fs_open(&src, src_path, FS_O_READ);
	if (ret) {
		k_free(buffer);
		return ret;
	}

	bytes = fs_read(&src, buffer, entry.size);
	fs_close(&src);
	if (bytes < 0) {
		k_free(buffer);
		return (int)bytes;
	}

	ret = fs_open(&dst, dst_path, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE);
	if (ret) {
		k_free(buffer);
		return ret;
	}

	bytes = fs_write(&dst, buffer, entry.size);
	fs_close(&dst);
	k_free(buffer);

	return bytes < 0 ? (int)bytes : 0;
}

static int zth_json_build_improve_path(const char *character_path,
				       char *out, size_t out_len)
{
	size_t path_len;
	size_t json_len = strlen(".json");
	size_t suffix_len = strlen(".improve.txt");

	if (character_path == NULL || out == NULL) {
		return -EINVAL;
	}

	path_len = strlen(character_path);
	if (path_len + suffix_len + 1 > out_len) {
		return -ENAMETOOLONG;
	}

	if (path_len > json_len &&
	    strcmp(character_path + path_len - json_len, ".json") == 0) {
		size_t base_len = path_len - json_len;

		if (base_len + suffix_len + 1 > out_len) {
			return -ENAMETOOLONG;
		}

		memcpy(out, character_path, base_len);
		memcpy(out + base_len, ".improve.txt", suffix_len + 1);
		return 0;
	}

	int ret;

	ret = snprintk(out, out_len, "%s%s", character_path, ".improve.txt");
	if (ret < 0 || ret >= (int)out_len) {
		return -ENAMETOOLONG;
	}
	return 0;
}

static int zth_json_trim_line(char *line)
{
	char *start = line;
	char *end;

	if (line == NULL) {
		return -EINVAL;
	}

	while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
		start++;
	}

	if (start != line) {
		memmove(line, start, strlen(start) + 1);
	}

	end = line + strlen(line);
	while (end > line && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' ||
			      end[-1] == '\n')) {
		end--;
	}
	*end = '\0';

	return 0;
}

struct zth_value_offset {
	const char *key;
	size_t offset;
};

static const struct zth_value_offset zth_coc_value_offsets[] = {
	{ "STR", offsetof(struct zth_coc_character, characteristics.str) },
	{ "CON", offsetof(struct zth_coc_character, characteristics.con) },
	{ "DEX", offsetof(struct zth_coc_character, characteristics.dex) },
	{ "INT", offsetof(struct zth_coc_character, characteristics.int_stat) },
	{ "SIZ", offsetof(struct zth_coc_character, characteristics.siz) },
	{ "POW", offsetof(struct zth_coc_character, characteristics.pow) },
	{ "APP", offsetof(struct zth_coc_character, characteristics.app) },
	{ "EDU", offsetof(struct zth_coc_character, characteristics.edu) },
	{ "Luck", offsetof(struct zth_coc_character, characteristics.luck) },
	{ "Accounting", offsetof(struct zth_coc_character, skills.accounting) },
	{ "Appraise", offsetof(struct zth_coc_character, skills.appraise) },
	{ "Archaeology", offsetof(struct zth_coc_character, skills.archaeology) },
	{ "Art/Craft", offsetof(struct zth_coc_character, skills.art_craft) },
	{ "Charm", offsetof(struct zth_coc_character, skills.charm) },
	{ "Climb", offsetof(struct zth_coc_character, skills.climb) },
	{ "Computer Use", offsetof(struct zth_coc_character, skills.computer_use) },
	{ "Credit Rating", offsetof(struct zth_coc_character, skills.credit_rating) },
	{ "Cthulhu Mythos",
	  offsetof(struct zth_coc_character, skills.cthulhu_mythos) },
	{ "Demolitions", offsetof(struct zth_coc_character, skills.demolitions) },
	{ "Disguise", offsetof(struct zth_coc_character, skills.disguise) },
	{ "Diving", offsetof(struct zth_coc_character, skills.diving) },
	{ "Dodge", offsetof(struct zth_coc_character, skills.dodge) },
	{ "Drive Auto", offsetof(struct zth_coc_character, skills.drive_auto) },
	{ "Elec. Repair", offsetof(struct zth_coc_character, skills.elec_repair) },
	{ "Fast Talk", offsetof(struct zth_coc_character, skills.fast_talk) },
	{ "Fighting (Brawl)",
	  offsetof(struct zth_coc_character, skills.fighting_brawl) },
	{ "Handgun", offsetof(struct zth_coc_character, skills.firearms.handgun) },
	{ "Rifle/Shotgun",
	  offsetof(struct zth_coc_character, skills.firearms.rifle_shotgun) },
	{ "First Aid", offsetof(struct zth_coc_character, skills.first_aid) },
	{ "History", offsetof(struct zth_coc_character, skills.history) },
	{ "Intimidate", offsetof(struct zth_coc_character, skills.intimidate) },
	{ "Jump", offsetof(struct zth_coc_character, skills.jump) },
	{ "English", offsetof(struct zth_coc_character, skills.language.english) },
	{ "Latin", offsetof(struct zth_coc_character, skills.language.latin) },
	{ "Own", offsetof(struct zth_coc_character, skills.language.own) },
	{ "Law", offsetof(struct zth_coc_character, skills.law) },
	{ "Library Use", offsetof(struct zth_coc_character, skills.library_use) },
	{ "Listen", offsetof(struct zth_coc_character, skills.listen) },
	{ "Locksmith", offsetof(struct zth_coc_character, skills.locksmith) },
	{ "Mech. Repair", offsetof(struct zth_coc_character, skills.mech_repair) },
	{ "Medicine", offsetof(struct zth_coc_character, skills.medicine) },
	{ "Natural World",
	  offsetof(struct zth_coc_character, skills.natural_world) },
	{ "Navigate", offsetof(struct zth_coc_character, skills.navigate) },
	{ "Occult", offsetof(struct zth_coc_character, skills.occult) },
	{ "Persuade", offsetof(struct zth_coc_character, skills.persuade) },
	{ "Vehicle", offsetof(struct zth_coc_character, skills.pilot.vehicle) },
	{ "Psychoanalysis",
	  offsetof(struct zth_coc_character, skills.psychoanalysis) },
	{ "Psychology", offsetof(struct zth_coc_character, skills.psychology) },
	{ "Read Lips", offsetof(struct zth_coc_character, skills.read_lips) },
	{ "Ride", offsetof(struct zth_coc_character, skills.ride) },
	{ "Disciple", offsetof(struct zth_coc_character, skills.science.disciple) },
	{ "Sleight of Hand",
	  offsetof(struct zth_coc_character, skills.sleight_of_hand) },
	{ "Spot Hidden", offsetof(struct zth_coc_character, skills.spot_hidden) },
	{ "Stealth", offsetof(struct zth_coc_character, skills.stealth) },
	{ "Type", offsetof(struct zth_coc_character, skills.survival.type) },
	{ "Swim", offsetof(struct zth_coc_character, skills.swim) },
	{ "Throw", offsetof(struct zth_coc_character, skills.throw_skill) },
	{ "Track", offsetof(struct zth_coc_character, skills.track) },
	{ "Specialty 1",
	  offsetof(struct zth_coc_character, skills.other.specialty_1) },
	{ "Specialty 2",
	  offsetof(struct zth_coc_character, skills.other.specialty_2) },
	{ "Specialty 3",
	  offsetof(struct zth_coc_character, skills.other.specialty_3) },
};

struct zth_skill_field {
	const char *name;
	size_t offset;
};

struct zth_skill_group {
	const char *name;
	const struct zth_skill_field *fields;
	size_t field_count;
};

struct zth_skill_entry {
	const char *name;
	size_t offset;
	const struct zth_skill_group *group;
	bool is_group;
};

static const struct zth_skill_field zth_coc_firearms_fields[] = {
	{ "Handgun", offsetof(struct zth_coc_character, skills.firearms.handgun) },
	{ "Rifle/Shotgun",
	  offsetof(struct zth_coc_character, skills.firearms.rifle_shotgun) },
};

static const struct zth_skill_field zth_coc_language_fields[] = {
	{ "English", offsetof(struct zth_coc_character, skills.language.english) },
	{ "Latin", offsetof(struct zth_coc_character, skills.language.latin) },
	{ "Own", offsetof(struct zth_coc_character, skills.language.own) },
};

static const struct zth_skill_field zth_coc_pilot_fields[] = {
	{ "Vehicle", offsetof(struct zth_coc_character, skills.pilot.vehicle) },
};

static const struct zth_skill_field zth_coc_science_fields[] = {
	{ "Disciple", offsetof(struct zth_coc_character, skills.science.disciple) },
};

static const struct zth_skill_field zth_coc_survival_fields[] = {
	{ "Type", offsetof(struct zth_coc_character, skills.survival.type) },
};

static const struct zth_skill_field zth_coc_other_fields[] = {
	{ "Specialty 1",
	  offsetof(struct zth_coc_character, skills.other.specialty_1) },
	{ "Specialty 2",
	  offsetof(struct zth_coc_character, skills.other.specialty_2) },
	{ "Specialty 3",
	  offsetof(struct zth_coc_character, skills.other.specialty_3) },
};

static const struct zth_skill_group zth_coc_firearms_group = {
	.name = "Firearms",
	.fields = zth_coc_firearms_fields,
	.field_count = ARRAY_SIZE(zth_coc_firearms_fields),
};

static const struct zth_skill_group zth_coc_language_group = {
	.name = "Language",
	.fields = zth_coc_language_fields,
	.field_count = ARRAY_SIZE(zth_coc_language_fields),
};

static const struct zth_skill_group zth_coc_pilot_group = {
	.name = "Pilot",
	.fields = zth_coc_pilot_fields,
	.field_count = ARRAY_SIZE(zth_coc_pilot_fields),
};

static const struct zth_skill_group zth_coc_science_group = {
	.name = "Science",
	.fields = zth_coc_science_fields,
	.field_count = ARRAY_SIZE(zth_coc_science_fields),
};

static const struct zth_skill_group zth_coc_survival_group = {
	.name = "Survival",
	.fields = zth_coc_survival_fields,
	.field_count = ARRAY_SIZE(zth_coc_survival_fields),
};

static const struct zth_skill_group zth_coc_other_group = {
	.name = "Other",
	.fields = zth_coc_other_fields,
	.field_count = ARRAY_SIZE(zth_coc_other_fields),
};

static const struct zth_skill_entry zth_coc_skill_entries[] = {
	{ "Accounting", offsetof(struct zth_coc_character, skills.accounting),
	  NULL, false },
	{ "Appraise", offsetof(struct zth_coc_character, skills.appraise), NULL,
	  false },
	{ "Archaeology", offsetof(struct zth_coc_character, skills.archaeology),
	  NULL, false },
	{ "Art/Craft", offsetof(struct zth_coc_character, skills.art_craft), NULL,
	  false },
	{ "Charm", offsetof(struct zth_coc_character, skills.charm), NULL, false },
	{ "Climb", offsetof(struct zth_coc_character, skills.climb), NULL, false },
	{ "Computer Use", offsetof(struct zth_coc_character, skills.computer_use),
	  NULL, false },
	{ "Credit Rating",
	  offsetof(struct zth_coc_character, skills.credit_rating), NULL, false },
	{ "Cthulhu Mythos",
	  offsetof(struct zth_coc_character, skills.cthulhu_mythos), NULL, false },
	{ "Demolitions", offsetof(struct zth_coc_character, skills.demolitions),
	  NULL, false },
	{ "Disguise", offsetof(struct zth_coc_character, skills.disguise), NULL,
	  false },
	{ "Diving", offsetof(struct zth_coc_character, skills.diving), NULL, false },
	{ "Dodge", offsetof(struct zth_coc_character, skills.dodge), NULL, false },
	{ "Drive Auto", offsetof(struct zth_coc_character, skills.drive_auto),
	  NULL, false },
	{ "Elec. Repair", offsetof(struct zth_coc_character, skills.elec_repair),
	  NULL, false },
	{ "Fast Talk", offsetof(struct zth_coc_character, skills.fast_talk), NULL,
	  false },
	{ "Fighting (Brawl)",
	  offsetof(struct zth_coc_character, skills.fighting_brawl), NULL, false },
	{ "Firearms", 0, &zth_coc_firearms_group, true },
	{ "First Aid", offsetof(struct zth_coc_character, skills.first_aid), NULL,
	  false },
	{ "History", offsetof(struct zth_coc_character, skills.history), NULL,
	  false },
	{ "Intimidate", offsetof(struct zth_coc_character, skills.intimidate),
	  NULL, false },
	{ "Jump", offsetof(struct zth_coc_character, skills.jump), NULL, false },
	{ "Language", 0, &zth_coc_language_group, true },
	{ "Law", offsetof(struct zth_coc_character, skills.law), NULL, false },
	{ "Library Use", offsetof(struct zth_coc_character, skills.library_use),
	  NULL, false },
	{ "Listen", offsetof(struct zth_coc_character, skills.listen), NULL, false },
	{ "Locksmith", offsetof(struct zth_coc_character, skills.locksmith), NULL,
	  false },
	{ "Mech. Repair", offsetof(struct zth_coc_character, skills.mech_repair),
	  NULL, false },
	{ "Medicine", offsetof(struct zth_coc_character, skills.medicine), NULL,
	  false },
	{ "Natural World",
	  offsetof(struct zth_coc_character, skills.natural_world), NULL, false },
	{ "Navigate", offsetof(struct zth_coc_character, skills.navigate), NULL,
	  false },
	{ "Occult", offsetof(struct zth_coc_character, skills.occult), NULL, false },
	{ "Persuade", offsetof(struct zth_coc_character, skills.persuade), NULL,
	  false },
	{ "Pilot", 0, &zth_coc_pilot_group, true },
	{ "Psychoanalysis",
	  offsetof(struct zth_coc_character, skills.psychoanalysis), NULL, false },
	{ "Psychology", offsetof(struct zth_coc_character, skills.psychology), NULL,
	  false },
	{ "Read Lips", offsetof(struct zth_coc_character, skills.read_lips), NULL,
	  false },
	{ "Ride", offsetof(struct zth_coc_character, skills.ride), NULL, false },
	{ "Science", 0, &zth_coc_science_group, true },
	{ "Sleight of Hand",
	  offsetof(struct zth_coc_character, skills.sleight_of_hand), NULL, false },
	{ "Spot Hidden", offsetof(struct zth_coc_character, skills.spot_hidden),
	  NULL, false },
	{ "Stealth", offsetof(struct zth_coc_character, skills.stealth), NULL,
	  false },
	{ "Survival", 0, &zth_coc_survival_group, true },
	{ "Swim", offsetof(struct zth_coc_character, skills.swim), NULL, false },
	{ "Throw", offsetof(struct zth_coc_character, skills.throw_skill), NULL,
	  false },
	{ "Track", offsetof(struct zth_coc_character, skills.track), NULL, false },
	{ "Other", 0, &zth_coc_other_group, true },
};

int zth_json_load_coc_character(const char *path,
				struct zth_coc_character *out)
{
	struct fs_file_t file;
	struct fs_dirent entry;
	char *buffer = NULL;
	ssize_t bytes;
	int ret;

	if (path == NULL || out == NULL) {
		return -EINVAL;
	}

	ret = fs_stat(path, &entry);
	if (ret) {
		return ret;
	}

	buffer = k_malloc(entry.size + 1);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_READ);
	if (ret) {
		k_free(buffer);
		return ret;
	}

	bytes = fs_read(&file, buffer, entry.size);
	fs_close(&file);
	if (bytes < 0) {
		k_free(buffer);
		return (int)bytes;
	}

	buffer[bytes] = '\0';
	ret = zth_json_parse_coc_character(buffer, (size_t)bytes, out);
	k_free(buffer);
	return ret;
}

int zth_json_parse_coc_character(const char *json, size_t len,
				 struct zth_coc_character *out)
{
	char *buffer = NULL;
	size_t json_len;
	int ret;

	if (json == NULL || out == NULL) {
		return -EINVAL;
	}

	json_len = len ? len : strlen(json);
	buffer = k_malloc(json_len + 1);
	if (buffer == NULL) {
		return -ENOMEM;
	}

	memcpy(buffer, json, json_len);
	buffer[json_len] = '\0';

	memset(out, 0, sizeof(*out));
	ret = (int)json_obj_parse(buffer, json_len, zth_coc_character_desc,
				  ARRAY_SIZE(zth_coc_character_desc), out);
	if (ret < 0) {
		LOG_ERR("Failed to parse JSON: %d", ret);
	}

	k_free(buffer);
	return ret < 0 ? ret : 0;
}

int zth_json_save_coc_character(const char *path,
				const struct zth_coc_character *sheet)
{
	char backup_path[ZTH_JSON_PATH_MAX];
	char *json_buf = NULL;
	size_t json_len = 0;
	struct fs_file_t file;
	int ret;

	if (path == NULL || sheet == NULL) {
		return -EINVAL;
	}

	ret = snprintk(backup_path, sizeof(backup_path), "%s.bak", path);
	if (ret < 0 || ret >= (int)sizeof(backup_path)) {
		return -ENAMETOOLONG;
	}

	ret = zth_json_encode_coc_character(sheet, &json_buf, &json_len);
	if (ret) {
		return ret;
	}

	ret = zth_json_copy_file(path, backup_path);
	if (ret) {
		LOG_WRN("Failed to create backup %s: %d", backup_path, ret);
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE);
	if (ret) {
		k_free(json_buf);
		return ret;
	}

	size_t total = 0;
	while (total < json_len) {
		ret = fs_write(&file, json_buf + total, json_len - total);
		if (ret < 0) {
			break;
		}
		if (ret == 0) {
			ret = -EIO;
			break;
		}
		total += (size_t)ret;
	}
	fs_close(&file);
	k_free(json_buf);

	return ret < 0 ? ret : 0;
}

int zth_coc_get_value_at_key(const struct zth_coc_character *sheet,
			     const char *key, int32_t *out)
{
	if (sheet == NULL || key == NULL || out == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(zth_coc_value_offsets); i++) {
		if (strcmp(key, zth_coc_value_offsets[i].key) == 0) {
			const int32_t *value = (const int32_t *)((const uint8_t *)sheet +
								 zth_coc_value_offsets[i].offset);
			*out = *value;
			return 0;
		}
	}

	return -ENOENT;
}

void zth_coc_iter_skills(const struct zth_coc_character *sheet,
			 zth_coc_skill_cb cb, void *user)
{
	if (sheet == NULL || cb == NULL) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(zth_coc_skill_entries); i++) {
		const struct zth_skill_entry *entry = &zth_coc_skill_entries[i];

		if (entry->is_group && entry->group != NULL) {
			cb(entry->name, 0, 0, true, user);
			for (size_t j = 0; j < entry->group->field_count; j++) {
				const struct zth_skill_field *field =
					&entry->group->fields[j];
				const int32_t *value = (const int32_t *)((const uint8_t *)sheet +
									 field->offset);
				cb(field->name, *value, 1, false, user);
			}
		} else {
			const int32_t *value = (const int32_t *)((const uint8_t *)sheet +
								 entry->offset);
			cb(entry->name, *value, 0, false, user);
		}
	}
}

int zth_coc_load_improvements(const char *character_path,
			      struct zth_skill_improvements *out)
{
	struct fs_file_t file;
	char path[ZTH_JSON_PATH_MAX];
	char buffer[ZTH_IMPROVE_MAX * ZTH_IMPROVE_LINE_MAX];
	ssize_t bytes;
	char *line;
	char *cursor;
	char *eol;
	int ret;

	if (character_path == NULL || out == NULL) {
		return -EINVAL;
	}

	memset(out, 0, sizeof(*out));

	ret = zth_json_build_improve_path(character_path, path, sizeof(path));
	if (ret) {
		return ret;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_READ);
	if (ret == -ENOENT) {
		return 0;
	}
	if (ret) {
		return ret;
	}

	bytes = fs_read(&file, buffer, sizeof(buffer) - 1);
	fs_close(&file);
	if (bytes < 0) {
		return (int)bytes;
	}

	buffer[bytes] = '\0';
	cursor = buffer;
	while (cursor != NULL && *cursor != '\0' && out->count < ZTH_IMPROVE_MAX) {
		eol = strchr(cursor, '\n');
		if (eol != NULL) {
			*eol = '\0';
		}

		line = cursor;
		zth_json_trim_line(line);
		if (line[0] != '\0') {
			strncpy(out->entries[out->count], line,
				ZTH_IMPROVE_LINE_MAX - 1);
			out->entries[out->count][ZTH_IMPROVE_LINE_MAX - 1] = '\0';
			out->count++;
		}
		cursor = (eol != NULL) ? (eol + 1) : NULL;
	}

	return 0;
}

static int zth_improvement_cmp(const void *a, const void *b)
{
	const char *left = a;
	const char *right = b;

	return strcmp(left, right);
}

int zth_coc_save_improvements(const char *character_path,
			      const struct zth_skill_improvements *in)
{
	struct fs_file_t file;
	struct zth_skill_improvements tmp;
	char path[ZTH_JSON_PATH_MAX];
	int ret;

	if (character_path == NULL || in == NULL) {
		return -EINVAL;
	}

	ret = zth_json_build_improve_path(character_path, path, sizeof(path));
	if (ret) {
		return ret;
	}

	tmp = *in;
	if (tmp.count > 1) {
		qsort(tmp.entries, tmp.count, sizeof(tmp.entries[0]),
		      zth_improvement_cmp);
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE);
	if (ret) {
		return ret;
	}

	for (size_t i = 0; i < tmp.count; i++) {
		ret = fs_write(&file, tmp.entries[i], strlen(tmp.entries[i]));
		if (ret < 0) {
			fs_close(&file);
			return ret;
		}
		ret = fs_write(&file, "\n", 1);
		if (ret < 0) {
			fs_close(&file);
			return ret;
		}
	}

	fs_close(&file);
	return 0;
}

int zth_coc_mark_improvement(struct zth_skill_improvements *improvements,
			     const char *skill)
{
	if (improvements == NULL || skill == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < improvements->count; i++) {
		if (strcmp(improvements->entries[i], skill) == 0) {
			return 0;
		}
	}

	if (improvements->count >= ZTH_IMPROVE_MAX) {
		return -ENOSPC;
	}

	strncpy(improvements->entries[improvements->count], skill,
		ZTH_IMPROVE_LINE_MAX - 1);
	improvements->entries[improvements->count][ZTH_IMPROVE_LINE_MAX - 1] = '\0';
	improvements->count++;
	return 0;
}

int zth_coc_unmark_improvement(struct zth_skill_improvements *improvements,
			       const char *skill)
{
	bool found = false;

	if (improvements == NULL || skill == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < improvements->count; i++) {
		if (!found && strcmp(improvements->entries[i], skill) == 0) {
			found = true;
			continue;
		}

		if (found) {
			strncpy(improvements->entries[i - 1], improvements->entries[i],
				ZTH_IMPROVE_LINE_MAX - 1);
			improvements->entries[i - 1][ZTH_IMPROVE_LINE_MAX - 1] = '\0';
		}
	}

	if (found) {
		improvements->count--;
	}

	return 0;
}
