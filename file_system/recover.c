#include "disk_info.h"

/*
  You can use function traverse_directory_find_deleted() and check_occupied() to finish this function
*/
int locate_deleted_file(struct DiskInfo *disk_info, 
                        const char *recover_target, 
                        struct msdos_dir_entry *target_dir_entry, 
                        int cleanse) {
    char *dirc, *basec;
    dirc = strdup(recover_target);
    basec = strdup(recover_target);
    char *recover_target_dirname = dirname(dirc);
    if(recover_target_dirname == NULL)
        die();
    char *recover_target_basename = basename(basec);
    if(recover_target_basename == NULL)
        die();
    if (traverse_directory_find_deleted(disk_info, recover_target_dirname, 2, recover_target_basename, target_dir_entry)==1 && check_occupied(disk_info, target_dir_entry)==0)
    {
        if (cleanse==0){
            printf("%s: recovered\n", recover_target_basename);
        }
 //       if (cleanse==1){
 //           printf("%s: cleansed\n", recover_target+1);

   //     }
 //   free(dirc);
  //  free(basec);
    return 1;
    }
    else if(traverse_directory_find_deleted(disk_info, recover_target_dirname, 2, recover_target_basename, target_dir_entry)==0){
        return 0;
    }
    else if (check_occupied(disk_info, target_dir_entry)==1){
        if (cleanse==0){
        printf("%s: error - fail to recover\n", recover_target_basename);
        }
        if (cleanse==1){        
        printf("%s: error - fail to cleanse\n", recover_target+1);        
        }       
        exit(EXIT_FAILURE);
    }
 //   else{
 //       if (cleanse==0){
 //           printf("%s: error - fail to recover\n", recover_target_basename);
 //       }
 //       if (cleanse==1){
 //           printf("%s: error - fail to cleanse\n", recover_target+1);
//        }
    else{
        return 0;
    }
    free(dirc);
    free(basec);
    return 0;

 //   }
    // Insert your code here


}

/* 
You can reuse most of the codes in the copy_deleted_file() function.
*/
int cleanse_deleted_file(struct DiskInfo *disk_info, struct msdos_dir_entry dir_entry, const char *cleanse_target) {
    FILE *dest_fp = fopen(cleanse_target+1, "r+");
    if(dest_fp == NULL) {
        printf("%s: failed to open\n", cleanse_target);
//        printf("%s: error - fail to cleanse\n", cleanse_target+1);
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

        if(dir_entry.size==0 || check_occupied(disk_info, &dir_entry)){
            printf("%s: error - fail to cleanse\n", cleanse_target+1);
            return 0;
        }
        char buf[file_size];

        for (int i=0; i<file_size; i++){
            buf[i]='\0';
        }
        while(next_cluster_number < FAT_EOF &&
              next_cluster_number != FAT_DAMAGED &&
              next_cluster_number != FAT_UNALLOCATED) {
            current_cluster_number = next_cluster_number;
            off_t cluster_offset = get_cluster_offset(
                disk_info->root_entry_offset, current_cluster_number, disk_info->bpc);
            fseek(disk_info->fp, cluster_offset, SEEK_SET);
            size_t ret = fread(buffer, 1, file_size, disk_info->fp);
            fwrite(buf, 1, file_size, disk_info->fp);
            next_cluster_number = next_cluster(disk_info->fp, disk_info->fat_offset,
                                               current_cluster_number);
        }
    

        free(buffer);
    }
    assert(dest_fp);
    fclose(dest_fp);
    return 1;
}