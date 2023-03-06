#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include<fcntl.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <ftw.h>
#include <grp.h>
#include <pwd.h>

#define BLOCK_SIZE 512
#define BUFFER_SIZE 1024

#define MAX_PATH_LENGTH 256
char *file2create,*fileDirectory;

enum Header {
	NAME = 0, MODE = 100, UID = 108, GID = 116, SIZE = 124, MTIME = 136,
	CHK = 148, TYPE = 156, LINK = 157, VERS = 263, UNAME = 265,
	GNAME = 297, MAJ = 329, MIN = 337, END = 512
};

enum Type {
	REG = '0', HARDLINK = '1', SYMLINK = '2', CHARDEV = '3', BLOCKDEV = '4',
	DIRECTORY = '5', FIFO = '6'
};

void checkSum(const char b[END], char *chk) {
	unsigned sum = 0, i;
	for(i = 0; i<END; i++)
		sum += (i >= CHK && i < CHK+8) ? ' ' : b[i];
	snprintf(chk, 8, "%.7o", sum);
}

static int
pOct(const char *p, size_t n)
{
	int i = 0;

	while ((*p < '0' || *p > '7') && n > 0) {
		++p;
		--n;
	}
	while (*p >= '0' && *p <= '7' && n > 0) {
		i *= 8;
		i += *p - '0';
		++p;
		--n;
	}
	return (i);
}

static void
dirCreate(char *nameP, int mode)
{
	char *p;
	int r;

	if (nameP[strlen(nameP) - 1] == '/')
		nameP[strlen(nameP) - 1] = '\0';
#if defined(_WIN32) && !defined(__CYGWIN__)
	r = _mkdir(nameP);
#else
	r = mkdir(nameP, mode);
#endif

	if (r != 0) {
		/* On failure, try creating parent directory. */
		p = strrchr(nameP, '/');
		if (p != NULL) {
			*p = '\0';
			dirCreate(nameP, 0755);
			*p = '/';
#if defined(_WIN32) && !defined(__CYGWIN__)
			r = _mkdir(nameP);
#else
			r = mkdir(nameP, mode);
#endif
		}
	}
	if (r != 0)
		fprintf(stderr, "Could not create directory %s\n", nameP);
}

void removeTar(char *str) {
  int len = strlen(str);
  if (len >= 4 && strcmp(str + len - 4, ".tar") == 0) {
    str[len - 4] = '\0';
  }
}

static FILE *
createFile(char *nameP, int mode)
{
	FILE *f;
	char *temp;
	char dir[100]="Dump/";
	char slash[100]="/";
	char *file_name = strrchr(nameP, '/');

	char fileDirectory_name[100];
	removeTar(fileDirectory);
        if (file_name == NULL) {
            file_name = nameP;
        } else {
            file_name++;
        }
        char dump_dest[100];
        sprintf(dump_dest,"%s%s",fileDirectory,dir);

	sprintf(temp,"%s%s",dump_dest,file_name);

	nameP=temp;
	f = fopen(nameP, "wb+");
	if (f == NULL) {
		/* Try creating parent dir and then creating file. */
		char *p = strrchr(nameP, '/');
		if (p != NULL) {
			*p = '\0';
			dirCreate(nameP, 0755);
			*p = '/';
			f = fopen(nameP, "wb+");
		}
	}
	return (f);
}



static int
v_check(const char *p)
{
	int n, u = 0;
	for (n = 0; n < 512; ++n) {
		if (n < 148 || n > 155)
			/* Standard tar checksum adds unsigned bytes. */
			u += ((unsigned char *)p)[n];
		else
			u += 0x20;

	}
	return (u == pOct(p + 148, 8));
}

