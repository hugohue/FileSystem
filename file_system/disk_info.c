#include "disk_info.h"

void die() {
    printf("DIE!\n");
    exit(EXIT_FAILURE);
}

void convert_83filename(const uint8_t *raw, char *filename) {
    int i, j = 0;
    char name[9] = { 0 };
    char ext[4] = { 0 };

    for(i = 0, j = 0; i < 8; i++) {
        if(raw[i] != ' ')
            name[j++] = raw[i];
        else
            break;
    }

    for(i = 8, j = 0; i < 11; i++) {
        if(raw[i] != ' ')
            ext[j++] = raw[i];
        else
            break;
    }

    strncpy(filename, name, 8);
    if(j != 0) {
        strncat(filename, ".", 1);
        strncat(filename, ext, 3);
    }
}

uint32_t next_cluster(FILE *fp, off_t fat_offset, uint32_t current_cluster) {
    uint32_t next_cluster_number = 0;
    fseek(fp, fat_offset + current_cluster * sizeof next_cluster_number,
          SEEK_SET);
    size_t ret = fread(&next_cluster_number, 1, sizeof next_cluster_number, fp);
    /* printf("0x%x\n", next_cluster_number); */
    return next_cluster_number & 0x0fffffff;
}

int allocated_dir_entry(const struct msdos_dir_entry *dir_entry) {
    if(dir_entry->name[0] == 0x00) { return 0; }
    return 1;
}

off_t get_cluster_offset(off_t root_entry_offset, uint32_t cluster_number, uint32_t bytes_per_cluster) {
    return root_entry_offset + (cluster_number - 2) * bytes_per_cluster;
}

uint32_t get_dir_first_cluster(struct msdos_dir_entry *dir_entry) { 
    return dir_entry->start + ((uint32_t) dir_entry->starthi << 16); 
}

uint32_t locate_cluster_by_name(struct DiskInfo *disk_info, uint32_t dir_entry_cluster, char *name) {
    uint32_t current_cluster_number = dir_entry_cluster;
    uint32_t next_cluster_number = dir_entry_cluster;
    uint32_t dir_first_cluster = 0;
    while(next_cluster_number < FAT_EOF && next_cluster_number != FAT_DAMAGED &&
          next_cluster_number != FAT_UNALLOCATED) {
        current_cluster_number = next_cluster_number;
        off_t cluster_offset = get_cluster_offset(
            disk_info->root_entry_offset, current_cluster_number, disk_info->bpc);
        fseek(disk_info->fp, cluster_offset, SEEK_SET);
        for(uint32_t i = 0; i < disk_info->bpc / sizeof(struct msdos_dir_entry); i++) {
            struct msdos_dir_entry dir_entry;
            size_t ret = fread(&dir_entry, 1, sizeof(struct msdos_dir_entry), disk_info->fp);
            if(allocated_dir_entry(&dir_entry)) {
                char filename[14] = { 0 };
                convert_83filename(dir_entry.name, filename);
                if(strcmp(name, filename) == 0) {
                    dir_first_cluster = get_dir_first_cluster(&dir_entry);
                    return dir_first_cluster;
                }
            }
        }
        next_cluster_number = next_cluster(disk_info->fp, disk_info->fat_offset,
                                           current_cluster_number);
    }
    return dir_first_cluster;
}

uint32_t locate_target_dir_cluster(struct DiskInfo *disk_info, char *target_dir) {

    char target_dirs[1024][13] = { {0} };
    int dir_idx = 0;
    int num_dir = 0;
    char *token, *saveptr;
    token = strtok_r(target_dir, "/", &saveptr);
    while(token != NULL) {
        strcpy(target_dirs[dir_idx++], token);
        token = strtok_r(NULL, "/", &saveptr);
    }
    num_dir = dir_idx;

    dir_idx = 0;
    uint32_t next_dir_cluster = 2;
    while(dir_idx != num_dir) {
        next_dir_cluster = locate_cluster_by_name(disk_info, next_dir_cluster,
                                                  target_dirs[dir_idx++]);
    }
    return next_dir_cluster;
}

