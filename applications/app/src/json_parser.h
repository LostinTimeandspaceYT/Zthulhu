#ifndef ZTH_JSON_PARSER_H
#define ZTH_JSON_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZTH_JSON_STR_MAX 64
#define ZTH_JSON_LONG_STR_MAX 128
#define ZTH_JSON_PATH_MAX 256
#define ZTH_IMPROVE_MAX 32
#define ZTH_IMPROVE_LINE_MAX 64

struct zth_coc_magic_points {
	int32_t maximum;
	int32_t current;
};

struct zth_coc_hit_points {
	int32_t maximum;
	int32_t current;
};

struct zth_coc_sanity {
	int32_t starting;
	int32_t current;
	int32_t insane;
	int32_t maximum;
	bool temporary_insanity;
	bool indefinite_insanity;
};

struct zth_coc_characteristics {
	int32_t str;
	int32_t con;
	int32_t dex;
	int32_t int_stat;
	int32_t siz;
	int32_t pow;
	int32_t app;
	int32_t edu;
	struct zth_coc_magic_points magic_points;
	struct zth_coc_hit_points hit_points;
	int32_t luck;
	struct zth_coc_sanity sanity;
	bool major_wound;
	bool unconscious;
	bool dying;
};

struct zth_coc_skill_firearms {
	int32_t handgun;
	int32_t rifle_shotgun;
};

struct zth_coc_skill_language {
	int32_t english;
	int32_t latin;
	int32_t own;
};

struct zth_coc_skill_pilot {
	int32_t vehicle;
};

struct zth_coc_skill_science {
	int32_t disciple;
};

struct zth_coc_skill_survival {
	int32_t type;
};

struct zth_coc_skill_other {
	int32_t specialty_1;
	int32_t specialty_2;
	int32_t specialty_3;
};

struct zth_coc_skills {
	int32_t accounting;
	int32_t appraise;
	int32_t archaeology;
	int32_t art_craft;
	int32_t charm;
	int32_t climb;
	int32_t computer_use;
	int32_t credit_rating;
	int32_t cthulhu_mythos;
	int32_t demolitions;
	int32_t disguise;
	int32_t diving;
	int32_t dodge;
	int32_t drive_auto;
	int32_t elec_repair;
	int32_t fast_talk;
	int32_t fighting_brawl;
	struct zth_coc_skill_firearms firearms;
	int32_t first_aid;
	int32_t history;
	int32_t intimidate;
	int32_t jump;
	struct zth_coc_skill_language language;
	int32_t law;
	int32_t library_use;
	int32_t listen;
	int32_t locksmith;
	int32_t mech_repair;
	int32_t medicine;
	int32_t natural_world;
	int32_t navigate;
	int32_t occult;
	int32_t persuade;
	struct zth_coc_skill_pilot pilot;
	int32_t psychoanalysis;
	int32_t psychology;
	int32_t read_lips;
	int32_t ride;
	struct zth_coc_skill_science science;
	int32_t sleight_of_hand;
	int32_t spot_hidden;
	int32_t stealth;
	struct zth_coc_skill_survival survival;
	int32_t swim;
	int32_t throw_skill;
	int32_t track;
	struct zth_coc_skill_other other;
};

struct zth_coc_weapon {
	char name[ZTH_JSON_STR_MAX];
	char skill[ZTH_JSON_STR_MAX];
	char damage[ZTH_JSON_STR_MAX];
	int32_t num_attacks;
	char range[ZTH_JSON_STR_MAX];
};

struct zth_coc_weapons {
	struct zth_coc_weapon weapon_1;
	struct zth_coc_weapon weapon_2;
	struct zth_coc_weapon weapon_3;
};

struct zth_coc_phobias {
	char phobia_1[ZTH_JSON_STR_MAX];
	char phobia_2[ZTH_JSON_STR_MAX];
	char phobia_3[ZTH_JSON_STR_MAX];
};

struct zth_coc_character {
	char name[ZTH_JSON_STR_MAX];
	char pronoun[ZTH_JSON_STR_MAX];
	char occupation[ZTH_JSON_STR_MAX];
	char archetype[ZTH_JSON_STR_MAX];
	char residence[ZTH_JSON_STR_MAX];
	char birthplace[ZTH_JSON_STR_MAX];
	int32_t age;
	struct zth_coc_characteristics characteristics;
	struct zth_coc_skills skills;
	struct zth_coc_weapons weapons;
	struct zth_coc_phobias phobias;
};

struct zth_skill_improvements {
	char entries[ZTH_IMPROVE_MAX][ZTH_IMPROVE_LINE_MAX];
	size_t count;
};

typedef void (*zth_coc_skill_cb)(const char *name, int32_t value, int indent,
				 bool is_group, void *user);

int zth_json_load_coc_character(const char *path,
				struct zth_coc_character *out);
int zth_json_parse_coc_character(const char *json, size_t len,
				 struct zth_coc_character *out);
int zth_json_save_coc_character(const char *path,
				const struct zth_coc_character *sheet);
int zth_coc_get_value_at_key(const struct zth_coc_character *sheet,
			     const char *key, int32_t *out);
void zth_coc_iter_skills(const struct zth_coc_character *sheet,
			 zth_coc_skill_cb cb, void *user);
int zth_coc_load_improvements(const char *character_path,
			      struct zth_skill_improvements *out);
int zth_coc_save_improvements(const char *character_path,
			      const struct zth_skill_improvements *in);
int zth_coc_mark_improvement(struct zth_skill_improvements *improvements,
			     const char *skill);
int zth_coc_unmark_improvement(struct zth_skill_improvements *improvements,
			       const char *skill);

#ifdef __cplusplus
}
#endif

#endif /* ZTH_JSON_PARSER_H */
