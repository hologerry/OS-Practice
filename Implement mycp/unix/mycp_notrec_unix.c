//
//  main.c
//  mycp
//
//  Created by Gerry on 26/03/2017.
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


#define F2F  0      // copy file to file
#define F2D  1      // copy file to directory
#define D2D  2      // copy directory to directory

char source[PATH_MAX];
char destination[PATH_MAX];
char absolute_dest[PATH_MAX];
char work_space[PATH_MAX];
char last_dir[PATH_MAX];
char src_root[PATH_MAX];

char error_msg[ERROR_MAX];

void oops(char *func_name, char *error_msg)
{
    printf("In function: %s\n",func_name);
    perror(error_msg);
    exit(1);
}
void remove_abd_last_dir(int depth)
{
    if (depth) {
        size_t len = strlen(absolute_dest);
        for (size_t i = len-1; i > 0; i--) {
            if (absolute_dest[i] == '/' && i != len-2) {
                strncpy(last_dir, absolute_dest+i+1, len-i-1);
                absolute_dest[i] = '\0';
                return;
            }
        }
        sprintf(error_msg, "error remove %s last-dir at depth %d", absolute_dest, depth);
        oops("remove_abd_last_dir", error_msg);
        return;
    }
}
void change_time(struct stat *stat_ptr, char* file)
{
    char* error_msg;
    struct utimbuf time_buf;
    time_buf.actime = stat_ptr->st_atime;
    time_buf.modtime = stat_ptr->st_mtime;
    if ((utime(file, &time_buf)) == -1) {
        sprintf(error_msg,"utime: change %s time error", file);
        oops("change_time", error_msg);
    }
}

void copy_file_reg(struct stat* stat_ptr, struct dirent* entry_ptr, int flag)
{
    int in_fd, out_fd;
    ssize_t n_chars;
    char buffer[BUFF_SIZE];
    char src_file[BUFF_SIZE], dest_file[BUFF_SIZE];
    
    umask(0);
    unsigned int access = mask & stat_ptr->st_mode;
    
    if (flag == D2D) {
        strcpy(src_file, source);
        strcat(src_file, "/");
        strcat(src_file, entry_ptr->d_name);
        
        strcpy(dest_file, absolute_dest);
        strcat(dest_file, "/");
        strcat(dest_file, entry_ptr->d_name);
        
    } else if (flag == F2D) {
        strcpy(src_file, source);
        
        strcpy(dest_file, absolute_dest);
        strcat(dest_file, "/");
        strcat(dest_file, source);
        
    } else if (flag == F2F) {
        strcpy(src_file, source);
        strcpy(dest_file, destination);
    }
    
    if ((in_fd = open(src_file, O_RDONLY)) == -1 ) {
        sprintf(error_msg,"open file %s error", src_file);
        oops("copy_file_reg", error_msg);
    }
    if ((out_fd = creat(dest_file, access)) == -1 ) {
        sprintf(error_msg,"create file %s error", dest_file);
        oops("copy_file_reg", error_msg);
    }
    while ((n_chars = read(in_fd, buffer, BUFF_SIZE)) > 0) {
        if (write(out_fd, buffer, n_chars) != n_chars) {
            sprintf(error_msg,"write error to %s", dest_file);
            oops("copy_file_reg", error_msg);
        }
        if ( n_chars == -1 ) {
            sprintf(error_msg,"read error to %s", src_file);
            oops("copy_file_reg", error_msg);
        }
    }
    
    change_time(stat_ptr, dest_file);
    
    /* close files */
    if (close(in_fd) == -1 || close (out_fd) == -1)
    {
        oops("copy_file_reg","error closing files");
    }
}

void copy_dir(struct stat* stat_ptr)
{
    umask(0);
    unsigned int access = mask & stat_ptr->st_mode;
    if (mkdir(absolute_dest, access) == -1) {
        sprintf(error_msg, "cannot create directory %s", absolute_dest);
        oops("copy_dir", error_msg);
    }
    change_time(stat_ptr, absolute_dest);
}

