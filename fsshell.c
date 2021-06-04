/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
* Student ID: N/A
* Project: Basic File System
*
* File: fsShell.c
*
* Description: Main driver for file system assignment.
*
* Make sure to set the #defined on the CMDxxxx_ON from 0 to 1 
* when you are ready to test that feature
*
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>

#include "mfs.h"
#include "fsVCB.h"
#include "fsLow.h"
#include "fsInode.h"
#include "bitMap.h"
#include "b_io.h"

/***************  START LINUX TESTING CODE FOR SHELL ***************/
#define TEMP_LINUX 0  //MUST be ZERO for working with your file system
#if (TEMP_LINUX == 1)
	// All the following it TEMPORARY ONLY - It allows testing the shell in linux
	// but using the test files system function calls  
	// This must be off for testing your file system
	
	#include <sys/stat.h>
	#include <dirent.h>
	#define fs_mkdir	mkdir
	#define fs_getcwd	getcwd
	#define fs_setcwd	chdir
	#define fs_rmdir	rmdir
	#define fs_delete unlink
	
	
	fdDir * fs_opendir(const char *name)
		{
		fs_dirEntry * dir;
		dir = fs_opendir(name);
		return ((fs_dirEntry *) dir);
		}
	
	struct fs_dir fsDi;	
	struct fs_dir *fs_readdir(fdDir *dirp)
		{
		fs_dirEntry *dir;
		dir = (fs_dirEntry *) dirp;
		struct dirent * de;
		de = readdir (dir);
		if (de == NULL)
			return (NULL);
			
			
		fsDi.d_reclen = (unsigned short) sizeof (fsDe);
		fsDi.fileType = de->d_type;
		strcpy(fsDi.d_name, de->d_name);
		return (&fsDi);
		}
		
	int fs_closedir(fs_dirEntry *dirp)
		{
		fs_dirEntry *dir;
		dir = (fs_dirEntry *) dirp;
		return (closedir (dir));
		}

	int fs_stat(const char *path, struct fs_stat *buf)
		{
		struct stat * path_stat;
		path_stat = (struct stat *) buf;
		return (stat (path, path_stat));
		}

	
	int fs_isFile (char * path)
		{
		struct stat path_stat;
		stat(path, &path_stat);
		return S_ISREG(path_stat.st_mode);
		}
	
	int fs_isDir (char * path)
		{
		struct stat path_stat;
		if (stat(path, &path_stat) != 0)
			return 0;
		return S_ISDIR(path_stat.st_mode);
		}
#endif
/***************  END LINUX TESTING CODE FOR SHELL ***************/




#define SINGLE_QUOTE	0x27
#define DOUBLE_QUOTE	0x22
#define BUFFERLEN		200
#define DIRMAX_LEN		4096

/****   SET THESE TO 1 WHEN READY TO TEST THAT COMMAND ****/
#define CMDLS_ON	1
#define CMDCP_ON	1
#define CMDMV_ON	1
#define CMDMD_ON	1
#define CMDRM_ON	1
#define CMDCP2L_ON	1
#define CMDCP2FS_ON	1
#define CMDCD_ON	1
#define CMDPWD_ON	1


typedef struct dispatch_t
	{
	char * command;
	int (*func)(int, char**);
	char * description;
	} dispatch_t, * dispatch_p;


int cmd_ls (int argcnt, char *argvec[]);
int cmd_cp (int argcnt, char *argvec[]);
int cmd_mv (int argcnt, char *argvec[]);
int cmd_md (int argcnt, char *argvec[]);
int cmd_rm (int argcnt, char *argvec[]);
int cmd_cp2l (int argcnt, char *argvec[]);
int cmd_cp2fs (int argcnt, char *argvec[]);
int cmd_cd (int argcnt, char *argvec[]);
int cmd_pwd (int argcnt, char *argvec[]);
int cmd_history (int argcnt, char *argvec[]);
int cmd_help (int argcnt, char *argvec[]);

