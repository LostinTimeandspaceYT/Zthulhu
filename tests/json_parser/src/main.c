#include <errno.h>
#include <string.h>

#include <ff.h>
#include <zephyr/fs/fs.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/ztest.h>

#include "json_parser.h"

static const char zth_sample_json[] =
	"{"
	"\"Name\":\"Randolph Carter\","
	"\"Pronoun\":\"He/Him\","
	"\"Occupation\":\"Student\","
	"\"Residence\":\"Arkham, MA\","
	"\"Birthplace\":\"\","
	"\"Age\":18,"
	"\"Characteristics\":{"
	"  \"STR\":50,"
	"  \"CON\":50,"
	"  \"DEX\":60,"
	"  \"INT\":80,"
	"  \"SIZ\":60,"
	"  \"POW\":75,"
	"  \"APP\":50,"
	"  \"EDU\":70,"
	"  \"Magic Points\":{\"Maximum\":24,\"Current\":12},"
	"  \"Hit Points\":{\"Maximum\":10,\"Current\":9},"
	"  \"Luck\":67,"
	"  \"Sanity\":{"
	"    \"Starting\":75,"
	"    \"Current\":75,"
	"    \"Insane\":0,"
	"    \"Maximum\":0,"
	"    \"Temporary Insanity\":false,"
	"    \"Indefinite Insanity\":true"
	"  },"
	"  \"Major Wound\":false,"
	"  \"Unconscious\":false,"
	"  \"Dying\":false"
	"},"
	"\"Skills\":{"
	"  \"Accounting\":5,"
	"  \"Firearms\":{\"Handgun\":20,\"Rifle/Shotgun\":25},"
	"  \"Language\":{\"English\":10,\"Own\":70},"
	"  \"Pilot\":{\"Vehicle\":1},"
	"  \"Science\":{\"Disciple\":1},"
	"  \"Survival\":{\"Type\":10},"
	"  \"Other\":{\"Specialty 1\":0,\"Specialty 2\":0,\"Specialty 3\":0}"
	"},"
	"\"Weapons\":{"
	"  \"Weapon 1\":{"
	"    \"Name\":\"Brawl\","
	"    \"Skill\":\"Fighting (Brawl)\","
	"    \"Damage\":\"1d3+db\","
	"    \"num of attacks\":1,"
	"    \"Range\":\"\""
	"  },"
	"  \"Weapon 2\":{"
	"    \"Name\":\".25 Derringer\","
	"    \"Skill\":\"Handgun\","
	"    \"Damage\":\"1d6\","
	"    \"num of attacks\":1,"
	"    \"Range\":\"3 yards\""
	"  },"
	"  \"Weapon 3\":{"
	"    \"Name\":\"\","
	"    \"Skill\":\"\","
	"    \"Damage\":\"\","
	"    \"num of attacks\":1,"
	"    \"Range\":\"\""
	"  }"
	"},"
	"\"Phobias\":{\"Phobia 1\":\"\",\"Phobia 2\":\"\",\"Phobia 3\":\"\"}"
	"}";

#define ZTH_TEST_DISK "RAM"
#define ZTH_TEST_MOUNT "/RAM:"
#define ZTH_TEST_DIR ZTH_TEST_MOUNT "/characters/call_of_cthulhu"
#define ZTH_TEST_PATH ZTH_TEST_DIR "/randolph_carter.json"

static FATFS zth_test_fatfs;

static struct fs_mount_t zth_test_mount = {
	.type = FS_FATFS,
	.mnt_point = ZTH_TEST_MOUNT,
	.fs_data = &zth_test_fatfs,
	.storage_dev = (void *)ZTH_TEST_DISK,
};