void copy_file_lnk(struct stat* stat_ptr, struct dirent* entry_ptr)
{
    char *src_file, *dest_file;
    strcpy(src_file, source);
    strcat(src_file, "/");
    strcat(src_file, entry_ptr->d_name);
    
    
    strcpy(dest_file, absolute_dest);
    strcat(dest_file, "/");
    strcat(dest_file, entry_ptr->d_name);
    
    if (symlink(src_file, dest_file) == -1) {
        sprintf(error_msg, "cannot create symlink from %s to %s", src_file, dest_file);
        oops("copy_file_lnk", error_msg);
    }
    
    // struct timeval new_times[2];
    // TIMESPEC_TO_TIMEVAL(&new_times[0], &stat_ptr->st_atimespec);
    // TIMESPEC_TO_TIMEVAL(&new_times[1], &stat_ptr->st_mtimespec);
    // struct timespec timsps[2] = {stat_ptr->st_atimespec, stat_ptr->st_mtimespec};
    // utimensat(AT_FDCWD, dest_file, timsps, AT_SYMLINK_NOFOLLOW);
}

void copy_files(struct stat* stat_ptr, struct dirent* entry_ptr)
{
    switch(stat_ptr->st_mode & S_IFMT) {
        case S_IFDIR:
            copy_dir(stat_ptr);
            break;
        case S_IFLNK:
            copy_file_lnk(stat_ptr, entry_ptr);
            break;
        default:
            copy_file_reg(stat_ptr, entry_ptr, D2D);
            break;
    }
}

void work_path(const char* dir, int depth)
{
    struct stat stat_buf;
    struct dirent* entry_ptr;
    DIR* dir_ptr = NULL;

    if ((dir_ptr = opendir(dir)) == NULL) {
        sprintf(error_msg,"mycp: cannot open %s for copying\n", dir);
        oops("work_path", error_msg);
    }
    chdir(dir);
    while((entry_ptr = readdir(dir_ptr)) != NULL) {
        stat(entry_ptr->d_name, &stat_buf);
        
        if (S_ISDIR(stat_buf.st_mode)) {
            if (strcmp(entry_ptr->d_name, ".") == 0 || strcmp(entry_ptr->d_name, "..") == 0)
                continue;
            printf("handling dir entry: %s/\n", entry_ptr->d_name);
            printf("getting into directory: %s/\n", entry_ptr->d_name);
            
            char buffer[PATH_MAX];
            getcwd(buffer, PATH_MAX);
            strcpy(source, buffer);
            

            strcat(absolute_dest, "/");
            strcat(absolute_dest, entry_ptr->d_name);
            
            copy_files(&stat_buf, entry_ptr);
            work_path(entry_ptr->d_name, depth+1);
        } else {
            printf("handling file entry: %s\n", entry_ptr->d_name);
            char buffer[PATH_MAX];
            getcwd(buffer, PATH_MAX);
            strcpy(source, buffer);
            copy_files(&stat_buf, entry_ptr);
        }
    }
    remove_abd_last_dir(depth);
    if (depth) {
        printf("getting out dirrectory: %s/\n", last_dir);
    }
    else {
        printf("current directory: %s", src_root);
        printf("%s\n", src_root[strlen(src_root)-1] == '/' ? "" : "/");
    }

    chdir("..");
    closedir(dir_ptr);
    
    return;
}

struct stat check_stat(char *filename)
{
    struct stat file_info;
    if (stat(filename, &file_info) >= 0)
    {
        return file_info;
    }
    return file_info;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: mycp source destination\n");
        return 0;
    }
    
    strcpy(source, argv[1]);
    strcpy(src_root, argv[1]);
    strcpy(destination, argv[2]);
    
//    strcpy(source, "src");
//    strcpy(destination, "dest");
    
    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);
    strcpy(work_space, buffer);
    
    strcpy(absolute_dest, work_space);
    strcat(absolute_dest, "/");
    strcat(absolute_dest, destination);
    
    struct stat src_stat, dest_stat;
    
    if (stat(source, &src_stat) == -1)
    {
        oops("main", "source file(dir) does not exist");
    }
    stat(destination, &dest_stat);
    
    if ((src_stat.st_mode & S_IFMT) != S_IFDIR)
    {
        if ((src_stat.st_mode & S_IFMT) != S_IFLNK)
        {
            if ((dest_stat.st_mode & S_IFMT) != S_IFDIR)
                copy_file_reg(&src_stat, NULL, F2F);
            else if ((dest_stat.st_mode & S_IFMT) == S_IFDIR)
                copy_file_reg(&src_stat, NULL, F2D);
        }
        else
        {
            if (symlink(source, absolute_dest) == -1)
            {
                oops("main", "Cannot create symlink");
            }
        }
    }
    else
    {
        if ((dest_stat.st_mode & S_IFMT) == 0)
            copy_dir(&src_stat);
        work_path(source, 0);
    }
    
    return 0;
}