dispatch_t dispatchTable[] = {
	{"ls", cmd_ls, "Lists the file in a directory"},
	{"cp", cmd_cp, "Copies a file - source [dest]"},
	{"mv", cmd_mv, "Moves a file - source dest"},
	{"md", cmd_md, "Make a new directory"},
	{"rm", cmd_rm, "Removes a file or directory"},
	{"cp2l", cmd_cp2l, "Copies a file from the test file system to the linux file system"},
	{"cp2fs", cmd_cp2fs, "Copies a file from the Linux file system to the test file system"},
	{"cd", cmd_cd, "Changes directory"},
	{"pwd", cmd_pwd, "Prints the working directory"},
	{"history", cmd_history, "Prints out the history"},
	{"help", cmd_help, "Prints out help"}
};

static int dispatchcount = sizeof (dispatchTable) / sizeof (dispatch_t);

// Display files for use by ls command
int displayFiles (fs_dir * dirp, int flall, int fllong)
	{
	if (dirp == NULL)	//get out if error
		return (-1);
		
	struct fs_dirEntry * di;
	struct fs_stat statbuf;
	
	di = fs_readdir (dirp);
	// printf("di: %s\n", di->d_name);
	printf("\n");
	while (di != NULL) 
		{
		if ((di->d_name[0] != '.') || (flall)) //if not all and starts with '.' it is hidden
			{
			if (fllong)
				{
				fs_stat (di->d_name, &statbuf);
				printf ("%s    %9ld   %s\n", fs_isDir(di->d_name)?"D":"-", statbuf.st_size, di->d_name);
				}
			else
				{
				printf ("%s\n", di->d_name);
				}
			}
		di = fs_readdir (dirp);
		}
	fs_closedir (dirp);
	return 0;
	}
	

/****************************************************
*  ls commmand
****************************************************/
int cmd_ls (int argcnt, char *argvec[])
	{
#if (CMDLS_ON == 1)				
	int option_index;
	int c;
	int fllong;
	int flall;
	char cwd[DIRMAX_LEN];
		
	static struct option long_options[] = 
		{
			/* These options set their assigned flags to value and return 0 */
			/* These options don't set flags and return the value */	 
			{"long",	no_argument, 0, 'l'},  
			{"all",		no_argument, 0, 'a'},
			{"help",	no_argument, 0, 'h'},
			{0,			0,       0,  0 }
		};
		
	option_index = 0;
#ifdef __GNU_LIBRARY__
    // WORKAROUND
    // Setting "optind" to 0 triggers initialization of getopt private
    // structure (holds pointers on data between calls). This helps
    // to avoid possible memory violation, because data passed to getopt_long()
    // could be freed between parse() calls.
    optind = 0;
#else
    // "optind" is used between getopt() calls to get next argument for parsing and should be
    // initialized before each parsing loop.
    optind = 1;
#endif
	fllong = 0;
	flall = 0;

	while (1)
		{	
		c = getopt_long(argcnt, argvec, "alh",
				long_options, &option_index);
				
		if (c == -1)
		   break;

		switch (c) {
			case 0:			//flag was set, ignore
			   printf("Unknown option %s", long_options[option_index].name);
			   if (optarg)
				   printf(" with arg %s", optarg);
			   printf("\n");
				break;
				
			case 'a':
				flall = 1;
				break;
				
			case 'l':
				fllong = 1;
				break;
				
			case 'h':
			default:
				printf ("Usage: ls [--all-a] [--long/-l] [pathname]\n");
				return (-1);
				break;
			}
		}
	
	
	if (optind < argcnt)
		{
		//processing arguments after options
		for (int k = optind; k < argcnt; k++)
			{
			if (fs_isDir(argvec[k]))
				{
				fs_dir * dirp;
				dirp = fs_opendir (argvec[k]);
				displayFiles (dirp, flall, fllong);
				}
			else // it is just a file ?
				{
				if (fs_isFile (argvec[k]))
					{
					//no support for long format here
					printf ("%s\n", argvec[k]);
					}
				else
					{
					printf ("%s is not found\n", argvec[k]);
					}
				}
			}		
		}
	else   // no pathname/filename specified - use cwd
		{
		char * path = fs_getcwd(cwd, DIRMAX_LEN);	//get current working directory
		fs_dir * dirp;
		dirp = fs_opendir (path);
		//printf("shell: dirp = %s\n", dirp->name);
		return (displayFiles (dirp, flall, fllong));
		}
	return 0;
#endif
	}

	
