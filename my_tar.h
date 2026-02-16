#ifndef MY_TAR_H
#define MY_TAR_H

//string operations
char* concat(char* str1, char* str2);
int my_strcmp (char* str1, char* str2);
char* my_strcpy(char* dest, char* src);
int my_strlen(char* str);

//conversion
char* padded_itoa(long num, int str_size);
long to_decimal_from_str(char* str);
long to_octal(long num);

//writing archive
int fill_header(char* file_name, int fd);
void write_file_contents(char* file_name, int fd);

//misc
int contain_flags (char** arr, char* flag1, char* flag2);
int size_of_dir(char* dir);

struct file_header
{                              
  char name[100];               
  char mode[8];                   
  char uid[8];                  
  char gid[8];                  
  char size[12];                
  char mtime[12];               
  char chksum[8];               
  char typeflag;                
  char linkname[100];           
  char magic_gnu[8];            
  char uname[32];               
  char gname[32];               
  char prefix[155];             
                                
  char padding[28];  
};

#endif