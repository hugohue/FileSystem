#ifndef DISK_INFO_H
#define DISK_INFO_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <libgen.h>
#include <stdarg.h>
#include <sys/types.h>
#include <linux/msdos_fs.h>

#define FAT_EOF 0x0ffffff8
#define FAT_DAMAGED 0x0ffffff7
#define FAT_UNALLOCATED 0x0

struct DiskInfo {
    FILE *fp;
    off_t fat_offset;
    off_t root_entry_offset;
    uint8_t num_fats;         //Number of FATs
    uint32_t fat_size;
    uint32_t bpc;
    uint32_t bps;             //Number of bytes per sector
    uint8_t spc;              //Number of sectors per cluster
    uint16_t reserved_sectors;//Number of reserved sectors
};


void die();
void convert_83filename(const uint8_t *raw, char *filename);
uint32_t next_cluster(FILE *fp, off_t fat_offset, uint32_t current_cluster);
int allocated_dir_entry(const struct msdos_dir_entry *dir_entry);
off_t get_cluster_offset(off_t root_entry_offset, uint32_t cluster_number, uint32_t bytes_per_cluster);
uint32_t get_dir_first_cluster(struct msdos_dir_entry *dir_entry);
uint32_t locate_cluster_by_name(struct DiskInfo *disk_info, uint32_t dir_entry_cluster, char *name);
uint32_t locate_target_dir_cluster(struct DiskInfo *disk_info, char *target_dir);
int deleted_file(struct msdos_dir_entry dir_entry, int argc, va_list va_args);
/*
Function: traverse_directory_find_deleted
Description:
Try to traverse the directory and locate to the first directory that contains deleted clusters.
If found:
   return 1 and set the directory_path to the target directory that contains the deleted file
Otherwise:
   return 0 and set the directory_path to NULL
*/
int traverse_directory_find_deleted(struct DiskInfo *disk_info, char *directory_path, int num, ...);
/*
Function: check_occupied
Description:
To check whether the target cluster is occupied or not
If occupied:
   return 1 
Otherwise:
   return 0 
*/
int check_occupied(struct DiskInfo *disk_info, struct msdos_dir_entry *dir_entry);
int locate_deleted_file(struct DiskInfo *disk_info, const char *recover_target, struct msdos_dir_entry *target_dir_entry, int cleanse);
int copy_deleted_file(struct DiskInfo *disk_info, struct msdos_dir_entry dir_entry, const char *recover_dest);
int cleanse_deleted_file(struct DiskInfo *disk_info, struct msdos_dir_entry dir_entry, const char *cleanse_target);

#endif