static void
non_tar(FILE *a, const char *path)
{
	char buff[512];
	FILE *f = NULL;
	size_t bytes_read;
	int filesize;

	for (;;) {
		bytes_read = fread(buff, 1, 512, a);
		if (bytes_read < 512) {
			return;
		}

		int n,k;
	        for (n = 511; n >= 0; --n){
		    if (buff[n] != '\0')
			k+=1;
		}
		if (k==0){
		    return;
		}

		if (!v_check(buff)) {
			fprintf(stderr, "Checksum failure\n");
			return;
		}
		filesize = pOct(buff + 124, 12);
		switch (buff[156]) {
		case '1':
			break;
		case '2':
			break;
		case '3':
				break;
		case '4':
			break;
		case '5':
			dirCreate(buff, pOct(buff + 100, 8));
			filesize = 0;
			break;
		case '6':
			break;
		default:

			f = createFile(buff, pOct(buff + 100, 8));
			break;
		}
		while (filesize > 0) {
			bytes_read = fread(buff, 1, 512, a);
			if (bytes_read < 512) {
				return;
			}
			if (filesize < 512)
				bytes_read = filesize;
			if (f != NULL) {
				if (fwrite(buff, 1, bytes_read, f)
				    != bytes_read)
				{
					fprintf(stderr, "Failed write\n");
					fclose(f);
					f = NULL;
				}
			}
			filesize -= bytes_read;
		}
		if (f != NULL) {
			fclose(f);
			f = NULL;
		}
	}
}

static void
listing(char *a, const char *path)
{
    int fd;
    char buffer[BLOCK_SIZE];
    int files_found = 0;



    if ((fd = open(a, O_RDONLY)) == -1) {
        printf("Error: unable to open tar archive %s\n", a);

    }

    struct stat st;
    stat(a, &st);
    int size_of_tar_file = st.st_size;

    char A[1000][1000];
    char B[1000][1000];
    while (1) {
        int bytes_read = read(fd, buffer, BLOCK_SIZE);
        if (bytes_read < BLOCK_SIZE) {
            break;
        }

        char filename[100];
        int filesize = 0;

        strncpy(filename, buffer, 100);

        sscanf(buffer + 124, "%12o", &filesize);

        if (filesize > 0) {
            char *file_name = strrchr(filename, '/');
            if (file_name == NULL) {
                file_name = filename;
            } else {
                file_name++;
            }
            strcpy(A[files_found],file_name);
            char temp[1000];
            sprintf(temp,"%d",filesize);
            strcpy(B[files_found],temp);
            files_found++;
        }

        lseek(fd, (filesize + BLOCK_SIZE - 1) / BLOCK_SIZE * BLOCK_SIZE, SEEK_CUR);
    }


    close(fd);
    FILE *fptr;

    fptr=fopen("tarStructure","w");
    fprintf(fptr,"%d", size_of_tar_file);
    fprintf(fptr,"\n%d", files_found);
    for(int i=0;i<files_found;i++){
    fprintf(fptr,"\n%s %s",A[i],B[i]);
    int k=0;
    }
    fclose(fptr);
    if (files_found == 0) {
        printf("No files found in tar archive %s\n", a);
    }


}


int fileC(const char* path, const struct stat* st, int type) {
	int l = END;
	char b[END] = { 0 };
	FILE *f = NULL;
	struct passwd *pw = getpwuid(st->st_uid);
	struct group *gr = getgrgid(st->st_gid);
	FILE *file;
	char dir[100]="Dump/";
	char slash[100]="/";
	char fileDirectory_name[100];
	removeTar(fileDirectory);

        char dump_dest[100];
        sprintf(dump_dest,"%s%s%s",fileDirectory,slash,file2create);
	file = fopen(file2create, "a+");

	memset(b+SIZE, '0', 12);
	strcpy(b+VERS, "00");
	snprintf(b+NAME, 100, "%s", path);
	snprintf(b+MODE, 8, "%.7o", (unsigned)st->st_mode&0777);
	snprintf(b+UID,  8, "%.7o", (unsigned)st->st_uid);
	snprintf(b+GID,  8, "%.7o", (unsigned)st->st_gid);
	snprintf(b+MTIME,12, "%.11o", (unsigned)st->st_mtime);
	snprintf(b+UNAME, 32, "%s", pw->pw_name);
	snprintf(b+GNAME, 32, "%s", gr->gr_name);
	mode_t mode = st->st_mode;
	if(S_ISREG(mode)) {
		b[TYPE] = REG;
		snprintf(b+SIZE, 12, "%.11o", (unsigned)st->st_size);
		f = fopen(path, "r");
	} else if(S_ISDIR(mode)) {
		b[TYPE] = DIRECTORY;
	} else if(S_ISLNK(mode)) {
		b[TYPE] = SYMLINK;
		readlink(path, b+LINK, 99);
	} else if(S_ISCHR(mode)) {
		b[TYPE] = CHARDEV;
		snprintf(b+MAJ,  8, "%.7o", (unsigned)major(st->st_dev));
		snprintf(b+MIN,  8, "%.7o", (unsigned)minor(st->st_dev));
	} else if(S_ISBLK(mode)) {
		b[TYPE] = BLOCKDEV;
		snprintf(b+MAJ,  8, "%.7o", (unsigned)major(st->st_dev));
		snprintf(b+MIN,  8, "%.7o", (unsigned)minor(st->st_dev));
	} else if(S_ISFIFO(mode)) {
		b[TYPE] = FIFO;
	}
	checkSum(b, b+CHK);
	if(!f) return 0;
	do {
		if(l<END)
			memset(b + l, 0, END - l);
		fwrite(b, END, 1, file);
	}
	while((l = fread(b, 1, END, f)) > 0);
	fclose(f);
	fclose(file);
	return 0;
}