/****************************************************
*  Copy file commmand
****************************************************/
	
int cmd_cp (int argcnt, char *argvec[])
	{
#if (CMDCP_ON == 1)	
	int testfs_src_fd;
	int testfs_dest_fd;
	char * src;
	char * dest;
	int readcnt;
	char buf[BUFFERLEN];
	
	switch (argcnt)
		{
		case 2:	//only one name provided
			src = argvec[1];
			dest = src;
			break;
			
		case 3:
			src = argvec[1];
			dest = argvec[2];
			break;
		
		default:
			printf("Usage: cp srcfile [destfile]\n");
			return (-1);
		}
	
	
	testfs_src_fd = b_open (src, O_RDONLY);
	testfs_dest_fd = b_open (dest, O_WRONLY | O_CREAT | O_TRUNC);
	do 
		{
		readcnt = b_read (testfs_src_fd, buf, BUFFERLEN);
		b_write (testfs_dest_fd, buf, readcnt);
		} while (readcnt == BUFFERLEN);
	b_close (testfs_src_fd);
	b_close (testfs_dest_fd);
	return 0;
#endif
	}
	
/****************************************************
*  Move file commmand
****************************************************/
int cmd_mv (int argcnt, char *argvec[])
	{
#if (CMDMV_ON == 1)				
	// **** TODO ****  For you to implement	
	//check if argcnt == 3
	if(argcnt != 3) {
		printf("Usage: mv source destination\n");
		return (-1);
	} else {
		//check if source and destination is relative path
		char* source = argvec[1];
		char* dest = argvec[2];
		char* cwd;
		char* destName;
		if(source[0] != '/') {
			//get current working directory
			getcwd(cwd, DIRMAX_LEN);

			//concatenate to source
			strcat(cwd, source);
			source = cwd;
		}

		if(dest[0] != '/') {
			destName = dest;
			//get current working directory and concatenate to dest
			getcwd(cwd, DIRMAX_LEN);
			strcat(cwd, dest);
			dest = cwd;
		}

		//if destination does not exist, create one
		fs_dir* destEntry = fs_opendir(dest);
		InodeType destType;
		if(destEntry == NULL) {
			//check whether the destination is file or not
			if(dest[strlen(dest) - 4] == '.') {
				destType = I_FILE;
			} else {
				destType = I_DIR;
			}

			destEntry = createInode(destType, dest);
		}

		//check whether destination is file or directory
		destType = destEntry->type;
		int moved;
		fs_dir* sourceEntry = fs_opendir(source);
		if(destType == I_DIR) {
			//if directory, put source as the child of dest
			char* srcParentPath;
			getParentPath(srcParentPath, source);
			fs_dir* oldParent = fs_opendir(srcParentPath);
			int removed = removeFromParent(oldParent, sourceEntry);
			if(removed == 0) {
				printf("Error: cannot remove from parent.");
				return -1;
			}
			int moved = setParent(destEntry,sourceEntry);
			if(moved == 0) {
				printf("Error: Cannot move to %s", dest);
				return -1;
			}
		} else {
			//if file, check if source is file
			if(sourceEntry->type == I_FILE) {
				//if source is file, rename the source and delete dest
				strcpy(sourceEntry->name, destName);
				strcpy(sourceEntry->path, dest);
				fs_delete(destEntry->name);
			} else {
				printf("Cannot move directory to file");
				return -1;
			}
		}
	}

	return 0;
#endif
	}

/****************************************************
*  Make Directory commmand
****************************************************/
// Make Directory	
int cmd_md (int argcnt, char *argvec[])
	{
#if (CMDMD_ON == 1)				
	if (argcnt != 2)
		{
		printf("Usage: md pathname\n");
		return -1;
		}
	else
		{
		return(fs_mkdir(argvec[1], 0777));
		}
#endif
	}
	
/****************************************************
*  Remove directory or file commmand
****************************************************/
int cmd_rm (int argcnt, char *argvec[])
	{
#if (CMDRM_ON == 1)
	if (argcnt != 2)
		{
		printf ("Usage: rm path\n");
		return -1;
		}
		
	char * path = argvec[1];	
	
	//must determine if file or directory
	if (fs_isDir (path))
		{
		return (fs_rmdir(path));
		}		
	if (fs_isFile (path))
		{
		return (fs_delete(path));
		}	
		
	printf("The path %s is neither a file not a directory\n", path);
	return -1;
#endif
	}
	
