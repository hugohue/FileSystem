# FileSystem
Operating System
- -d [filename]

It is a filename of a disk containing a FAT32 file system. It can be an image file,
which is just an ordinary file or a device file. Here are two assumptions of the file: (1)
the input file is a file carrying the FAT32 disk format; (2) the provided filename always
refers to an existing and readable device file.

- -i: Print the information about a FAT32 file system

- -l: List the directory content

- -r: Recover one-cluster-sized files only