int deleted_file(struct msdos_dir_entry dir_entry, int argc, va_list va_args) {
    char *target_filename = va_arg(va_args, char *);
    struct msdos_dir_entry *target_dir_entry = va_arg(va_args, struct msdos_dir_entry *);
    if(dir_entry.name[0] == 0xe5) {
        char filename[14] = { 0 };
        convert_83filename(dir_entry.name, filename);
        if(strlen(filename) < 1 || strlen(target_filename) < 1)
            return 0;
        if(strcmp(filename + 1, target_filename + 1) == 0) {
            memcpy(target_dir_entry, &dir_entry, sizeof(struct msdos_dir_entry));
            return 1;
        }
    }
    return 0;
}
int traverse_directory_find_deleted(struct DiskInfo *disk_info, char *directory_path, int num, ...) {
    FILE *fp = disk_info->fp;
    off_t root_entry_offset = disk_info->root_entry_offset;
    uint32_t bpc = disk_info->bpc;
    uint32_t current_cluster_number = 2;

    uint32_t target_dir_cluster = locate_target_dir_cluster(disk_info, directory_path);
    uint32_t next_cluster_number = target_dir_cluster;

    while(next_cluster_number < FAT_EOF && next_cluster_number != FAT_DAMAGED &&
          next_cluster_number != FAT_UNALLOCATED) {
        current_cluster_number = next_cluster_number;
        off_t cluster_offset = get_cluster_offset(root_entry_offset, current_cluster_number, bpc);
        fseek(fp, cluster_offset, SEEK_SET);
        for(uint32_t i = 0; i < bpc / sizeof(struct msdos_dir_entry); i++) {
            struct msdos_dir_entry dir_entry;
            size_t dir_entry_size = fread(&dir_entry, 1, sizeof(struct msdos_dir_entry), fp);
            int ret;
            va_list va_args;
            va_start(va_args, num);
            if((ret = deleted_file(dir_entry, num, va_args)) != 0) {
                return ret;
            }
            va_end(va_args);
        }
        next_cluster_number = next_cluster(fp, disk_info->fat_offset, current_cluster_number);
    }
    return 0;
}



int check_occupied(struct DiskInfo *disk_info, struct msdos_dir_entry *dir_entry) {
    uint32_t dir_first_cluster = get_dir_first_cluster(dir_entry);
    if(dir_first_cluster == 0)
        return 0;
    fseek(disk_info->fp, disk_info->fat_offset, SEEK_SET);
    for(uint32_t i = 0;
        i < disk_info->num_fats * disk_info->fat_size * disk_info->bpc;
        i += sizeof(uint32_t)) {
        uint32_t fat_item;
        size_t ret = fread(&fat_item, sizeof fat_item, 1, disk_info->fp);
        if(fat_item == dir_first_cluster) {
            return 1;
        }
    }
    return 0;
}


int copy_deleted_file(struct DiskInfo *disk_info, struct msdos_dir_entry dir_entry, const char *recover_dest) {
    FILE *dest_fp = fopen(recover_dest, "w");
    if(dest_fp == NULL) {
        printf("%s: failed to open\n", recover_dest);
        exit(EXIT_FAILURE);
    }
    uint32_t dir_first_cluster = get_dir_first_cluster(&dir_entry);
    if(dir_first_cluster != 0) {
        uint32_t next_cluster_number = dir_first_cluster;
        uint32_t current_cluster_number;
        uint32_t file_size = dir_entry.size;
        char *buffer = malloc(disk_info->bpc);
        if(file_size > disk_info->bpc) {
            printf("More than one cluster!!!\n");
            exit(1);
        }

        while(next_cluster_number < FAT_EOF &&
              next_cluster_number != FAT_DAMAGED &&
              next_cluster_number != FAT_UNALLOCATED) {
            current_cluster_number = next_cluster_number;
            off_t cluster_offset = get_cluster_offset(
                disk_info->root_entry_offset, current_cluster_number, disk_info->bpc);
            fseek(disk_info->fp, cluster_offset, SEEK_SET);
            size_t ret = fread(buffer, 1, file_size, disk_info->fp);
            fwrite(buffer, 1, file_size, dest_fp);
            next_cluster_number = next_cluster(disk_info->fp, disk_info->fat_offset,
                                               current_cluster_number);
        }

        free(buffer);
    }
    assert(dest_fp);
    fclose(dest_fp);
    return 1;
}