static int zth_test_setup_fs(void)
{
	static const MKFS_PARM mkfs_cfg = {
		.fmt = FM_ANY | FM_SFD,
		.n_fat = 1,
		.align = 0,
		.n_root = CONFIG_FS_FATFS_MAX_ROOT_ENTRIES,
		.au_size = 0,
	};
	int ret;

	ret = disk_access_init(ZTH_TEST_DISK);
	if (ret) {
		return ret;
	}

	ret = fs_mkfs(FS_FATFS, (uintptr_t)ZTH_TEST_DISK ":", &mkfs_cfg, 0);
	if (ret) {
		return ret;
	}

	ret = fs_mount(&zth_test_mount);
	if (ret) {
		return ret;
	}

	ret = fs_mkdir(ZTH_TEST_MOUNT "/characters");
	if (ret && ret != -EEXIST) {
		return ret;
	}

	ret = fs_mkdir(ZTH_TEST_DIR);
	if (ret && ret != -EEXIST) {
		return ret;
	}

	return 0;
}

static void zth_test_teardown_fs(void)
{
	fs_unmount(&zth_test_mount);
}

static int zth_test_write_file(const char *path, const char *contents)
{
	struct fs_file_t file;
	int ret;

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_TRUNC | FS_O_WRITE);
	if (ret) {
		return ret;
	}

	ret = fs_write(&file, contents, strlen(contents));
	fs_close(&file);
	return ret < 0 ? ret : 0;
}

static int zth_test_read_file(const char *path, char *buf, size_t buf_len)
{
	struct fs_file_t file;
	ssize_t bytes;
	int ret;

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_READ);
	if (ret) {
		return ret;
	}

	bytes = fs_read(&file, buf, buf_len - 1);
	fs_close(&file);
	if (bytes < 0) {
		return (int)bytes;
	}

	buf[bytes] = '\0';
	return 0;
}

ZTEST(json_parser, test_parse_basic)
{
	struct zth_coc_character sheet;
	int32_t value = 0;
	int ret = zth_json_parse_coc_character(zth_sample_json, 0, &sheet);

	zassert_ok(ret, "JSON parse failed: %d", ret);
	zassert_equal(strcmp(sheet.name, "Randolph Carter"), 0, "Name mismatch");
	zassert_equal(strcmp(sheet.pronoun, "He/Him"), 0, "Pronoun mismatch");
	zassert_equal(sheet.age, 18, "Age mismatch");
	zassert_equal(sheet.characteristics.str, 50, "STR mismatch");
	zassert_equal(sheet.characteristics.sanity.indefinite_insanity, true,
		      "Sanity flag mismatch");
	zassert_equal(sheet.weapons.weapon_2.num_attacks, 1, "Weapon parse mismatch");
	zassert_equal(strcmp(sheet.weapons.weapon_2.range, "3 yards"), 0,
		      "Weapon range mismatch");

	ret = zth_coc_get_value_at_key(&sheet, "STR", &value);
	zassert_ok(ret, "Failed to read STR");
	zassert_equal(value, 50, "STR value mismatch");

	ret = zth_coc_get_value_at_key(&sheet, "Handgun", &value);
	zassert_ok(ret, "Failed to read Handgun");
	zassert_equal(value, 20, "Handgun value mismatch");
}

struct skill_capture {
	bool saw_firearms;
	bool saw_handgun;
	bool saw_language;
	int32_t handgun_value;
};

static void skill_cb(const char *name, int32_t value, int indent, bool is_group,
		     void *user)
{
	struct skill_capture *capture = user;

	if (is_group && strcmp(name, "Firearms") == 0) {
		capture->saw_firearms = true;
	}

	if (!is_group && indent == 1 && strcmp(name, "Handgun") == 0) {
		capture->saw_handgun = true;
		capture->handgun_value = value;
	}

	if (is_group && strcmp(name, "Language") == 0) {
		capture->saw_language = true;
	}
}

ZTEST(json_parser, test_iter_skills)
{
	struct zth_coc_character sheet;
	struct skill_capture capture = {0};
	int ret = zth_json_parse_coc_character(zth_sample_json, 0, &sheet);

	zassert_ok(ret, "JSON parse failed: %d", ret);

	zth_coc_iter_skills(&sheet, skill_cb, &capture);
	zassert_true(capture.saw_firearms, "Firearms group missing");
	zassert_true(capture.saw_language, "Language group missing");
	zassert_true(capture.saw_handgun, "Handgun entry missing");
	zassert_equal(capture.handgun_value, 20, "Handgun value mismatch");
}