/****************************************************
*  Copy file from test file system to Linux commmand
****************************************************/
int cmd_cp2l (int argcnt, char *argvec[])
	{
#if (CMDCP2L_ON == 1)				
	int testfs_fd;
	int linux_fd;
	char * src;
	char * dest;
	int readcnt;
	char buf[BUFFERLEN];
	
	switch (argcnt)
		{
		case 2:	//only one name provided
			src = argvec[1];
			dest = src;
			break;
			
		case 3:
			src = argvec[1];
			dest = argvec[2];
			break;
		
		default:
			printf("Usage: cp2l srcfile [Linuxdestfile]\n");
			return (-1);
		}
	
	
	testfs_fd = b_open (src, O_RDONLY);
	linux_fd = open (dest, O_WRONLY | O_CREAT | O_TRUNC);
	do 
		{
		readcnt = b_read (testfs_fd, buf, BUFFERLEN);
		printf("CP2l: readcnt = %d\n", readcnt);
		write (linux_fd, buf, readcnt);
		} while (readcnt == BUFFERLEN);
	b_close (testfs_fd);
	close (linux_fd);
	return 0;
#endif
	}
	
/****************************************************
*  Copy file from Linux to test file system commmand
****************************************************/
int cmd_cp2fs (int argcnt, char *argvec[])
	{
#if (CMDCP2FS_ON == 1)				
	int testfs_fd;
	int linux_fd;
	char * src;
	char * dest;
	int readcnt;
	char buf[BUFFERLEN];
	
	switch (argcnt)
		{
		case 2:	//only one name provided
			src = argvec[1];
			dest = src;
			break;
			
		case 3:
			src = argvec[1];
			dest = argvec[2];
			break;
		
		default:
			printf("Usage: cp2fs Linuxsrcfile [destfile]\n");
			return (-1);
		}
	
	
	testfs_fd = b_open (dest, O_WRONLY | O_CREAT | O_TRUNC);
	linux_fd = open (src, O_RDONLY);
	do 
		{
		readcnt = read (linux_fd, buf, BUFFERLEN);
		b_write (testfs_fd, buf, readcnt);
		} while (readcnt == BUFFERLEN);
	b_close (testfs_fd);
	close (linux_fd);
	return 0;
#endif
	}
	
/****************************************************
*  cd commmand
****************************************************/
int cmd_cd (int argcnt, char *argvec[])
	{
#if (CMDCD_ON == 1)	
	if (argcnt != 2)
		{
		printf ("Usage: cd path\n");
		return (-1);
		}
	char * path = argvec[1];	//argument
	
	if (path[0] == '"')
		{
		if (path[strlen(path)-1] == '"')
			{
			//remove quotes from string
			path = path + 1;
			path[strlen(path) - 1] = 0;
			}
		}
	int ret = fs_setcwd(path);
	if (ret != 0)	//error
		{
		printf ("Could not change path to %s\n", path);
		return (ret);
		}			
	return 0;
#endif
	}
	
/****************************************************
*  PWD commmand
****************************************************/
int cmd_pwd (int argcnt, char *argvec[])
	{
#if (CMDPWD_ON == 1)
	char * dir_buf = malloc (DIRMAX_LEN +1);
	char * ptr;	
	ptr = fs_getcwd (dir_buf, DIRMAX_LEN);	
	if (ptr == NULL)			//an error occurred
		{
		printf ("An error occurred while trying to get the current working directory\n");
		}
	else
		{
		printf ("%s\n", ptr);
		}
	free (dir_buf);
	dir_buf = NULL;
	ptr = NULL;
	
	return 0;
#endif
	}

/****************************************************
*  History commmand
****************************************************/
int cmd_history (int argcnt, char *argvec[])
	{
	HIST_ENTRY * he;
	int i = 0;
	
	
	for (i = history_base; i <= history_length; i++)
		{
		he = history_get(i);
		
		if (he != NULL)
			{
			printf ("%s\n", he->line);
			}
		}
	return 0;
	}
	
