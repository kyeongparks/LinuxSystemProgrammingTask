#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <time.h>
#include <utime.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DIRECTORY_SIZE MAXNAMLEN
#define BUFFER_SIZE 2048

typedef struct data {
	char pathname[255];
} DATA;

DATA dat[100];

int ssu_daemon_init(int, char **);
int ssu_checkfile(char *, time_t *, int, int);
void file_count(char *);
void file_deletion(char *, int);
void directory_search(char *, int);
void *ssu_thread(void *arg);
void recovery_option(int, char **);
void compare_option(int, char **);

struct stat sstatbuf;
int flag_d = 0, flag_r = 0, flag_m = 0, flag_n = 0, flag_c = 0;
int flag_default = 0;
int d_file_count = 0;
int m_file_check = 0;
int directory_cnt = 0;
int d_index = 0;
int period = 5;

char directory_name[100][255];

int main(int argc, char *argv[])
{
	struct stat statbuf;
	struct tm *timeinfo;
	struct utimbuf tim_buf;
	char time_buf[100];
	char syslog_buf[200];
	char pid_buf[10];
	int opt;
	char **arv;
	int fd, fd1, fd2, fd3;
	char buf[100];
	pid_t pid;

	arv = (char **)malloc(sizeof(char *) * argc);
	for(int i = 0; i < argc; i++)
	{
		arv[i] = (char *)malloc(sizeof(char) * strlen(argv[i]));
		strcpy(arv[i], argv[i]);
	}

	while((opt = getopt(argc, argv, "drmn:c")) != -1) {
		switch(opt) {
			case 'd' :
				flag_d = 1;
				break;

			case 'r':
				flag_r = 1;
				break;

			case 'm':
				flag_m = 1;
				break;

			case 'n':
				flag_n = 1;
				break;

			case 'c':
				flag_c = 1;
				break;

			default:
				flag_default = 1;
				break;
		}
	}

	if(flag_d || flag_m || flag_n) {
		if(flag_r || flag_c) {
			fprintf(stderr, "usage : option -r/-c cannot use with other options<d, m, n>\n");
			exit(1);
		}
	}

	if(argc < 4 && !flag_c)
		flag_default = 1;

	if(flag_n) {
		flag_default = 1;
		if(flag_m) 
			flag_default = 0;
	}

	if(flag_r) {
		pid = getpid();
		if((fd1 = open("prepid.txt", O_RDONLY)) > 0) {
			read(fd1, buf, sizeof(buf));
			printf("Send signal to ssu_backup process<%s>\n", buf);
			close(fd1);
			if(strlen(buf) != 0) {
				kill(atoi(buf), SIGKILL);
				sprintf(syslog_buf, "%s%s<pid:%s> exit\n", time_buf, argv[0], buf);
				fd2 = open("backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
				write(fd2, syslog_buf, strlen(syslog_buf));
				close(fd2);
			}
		}
		recovery_option(argc, arv);

	}

	if(flag_c) {
		pid = getpid();
		if((fd1 = open("prepid.txt", O_RDONLY)) > 0) {
			read(fd1, buf, sizeof(buf));
			printf("Send signal to ssu_backup process<%s>\n", buf);
			close(fd1);
			if(strlen(buf) != 0) {
				kill(atoi(buf), SIGKILL);
				sprintf(syslog_buf, "%s%s<pid:%s> exit\n", time_buf, argv[0], buf);
				fd2 = open("backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
				write(fd2, syslog_buf, strlen(syslog_buf));
				close(fd2);
			}
		}
		compare_option(argc, arv);

	}


	time(&statbuf.st_atime);
	timeinfo = localtime(&statbuf.st_atime);
	strftime(time_buf, 100, "[%m%d %H:%M:%S] ", timeinfo);

	pid = getpid();
	printf("parent process : %d\n", pid);
	printf("daemon process initialization\n");

	if((fd1 = open("prepid.txt", O_RDONLY)) > 0) {
		read(fd1, buf, sizeof(buf));
		close(fd1);
		if(strlen(buf) != 0) {
			kill(atoi(buf), SIGKILL);
			sprintf(syslog_buf, "%s%s<pid:%s> exit\n", time_buf, argv[0], buf);
			fd2 = open("backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
			write(fd2, syslog_buf, strlen(syslog_buf));
			close(fd2);
		}
	}

	if(ssu_daemon_init(argc, arv) < 0) {
		fprintf(stderr, "ssu_daemon_init failed\n");
		exit(1);
	}

	for(int i = 0; i < argc; i++)
		free(arv[i]);
	free(arv);
	exit(0);
}

int ssu_daemon_init(int argc, char *argv[])
{
	FILE *fp;
	DIR *dirp;

	pthread_t tid[100];

	struct stat statbuf;
	struct tm *timeinfo;
	struct tm *timeinfo2;
	struct utimbuf tim_buf;
	struct utimbuf tim_buf2;
	char time_buf[100];
	char time_buf2[100];
	char syslog_buf[100];

	pid_t pid;
	int fd, fd1, fd2, maxfd, i, j;
	int size = 0;
	int check = 0;
	int d_count = 0;
	time_t intertime;
	char *s;
	char *cw_directory;
	char c;
	char prepid[10] = "\0";
	char pprepid[100] = "\0";
	char buf[1024] = "\0";
	char bbuf[255] = "\0";
	char cpbuf[1024] = "\0";
	char *d = NULL;
	char dir_buf[100] = "\0";
	char *fname = argv[1];
	char f_name[200] = "\0";
	char _syslog[200] = "\0";
	char *dname = argv[1];
	int max_file_count = 0;
	int status;

	openlog("lpd", LOG_PID, LOG_LPR);

	if(flag_n) {
		max_file_count = atoi(argv[4]);
	}

	if(flag_d) {
		if((dirp = opendir(dname)) == NULL) {
			syslog(LOG_ERR, "option -d : not valid directory name %s\n", dname);
			exit(1);
		}
		if(strchr(dname, '/') == NULL) {
			d = getcwd(NULL, 0);
			strcpy(dir_buf, d);
			strcat(dir_buf, "/");
			strcat(dir_buf, dname);
		}
		else {
			strcpy(dir_buf, dname);
		}
		closedir(dirp);
	}

	if((pid = fork()) < 0) {
		syslog(LOG_ERR, "fork error\n");
		exit(1);
	}

	else if(pid != 0)
		exit(0);

	pid = getpid();
	sprintf(prepid, "%d", pid);
	fd1 = open("prepid.txt", O_RDWR | O_CREAT | O_TRUNC , 0666);
	write(fd1, &prepid, strlen(prepid));
	close(fd1);

	if(argc < 2) {
		syslog(LOG_ERR, "usage: <%s> [file name] [period]\n", argv[0]);
		exit(1);
	}

	if(argv[2] != NULL)
		period = atoi(argv[2]);

	pid = getpid();
	printf("process %d running as daemon\n", pid);
	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();

	s = getcwd(NULL, 0);
	mkdir("backup", 0766);
	strcpy(buf, s);
	strcat(buf, "/");
	strcat(buf, fname);

	fd2 = open(buf, O_RDONLY) ;
	size = lseek(fd2, 0, SEEK_END);
	close(fd2);

	if(stat(fname, &sstatbuf) < 0) {
		syslog(LOG_ERR, "stat error for %s\n", fname);
		exit(1);
	}
	intertime = sstatbuf.st_mtime;


	for(i = 0, j = 0; i < strlen(buf); i++) {
		sprintf(&bbuf[j], "%x", buf[i]);
		j+=2;
	}

	for(fd = 0; fd < maxfd; fd++)
		close(fd);
	chdir("backup");
	cw_directory = getcwd(NULL, 0);
	umask(0);
	fd = open("/dev/null", O_RDWR);
	dup(0);
	dup(0);
	int cnt = 0;
	while(1) {

		if(flag_m) {

			check = ssu_checkfile(buf, &intertime, period, size);

			if(check == 1) {

				fd1 = open(buf, O_RDONLY);
				size = lseek(fd1, 0, SEEK_END);
				lseek(fd1, 0, SEEK_SET);
				read(fd1, cpbuf, size);
				close(fd1);


				time(&statbuf.st_atime);
				timeinfo = localtime(&statbuf.st_atime);
				strftime(time_buf, 100, "%m%d%H%M%S", timeinfo);
				strcat(bbuf, "_");
				strcat(bbuf, time_buf);

				for(i = 0; i < strlen(time_buf); i++)
					time_buf[i] = '\0';

				fd2 = open(bbuf, O_RDWR | O_CREAT | O_TRUNC, 0644);
				write(fd2, cpbuf, size);
				close(fd2);


				for(i = 0; i < size; i++)
					cpbuf[i] = '\0';

				for(i = strlen(bbuf); i >= 0; i--){
					if(bbuf[i] == '_') {
						bbuf[i] = '\0';
						break;
					}
					bbuf[i] = '\0';
				}
				if(!flag_n)
					sleep(period);
			}
		}

		if(flag_n) {
			d_file_count = 0;
			file_count(cw_directory);
			if(d_file_count > max_file_count) {
				file_deletion(cw_directory, max_file_count);
			}
			if(flag_m)
				sleep(period);

		}

		if(flag_d) {
			directory_cnt = 0;
			directory_search(dir_buf, 0);

			for(int j = 0; j < directory_cnt-1; j++) {
				strcpy(dat[j].pathname, directory_name[j]);
			}

			for(int j = 0; j < directory_cnt-1; j++) {

				if(pthread_create(&tid[j], NULL, ssu_thread, (void *)&dat[j]) != 0) {
					syslog(LOG_ERR, "pthread_create error\n");
					exit(1);
				}
			}
			sleep(period);
			for(int j = 0; j < directory_cnt - 1; j++) {
				memset(directory_name[j], '\0', strlen(directory_name[j]));
				memset(dat[j].pathname, '\0', strlen(dat[j].pathname));
			}
		}

		if(flag_default){

			check = ssu_checkfile(buf, &intertime, period, size);

			if((fd1 = open(buf, O_RDONLY)) < 0) {
				if(cnt == 0) {
					syslog(LOG_ERR, "file %s not exist\n", fname);
					exit(1);
				}
				fd2 = open("../backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
				time(&statbuf.st_atime);
				timeinfo = localtime(&statbuf.st_atime);
				strftime(time_buf, 100, "[%m%d %H:%M:%S] ", timeinfo);
				sprintf(_syslog, "%s%s is deleted.\n", time_buf, fname);
				write(fd2, _syslog, strlen(_syslog));
				exit(0);
			}

			if(read(fd1, cpbuf, 1024) < 0) {
				syslog(LOG_ERR, "read error\n");
				exit(1);
			}

			if((size = lseek(fd1, 0, SEEK_END)) < 0) {
				syslog(LOG_ERR, "lseek error\n");
				exit(1);
			}
			lseek(fd1, 0, SEEK_SET);
			close(fd1);


			if(strlen(bbuf) > 255) {
				syslog(LOG_ERR, "backup file name is too long : less than 255\n");
				exit(1);
			}
			time(&statbuf.st_atime);
			timeinfo = localtime(&statbuf.st_atime);
			strftime(time_buf, 100, "%m%d%H%M%S", timeinfo);
			strcat(bbuf, "_");
			strcat(bbuf, time_buf);

			for(i = 0; i < strlen(time_buf); i++)
				time_buf[i] = '\0';


			if((fd = open(bbuf, O_RDWR | O_CREAT | O_TRUNC, 0666)) <0) {
				syslog(LOG_ERR, "open error for %s\n", bbuf);
				exit(1);
			}
			if(write(fd, cpbuf, size) < 0) {
				syslog(LOG_ERR, "write error\n");
				exit(1);
			}

			fd2 = open("../backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
			time(&statbuf.st_mtime);
			timeinfo2 = localtime(&statbuf.st_mtime);
			strftime(syslog_buf, 100, "/mtime:%m%d %H:%M:%S] ", timeinfo2);
			strftime(time_buf, 100, "[%m%d %H:%M:%S] ", timeinfo);
			sprintf(_syslog, "%s%s [size:%d%s\n", time_buf, fname, size, syslog_buf);
			write(fd2, _syslog, strlen(_syslog));

			for(i = strlen(bbuf); i >= 0; i--){
				if(bbuf[i] == '_') {
					bbuf[i] = '\0';
					break;
				}
				bbuf[i] = '\0';
			}
			cnt++;
			sleep(period);
		}
	}
	return 0;
}

int ssu_checkfile(char *file_name, time_t *time_, int period, int size)
{
	struct stat statbuf;
	struct tm *timeinfo;
	struct tm *timeinfo2;
	struct utimbuf tim_buf;
	char time_buf[100] = "\0";
	char time_buf2[100] = "\0";
	char syslog_buf[200] = "\0";
	int fd, fd1;
	int check = 0;

	if((fd1 = open(file_name, O_RDONLY)) < 0) {
		syslog(LOG_ERR, "open error\n");
		exit(1);
	}
	else {
		size = lseek(fd1, 0, SEEK_END);
		close(fd1);
	}

	if(stat(file_name, &sstatbuf) < 0) {
		syslog(LOG_ERR, "Warning : ssu_checkfile() error\n");
		exit(1);
	}

	else
		if(sstatbuf.st_mtime != *time_) {
			time(&statbuf.st_mtime);
			timeinfo = localtime(&statbuf.st_mtime);
			strftime(time_buf, 100, "[%m%d %H:%M:%S] ", timeinfo);

			time(time_);
			timeinfo2 = localtime(time_);
			strftime(time_buf2, 100, "/mtime:%m%d %H:%M:%S]", timeinfo2);

			sprintf(syslog_buf, "%s%s is modified [size:%d%s\n", time_buf, file_name, size, time_buf2);
			fd = open("../backup_log.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
			write(fd, syslog_buf, strlen(syslog_buf));
			*time_ = sstatbuf.st_mtime;
			return check = 1;
		}
	return check = 0;
}

void file_count(char *dir_name)
{
	struct dirent *dentry;
	int count = 0;
	DIR *dirp;
	char c;
	int fd;

	char dbuf[100][100];

	if((dirp = opendir(dir_name)) == NULL) {
		syslog(LOG_ERR, "opendir error for%s\n", dir_name);
		exit(1);
	}


	while((dentry = readdir(dirp)) != NULL) {
		if(dentry->d_ino == 0) continue;
		if(dentry->d_name[0] == '.') continue;
		d_file_count++;
	}

	closedir(dirp);

}

void file_deletion(char *dir_name, int max_file_count) {

	struct dirent *dentry;
	DIR *dirp;

	struct dirent *dentry2;
	DIR *dirp2;

	int count = 0;
	int i;
	int fd;

	char dbuf[100][100];
	char temp[100] = "\0";

	if((dirp = opendir(dir_name)) == NULL) {
		syslog(LOG_ERR, "opendir error for %s\n", dir_name);
		exit(1);
	}


	while((dentry = readdir(dirp)) != NULL) {
		if(dentry -> d_ino == 0) continue;
		if(dentry -> d_name[0] == '.') continue;
		memcpy(dbuf[count], dentry->d_name, DIRECTORY_SIZE);
		count++;
	}

	closedir(dirp);


	// sorting file by time earliest -> latest
	for(i = 0; i < count-1; i++) {
		for(int j = i; j < count; j++) {
			if(strcmp(dbuf[i], dbuf[j]) > 0) {
				strcpy(temp, dbuf[i]);
				strcpy(dbuf[i], dbuf[j]);
				strcpy(dbuf[j], temp);
			}
		}
	}


	if((dirp2 = opendir(dir_name)) == NULL) {
		syslog(LOG_ERR, "opendir error for %s\n", dir_name);
		exit(1);
	}


	while((dentry2 = readdir(dirp2)) != NULL) {
		if(dentry2 -> d_ino == 0) continue;
		if(dentry2 -> d_name[0] == '.') continue;
		for(i = 0; i < count - max_file_count+1; i++) {
			if(strcmp(dentry2 -> d_name, dbuf[i]) == 0) 
				remove(dentry2 -> d_name);
		}
	}
	closedir(dirp2);
}

void directory_search(char *dname, int d_cnt)
{
	struct stat statbuf;
	struct tm *timeinfo;
	struct utimbuf tim_buf;
	struct dirent *dentry;
	DIR *dirp;

	DIR *dirp2;

	int fd;
	char file_buf[100] = "\0";
	char dir_buf[100][100];
	char time_buf[100] = "\0";
	char buf[255];
	char cpbuf[BUFFER_SIZE] = "\0";
	int cnt = 0;
	int size = 0;
	char c;
	int d_count = d_cnt;
	int k = 0;

	if((dirp = opendir(dname)) == NULL) {
		syslog(LOG_ERR, "opendir error \n");
		exit(1);
	}

	while((dentry = readdir(dirp)) != NULL) {
		if(dentry -> d_ino == 0) continue;
		if(dentry -> d_name[0] == '.') continue;
		memcpy(dir_buf[cnt], dentry->d_name, DIRECTORY_SIZE);
		strcpy(directory_name[directory_cnt], dname);
		strcat(directory_name[directory_cnt], "/");
		strcat(directory_name[directory_cnt], dir_buf[cnt]);
		if((dirp2 = opendir(directory_name[directory_cnt])) != NULL) {
			directory_search(directory_name[directory_cnt], d_cnt);
		}
		directory_cnt++;
		cnt++;	
	}

}

void *ssu_thread(void *arg) {
	DATA *_dat = (DATA *)arg;
	struct stat statbuf;
	struct tm *timeinfo;
	struct utimbuf tim_buf;
	struct dirent *dentry;
	DIR *dirp;

	pthread_t tid;
	pid_t pid;

	int fd2, fd1;
	int size = 0;
	int j = 0;

	char cpbuf[2048] = "\0";
	char buf[1024] = "\0";
	char time_buf[100] ="\0";
	char dname[255];

	strcpy(dname, _dat -> pathname);

	if((fd2 = open(dname, O_RDONLY)) < 0) {
		exit(1);
	}
	else {
		size = lseek(fd2, 0, SEEK_END);
		lseek(fd2, 0, SEEK_SET);
		read(fd2, cpbuf, size);
	}
	close(fd2);

	for(int i = 0; i < strlen(dname); i++) {
		sprintf(&buf[j], "%x", dname[i]);
		j+=2;
	}

	time(&statbuf.st_atime);
	timeinfo = localtime(&statbuf.st_atime);
	strftime(time_buf, 100, "%m%d%H%M%S", timeinfo);
	strcat(buf, "_");
	strcat(buf, time_buf);

	fd1 = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0644);
	write(fd1, (char *)&cpbuf, strlen(cpbuf));
	close(fd1);

}

void recovery_option(int argc, char *argv[])
{
	struct dirent *dentry;
	DIR *dirp;
	int fd, fd1, size;
	int i = 0;
	int k = 0;
	int cnt = 0;
	int len = 0;
	int opt;
	char buf[255];
	char file_ary[100][255];
	char *s = getcwd(NULL, 0);
	char cpbuf[2048];
	strcat(s, "/");
	strcat(s, argv[1]);

	for(int j = 0; j < strlen(s); j++) {
		sprintf(&buf[k], "%x", s[j]);
		k+=2;
	}

	if((fd = open(argv[1], O_RDONLY)) < 0)
	{
		fprintf(stderr, "[file : %s] open error\n", argv[1]);
		exit(1);
	}

	if((dirp = opendir("backup")) == NULL) {
		fprintf(stderr, "[directory : backup] directory not exist\n");
		exit(1);
	}

	printf("<%s backup list>\n", argv[1]);
	while((dentry = readdir(dirp)) != NULL) {
		if(dentry -> d_ino == 0) continue;
		if(dentry -> d_name[0] == '.') continue;
		for(i = 0; i < strlen(buf); i++) {
			if(buf[i] == dentry -> d_name[i]) {
				len++;
			}
		}
		if(len == strlen(buf)) {
			memcpy(file_ary[cnt], dentry->d_name, strlen(dentry->d_name));
			cnt++;
			len = 0;
		}
	}


	if(cnt == 0) {
		fprintf(stderr, "no backup file exist\n");
		exit(1);
	}

	chdir("backup");
	
	for(i = 0; i < cnt; i++) {
		fd1 = open(file_ary[i], O_RDONLY);
		size = lseek(fd1, 0, SEEK_END);
		lseek(fd1, 0, SEEK_SET);
		close(fd1);
		printf("%d. %s [size:%d]\n", i, file_ary[i], size);
	}
	printf("%d. exit\n", i);
	printf("input : ");
	scanf("%d", &opt);


	printf("Recovery backup file...\n");
	fd1 = open(file_ary[opt], O_RDONLY);
	size = lseek(fd1, 0, SEEK_END);
	lseek(fd1, 0, SEEK_SET);
	read(fd1, cpbuf, size);
	close(fd1);


	chdir("..");
	fd1 = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
	write(fd1, cpbuf, size);
	close(fd1);
	printf("[%s]\n", argv[1]);
	printf("%s\n", cpbuf);

	exit(0);

}

void compare_option(int argc, char *argv[]) 
{
	struct dirent *dentry;
	DIR *dirp;
	int fd, fd1, size, size1;
	int i = 0;
	int k = 0;
	int cnt = 0;
	int len = 0;
	int opt;
	char buf[255];
	char temp[255];
	char file_ary[100][255];
	char *s = getcwd(NULL, 0);
	char cpbuf[2048];
	char c_pbuf[500][200];
	char cpbuf2[2048];
	char c_pbuf2[500][200];
	strcat(s, "/");
	strcat(s, argv[1]);

	int lcount1 = 0, lcount2 = 0;

	for(int j = 0; j < strlen(s); j++) {
		sprintf(&buf[k], "%x", s[j]);
		k+=2;
	}


	if((dirp = opendir("backup")) == NULL) {
		fprintf(stderr, "open directory error\n");
		exit(1);
	}

	while((dentry = readdir(dirp)) != NULL) {
		if(dentry -> d_ino == 0) continue;
		if(dentry -> d_name[0] == '.') continue;
		for(i = 0; i < strlen(buf); i++) {
			if(buf[i] == dentry -> d_name[i]) {
				len++;
			}
		}
		if(len == strlen(buf)) {
			memcpy(file_ary[cnt], dentry->d_name, strlen(dentry->d_name));
			cnt++;
			len = 0;
		}
	}

	for(i = 0; i < cnt-1; i++) {
		for(int j = i+1; j < cnt; j++) {
			if(strcmp(file_ary[i], file_ary[j]) > 0) {
				strcpy(temp, file_ary[i]);
				strcpy(file_ary[i], file_ary[j]);
				strcpy(file_ary[j], temp);
			}
		}
	}


	printf("<Compare with backup file[%s", argv[1]);
	
	for(int i = 0; i < strlen(file_ary[cnt-1]); i++) {
		if(file_ary[cnt-1][i] == '_') {
			printf("%s", &(file_ary[cnt-1][i]));
			break;
		}
	}
	printf("]>\n");

	fd1 = open(argv[1], O_RDONLY);
	size = lseek(fd1, 0, SEEK_END);
	lseek(fd1, 0, SEEK_SET);
	read(fd1, cpbuf, size);
	close(fd1);

	for(i = 0; i < strlen(cpbuf); i++) {
		if(cpbuf[i] == '\n')
			lcount1++;
	}
	
	int jj = 0, kk = 0;
	for(i = 0; i < strlen(cpbuf); i++) {
		if(kk == 0) 
			if(cpbuf[i] == '\n') c_pbuf[jj][kk] = '\0';
		if(cpbuf[i] == '\n') {
			jj++;
			kk = 0;
		}
		c_pbuf[jj][kk] = cpbuf[i];
		kk++;
	}




	chdir("backup");
	fd1 = open(file_ary[cnt-1], O_RDONLY);
	size1 = lseek(fd1, 0, SEEK_END);
	lseek(fd1, 0, SEEK_SET);
	read(fd1, cpbuf2, size1);
	close(fd1);
	
	for(i = 0; i < strlen(cpbuf2); i++) {
		if(cpbuf2[i] == '\n')
			lcount2++;
	}
	
	jj = 0, kk = 0;
	for(i = 0; i < strlen(cpbuf); i++) {
		if(cpbuf[i] == '\n') {
			jj++;
			kk = 0;
		}
		c_pbuf2[jj][kk] = cpbuf2[i];
		kk++;
	}

	for(i = 0; i < lcount2; i++)
		if((strcmp(c_pbuf[i], c_pbuf2[i])) != 0) {
			printf("%dc%d\n", i+1, i+1);
			printf("<%s\n", c_pbuf[i]);
			printf("----\n");
			printf(">%s\n", c_pbuf2[i]);
		}

	exit(0);
}













