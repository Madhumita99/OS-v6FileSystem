# OS-v6FileSystem
V6 file system is highly restrictive. A modification has been done: Block size is 1024 Bytes, i-node
size is 64 Bytes and i-node’s structure and directory entry structure's have been modified as well.

# How to compile:
1. Open SSH connection to csgrads1.utdallas.edu UTD linux server
2. Copy/Load mod-v6.c file to server directory
3. Compile using the following command in the terminal: "gcc mod-v6.c -o modv6"

# How to run the program:
1. In terminal, enter the following command to start the program: "./modv6"
2. Program will prompt to "Enter command:"
  
User can enter the following commands:

(a) initfs n1 n2
where n1 is the file system size in number of blocks and n2 is the number of blocks devoted to
the i-nodes. Set all the data blocks (except the root directory represented by inode number 1) and 
the inodes to free. All free blocks are accessible from free[] array of the super block 

(b) openfs file_name
file_name is the name of the file in the native unix machine (where you are running your program) 
that represents the disk drive.

(c) cpin externalfile v6-file  
Create a new file called v6-file in the v6 file system and fill the contents of the newly created  
file with the contents of the externalfile.  
  
(d) cpout v6-file externalfile  
If the v6-file exists, create externalfile and make the externalfile's contents equal to v6-file.  
  
(e) mkdir v6-dir  
If v6-dir does not exist, create the directory and set its first two entries . and ..  
To create a directory, the whole filepath has to be mentioned, starting from the root,
to specify the correct location of the directory created.
For example: mkdir /user/Venky will create directory /Venky inside /user
  
(f) rm v6-file  
If v6-file exists, delete the file, free the i-node, remove the file name from the  
(parent) directory that has this file and add all data blocks of this file to the free list      
  
(g) cd dirname  
Change current (working) directory to the dirname. The directory will be changed if it is present
in the current working directory.
  
(h) q
Save all changes and quit. 

When user is done with program, can use "q" command to exit the program.
