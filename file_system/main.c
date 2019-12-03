#include "disk_info.h"

void usage(char *argv0) {
    printf("Usage: %s -d [device filename] [other arguments]\n", argv0);
    printf("-i                  Print file system information\n");
    printf("-l                  List the root directory\n");
    printf("-r target -o dest   Recover the target deleted file\n");
    printf("-x target           Cleanse the target deleted file\n");
}
/*
Function: info()
Description:
  Print the information about the FAT32 file system
*/
// Need to insert your own code 
void info(struct DiskInfo *disk_info) {
    
    printf("Number of FATs = %d\n", disk_info->num_fats);
    printf("Number of bytes per sector = %d\n", disk_info->bps);
    printf("Number of sectors per cluster = %d\n", disk_info->spc);
    printf("Number of reserved sectors = %d\n", disk_info->reserved_sectors);
    printf("First FAT starts at byte = %ld\n",disk_info->fat_offset);
    printf("Data area starts at byte = %ld\n", disk_info->root_entry_offset);



    
}

/*
Function: list()
Description:
  List the directory content 
*/
// Need to insert your own code 
void list(struct DiskInfo *disk_info, char *list_target) {
    FILE *fp = disk_info->fp;
    off_t root_entry_offset = disk_info->root_entry_offset;
    uint32_t bpc = disk_info->bpc;
    uint32_t current_cluster_number = 2;
    uint32_t idx = 1;

    uint32_t target_dir_cluster = locate_target_dir_cluster(disk_info, list_target);
    uint32_t next_cluster_number = target_dir_cluster;

    while(next_cluster_number < FAT_EOF && next_cluster_number != FAT_DAMAGED && next_cluster_number != FAT_UNALLOCATED) {
        current_cluster_number = next_cluster_number;
        off_t cluster_offset = get_cluster_offset(root_entry_offset, current_cluster_number, bpc);
        fseek(fp, cluster_offset, SEEK_SET);
        for(uint32_t i = 0; i < bpc / sizeof(struct msdos_dir_entry); i++) {
            struct msdos_dir_entry dir_entry;
            size_t ret = fread(&dir_entry, 1, sizeof(struct msdos_dir_entry), fp);
            if(allocated_dir_entry(&dir_entry)) {
                if(dir_entry.attr != 0x0f) {

                    uint32_t dir_first_cluster = get_dir_first_cluster(&dir_entry);
                    printf("%d, ", idx++);
                	char filename[12];
                	if(dir_entry.name[0]==0xe5){
                		dir_entry.name[0]='?';
                		convert_83filename(dir_entry.name, filename);
                		printf("%s", filename);
                	}
                	else{
                		convert_83filename(dir_entry.name, filename);
                		printf("%s", filename);
                	}

             //   	if((dir_entry.name[0]& 0xe5) !=0&& (dir_entry.attr != 0x10)  ){
                	if((dir_entry.attr & 0x10) != 0){
                		printf("/");
                	}




                    printf(", ");
                    printf("%d, ", dir_entry.size);
             //       printf("%s, ", _);
                    printf("%d ", dir_first_cluster);
             //       printf("%d", _);
                    printf("\n");
                	}
                    // Insert your code here
                    /*
                    Read the file name:
                      1. Check whether the file is deleted or not
                      2. Convert to 8.3 format names
                      3. Check whether it is a subdirectory

                    uint32_t dir_first_cluster = get_dir_first_cluster(&dir_entry);
                    printf("%d, ", _);
                    printf("%s, ", _);
                    printf("%d, ", _);
                    printf("%d", _);
                    printf("\n");
                    */

                
                else {
                    printf("%d, LFN entry\n", idx++);
                }
            }
        }
        next_cluster_number = next_cluster(fp, disk_info->fat_offset, current_cluster_number);
    }
}

void recover(struct DiskInfo *disk_info, const char *recover_target, const char *recover_dest) {
    struct msdos_dir_entry target_dir_entry;
    int ret = locate_deleted_file(disk_info, recover_target, &target_dir_entry, 0 /* cleanse */);
    if(ret) {
        copy_deleted_file(disk_info, target_dir_entry, recover_dest);
    }
    else {
        printf("%s: error - file not found\n", recover_target + 1);
    }
}

