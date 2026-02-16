# Welcome to My Tar
***

## Task
The task was to create a simplified version of the tar command.

## Description
My_tar is a simplified version of the tar command that allows you to create, list, extract, and append 
to tape archives. The program supports basic tar operations: creating new archives (flag "-cf"), 
viewing archive contents (flag "-tf"), extracting files (flag "-xf") and appending entries without conditions 
(flag "-rf") or with conditions based on modification time (flag "-uf"). Archives created by my_tar are 
compatible with the standard tar utility. The program also includes a number of helper functions, including 
functions for string manipulation that mimic "forbidden" library functions.

## Installation
You can install the program by running make.
As a prerequisite you need to have a GCC compiler and the Make utility.

## Usage
When running the program, you enter the command in the terminal the following way:
```
./my_tar [flags] [archive] [files/directories]
```
- You can use the flags "-cf", "-rf", "-uf", "-tf" and "-xf". You can also enter them as two separate flags (e.g. "-c" and "-f")
- Only one flag can be used for one command
- Flags can be entered only right after ./my_tar
- You can list multiple files and directories that are added to the archive when using -cf
- You can list multiple files that are added to the archive when using -rf and -uf
- Examples:
./my_tar -cf archive file1 file2 directory <br>
./my_tar -rf archive file1 <br>
./my_tar -uf archive file1 file2 <br>
./my_tar -tf archive <br>
./my_tar -xf archive <br>
