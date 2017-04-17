//
//  mycp_unix.c
//  mycp
//
//  Created by Gerry on 30/03/2017.
//  Copyright © 2017 Gao. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define BUFF_SIZE 1024
#define ERROR_MAX 100
const unsigned int mask = 0777;

#define DEBUG 0

#define F2F  0      // copy file to file
#define F2D  1      // copy file to directory
#define D2D  2      // copy directory to directory

#define REG  0      // for copy_time
#define DIRY 1      // for copy_time
#define LNK  2      // for copy_time 

#define SUCCESS 1
#define FAIL   -1


char error_msg[ERROR_MAX];

void oops(char *func_name, char *error)
{
    printf("In function: %s\n",func_name);
    perror(error);
    exit(1);
}

void copy_time(const char* source, const char* destination, int file_type)
{
    struct stat src_stat;
    struct timeval time_buf[2];
    lstat(source, &src_stat);
    time_buf[0].tv_sec = src_stat.st_atime;
    time_buf[0].tv_usec = 0;
    time_buf[1].tv_sec = src_stat.st_mtime;
    time_buf[1].tv_usec = 0;
    
    if (file_type == LNK) {
        if ((lutimes(destination, time_buf)) == -1) {
            sprintf(error_msg,"lutime: change symlink %s time error", destination);
            oops("copy_time", error_msg);
        }
    }
    else {
        if ((utimes(destination, time_buf)) == -1) {
            sprintf(error_msg,"utime: change %s time error", destination);
            oops("copy_time", error_msg);
        }
    }
}

int copy_file_lnk(const char* symlink_target,const char* symlink_source, const char* symlink_name)
{
    struct stat src_stat;
    char lnk_target[PATH_MAX], lnk_source[PATH_MAX], lnk_name[PATH_MAX];
    strcpy(lnk_target, symlink_target);
    strcpy(lnk_source, symlink_source);
    strcpy(lnk_name, symlink_name);

    lstat(symlink_source, &src_stat);
    umask(0);
    unsigned int access = mask & src_stat.st_mode;

    if (symlink(lnk_target, lnk_name) == -1) {
        sprintf(error_msg, "Cannot create symlink %s to target %s", lnk_name, lnk_target);
        oops("copy_file_lnk", error_msg);
    }
    lchmod(lnk_name, access);
    copy_time(lnk_source, lnk_name, LNK);
    
    return SUCCESS;
}

int copy_file_reg(const char* source, const char* destination)
{
    int in_fd, out_fd;
    struct stat src_stat, dest_stat;
    ssize_t n_chars;
    char buffer[BUFF_SIZE];
    char src_file[BUFF_SIZE], dest_file[BUFF_SIZE];
	
	strcpy(src_file, source);
	strcpy(dest_file, destination);

    if (stat(src_file, &src_stat) == -1) {
        oops("main", "Source file(directory) does not exist!");
    }

    umask(0);
    unsigned int access = mask & src_stat.st_mode;

	if ((in_fd = open(src_file, O_RDONLY)) == -1 ) {
        sprintf(error_msg,"Open file %s error", src_file);
        oops("copy_file_reg", error_msg);
    }
    if ((out_fd = creat(dest_file, access)) == -1 ) {
        sprintf(error_msg,"Create file %s error", dest_file);
        oops("copy_file_reg", error_msg);
    }
    while ((n_chars = read(in_fd, buffer, BUFF_SIZE)) > 0) {
        if (write(out_fd, buffer, n_chars) != n_chars) {
            sprintf(error_msg,"Write error to %s", dest_file);
            oops("copy_file_reg", error_msg);
        }
        if ( n_chars == -1 ) {
            sprintf(error_msg,"Read error to %s", src_file);
            oops("copy_file_reg", error_msg);
        }
    }

	copy_time(source, destination, REG);

	return SUCCESS;
}

void copy_file(const char* source, const char* destination)
{
    int lnk_len = 0;
    struct stat src_stat;
    char src_file[BUFF_SIZE], dest_file[BUFF_SIZE];
	char lnk_target_buf[PATH_MAX];

	strcpy(src_file, source);
	strcpy(dest_file, destination);
    
    // use lstat to get symlink stat
    if (lstat(src_file, &src_stat) == -1) {
        oops("copy_file", "Source file(directory) does not exist!");
    }

    switch(src_stat.st_mode & S_IFMT) {
        case S_IFLNK:
            if ((lnk_len = readlink(src_file, lnk_target_buf, PATH_MAX)) == -1) {
                sprintf(error_msg, "Read symboliclink %s error.", src_file);
                oops("copy_file", error_msg);
            }
            lnk_target_buf[lnk_len] = '\0';
            copy_file_lnk(lnk_target_buf, src_file, dest_file);
            break;
        default:
            copy_file_reg(src_file, dest_file);
            break;
    }
}

