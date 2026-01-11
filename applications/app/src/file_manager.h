#ifndef ZTH_FILE_MANAGER_H
#define ZTH_FILE_MANAGER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZTH_MOUNT_POINT "/SD"
#define ZTH_CHARACTER_PATH ZTH_MOUNT_POINT "/characters/"
#define ZTH_IMAGES_PATH ZTH_MOUNT_POINT "/assets/images/"
#define ZTH_SD_DISK_NAME "SD"

#define ZTH_FILE_MANAGER_PATH_MAX 256

typedef void (*zth_file_name_cb)(const char *name, void *user);

int zth_file_manager_mount_sdcard(void);
int zth_file_manager_write_to(const char *path, const char *contents);
int zth_file_manager_get_character_path(char *out, size_t out_len,
					const char *game, const char *character_name);
int zth_file_manager_list_characters(const char *game, zth_file_name_cb cb,
				     void *user);
int zth_file_manager_character_exists(const char *game, const char *name);
int zth_file_manager_list_all_character_paths(const char *game,
					      zth_file_name_cb cb, void *user);
int zth_file_manager_get_image_path(char *out, size_t out_len,
				    const char *image_name);
int zth_file_manager_print_directory(const char *path, int tabs);

#ifdef __cplusplus
}
#endif

#endif /* ZTH_FILE_MANAGER_H */
