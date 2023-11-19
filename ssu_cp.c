#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pwd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <utime.h>

#define DIRECTORY_SIZE MAXNAMLEN
#define FILE_SIZE 10000

void search_dir(char *, char *); // function of searching directory
void directory_count_func(char *fname1, char *fname2); // function of how many directories are existing under fname1
void file_copy(char *fname1, char *fname2); // function of copying file under fname1 -> fname2

char directory_name1[100][200]; // global array variable : save directory name under fname1
char directory_name2[100][200]; // global array variable : save directory name under fname2
int dir_count = 0; // global integer variable : counting directory
int dir_i = 0; // index variable for function "void file_copy()"

int main(int argc, char *argv[])
{
	struct stat statbuf;

	struct tm *timeinfo;
	struct utimbuf tim_buf;
	struct passwd *uid;
	struct passwd *gid;

	DIR *dp; // directory pointer varialbe

	char *fname1, *fname2;
	char *Nval; // N value of option d : -d [1~9]
	char time_buf[100]; 
	char *buf; // buf array, save file text to here
	int fsize; // variable for file size count
	int fd1, fd2; // file descriptor 1, 2
	char yn; // for option i, variable for y/n
	int opt; // option variable
	int flag_p = 0, flag_r = 0, flag_i = 0, flag_n = 0,
		flag_s = 0, flag_l = 0, flag_d = 0; // option variable
	int flag_default = 1;
	int status;

	while((opt = getopt(argc, argv, "isd:nprl")) != -1) {
		switch(opt) {
			case 'i':
				flag_i = 1;
				printf("  i option is on...\n");
				break;

			case 's':
				flag_s = 1;
				printf("  s option is on...\n");
				break;

			case 'n':
				flag_n = 1;
				printf("  n option is on...\n");
				break;

			case 'p':
				flag_p = 1;
				printf("  p option is on...\n");
				break;

			case 'r':
				flag_r = 1;
				printf("  r option is on...\n");
				break;

			case 'l':
				flag_l = 1;
				printf("  l option is on...\n");
				break;

			case 'd':
				flag_d = 1;
				Nval = optarg;
				printf("  d option is on...\n");
				break;

			default :
				flag_default = 1;
				break;
		}

	}


	if(flag_i || flag_n || flag_p || flag_r || flag_s || flag_l || flag_d){

		if(argc < 4) {
			fprintf(stderr, "Missing [SOURCE] / [TARGET}... STOP\n");
			exit(1);
		}

		if(argc == 4) {
			fname1 = argv[2];
			fname2 = argv[3];
		}	

		if (argc == 5) {
			fname1 = argv[3];
			fname2 = argv[4];
		}		
		printf("target : %s\n", fname2);
		printf("src    : %s\n", fname1);
	}

	if(flag_p) { // printing Data information : Last read, modify, change time / OWNER|GROUP name, file size

		if(flag_r || flag_d)
		{
			if((dp = opendir(fname1)) == NULL) {
				fprintf(stderr, "open directory error for %s\n", fname1);
				exit(1);
			}

			if((stat(fname1, &statbuf)) < 0) {
				fprintf(stderr, "stat error for %s\n", fname1);
				exit(1);
			}
		}

		else {
			if((fd1 = open(fname1, O_RDONLY)) < 0) {
				fprintf(stderr, "open error for %s\n", fname1);
				exit(1);
			}

			if((fsize = lseek(fd1, 0, SEEK_END)) < 0) {
				fprintf(stderr, "lseek error\n");
				exit(1);
			}
			lseek(fd1, 0, SEEK_SET);

			buf = (char *)malloc(sizeof(char) * fsize);

			if(read(fd1, buf, fsize) < 0) {
				fprintf(stderr, "read errof\n");
				exit(1);
			}

			if((fd2 = open(fname2, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
				fprintf(stderr, "open error for %s\n", fname2);
				exit(1);
			}

			write(fd2, buf, fsize);

			if((stat(fname1, &statbuf)) < 0) {
				fprintf(stderr, "stat error for %s\n", fname1);
				exit(1);
			}
			free(buf);
		}

		printf("*************** file info ***************\n");
		printf("File name : %s\n", fname1);
		time(&statbuf.st_atime);
		timeinfo = localtime(&statbuf.st_atime);
		strftime(time_buf, 100, "%Y.%m.%d %H:%M:%S", timeinfo);
		printf("Last Read time for Data : %s\n", time_buf);
		time(&statbuf.st_mtime);
		timeinfo = localtime(&statbuf.st_mtime);
		strftime(time_buf, 100, "%Y.%m.%d %H:%M:%S", timeinfo);
		printf("Last Modified time for data : %s\n", time_buf);
		time(&statbuf.st_ctime);
		timeinfo = localtime(&statbuf.st_ctime);
		strftime(time_buf, 100, "%Y.%m.%d %H:%M:%S", timeinfo);
		printf("Last Change time for Data : %s\n", time_buf);
		uid = getpwuid(statbuf.st_uid);
		printf("OWNER : %s\n", uid->pw_name);
		gid = getpwuid(statbuf.st_gid);
		printf("GROUP : %s\n", gid->pw_name);
		printf("file size : %d\n", fsize);

	}


	if(flag_i || flag_n) { // option i : if file already exist in the system, ask user whether copy on the address or not
		// option n : if file already exist in the system, do not copy

		if(flag_r || flag_d) {
			if((dp = opendir(fname1)) == NULL) {
				fprintf(stderr, "opendir error for %s\n", fname1);
				exit(1);
			}
			if((dp = opendir(fname2)) != NULL) {
				if(flag_n) {
					printf("-----------------------------------------------\n");
					printf("Target already exist :  Option 'n' should not have same [Target] on the system..\n");
					exit(0);
				}
				else {
					printf("overwrite to %s (y/n) : ", fname2);
					scanf("%c", &yn);
					if(yn == 'n') exit(0);
				}
			}
		}
		else {
			if((fd1 = open(fname1, O_RDONLY)) < 0) {
				fprintf(stderr, "Open error for %s\n", fname1);
				exit(1);
			}

			if((fsize = lseek(fd1, 0, SEEK_END)) < 0) {
				fprintf(stderr, "lseek error\n");
				exit(1);
			}
			lseek(fd1, 0, SEEK_SET);

			buf = (char *)malloc(sizeof(char) * fsize);

			if(read(fd1, buf, fsize) < 0) {
				fprintf(stderr, "read error\n");
				exit(1);
			}

			if((fd2 = open(fname2, O_RDWR)) < 0) {
				fd2 = open(fname2, O_RDWR | O_CREAT | O_TRUNC, 0644);
				write(fd2, buf, fsize);
				close(fd2);
			}
			else if(flag_n) { // if flag_n is on, print [TARGET] exist statement and finish the program.
				printf("-----------------------------------------------\n");
				printf("Target already exist :  Option 'n' should not have same [Target] on the system..\n");
				exit(0);
			}

			else {
				close(fd2);
				fd2 = open(fname2, O_RDWR | O_CREAT | O_TRUNC, 0644);
				printf("overwrite to %s (y/n) ? : ", fname2);
				scanf("%c", &yn);
				if(yn == 'y')
					write(fd2, buf, fsize);
				else if(yn == 'n') exit(0);
				else {
					fprintf(stderr, "wrong input, option only valid for 'y' and 'n'\n");
					exit(1);
				}
			}
			free(buf);
		}
	}

	if(flag_s) { // building symlink 

		if((dp = opendir(fname1)) != NULL) {
			fprintf(stderr, "Can't make symlink for directory\n");
			exit(1);
		}

		if((fd1 = open(fname1, O_RDONLY)) < 0) {
			fprintf(stderr, "open error for %s\n", fname1);
			exit(1);
		}

		if(symlink(fname1, fname2) < 0) {
			fprintf(stderr, "symlink error\n");
			exit(1);
		}

	}

	if(flag_l) { // copy file : includig file attribution and time information

		if(flag_r || flag_d) {
			if((dp = opendir(fname1)) == NULL) {
				fprintf(stderr, "directory not exist\n");
				exit(1);
			}
			closedir(dp);
		}

		else {
			if((fd1 = open(fname1, O_RDONLY)) < 0) { // open file with Read Only option
				fprintf(stderr, "open error for %s\n", fname1);
				exit(1);
			}

			if((fsize = lseek(fd1, 0, SEEK_END)) < 0) { // size varification with lseek
				fprintf(stderr, "lseek error \n");
				exit(1);
			}
			lseek(fd1, 0, SEEK_SET); // move offset to the initial location

			buf = (char *)malloc(sizeof(char) * fsize); // allocate buf with size 'fsize'

			if(read(fd1, buf, fsize) < 0) { // read file into buf
				fprintf(stderr, "read error\n");
				exit(1);
			}

			if((fd2 = open(fname2, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
				fprintf(stderr, "open error for %s\n", fname2);
				exit(1);
			}
			write(fd2, buf, fsize);

			if(stat(fname1, &statbuf) < 0) {
				fprintf(stderr, "stat error for %s\n", fname1);
				exit(1);
			}

			if(chmod(fname2, statbuf.st_mode) < 0) {
				fprintf(stderr, "%s : chmod error\n", fname2);
				exit(1);
			}

			if(chown(fname2, statbuf.st_uid, statbuf.st_gid) < 0) {
				fprintf(stderr, "chown error for %s\n", fname2);
				exit(1);
			}

			tim_buf.actime = statbuf.st_atime;
			tim_buf.modtime = statbuf.st_mtime;

			if(utime(fname2, &tim_buf) < 0) {
				fprintf(stderr, "utime error for %s\n", fname2);
				exit(1);
			}
			free(buf);
		}

	}

	// option r : copy directory // option d : copy directory through child process
	// both option should run individually - if r option is on, d option is not running
	if(flag_r == 1 && flag_d != 1) {

		mkdir(fname2, 0766);
		char dir_buf1[200] = "\0"; // dir_buf1 : array of fname1's absolute path name String, initialize with NULL value.
		char dir_buf2[200] = "\0"; // dir_buf2 : array of fname2's absolute path name String, initialize with NULL value.
		char *s = getcwd(NULL, 0); // path of current working directory
		s = getcwd(NULL, 0);

		strcpy(dir_buf1, s); // copy current working directory to dir_buf1
		strcpy(dir_buf2, s); // copy current working directory to dir_buf2

		strcat(dir_buf1, "/"); // separation between cur-dir and fname1
		strcat(dir_buf2, "/"); // separation between cur-dir and fname2

		strcat(dir_buf1, fname1); // full path of fname1
		strcat(dir_buf2, fname2); // full path of fname2

		search_dir(dir_buf1, dir_buf2); // call directory copy function

	}

	if(flag_d == 1 && flag_r != 1) {

		if(argc < 5) {
			fprintf(stderr, "d option should have N value : usage -d [0~9] [SRC] [TARGET]...\n");
			exit(1);
		}

		mkdir(fname2, 0766);
		char dir_buf1[200] = "\0";
		char dir_buf2[200] = "\0";
		char *s = getcwd(NULL, 0);
		int pid;
		int Num = atoi(Nval);

		if(Num > 9 || Num < 0) { // d option can have N value from 1 to 9
			fprintf(stderr, "-d : [N-value] is a integer number from '1' to '9'...\n");
			exit(1);
		}

		s = getcwd(NULL, 0);

		strcpy(dir_buf1, s);
		strcpy(dir_buf2, s);

		strcat(dir_buf1, "/");
		strcat(dir_buf2, "/");

		strcat(dir_buf1, fname1);
		strcat(dir_buf2, fname2);

		strcpy(directory_name1[0], dir_buf1);
		strcpy(directory_name2[0], dir_buf2);

		dir_count++;

		mkdir(directory_name2[0], 0766);
		directory_count_func(dir_buf1, dir_buf2);
		file_copy(directory_name1[0], directory_name2[0]);

		printf("---------------------------------------\n");
		printf("Parents : %d\n", getpid());

		for(int i = 1; i < Num; i++)
		{
			pid = fork();
			if(pid < 0) {
				fprintf(stderr, "FORK ERROR\n");
				exit(1);
			}

			else if(pid == 0) {

				if(i < dir_count) {

					mkdir(directory_name2[i], 0766);
					file_copy(directory_name1[i], directory_name2[i]);
					exit(1);
				}

				else exit(1);
			}

			else {
				wait(&status);
				if(i >= dir_count) printf("no directory copied : ");
				printf("Child   : %d\n", pid);
			}
		}
		printf("---------------------------------------\n");
	}

	if(flag_default) { // Default : flag_default is always 1.

		if(flag_i || flag_n || flag_p || flag_r || flag_s || flag_l || flag_d) exit(0); 
		// if any of the option above is inputted, default is not running.

		fname1 = argv[1];
		fname2 = argv[2];

		if(argc < 3) {
			fprintf(stderr, "Usage : ./ssu_cp [OPTION] [SOURCE] [TARGET]\n");
			exit(1);
		}

		if((fd1 = open(fname1, O_RDONLY)) < 0) {
			fprintf(stderr, "open error for %s\n", fname1);
			exit(1);
		}

		if((fsize = lseek(fd1, 0, SEEK_END)) < 0) {
			fprintf(stderr, "lseek error\n");
			exit(1);
		}
		lseek(fd1, 0, SEEK_SET);

		buf = (char *)malloc(sizeof(char) * fsize);

		if(read(fd1, buf, fsize) < 0) {
			fprintf(stderr, "read errof\n");
			exit(1);
		}

		if((fd2 = open(fname2, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
			fprintf(stderr, "open error for %s\n", fname2);
			exit(1);
		}

		write(fd2, buf, fsize);

		printf("target : %s\n", argv[2]);
		printf("src    : %s\n", argv[1]);
		free(buf);
	}
	exit(0);
}

// void search_dir(char *original path, char *copy path)
// this function is working for option 'r'
void search_dir(char *fname1, char *fname2)
{
	struct stat statbuf;
	struct dirent **namelist;
	struct dirent *dentry;
	DIR *dirp;

	char dbuf[100][200];
	char dir_buffer1[100][200];
	char dir_buffer2[100][200];
	char reg_buf[100][FILE_SIZE];
	char dir_buf1[200] = "\0";
	char dir_buf2[200] = "\0";
	int cnt = 0;
	int fsize;
	int length;
	int flength;
	int i;
	int ffd = 0;
	int ffd2 = 0;
	int pid;
	int status;

	strcpy(dir_buf1, fname1);
	strcpy(dir_buf2, fname2);

	if((dirp = opendir(fname1)) == NULL || chdir(fname1) == -1) {
		fprintf(stderr, "opendir, chdir error for %s\n", dir_buf1);
		exit(1);
	}


	while((dentry = readdir(dirp)) != NULL) {
		if(dentry->d_ino == 0) continue;
		memcpy(dbuf[cnt], dentry->d_name, DIRECTORY_SIZE);
		cnt++;
	}

	// by using string function, save every file path to buf array
	for(i = 0; i < cnt; i++) {
		if(dbuf[i][0] != '.') {
			strcat(dir_buf1, "/");
			strcat(dir_buf2, "/");
			strcat(dir_buf1, dbuf[i]);
			strcat(dir_buf2, dbuf[i]);
			strcpy(dir_buffer1[i], dir_buf1);
			strcpy(dir_buffer2[i], dir_buf2);
			for(int j = strlen(dir_buf1); j > 0; j--) {
				if(dir_buf1[j] == '/') {
					dir_buf1[j] = '\0';
					break;
				}
				dir_buf1[j] = '\0';
			}
			for(int j = strlen(dir_buf2); j > 0; j--) {
				if(dir_buf2[j] == '/') {
					dir_buf2[j] = '\0';
					break;
				}
				dir_buf2[j] = '\0';
			}
		}
		else {
			strcpy(dir_buffer1[i], dbuf[i]);
			strcpy(dir_buffer2[i], dbuf[i]);
		}
	}

	for(i = 0; i < cnt; i++) {
		if(dir_buffer1[i][0] == '.') continue;
		if((dirp = opendir(dir_buffer1[i])) == NULL) { // if it is file, run copy
			if((ffd = open(dir_buffer1[i], O_RDONLY)) < 0 ) {
				fprintf(stderr, "open error for %s\n", dir_buffer1[i]);
				exit(1);
			}

			if((fsize = lseek(ffd, 0, SEEK_END)) < 0) {
				fprintf(stderr, "lseek error\n");
				exit(1);
			}
			lseek(ffd, 0, SEEK_SET);

			if(read(ffd, reg_buf[i], fsize) < 0) {
				fprintf(stderr, "read error\n");
				exit(1);
			}

			if((ffd2 = open(dir_buffer2[i], O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
				fprintf(stderr, "open error for %s\n", dir_buffer2[i]);
				exit(1);
			}
			write(ffd2, reg_buf[i], fsize);

		}
		else { // if directory, make directory to copy path and recursively call function
			dir_count++;
			mkdir(dir_buffer2[i], 0766);
			search_dir(dir_buffer1[i], dir_buffer2[i]);
		}
	}
}

// void directory_count_func(char *originalpath, char *copy path)
// this function run in option 'd'
// count how many directories are existing under original path
void directory_count_func(char *fname1, char *fname2)
{
	struct dirent *dentry;
	DIR *dirp;

	char dbuf[100][200];
	char dir_buffer1[100][200];
	char dir_buffer2[100][200];
	char reg_buf[100][FILE_SIZE];
	char dir_buf1[200] = "\0";
	char dir_buf2[200] = "\0";
	int cnt = 0;
	int i;

	strcpy(dir_buf1, fname1);
	strcpy(dir_buf2, fname2);

	// try open and change directory to fname1
	if((dirp = opendir(fname1)) == NULL || chdir(fname1) == -1) {
		fprintf(stderr, "opendir, chdir error for %s\n", dir_buf1);
		exit(1);
	}


	// list all the file existing under original directory
	while((dentry = readdir(dirp)) != NULL) {
		if(dentry->d_ino == 0) continue;
		memcpy(dbuf[cnt], dentry->d_name, DIRECTORY_SIZE);
		cnt++;
	} 

	// save file's absolute path to buf array by using string function
	for(i = 0; i < cnt; i++) {
		if(dbuf[i][0] != '.') {
			strcat(dir_buf1, "/");
			strcat(dir_buf2, "/");
			strcat(dir_buf1, dbuf[i]);
			strcat(dir_buf2, dbuf[i]);
			strcpy(dir_buffer1[i], dir_buf1);
			strcpy(dir_buffer2[i], dir_buf2);
			for(int j = strlen(dir_buf1); j > 0; j--) {
				if(dir_buf1[j] == '/') {
					dir_buf1[j] = '\0';
					break;
				}
				dir_buf1[j] = '\0';
			}
			for(int j = strlen(dir_buf2); j > 0; j--) {
				if(dir_buf2[j] == '/') {
					dir_buf2[j] = '\0';
					break;
				}
				dir_buf2[j] = '\0';
			}
		}
		else {
			strcpy(dir_buffer1[i], dbuf[i]);
			strcpy(dir_buffer2[i], dbuf[i]);
		}
	}

	// if the path name is directory, dir_count++ and recursively call this function
	for(i = 0; i < cnt; i++) {
		if(dir_buffer1[i][0] == '.') continue;
		if((dirp = opendir(dir_buffer1[i])) != NULL) { 
			strcpy(directory_name1[dir_count], dir_buffer1[i]);
			strcpy(directory_name2[dir_count], dir_buffer2[i]);
			dir_count++; // global variable
			directory_count_func(dir_buffer1[i], dir_buffer2[i]);
		}
	}
}

// void file_copy(char *originalpath, char *copy path)
// this function run in option 'd'
// copy file if it is not directory
void file_copy(char *fname1, char *fname2)
{
	struct dirent *dentry;
	DIR *dirp;

	char dbuf[100][200];
	char dir_buffer1[100][200];
	char dir_buffer2[100][200];
	char reg_buf[100][FILE_SIZE];
	char dir_buf1[200] = "\0";
	char dir_buf2[200] = "\0";
	int cnt = 0;
	int fsize;
	int i;
	int fd1, fd2;

	strcpy(dir_buf1, fname1);
	strcpy(dir_buf2, fname2);

	if((dirp = opendir(fname1)) == NULL || chdir(fname1) == -1) {
		fprintf(stderr, "opendir, chdir error for %s\n", dir_buf1);
		exit(1);
	}


	while((dentry = readdir(dirp)) != NULL) {
		if(dentry->d_ino == 0) continue;
		memcpy(dbuf[cnt], dentry->d_name, DIRECTORY_SIZE);
		cnt++;
	}

	for(i = 0; i < cnt; i++) {
		if(dbuf[i][0] != '.') {
			strcat(dir_buf1, "/");
			strcat(dir_buf2, "/");
			strcat(dir_buf1, dbuf[i]);
			strcat(dir_buf2, dbuf[i]);
			strcpy(dir_buffer1[i], dir_buf1);
			strcpy(dir_buffer2[i], dir_buf2);
			for(int j = strlen(dir_buf1); j > 0; j--) {
				if(dir_buf1[j] == '/') {
					dir_buf1[j] = '\0';
					break;
				}
				dir_buf1[j] = '\0';
			}
			for(int j = strlen(dir_buf2); j > 0; j--) {
				if(dir_buf2[j] == '/') {
					dir_buf2[j] = '\0';
					break;
				}
				dir_buf2[j] = '\0';
			}
		}
		else {
			strcpy(dir_buffer1[i], dbuf[i]);
			strcpy(dir_buffer2[i], dbuf[i]);
		}
	}

	for(i = 0; i < cnt; i++)
	{
		if(dbuf[i][0] == '.') continue;
		if((dirp = opendir(dir_buffer1[i])) == NULL) {
			if((fd1 = open(dir_buffer1[i], O_RDONLY)) < 0)
			{
				fprintf(stderr, "-d : open error for %s\n", dir_buffer1[i]);
				exit(1);
			}
			if((fsize = lseek(fd1, 0, SEEK_END)) < 0) {
				fprintf(stderr, "-d : lseek error\n");
				exit(1);
			}
			lseek(fd1, 0, SEEK_SET);

			if(read(fd1, reg_buf[i], fsize)<0)
			{
				fprintf(stderr, "-d : read error\n");
				exit(1);
			}

			if((fd2 = open(dir_buffer2[i], O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0)
			{
				fprintf(stderr, "-d : open error for %s\n", dir_buffer2[i]);
				exit(1);
			}
			write(fd1, reg_buf[i], fsize);

		}
	}
}