int t(char *fname, int l, char b[END]){
	puts(fname);
	for(; l > 0; l -= END)
		fread(b, END, 1, stdin);
	return 0;
}


int c(char *p) {
	static struct stat st;
	if(lstat(p, &st)){
		return fileC(p, &st, 0);
	}else if(S_ISDIR(st.st_mode)){
	        int temp=ftw(p, fileC, 1024);
		return 0;
	}else
		return fileC(p, &st, 0);
	return 1;
}



int main(int argc, char *argv[]) {
	if (strcmp(argv[1], "-c") == 0)
	{
		file2create=argv[3];

		fileDirectory=argv[2];
		while(argc-- >= 3)
			if(c(argv[argc])) return EXIT_FAILURE;

	        FILE *source_file, *target_file;
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    source_file = fopen(file2create, "r");
    if (source_file == NULL) {
        printf("Unable to open source file\n");
        exit(EXIT_FAILURE);
    }
    char tem[100];
    char p2lash[100]="/";
    sprintf(tem,"%s%s%s",fileDirectory,p2lash,file2create);
    target_file = fopen(tem,"w");
    if (target_file == NULL) {
        printf("Unable to open target file\n");
        exit(EXIT_FAILURE);
    }
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, target_file) != bytes_read) {
            printf("Error writing to target file\n");
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(source_file)) {
        printf("Error reading from source file\n");
        exit(EXIT_FAILURE);
    }


    fclose(source_file);
    fclose(target_file);
    remove(file2create);
		return EXIT_SUCCESS;
	}
	else if (strcmp(argv[1], "-d") == 0)
	{
		FILE *a;
		fileDirectory=argv[2];
		a = fopen(argv[2], "rb");
		if (a == NULL)
			fprintf(stderr, "Failed to complete decompression operation");
		else {
			non_tar(a, *argv);
			fclose(a);
		}
	}
	else if (strcmp(argv[1], "-l") == 0)
	{

        FILE *a;
                fileDirectory=argv[2];
                char temp[100];
                char slash[100]="/";
		sprintf(temp,"%s%s",fileDirectory);
		a = fopen(argv[2], "rb");
		if (a == NULL)
			fprintf(stderr, "Failed to complete list operation");
		else {
			listing(argv[2], *argv);

			fclose(a);
			FILE *source_file, *target_file;
			file2create="tarStructure";
			fileDirectory=argv[2];
			removeTar(fileDirectory);
			char buffer[BUFFER_SIZE];
    size_t bytes_read;

    source_file = fopen(file2create, "r");
    if (source_file == NULL) {
        printf("Unable to open source file\n");
        exit(EXIT_FAILURE);
    }
    char tem[100];
    char p2lash[100]="/";
    char path[MAX_PATH_LENGTH];
    char *last_slash;

    strcpy(path, argv[2]);

    last_slash = strrchr(path, '/');
    if (last_slash == NULL) {
        printf("Invalid file path\n");
        exit(EXIT_FAILURE);
    }

    *last_slash = '\0';

    sprintf(tem,"%s%s%s",path,p2lash,file2create);
    target_file = fopen(tem,"w");
    if (target_file == NULL) {
        printf("Unable to open target file\n");
        exit(EXIT_FAILURE);
    }
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, source_file)) > 0) {
        if (fwrite(buffer, 1, bytes_read, target_file) != bytes_read) {
            printf("Error writing to target file\n");
            exit(EXIT_FAILURE);
        }
    }

    if (ferror(source_file)) {
        printf("Error reading from source file\n");
        exit(EXIT_FAILURE);
    }


    fclose(source_file);
    fclose(target_file);
    remove(file2create);
			
        }
    }
    else
    {
        printf("Incorrect Usage");
    }
	return EXIT_FAILURE;
}
