#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <utime.h>
#include "my_tar.h"


int main(int argc, char** argv) {

    if (argc < 3) {
        return 1;
    }
 
    char** input = malloc(argc * sizeof(char*)); 
    int input_size = 0;
    for (int i = 1; i < argc; i++) {
        input[input_size] = argv[i];
        input_size++;
    }

    char* flags = "none";
    int numflags = 0;

    //check flags and store them and their position
    if (contain_flags(input, "-cf", "-c") != 0) {
        flags = "-cf";
        numflags = contain_flags(input, "-cf", "-c");
    } else if (contain_flags(input, "-rf", "-r") != 0) {
        flags = "-rf";
        numflags = contain_flags(input, "-rf", "-r");
    } else if (contain_flags(input, "-uf", "-u") != 0) {
        flags = "-uf";
        numflags = contain_flags(input, "-uf", "-u");
    } else if (contain_flags(input, "-tf", "-t") != 0) {
        flags = "-tf";
        numflags = contain_flags(input, "-tf", "-t");
    } else if (contain_flags(input, "-xf", "-x") != 0) {
        flags = "-xf";
        numflags = contain_flags(input, "-xf", "-x");
    } else {
        free(input);
        return 1;
    }

    //get archive name and file names depending on number of flags
    char* arch_name = NULL;
    char** file_names = malloc(argc * sizeof(char*));
    int num_files = 0;
    if (numflags == 1) {
        arch_name = input[1];
        for (int j = 2; j < input_size; j++) {
            file_names[num_files] = input[j];
            num_files++;
        }
    } else if (numflags == 2) {
        arch_name = input[2];
        for (int j = 3; j < input_size; j++) {
            file_names[num_files] = input[j];
            num_files++;
        }        
    }

    //creating a new archive
    if (my_strcmp(flags, "-cf") == 0) {

        if (num_files == 0) {
            write(2, "my_tar: Cowardly refusing to create an empty archive\n", 53);
            free(input);
            free(file_names);
            return 1;
        }

        int fd = open(arch_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        for (int i = 0; i < num_files; i++) {
            //for all files - write headers
            int typedescr = fill_header(file_names[i], fd);
        
            //for regular files - write file contents
            if (typedescr == 0) {
                write_file_contents(file_names[i], fd);
            }

            //for folders - go into folders, write their contents
            if (typedescr == 5) {
                char* dir_name = concat(file_names[i], "/");
                
                char** dir_cont = malloc(size_of_dir(file_names[i]) * sizeof(char*));
                int num_cont = 0;

                DIR* folder;
                folder = opendir(file_names[i]);
                struct dirent *entry;
                entry = readdir(folder);

                //get content names with concatenated path
                while (entry != NULL)
                {
                    if (entry->d_name[0] != '.') {
                        char* cont_name = concat(dir_name, entry->d_name);
                        dir_cont[num_cont] = cont_name;
                        num_cont++;
                    }
                    entry = readdir(folder);
                }
                free(dir_name);     
                closedir(folder);       

                //run header fill loop for each item of content
                for (int i = 0; i < num_cont; i++) {
                    typedescr = fill_header(dir_cont[i], fd);
                    if (typedescr == 0) {
                        write_file_contents(dir_cont[i], fd);
                    }
                }

                for (int i = 0; i < num_cont; i++) {
                    free(dir_cont[i]);
                }
                free(dir_cont);
            }
        }

        //padding at the end of the file
        char final_pad[1024] = {0};
        write(fd, final_pad, 1024);

        close(fd);
    }

    //listing archive contents or appending a new entry (simple)
    if (my_strcmp(flags, "-tf") == 0 || my_strcmp(flags, "-rf") == 0) {
        if (my_strcmp(flags, "-rf") == 0 && num_files == 0) {
            free(file_names);
            free(input);
            return 1;
        }
        
        int fd = open(arch_name, O_RDWR);

        if (fd == -1) {
            write(2, "my_tar: Cannot open ", 20);
            write(2, arch_name, my_strlen(arch_name));
            write(2, "\n", 1);
            free(input);
            free(file_names);
            return 1;
        }

        //traverse the archive structure to get file names in case of "-tf"
        //or position from which to write in case of "-rf"
        int write_pos = 0;
        int bytes_read = 0;
        int file_size = 0;
        int padding = 0;
        struct file_header header;
        bytes_read = read(fd, &header, 512);
        while (header.name[0] != '\0') {
            write_pos += bytes_read;
            if (my_strcmp(flags, "-tf") == 0) {
                write(1, header.name, my_strlen(header.name));
                write(1, "\n", 1);
            }
            file_size = to_decimal_from_str(header.size);
            if (file_size > 0) {
                padding = 512 - (file_size % 512);
                file_size = file_size + padding;
            }
            char buf[file_size];
            bytes_read = read(fd, buf, file_size);
            write_pos += bytes_read;
            bytes_read = read(fd, &header, 512);
        }

        if (my_strcmp(flags, "-rf") == 0) {
            lseek(fd, write_pos, SEEK_SET);
            for (int i = 0; i < num_files; i++) {
                int typedescr = fill_header(file_names[i], fd);
                if (typedescr == 0) {
                    write_file_contents(file_names[i], fd);
                }
            }
            char final_pad[1024] = {0};
            write(fd, final_pad, 1024);
        }
        close(fd);
    }

    //appending new entry if it has newer modification time (if file with same name exists)
    if (my_strcmp(flags, "-uf") == 0) {
        if (num_files == 0) {
            free(file_names);
            free(input);
            return 1;
        }

        for (int i = 0; i < num_files; i++) {
            int fd = open(arch_name, O_RDWR);

            if (fd == -1) {
                write(2, "my_tar: Cannot open ", 20);
                write(2, arch_name, my_strlen(arch_name));
                write(2, "\n", 1);
                free(input);
                free(file_names);
                return 1;
            }

            int do_write = 0;

            struct stat file_stat;
            stat(file_names[i], &file_stat);
            long time_new = file_stat.st_mtime;

            //traverse the archive structure while comparing file names and mtime
            int write_pos = 0;
            int bytes_read = 0;
            int file_size = 0;
            int padding = 0;
            struct file_header header;
            bytes_read = read(fd, &header, 512);
            while (header.name[0] != '\0') {
                if (my_strcmp(file_names[i], header.name) == 0) {
                    long time_exist = to_decimal_from_str(header.mtime);
                    if (time_exist >= time_new) {
                        do_write = -1;
                    }
                }
                write_pos += bytes_read;
                file_size = to_decimal_from_str(header.size);
                if (file_size > 0) {
                    padding = 512 - (file_size % 512);
                    file_size = file_size + padding;
                }
                char buf[file_size];
                bytes_read = read(fd, buf, file_size);
                write_pos += bytes_read;
                bytes_read = read(fd, &header, 512);
            }

            if (do_write == 0) {
                lseek(fd, write_pos, SEEK_SET);
                int typedescr = fill_header(file_names[i], fd);
                if (typedescr == 0) {
                    write_file_contents(file_names[i], fd);
                }
                
                char final_pad[1024] = {0};
                write(fd, final_pad, 1024);
            }            
            close(fd);
        }
    }

    //extracting an archive
    if (my_strcmp(flags, "-xf") == 0) {
        int fd = open(arch_name, O_RDWR);

        if (fd == -1) {
            write(2, "my_tar: Cannot open ", 20);
            write(2, arch_name, my_strlen(arch_name));
            write(2, "\n", 1);
            free(input);
            free(file_names);
            return 1;
        }

        int new_file;
        int file_size = 0;
        int padding = 0;
        struct file_header header;
        read(fd, &header, 512);
        while (header.name[0] != '\0') {
            file_size = to_decimal_from_str(header.size);

            //for file: create, write contents, set permissions
            if (header.typeflag == '0') {
                new_file = open(header.name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                char buf[file_size];
                read(fd, buf, file_size);
                write(new_file, buf, file_size);
                close(new_file);
                chmod(header.name, to_decimal_from_str(header.mode));

            }

            //for dir: create and set permissions
            if (header.typeflag == '5') {
                mkdir(header.name, to_decimal_from_str(header.mode));
            }

            //for file and dir: set mtime
            long mod_time = to_decimal_from_str(header.mtime);
            struct utimbuf new_time;
            utime(header.name, &new_time);
            new_time.modtime = mod_time;
            new_time.actime = mod_time;

            if (file_size > 0) {
                padding = 512 - (file_size % 512);
            } else {
                padding = 0;
            }
            char new_buf[padding];
            read(fd, new_buf, padding);
            read(fd, &header, 512);
        }

        close(fd);
    }

    free(file_names);
    free(input);

    return 0;
}




//HELPER FUNCTIONS

//function that check for flags. returns 0 if flag not found, else returns the number of flags (1 or 2)
int contain_flags (char** arr, char* flag1, char* flag2) {
    if (my_strcmp(arr[0], flag1) == 0) {
        return 1;
    } else if (my_strcmp(arr[0], flag2) == 0 && my_strcmp(arr[1], "-f") == 0) {
        return 2;
    }
    return 0;
}

int my_strlen(char* str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int my_strcmp (char* str1, char* str2) {
    int i = 0;
    while (str1[i] != '\0' || str2[i] != '\0') {
        if (str1[i] != str2[i]) {
            return 1;
        }
        i++;
    }
    if (str1[i] == '\0' && str2[i] == '\0') {
        return 0;
    }
    return 1;
}

char* my_strcpy(char* dest, char* src) {
    int pos_o = 0;
    for (int i = 0; src[i] != '\0'; i++) {
        dest[i] = src[i];
        pos_o = i + 1;
    }
    dest[pos_o] = '\0';
    return dest;
}

long to_octal(long num) {
    long result = 0;
    long i = 1;  
    while (num > 0) {
        long rem = num % 8;
        result = result + (rem * i);
        num = num / 8;
        i = i * 10;
    }
    return result;
}

//creates a string for fields that require an octal number padded with '0' before it
//str_size is the size of the field in bytes minus 1
char* padded_itoa(long num, int str_size) {
    int len = 0;
    long num_copy = num;
    while (num > 0) {
        num = num / 10;
        len++;
    }
    num = num_copy;

    char* result = malloc((str_size + 1) * sizeof(char));
    int pad = str_size - len;
    if (len == str_size) {
        pad = 0;
    }

    for (int i = 0; i < pad; i++) {
        result[i] = '0';
    }

    for (int i = str_size - 1; i > pad - 1; i--) {
        int n = num % 10;
        num = num / 10;
        result[i] = 48 + n; 
    }
    result[str_size] = '\0';

    return result;
}

//converts octal number as a string to decimal number as a numeral
long to_decimal_from_str(char* str) {
    long octal = 0;
    long result = 0;
    int len = my_strlen(str);

    for (int i = 0; i < len; i++) {
        octal = octal + (str[i] - 48);
        if (i < len - 1) {
            octal = octal * 10;
        }
    }

    for (long i = 1; octal > 0; i = i * 8) {
        result = result + (octal % 10) * i;
        octal = octal / 10;   
    }
    return result;
}


char* concat(char* str1, char* str2) {
    char* result = malloc((my_strlen(str1) + my_strlen(str2) + 1) * sizeof(char));
    int pos = 0;
    for (int i = 0; str1[i] != '\0'; i++) {
        result[pos] = str1[i];
        pos++;
    }
    
    for (int i = 0; str2[i] != '\0'; i++) {
        result[pos] = str2[i];  
        pos++;
    }
    result[pos] = '\0';
    return result;
}

//fill in header
int fill_header(char* file_name, int fd) {
    struct stat file_stat;
    if (stat(file_name, &file_stat) == -1) {
        
        write(2, "my_tar: ", 8);
        write(2, file_name, my_strlen(file_name));
        write(2, ": Cannot stat: No such file or directory\n", 41);
        return -1;
    }

    struct file_header header = {0};

    //getting typeflag
    if (S_ISREG(file_stat.st_mode)) {
        header.typeflag = '0';
    }        
    if (S_ISDIR(file_stat.st_mode)) {
        header.typeflag = '5';
    }

    //getting header name
    if (header.typeflag == '5') {
        char* dirname = concat(file_name, "/");
        my_strcpy(header.name, dirname);
        free(dirname); 
    } else {
        my_strcpy(header.name, file_name);          
    }
    
    //getting permissions
    int perm = file_stat.st_mode & 0777;
    char* mode = padded_itoa(to_octal(perm), 7);
    my_strcpy(header.mode, mode);
    free(mode);

    //getting uid
    int uid = file_stat.st_uid;
    char* file_uid = padded_itoa(to_octal(uid), 7);
    my_strcpy(header.uid, file_uid);
    free(file_uid);

    //getting gid
    int gid = file_stat.st_gid;
    char* file_gid = padded_itoa(to_octal(gid), 7);
    my_strcpy(header.gid, file_gid);
    free(file_gid);

    //getting size
    if (header.typeflag == '5') {
        char* size = padded_itoa(0, 11);
        my_strcpy(header.size, size);
        free(size);            
    } else {
        int file_size = file_stat.st_size;
        char* size = padded_itoa(to_octal(file_size), 11);
        my_strcpy(header.size, size);
        free(size);
    }

    //getting time
    long time = file_stat.st_mtime;
    char* time_str = padded_itoa(to_octal(time), 11);
    my_strcpy(header.mtime, time_str);
    free(time_str);

    //setting magic field
    my_strcpy(header.magic_gnu, "ustar  ");

    //getting uname
    struct passwd *uname;
    if ((uname = getpwuid(uid)) != NULL) {
        my_strcpy(header.uname, uname->pw_name);
    }

    //getting gname
    struct group *gname;
    if ((gname = getgrgid(gid)) != NULL) {
        my_strcpy(header.gname, gname->gr_name);
    }

    //get checksum
    for (int i = 0; i < 8; i++) {
        header.chksum[i] = ' ';
    }     
    char* ptr = (char*)&header;
    int checksum = 0;
    for (int i = 0; i < 512; i++) {
        checksum = checksum + ptr[i];
    }
    char* chk_str = padded_itoa(to_octal(checksum), 6);
    my_strcpy(header.chksum, chk_str);
    header.chksum[7] = ' ';
    free(chk_str);

    //write header to archive file
    write(fd, &header, sizeof(header));

    if (header.typeflag == '0') {
        return 0;
    } else if (header.typeflag == '5') {
        return 5;
    } else {
        return -1;
    }
}

//read file contents in 512 byte increments, write them to archive
void write_file_contents(char* file_name, int fd) {
    int file_fd = open(file_name, O_RDONLY);
    char read_buf[512];
    int bytes_read = read(file_fd, read_buf, 512);

    while (bytes_read == 512) {
        write(fd, read_buf, bytes_read);
        bytes_read = read(file_fd, read_buf, 512);
    }

    if (bytes_read < 512 && bytes_read != 0) {
        write(fd, read_buf, bytes_read);
        int pad_size = 512 - bytes_read;
        char padding[pad_size];
        for (int i = 0; i < pad_size; i++) {
            padding[i] = '\0';
        }
        write(fd, padding, pad_size);        
    }
    close(file_fd);
}

//counts number of files in directory for malloc
int size_of_dir(char* dir) {
    DIR* fold;
    fold = opendir(dir);
    struct dirent *entry;
    entry = readdir(fold);        
    int counter = 0;

    while (entry != NULL) {
        counter++;
        entry = readdir(fold);
    }
   
    closedir(fold);
    return counter;
}