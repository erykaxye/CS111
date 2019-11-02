/*
NAME: Erica Xie
EMAIL: ericaxie@ucla.edu
ID: 404920875
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <time.h>   
#include "ext2_fs.h"

char find_file_type(__uint16_t i_mode) {
    char ret; 

    if((i_mode & 0xF000) == 0xA000) {
        ret = 's'; 
        return ret;
    }
    else if((i_mode & 0xF000) == 0x8000) {
        ret = 'f'; 
        return ret;
    }
    else if((i_mode & 0xF000) == 0x4000) {
        ret = 'd'; 
        return ret;
    }
    ret = '?'; 
    return ret;
}

int main(int argc, char **argv) {

    /* declare all variables */
    char output[2048], buf, file_type; 
    int add1[256], add2[256], add3[256];
    int offset, block_size, num_fblocks, num_finodes;    
    int fd0; 
    int has_block_ptr = 1; 
    unsigned int i, n, m, l, k, dir_offset, indir_offset; 
    ssize_t num; 
    struct ext2_super_block super; 
    struct ext2_group_desc group; 
    struct ext2_inode inodes;  
    time_t rawtime; 
    struct tm *timestamp; 
    struct ext2_dir_entry dir; 

    /* open file */ 
    if(argc == 2) {
        fd0 = open(argv[1], O_RDONLY); 
    }
    else {
        fprintf(stderr, "Incorrect number of arguments\n"); 
        exit(1);
    }

    /* read superblock */
    offset = 1024; 
    num = pread(fd0, &super, sizeof(struct ext2_super_block), offset); 
    if(num < 0) {
        fprintf(stderr, "pread error: %s\n", strerror(errno)); 
        exit(1);
    } 

    block_size = EXT2_MIN_BLOCK_SIZE << super.s_log_block_size; 

    /* printing superblock csv */ 
    sprintf(output, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", 
            super.s_blocks_count, super.s_inodes_count, block_size, super.s_inode_size, 
            super.s_blocks_per_group, super.s_inodes_per_group, super.s_first_ino);
    if(write(1, &output, strlen(output)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
        exit(1);
    } 

    /* read group (only 1 in this assignment) */
    offset += 1024; 
    num = pread(fd0, &group, sizeof(struct ext2_group_desc), offset); 
    if(num < 0) {
        fprintf(stderr, "pread error: %s\n", strerror(errno)); 
        exit(1);
    } 

    /* printing group csv */ 
    sprintf(output, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n", 
            super.s_block_group_nr, super.s_blocks_count, super.s_inodes_count, group.bg_free_blocks_count, 
            group.bg_free_inodes_count, group.bg_block_bitmap, group.bg_inode_bitmap, group.bg_inode_table);
    if(write(1, &output, strlen(output)) < 0) {
        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
        exit(1);
    }

    /* printing free block csvs */ 
    offset += block_size; 
    num_fblocks = 0; 
    for(i = 0; i < super.s_blocks_count; i++) {
        num = pread(fd0, &buf, 1, offset+i); 
        if(num < 0) {
            fprintf(stderr, "pread error: %s\n", strerror(errno)); 
            exit(1);
        } 
        for(n = 1; n <= 8; n++) {
            if(((buf >> (n - 1)) & 0x01) == 0) { 
                num_fblocks++; 
                /* printing free block csv */ 
                sprintf(output, "BFREE,%d\n", i * 8 + n);
                if(write(1, &output, strlen(output)) < 0) {
                    fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                    exit(1);
                }
            }
        }
    }
    if(num_fblocks != group.bg_free_blocks_count) {
        fprintf(stderr, "Incorrect number of blocks\n"); 
        exit(1);
    }

    /* printing free inode csv */ 
    offset += block_size; 
    num_finodes = 0; 
    for(i = 0; i < super.s_blocks_count; i++) {
        num = pread(fd0, &buf, 1, offset+i); 
        if(num < 0) {
            fprintf(stderr, "pread error: %s\n", strerror(errno)); 
            exit(1);
        } 
        for(n = 1; n <= 8; n++) {
            if(((buf >> (n - 1)) & 0x01) == 0) { 
                num_finodes++; 
                /* printing free inode csv */ 
                sprintf(output, "IFREE,%d\n", i * 8 + n);
                if(write(1, &output, strlen(output)) < 0) {
                    fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                    exit(1);
                }
            }
        }
    }
    if(num_finodes != group.bg_free_inodes_count) {
        fprintf(stderr, "Incorrect number of blocks\n"); 
        exit(1);
    }

    /* printing inodes csvs */ 
    offset += block_size; 
    for(i = 1; i <= super.s_inodes_count; i++) { 
        num = pread(fd0, &inodes, sizeof(struct ext2_inode), offset);  
        if(num < 0) {
            fprintf(stderr, "pread error: %s\n", strerror(errno)); 
            exit(1);
        }
        file_type = find_file_type(inodes.i_mode); 
        if(inodes.i_mode != 0 && inodes.i_links_count != 0) {
            sprintf(output, "INODE,%d,%c,%o,%d,%d,%d,", 
            i, file_type, inodes.i_mode & 0x0FFF, inodes.i_uid, inodes.i_gid, inodes.i_links_count);
            if(write(1, &output, strlen(output)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                exit(1);
            } 
            //calculate time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
            rawtime = inodes.i_ctime; 
            timestamp = gmtime(&rawtime); 
            sprintf(output, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d,", 
            timestamp->tm_mon+1, timestamp->tm_mday, timestamp->tm_year-100, 
            timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec);
            if(write(1, &output, strlen(output)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                exit(1);
            } 
            //calculate modification time (mm/dd/yy hh:mm:ss, GMT)
            rawtime = inodes.i_mtime; 
            timestamp = gmtime(&rawtime); 
            sprintf(output, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d,", 
            timestamp->tm_mon+1, timestamp->tm_mday, timestamp->tm_year%100, 
            timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec);
            if(write(1, &output, strlen(output)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                exit(1);
            } 
            //calculate time of last access (mm/dd/yy hh:mm:ss, GMT)
            rawtime = inodes.i_atime; 
            timestamp = gmtime(&rawtime); 
            sprintf(output, "%.2d/%.2d/%.2d %.2d:%.2d:%.2d,", 
            timestamp->tm_mon+1, timestamp->tm_mday, timestamp->tm_year%100, 
            timestamp->tm_hour, timestamp->tm_min, timestamp->tm_sec);
            if(write(1, &output, strlen(output)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                exit(1);
            }  
            sprintf(output, "%d,%d", inodes.i_size, inodes.i_blocks);        
            if(write(1, &output, strlen(output)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                exit(1);
            }
            if(file_type == 's') { 
                if(inodes.i_size < 60) {
                    has_block_ptr = 0; 
                } 
            } 
            if(has_block_ptr) {
                for(n = 0; n < 15; n++) {
                    sprintf(output, ",%d", inodes.i_block[n]);        
                    if(write(1, &output, strlen(output)) < 0) {
                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                        exit(1);
                    }
                }
            }
            has_block_ptr = 1;
            sprintf(output, "\n");        
            if(write(1, &output, strlen(output)) < 0) {
                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                exit(1);
            }
        } 

        /* print directory entries csv */ 
        if(file_type == 'd') {
            for(n = 0; n < 12; n++) { 
                if(inodes.i_block[n] != 0) {
                    dir_offset = inodes.i_block[n] * block_size;  
                    while(dir_offset < ((inodes.i_block[n] + 1) * block_size)) {
                        num = pread(fd0, &dir, sizeof(struct ext2_dir_entry), dir_offset);  
                        if(num < 0) {
                            fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                            exit(1);
                        }
                        if(dir.inode != 0) {
                            sprintf(output, "DIRENT,%d,%d,%d,%d,%d,'%s'\n", 
                            i, dir_offset - inodes.i_block[n] * block_size, dir.inode, dir.rec_len, dir.name_len, dir.name);
                            if(write(1, &output, strlen(output)) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1); 
                            } 
                        }
                        dir_offset += dir.rec_len; 
                    }
                }
            }
        }
        
        /* print indirect entries csv */ 
        for(n = 12; n < 15; n++) { 
            if(inodes.i_block[n] != 0) {
                indir_offset = inodes.i_block[n] * block_size; 
                if(n == 12) {
                    num = pread(fd0, &add1, 256*sizeof(int), indir_offset);  
                    if(num < 0) {
                        fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                        exit(1);
                    }  
                    for(m = 0; m < 256; m++) {
                        if(add1[m] != 0) {
                            sprintf(output, "INDIRECT,%d,%d,%d,%d,%d\n", i, 1, m + 12, inodes.i_block[n], add1[m]);
                            if(write(1, &output, strlen(output)) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1);
                            }
                        }   
                    }
                }
                else if(n == 13) {
                    num = pread(fd0, &add1, 256*sizeof(int), indir_offset);  
                    if(num < 0) {
                        fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                        exit(1);
                    }
                    for(m = 0; m < 256; m++) { 
                        if(add1[m] != 0) {
                            sprintf(output, "INDIRECT,%d,%d,%d,%d,%d\n", i, 2, 256*(m + 1) + 12, inodes.i_block[n], add1[m]);
                            if(write(1, &output, strlen(output)) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1);
                            }
                            num = pread(fd0, &add2, 256*sizeof(int), add1[m]*block_size); 
                            if(num < 0) {
                                fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                                exit(1);
                            }  
                            for(l = 0; l < 256; l++) {
                                if(add2[l] != 0) {
                                    sprintf(output, "INDIRECT,%d,%d,%d,%d,%d\n", i, 1, 256*(m + 1) + l + 12, add1[m], add2[l]);
                                    if(write(1, &output, strlen(output)) < 0) {
                                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                        exit(1);
                                    }
                                }   
                            }
                        }
                    }
                }
                else {
                    num = pread(fd0, &add1, 256*sizeof(int), indir_offset);  
                    if(num < 0) {
                        fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                        exit(1);
                    }
                    for(m = 0; m < 256; m++) { 
                        if(add1[m] != 0) {
                            sprintf(output, "INDIRECT,%d,%d,%d,%d,%d\n", i, 3, 256*(256*(m + 1) + 1) + 12, inodes.i_block[n], add1[m]);
                            if(write(1, &output, strlen(output)) < 0) {
                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                exit(1);
                            }
                            num = pread(fd0, &add2, 256*sizeof(int), add1[m]*block_size); 
                            if(num < 0) {
                                fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                                exit(1);
                            }  
                            for(l = 0; l < 256; l++) {
                                if(add2[l] != 0) {
                                    sprintf(output, "INDIRECT,%d,%d,%d,%d,%d\n", i, 2, 256*(256*(m + 1) + l + 1) + 12, add1[m], add2[l]);
                                    if(write(1, &output, strlen(output)) < 0) {
                                        fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                        exit(1);
                                    }
                                    num = pread(fd0, &add3, 256*sizeof(int), add2[l]*block_size); 
                                    if(num < 0) {
                                        fprintf(stderr, "pread error: %s\n", strerror(errno)); 
                                        exit(1);
                                    }  
                                    for(k = 0; k < 256; k++) {
                                        if(add3[k] != 0) {
                                            sprintf(output, "INDIRECT,%d,%d,%d,%d,%d\n", i, 1, 256*(256*(m + 1) + l + 1) + k + 12, add2[l], add3[k]);
                                            if(write(1, &output, strlen(output)) < 0) {
                                                fprintf(stderr, "Write error: %s\n", strerror(errno)); 
                                                exit(1);
                                            }
                                        }
                                    }
                                }   
                            }
                        }
                    }
                }
            }
        }
        offset += sizeof(struct ext2_inode);
    }
    exit(0); 
} 