void copy_dir(const char* source, const char* destination)
{
    struct stat src_stat;
    stat(source, &src_stat);
    umask(0);
    unsigned int access = mask & src_stat.st_mode;
    if (mkdir(destination, access) == -1) {
        sprintf(error_msg, "Cannot create directory %s", destination);
        oops("copy_dir", error_msg);
    }   
}

void work_path_recursion(const char* source, const char* destination)
{
    struct stat stat_buf;
    struct dirent* entry_ptr;
    DIR* dir_ptr = NULL;
    
    char current_src[PATH_MAX];
    char current_dest[PATH_MAX];
    char temp_entry_name[PATH_MAX];   // used for stat() to get file type
	
	strcpy(current_src, source);
    strcat(current_src, "/");
	strcpy(current_dest, destination);
    strcat(current_dest, "/");
   
   if ((dir_ptr = opendir(current_src)) == NULL) {
        sprintf(error_msg,"mycp: cannot open %s for copying\n", current_src);
        oops("work_path_recursion", error_msg);
    }
	
	while ((entry_ptr = readdir(dir_ptr)) != NULL) {
        strcpy(temp_entry_name, current_src);
        strcat(temp_entry_name, entry_ptr->d_name);
        stat(temp_entry_name, &stat_buf);
        if (S_ISDIR(stat_buf.st_mode)) {
            if (strcmp(entry_ptr->d_name, ".") == 0 || strcmp(entry_ptr->d_name, "..") == 0)
                continue;
            if (DEBUG) printf("handling directory entry: %s/\n", entry_ptr->d_name);
            strcat(current_src, entry_ptr->d_name);
			strcat(current_dest, entry_ptr->d_name);
            copy_dir(current_src, current_dest);

            if (DEBUG) printf("entering directory: %s/\n", current_src);
            work_path_recursion(current_src, current_dest);
            if (DEBUG) printf("getting out directory: %s/\n", current_src);
            copy_time(current_src, current_dest, DIRY);   // copy directory's time only when get out
        } else {
            if (DEBUG) printf("handling file entry: %s\n", entry_ptr->d_name);
            if (DEBUG) printf("copying file entry: %s\n", entry_ptr->d_name);
            strcat(current_src, entry_ptr->d_name);
			strcat(current_dest, entry_ptr->d_name);
            copy_file(current_src, current_dest);
        }
		strcpy(current_src, source);
		strcat(current_src, "/");
		strcpy(current_dest, destination);
		strcat(current_dest, "/");
	}
	return;
}

void mycp(const char* source, const char* destination, int op_type)
{
	struct stat src_stat, dest_stat;
	char src[PATH_MAX];
	char dest[PATH_MAX];
	strcpy(src, source);
	strcpy(dest, destination);
	
	if (lstat(src, &src_stat) == -1) {
        oops("mycp", "Source file(directory) does not exist!");
    }
    lstat(dest, &dest_stat);

	if (op_type == F2F) {
		copy_file(src, dest);
        printf("Copy file %s to file %s succeed.\n", src, dest);
	}
	else if (op_type == F2D) {
		strcat(dest, "/");
		strcat(dest, src);
        copy_file(src, dest);
        printf("Copy file %s to directory %s/ succeed.\n", src, destination);
	}
	else if (op_type == D2D) {
		if ((dest_stat.st_mode & S_IFMT) == 0)
            copy_dir(src, dest);
		work_path_recursion(src, dest);
        copy_time(src, dest, DIRY);
		printf("Copy directory %s/ to directory %s/ succeed.\n", source, destination);
	}
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: mycp source destination\n");
        return 0;
    }
	char source[PATH_MAX];
	char destination[PATH_MAX];
    struct stat src_stat, dest_stat;
	
    strcpy(source, argv[1]);
	strcpy(destination, argv[2]);

    if (source[strlen(source)-1] == '/') {
        source[strlen(source)-1] = '\0';
    }
    if (destination[strlen(destination)-1] == '/') {
        destination[strlen(destination)-1] = '\0';
    }
    
    if (stat(source, &src_stat) == -1) {
        oops("main", "Source file(directory) does not exist!");
    }
    stat(destination, &dest_stat);
    
    if ((src_stat.st_mode & S_IFMT) != S_IFDIR) {
        if ((dest_stat.st_mode & S_IFMT) != S_IFDIR) {
            printf("Copy File to File.\n");
            mycp(source, destination, F2F);
        }
        else if ((dest_stat.st_mode & S_IFMT) == S_IFDIR) {
            printf("Copy File to Directory.\n");
            mycp(source, destination, F2D);
        }
    } else {
        printf("Copy Directory to Directory\n");
		mycp(source, destination, D2D);
    }
    
    return 0;
}