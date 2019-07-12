/********************************************************
* read_mbr.c - Read MBR, Print out MBR Infomation       *
*                                                       *
* Author:  KyuHyuk Lee <lee@kyuhyuk.kr>                 *
* Usage:                                                *
*          Runs the program and the message appears.    *
********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "main.h"

char *convertTime(uint32_t time);

int main(int argc, char** argv){
        FILE *file = fopen(argv[1], "r");
        super_block_t superBlock;
        if(argc != 2) {
                printf("Usage: %s <disk file name>\n",argv[0]);
                return EXIT_FAILURE;
        }

        if(file == NULL) {
                printf("Error opening disk file\n");
                return EXIT_FAILURE;
        }

        fseek(file, BOOT_CODE_SIZE, SEEK_SET);
        fread((void *)&superBlock, sizeof(super_block_t), 1, file);
        
        printf("Inodes count : %d\n", superBlock.inodes_count);
        printf("Blocks count : %d\n", superBlock.blocks_count_lo);
        printf("Reserved blocks count : %d\n", superBlock.r_blocks_count_lo);
        printf("Free blocks count : %d\n", superBlock.free_blocks_count_lo);
        printf("Free inodes count : %d\n", superBlock.free_inodes_count);
        printf("First Data Block : %d\n", superBlock.first_data_block);
        printf("Block size : %d\n", superBlock.log_block_size);
        printf("Allocation cluster size : %d\n", superBlock.log_cluster_size);
        printf("Blocks per group : %d\n", superBlock.blocks_per_group);
        printf("Clusters per group : %d\n", superBlock.clusters_per_group);
        printf("Inodes per group : %d\n", superBlock.inodes_per_group);
        printf("Mount time : %s\n", convertTime(superBlock.mtime));
        printf("Write time : %s\n", convertTime(superBlock.wtime));
        printf("Mount count : %d\n", superBlock.mnt_count);
        printf("Maximal mount count : %d\n", superBlock.max_mnt_count);
        printf("Magic signature : 0x%X\n", superBlock.magic);
        printf("File system state : %d\n", superBlock.state);
        printf("Behaviour when detecting errors : %d\n", superBlock.errors);
        printf("minor revision level : %d\n", superBlock.minor_rev_level);
        printf("time of last check : %s\n", convertTime(superBlock.lastcheck));
        printf("max. time between checks : %d\n", superBlock.checkinterval);
        printf("OS : %d\n", superBlock.creator_os);
        printf("Revision level : %d\n", superBlock.rev_level);
        printf("Default uid for reserved blocks : %d\n", superBlock.def_resuid);
        printf("Default gid for reserved blocks : %d\n", superBlock.def_resgid);

        return EXIT_SUCCESS;
}

char *convertTime(uint32_t time) {
        char *buff;
        buff = malloc(20);
        time_t now = time;
        strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
        return buff;
}