void cleanse(struct DiskInfo *disk_info, const char *cleanse_target) {
    struct msdos_dir_entry target_dir_entry;
    int ret = locate_deleted_file(disk_info, cleanse_target, &target_dir_entry, 1 /* cleanse */);
    if(ret) {
        cleanse_deleted_file(disk_info, target_dir_entry, cleanse_target);
//        char filename[14]={0};
//        convert_83filename(cleanse_target, filename);
    }
    else {
        printf("%s: error - file not found\n", cleanse_target + 1);
    }
}

int open_disk(const char *device_name, struct DiskInfo *disk_info) {
    FILE *fp = NULL;
    struct fat_boot_sector boot_entry;

    fp = fopen(device_name, "r+");
    if(fp == NULL)
        die();
    uint32_t boot_entry_size = fread(&boot_entry, 1, sizeof(struct fat_boot_sector), fp);
    if(boot_entry_size != sizeof(struct fat_boot_sector))
        die();
    //  Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
    uint16_t bps = boot_entry.sector_size[0] + ((uint16_t) boot_entry.sector_size[1] << 8);
    off_t root_entry_offset = ( boot_entry.reserved +
                                boot_entry.fats * boot_entry.fat32.length) * bps;
    uint32_t bpc = bps * boot_entry.sec_per_clus;
    off_t fat_offset = bps * boot_entry.reserved;

    disk_info->fp = fp;
    disk_info->root_entry_offset = root_entry_offset;
    disk_info->bpc = bpc;
    disk_info->bps = bps;
    disk_info->spc = boot_entry.sec_per_clus;
    disk_info->reserved_sectors = boot_entry.reserved;
    disk_info->fat_offset = fat_offset;
    disk_info->num_fats = boot_entry.fats;
    disk_info->fat_size = boot_entry.fat32.length;
    return 1;
}

int close_disk(struct DiskInfo *disk_info) {
    assert(disk_info->fp);
    return fclose(disk_info->fp);
}


int main(int argc, char *argv[]) {
    int opt = 0;
    int i_flag = 0;
    int l_flag = 0;
    char *device_name = NULL;
    char *recover_target = NULL;
    char *recover_dest = NULL;
    char *cleanse_target = NULL;
    struct DiskInfo disk_info;

    if(argc > 3) {
        if(strcmp(argv[1], "-d") != 0) {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    else {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    while((opt = getopt(argc, argv, "d:ilr:o:x:")) != -1) {
        switch(opt) {
        case 'd':
            device_name = optarg;
            break;
        case 'i':
            i_flag = 1;
            break;
        case 'l':
            l_flag = 1;
            break;
        case 'r':
            recover_target = optarg;
            break;
        case 'o':
            recover_dest = optarg;
            break;
        case 'x':
            cleanse_target = optarg;
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // print file system information
    if(device_name != NULL &&
       i_flag &&
       !l_flag &&
       recover_target == NULL && recover_dest == NULL &&
       cleanse_target == NULL) {
        if(argc != 4) {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
        open_disk(device_name, &disk_info);
        info(&disk_info);
        close_disk(&disk_info);
    }
    // list directory content
    else if(device_name != NULL &&
            !i_flag &&
            l_flag &&
            recover_target == NULL && recover_dest == NULL &&
            cleanse_target == NULL) {
        if(argc != 4) {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
        open_disk(device_name, &disk_info);
        list(&disk_info, "/");
        close_disk(&disk_info);
    }
    // recover the <recover_target> file to <recover_dest>
    else if(device_name != NULL &&
            !i_flag &&
            !l_flag &&
            recover_target != NULL && recover_dest != NULL &&
            cleanse_target == NULL) {
        if(argc != 7) {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
        open_disk(device_name, &disk_info);
        char recover_target_path[16] = { 0 };
        strcat(recover_target_path, "/");
        strcat(recover_target_path, recover_target);
        recover(&disk_info, recover_target_path, recover_dest);
        close_disk(&disk_info);
    }
    // Cleanse the <cleanse_target> file
    else if(device_name != NULL &&
            !i_flag &&
            !l_flag &&
            recover_target == NULL && recover_dest == NULL &&
            cleanse_target != NULL) {
        if(argc != 5) {
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
        open_disk(device_name, &disk_info);
        char cleanse_target_path[16] = { 0 };
        strcat(cleanse_target_path, "/");
        strcat(cleanse_target_path, cleanse_target);
        cleanse(&disk_info, cleanse_target_path);
        close_disk(&disk_info);
    }
    else {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    return 0;
}
