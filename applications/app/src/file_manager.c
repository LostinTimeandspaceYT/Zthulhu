#include "file_manager.h"

#include <string.h>

#include <zephyr/fs/fs.h>
#include <ff.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/storage/disk_access.h>

LOG_MODULE_REGISTER(file_manager, LOG_LEVEL_DBG);

static FATFS zth_fat_fs;
static struct fs_mount_t zth_mount = {
	.type = FS_FATFS,
	.fs_data = &zth_fat_fs,
	.mnt_point = ZTH_MOUNT_POINT,
	.storage_dev = (void *)ZTH_SD_DISK_NAME,
};

static int zth_file_manager_make_path(char *out, size_t out_len,
				      const char *prefix, const char *suffix,
				      const char *ext)
{
	int ret = snprintk(out, out_len, "%s%s%s", prefix, suffix, ext);

	if (ret < 0 || (size_t)ret >= out_len) {
		return -ENAMETOOLONG;
	}

	return 0;
}

int zth_file_manager_mount_sdcard(void)
{
	int ret = disk_access_init(ZTH_SD_DISK_NAME);

	if (ret) {
		LOG_ERR("disk_access_init(%s) failed: %d", ZTH_SD_DISK_NAME, ret);
		return ret;
	}

	ret = fs_mount(&zth_mount);
	if (ret) {
		LOG_ERR("fs_mount(%s) failed: %d", ZTH_MOUNT_POINT, ret);
	}

	return ret;
}

int zth_file_manager_write_to(const char *path, const char *contents)
{
	struct fs_file_t file;
	ssize_t wrote;
	int ret;

	if (path == NULL || contents == NULL) {
		return -EINVAL;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (ret) {
		return ret;
	}

	ret = fs_seek(&file, 0, FS_SEEK_END);
	if (ret) {
		fs_close(&file);
		return ret;
	}

	wrote = fs_write(&file, contents, strlen(contents));
	if (wrote < 0) {
		ret = (int)wrote;
	}

	fs_close(&file);
	return ret;
}

int zth_file_manager_get_character_path(char *out, size_t out_len,
					const char *game,
					const char *character_name)
{
	if (out == NULL || game == NULL || character_name == NULL) {
		return -EINVAL;
	}

	char tmp[ZTH_FILE_MANAGER_PATH_MAX];
	int ret = zth_file_manager_make_path(tmp, sizeof(tmp), ZTH_CHARACTER_PATH,
					     game, "/");
	if (ret) {
		return ret;
	}

	return zth_file_manager_make_path(out, out_len, tmp, character_name,
					  ".json");
}

static int zth_file_manager_list_dir(const char *path, bool strip_json,
				     zth_file_name_cb cb, void *user)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int ret;

	if (path == NULL || cb == NULL) {
		return -EINVAL;
	}

	fs_dir_t_init(&dir);
	ret = fs_opendir(&dir, path);
	if (ret) {
		return ret;
	}

	while ((ret = fs_readdir(&dir, &entry)) == 0 && entry.name[0] != '\0') {
		if (entry.type == FS_DIR_ENTRY_FILE) {
			if (strip_json) {
				size_t len = strlen(entry.name);

				if (len > 5 && strcmp(entry.name + len - 5, ".json") == 0) {
					char name_buf[sizeof(entry.name)];

					memcpy(name_buf, entry.name, len - 5);
					name_buf[len - 5] = '\0';
					cb(name_buf, user);
				}
			} else {
				cb(entry.name, user);
			}
		}
	}

	fs_closedir(&dir);
	return ret;
}

int zth_file_manager_list_characters(const char *game, zth_file_name_cb cb,
				     void *user)
{
	char path[ZTH_FILE_MANAGER_PATH_MAX];
	int ret;

	if (game == NULL) {
		return -EINVAL;
	}

	ret = zth_file_manager_make_path(path, sizeof(path), ZTH_CHARACTER_PATH,
					 game, "");
	if (ret) {
		return ret;
	}

	return zth_file_manager_list_dir(path, true, cb, user);
}

int zth_file_manager_character_exists(const char *game, const char *name)
{
	char path[ZTH_FILE_MANAGER_PATH_MAX];
	struct fs_dirent entry;
	int ret;

	ret = zth_file_manager_get_character_path(path, sizeof(path), game, name);
	if (ret) {
		return ret;
	}

	ret = fs_stat(path, &entry);
	if (ret) {
		return 0;
	}

	return 1;
}

int zth_file_manager_list_all_character_paths(const char *game,
					      zth_file_name_cb cb, void *user)
{
	char path[ZTH_FILE_MANAGER_PATH_MAX];
	int ret;

	ret = zth_file_manager_make_path(path, sizeof(path), ZTH_CHARACTER_PATH,
					 game, "");
	if (ret) {
		return ret;
	}

	return zth_file_manager_list_dir(path, false, cb, user);
}

int zth_file_manager_get_image_path(char *out, size_t out_len,
				    const char *image_name)
{
	if (out == NULL || image_name == NULL) {
		return -EINVAL;
	}

	return zth_file_manager_make_path(out, out_len, ZTH_IMAGES_PATH,
					  image_name, ".bmp");
}

int zth_file_manager_print_directory(const char *path, int tabs)
{
	struct fs_dir_t dir;
	struct fs_dirent entry;
	int ret;

	if (path == NULL) {
		return -EINVAL;
	}

	fs_dir_t_init(&dir);
	ret = fs_opendir(&dir, path);
	if (ret) {
		return ret;
	}

	while ((ret = fs_readdir(&dir, &entry)) == 0 && entry.name[0] != '\0') {
		char pretty[ZTH_FILE_MANAGER_PATH_MAX];
		int offset = 0;

		for (int i = 0; i < tabs && offset < (int)(sizeof(pretty) - 1); i++) {
			offset += snprintk(pretty + offset, sizeof(pretty) - offset, "   ");
		}

		snprintk(pretty + offset, sizeof(pretty) - offset, "%s%s",
			 entry.name, entry.type == FS_DIR_ENTRY_DIR ? "/" : "");
		LOG_INF("%s", pretty);

		if (entry.type == FS_DIR_ENTRY_DIR) {
			char sub_path[ZTH_FILE_MANAGER_PATH_MAX];

			ret = snprintk(sub_path, sizeof(sub_path), "%s/%s", path,
				       entry.name);
			if (ret < 0 || ret >= (int)sizeof(sub_path)) {
				fs_closedir(&dir);
				return -ENAMETOOLONG;
			}

			ret = zth_file_manager_print_directory(sub_path, tabs + 1);
			if (ret) {
				fs_closedir(&dir);
				return ret;
			}
		}
	}

	fs_closedir(&dir);
	return ret;
}