/****************************************************
*  Help commmand
****************************************************/
int cmd_help (int argcnt, char *argvec[])
	{
	for (int i = 0; i < dispatchcount; i++)
		{
		printf ("%s\t%s\n", dispatchTable[i].command, dispatchTable[i].description);
		}
	return 0;
	} 

void processcommand (char * cmd)
	{
	int cmdLen;
	char ** cmdv;	//command vector
	int cmdc;		//command count
	int i, j;
	
	cmdLen = strlen(cmd);
	cmdv = (char **) malloc (sizeof(char *) * ((cmdLen/2)+2));
	cmdc = 0;
	
	cmdv[cmdc] = cmd;
	++cmdc;
	
	for (i = 0; i < cmdLen; i++)
		{
		switch (cmd[i])
			{
			case ' ':
				cmd[i] = 0;	//NULL terminate prior string
				while (cmd[i+1] == ' ')  // null at end will prevent from overshooting string
					{
					i++;
					}
				if ((i+1) < cmdLen)	//there is still more
					{
					cmdv[cmdc] = &cmd[i+1];
					++cmdc;
					}
				break;
				
			case '\\':
				++i;	//skip next character
				break;
				
			case DOUBLE_QUOTE:	//ignore everything till next quote (unless unterminated)
				for (j = i+1; j < cmdLen; j++)
					{
					if (cmd[j] == '\\')
						{
						++j;	//skip next character
						}	
					else if (cmd[j] == DOUBLE_QUOTE)
						{
						break;
						}
					}
				if (j >= cmdLen)
					{
					printf("Unterminated string\n");
					free(cmdv);
					cmdv = NULL;
					return;
					}
				i=j;
				break;
				
			case SINGLE_QUOTE:
				for (j = i+1; j < cmdLen; j++)
					{
					if (cmd[j] == '\\')
						{
						++j;	//skip next character
						}	
					else if (cmd[j] == SINGLE_QUOTE)
						{
						break;
						}
					}
				if (j >= cmdLen)
					{
					printf("Unterminated string\n");
					free(cmdv);
					cmdv = NULL;
					return;
					}
				i=j;
				break;
			default:
				break;
			}
		}
		
#ifdef COMMAND_DEBUG
	for (i = 0; i < cmdc; i++)
		{
		printf("%s: length %d\n", cmdv[i], strlen(cmdv[i]));
		}
#endif		
	cmdv[cmdc] = 0;		//just be safe - null terminate array of arguments
	
	for (i = 0; i < dispatchcount; i++)
		{
		if (strcmp(dispatchTable[i].command, cmdv[0]) == 0)
			{
			dispatchTable[i].func(cmdc,cmdv);
			free (cmdv);
			cmdv = NULL;
			return;
			}
		}
	printf("%s is not a regonized command.\n", cmdv[0]);
	cmd_help(cmdc, cmdv);	
	free (cmdv);
	cmdv = NULL;
	}

void initFileSystem(char* volumeName) 
{
	openVolume(volumeName);
	fs_init();
}

void closeFileSystem() 
{
	fs_close();
	closeVolume();
}


int main (int argc, char * argv[])
	{
	char * cmdin;
	char * cmd;
	HIST_ENTRY *he;
		
	using_history();
	stifle_history(200);	//max history entries

	char volumeName[MAX_FILENAME_SIZE];
    strcpy(volumeName, argv[1]);
	initFileSystem(volumeName);
	
	while (1)
		{
		cmdin = readline("Prompt > ");
#ifdef COMMAND_DEBUG
		printf ("%s\n", cmdin);
#endif
		
		cmd = malloc (strlen(cmdin) + 30);
		strcpy (cmd, cmdin);
		free (cmdin);
		cmdin = NULL;
		
		if (strcmp (cmd, "exit") == 0)
			{
			free (cmd);
			cmd = NULL;
			// exit while loop and terminate shell
			break;
			}
			
		if ((cmd != NULL) && (strlen(cmd) > 0))
			{
			he = history_get(history_length);
			if (!((he != NULL) && (strcmp(he->line, cmd)==0)))
				{
				add_history(cmd);
				}
			processcommand (cmd);
			}
				
		free (cmd);
		cmd = NULL;		
		} // end while

		closeFileSystem();
	}