ZTEST(json_parser, test_save_backup_and_improvements)
{
	struct zth_coc_character sheet;
	struct zth_coc_character loaded = {0};
	struct zth_skill_improvements improvements;
	struct zth_skill_improvements reloaded;
	char backup_path[ZTH_JSON_PATH_MAX];
	char original_buf[128];
	char backup_buf[128];
	int ret;

	ret = zth_test_setup_fs();
	zassert_ok(ret, "fs setup failed: %d", ret);

	ret = zth_test_write_file(ZTH_TEST_PATH, "{\"Name\":\"Old\"}");
	zassert_ok(ret, "failed to write original file: %d", ret);

	ret = zth_json_parse_coc_character(zth_sample_json, 0, &sheet);
	zassert_ok(ret, "JSON parse failed: %d", ret);

	ret = zth_json_save_coc_character(ZTH_TEST_PATH, &sheet);
	zassert_ok(ret, "save failed: %d", ret);

	ret = zth_test_read_file(ZTH_TEST_PATH, original_buf, sizeof(original_buf));
	zassert_ok(ret, "read saved file failed: %d", ret);
	zassert_true(strstr(original_buf, "\"Name\":\"Randolph Carter\"") != NULL,
		     "saved JSON missing name");

	ret = snprintk(backup_path, sizeof(backup_path), "%s.bak", ZTH_TEST_PATH);
	zassert_true(ret > 0 && ret < (int)sizeof(backup_path), "backup path invalid");

	ret = zth_test_read_file(backup_path, backup_buf, sizeof(backup_buf));
	zassert_ok(ret, "read backup file failed: %d", ret);
	zassert_true(strstr(backup_buf, "\"Name\":\"Old\"") != NULL,
		     "backup content mismatch");

	ret = zth_json_load_coc_character(ZTH_TEST_PATH, &loaded);
	zassert_ok(ret, "load failed: %d", ret);
	zassert_equal(strcmp(loaded.name, "Randolph Carter"), 0,
		      "loaded name mismatch");

	ret = zth_coc_load_improvements(ZTH_TEST_PATH, &improvements);
	zassert_ok(ret, "load improvements failed: %d", ret);
	zassert_equal(improvements.count, 0, "expected empty improvements");

	ret = zth_coc_mark_improvement(&improvements, "Accounting");
	zassert_ok(ret, "mark improvement failed: %d", ret);
	ret = zth_coc_mark_improvement(&improvements, "Stealth");
	zassert_ok(ret, "mark improvement failed: %d", ret);

	ret = zth_coc_save_improvements(ZTH_TEST_PATH, &improvements);
	zassert_ok(ret, "save improvements failed: %d", ret);

	ret = zth_coc_load_improvements(ZTH_TEST_PATH, &reloaded);
	zassert_ok(ret, "reload improvements failed: %d", ret);
	zassert_equal(reloaded.count, 2, "unexpected improvements count");

	ret = zth_coc_unmark_improvement(&reloaded, "Accounting");
	zassert_ok(ret, "unmark improvement failed: %d", ret);
	ret = zth_coc_save_improvements(ZTH_TEST_PATH, &reloaded);
	zassert_ok(ret, "save improvements failed: %d", ret);

	ret = zth_coc_load_improvements(ZTH_TEST_PATH, &reloaded);
	zassert_ok(ret, "reload improvements failed: %d", ret);
	zassert_equal(reloaded.count, 1, "unexpected improvements count");
	zassert_equal(strcmp(reloaded.entries[0], "Stealth"), 0,
		      "remaining improvement mismatch");

	zth_test_teardown_fs();
}

ZTEST_SUITE(json_parser, NULL, NULL, NULL, NULL, NULL);
