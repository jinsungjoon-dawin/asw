//  gcc -m64 -Wall -W -o asw_hsm asw_hsm.c
//
// ./asw_hsm asw_hsm_20241129_clean.aml /log/hsm/app_20241129_clean /app /test /test
// ./asw_hsm asw_hsm_20250124.aml CRC FILEINFO
// ./asw_hsm SRC_INFO_FILENAME DST_INFO_FILENAME FILECMP 
//
// find  . -type f  -name "*.c" -exec chmod 644 {} \;
// find  . -type f  -name "*.pc" -exec chmod 644 {} \;
// find  . -type f  -name "*.sc" -exec chmod 644 {} \;
// find  . -type f  -name "*.h" -exec chmod 644 {} \;
// ./asw_hsm asw_hsm_20250305.aml  /log/app/app_20250305 /log/app/app_20250305 /log/app/app_20250305_20250220 /log/app/app_20250305_20250220
// ./asw_hsm asw_hsm_20250310.aml  /log/app/app_20250310 /log/app/app_20250310 /log/app/app_20250310_20250305 /app
// ./asw_hsm asw_hsm_20250407.aml  /log/app/app_20250407 /log/app/app_20250403 /log/app/app_20250407_20250403 /app

/* .comp file
 DIFF
 NEW
 ERR
 AERR
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <libgen.h>
#include <errno.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#define  POLINOMIAL       0x04C11DB7

#define  SUCC             0
#define  FAIL             1
#define  FALSE            0
#define  TRUE             1

#define  ERR              -1
#define  ERR_SYS          -2
#define  ERR_NULL         -3
#define  ERR_OVER         -4

#define  PATH_LEN         3072
#define  FILE_LEN         1024
#define  PATH_FILE_LEN    4096

#define  DATE_LEN         32
#define  NUM_LEN          20
#define  SYS_NUN_LEN      64
#define  SYS_NAME_LEN     128

#define  FIND_LIST_FILE   "find.list"

#define DEBUG_PRINT(log_level, fmt, args...) fprintf(stderr, "DEBUG:%s:%d:%s:" fmt, __FILE__, __LINE__, __func__, ##args)

typedef struct PTR_ST {    /* PTR structure */
    size_t     len;
    char       *ptr;
    size_t     slen;
    char       *sptr;
} PTR_ST;

typedef struct VAR_ST {    /* VAR structure */
    size_t     size;
    size_t     len;
    char       *var;
} VAR_ST;

typedef struct MVAR_ST {   /* MVAR structure */
    int        cnt;
    VAR_ST     **mvar;
} MVAR_ST;

/***************** SYSTEM ***********************/

/* local system info struct */
typedef struct SYS_INFO_ST {
    char       hostname [SYS_NAME_LEN];
    MVAR_ST    ipname;
    MVAR_ST    ipaddr;
    char       gid      [SYS_NUN_LEN];
    char       gname    [SYS_NAME_LEN];
    char       uid      [SYS_NUN_LEN];
    char       uname    [SYS_NAME_LEN];
} SYS_INFO_ST;

/* program execute information */
typedef struct PROG_INFO_ST {
    char       cwd      [PATH_LEN];
    char       exec_nm  [PATH_FILE_LEN];
    char       path_nm  [PATH_LEN];
    char       file_nm  [FILE_LEN];
    MVAR_ST    arg_nm;
    char       start_date[DATE_LEN];
    char       end_date [DATE_LEN];
    char       func_nm  [FILE_LEN];
    char       message  [512];
    /* aml file info get */
    char       os_ver[128];
    MVAR_ST    log_level;
} PROG_INFO_ST;

/* aml FIND info */
typedef struct AML_FIND_ST {  /* FIND INFO structure */
    /** report body name */
    char       datetime[20];
    char       report_file_body[FILE_LEN];
    MVAR_ST    report_file_opts;
    /** find info file **/
    char       find_list_file[PATH_FILE_LEN];
    char       find_list_except_file[PATH_FILE_LEN];

    /** find condition(1) **/
    MVAR_ST    home;
    MVAR_ST    opts;
    MVAR_ST    name;
    MVAR_ST    pipe;

    /** find condition(2) **/
    MVAR_ST    full_find;

    /* manual find list file(3) */
    MVAR_ST    list_file;

    /** except condition **/
    char       fdate[DATE_LEN];
    char       tdate[DATE_LEN];

    /* include file */
    MVAR_ST    file_inc;
    /* exclude file */
    MVAR_ST    file_ext;
    /* except dir */
    MVAR_ST    dir_a;
    MVAR_ST    dir_l;
    MVAR_ST    dir_r;
    /* except file */
    MVAR_ST    file_a;
    MVAR_ST    file_l;
    MVAR_ST    file_r;
    /* dependency FILE ext */
    MVAR_ST    depend_s;
    MVAR_ST    depend_d;
} AML_FIND_ST;

/***************** FILE **************************/
typedef struct FILE_INFO_ST {
    char       type     [10];
    char       name     [PATH_FILE_LEN];
    char       dirnm    [PATH_LEN];
    char       filenm   [FILE_LEN];
    char       extnm    [FILE_LEN];
    char       ctime    [DATE_LEN];
    char       atime    [DATE_LEN];
    char       mtime    [DATE_LEN];
    char       size     [NUM_LEN];
    char       gid      [NUM_LEN];
    char       gname    [NUM_LEN];
    char       uid      [NUM_LEN];
    char       uname    [NUM_LEN];
    char       modes    [NUM_LEN];
    char       crc      [NUM_LEN];
} FILE_INFO_ST;

PROG_INFO_ST  prog_info;
SYS_INFO_ST   sys_info;

AML_FIND_ST   aml_find_info;
AML_FIND_ST   aml_find_info_dst;

FILE_INFO_ST  src_file_info;
FILE_INFO_ST  dst_file_info;

int AswGet_AmlInfo(char *aml_file, void *aml_st, char *aml_id);
int AswGet_AmlFindProc(AML_FIND_ST *find);
int AswGet_AmlFindParse(VAR_ST *aml_main, AML_FIND_ST *find);
int AswGet_AmlProgParse(VAR_ST *aml_main, PROG_INFO_ST *prog);
int AswGet_AmlMainMarkup(VAR_ST *aml_var, VAR_ST *aml_main, char *aml_main_id);
int AswGet_AmlSubMarkup(VAR_ST *aml_main, VAR_ST *aml_sub, char *aml_sub_id);
int AswGet_AmlSubDivide(VAR_ST *aml_sub, MVAR_ST *aml_div, char *aml_div_id);

int AswGet_ProgInfo(int argc, char *argv[], PROG_INFO_ST *pinfo);
int AswPrt_ProgInfo(PROG_INFO_ST *pinfo);
int AswGet_SysInfo(SYS_INFO_ST *sinfo);
int AswPrt_SysInfo(SYS_INFO_ST *sinfo);

int Str2MVAR(char *src, MVAR_ST *dst);
int Strn2MVAR(char *src, size_t src_len, MVAR_ST *dst);
int Str2Strs3PathFileSep(char *filename, char *path, char *file);
int Strn2Str3RcutSize(char *src, size_t src_len, char *dst, size_t cut_size);
int Strn2Str3RcutStr(char *src, size_t src_len, char *dst, char *cut_str, int cut_flag);
int Strn2Str3Crc(unsigned char *data, size_t length, char *dst);
int File2VAR(char *filename, VAR_ST *dst);
int var2free(void *src);
int VAR2free(VAR_ST *src);
int MVAR2free(MVAR_ST *src);

int Str3sysdate        (char *date_buf, int flag);
int Strn3Replace       (char *src, size_t src_len, char *dst, size_t dst_len, char *conv, size_t conv_len);

int isFileAccess(char *filename, int flag);
int isFileTimeChk(char *filename, char *fdate, char *tdate, char time_flag);
int isStrRComp(char *src, char *dst, size_t dst_len);
int isFilesCompare(char *src_filename, char *dst_filename);

int AswWrt_FileCompList(char *list_filename, char *src, char *dst, char *cpy, char *clean_dst, char *workdate);
int AswWrt_InfoFileCompList(char *src_infofilename, char *dst_infofilename, char *workdate);
int AswGet_FileInfo(char *filename, FILE_INFO_ST *fcinfo, int crc_flag);
int AswPrt_FileInfo(FILE_INFO_ST *fcinfo);
int AswWrt_FileInfoList(char *list_filename, int form_flag);

/********************/
/* MAIN    FUNCTION */
/********************/
int main(int argc, char *argv[])
{
    void  *st_ptr;
    //void  *st_dst;
    char  cmd_buf[40240] = {0};
    char  list_file_name[128];

   /* if(argc < 4 || argc >= 6) {
        DEBUG_PRINT(0, "Usage : %s [src aml file] 'CRC|NOCRC' 'AML'\n", argv[0]);
        DEBUG_PRINT(0, "Usage : %s [src aml file] 'CRC|NOCRC' 'FIND' [find_list_file_name]\n", argv[0]);
        return(ERR_NULL);
    }*/
    if(strcmp(argv[3] , "AML") == 0 && argc != 4 ){
        DEBUG_PRINT(0, "Usage : %s [src aml file] 'CRC|NOCRC' 'AML'\n", argv[0]);
	    return(ERR_NULL);
    }
    if(strcmp(argv[3] , "FIND") == 0 && (argc < 4 || argc > 5) ){
        DEBUG_PRINT(0, "Usage : %s [src aml file] 'CRC|NOCRC' 'FIND' [find_list_file_name]\n", argv[0]);
	    return(ERR_NULL);
    }
    memset(&prog_info, 0x00, sizeof(PROG_INFO_ST));
    AswGet_ProgInfo(argc, argv, &prog_info);
    //AswPrt_ProgInfo(&prog_info);

    memset(&sys_info, 0x00, sizeof(SYS_INFO_ST)); 
    AswGet_SysInfo(&sys_info);
    //AswPrt_SysInfo(&sys_info);
    
    /* src aml file FIND */
    /* log file datetime set */
    if(strlen(prog_info.start_date) != 0) {
        strcpy(aml_find_info.datetime, prog_info.start_date);
    }
    else {
        Str3sysdate(aml_find_info.datetime, 1);
    }

	memset(list_file_name, 0x00, sizeof(list_file_name));
    if(strcmp(argv[3] , "AML") == 0 ) {
        st_ptr = (void *)&aml_find_info;
        AswGet_AmlInfo(argv[1], st_ptr, "FIND");
        AswGet_AmlFindProc(st_ptr); // find -> src_app_20240802_20240804091000.list (AML <REPORT_*>reportfile_info</REPORT_*>.list)
        strcpy(list_file_name, aml_find_info.find_list_file);
	}
	else if(strcmp(argv[3] , "FIND") == 0) {
        if(argc == 5) {
            strcpy(list_file_name, argv[4]);
		    snprintf(cmd_buf , sizeof(cmd_buf), "find %s > %s", argv[1], argv[4]);
		}
	    else {
            strcpy(list_file_name, FIND_LIST_FILE);
		    snprintf(cmd_buf , sizeof(cmd_buf), "find %s > %s", argv[1], FIND_LIST_FILE);
		}
        //printf("%s\n", cmd_buf);
        system(cmd_buf);
    }
    /* dst aml file FIND */
    /* log file datetime set */
    if(strlen(prog_info.start_date) != 0) {
        strcpy(aml_find_info_dst.datetime, prog_info.start_date);
    }
    else {
        Str3sysdate(aml_find_info_dst.datetime, 1);
    }

    if(strcasecmp(argv[2], "NOCRC") == 0) {
        AswWrt_FileInfoList(list_file_name, 1);      // src.list -> src.info ( FILE_INFO_ST )
    }
    else {
        AswWrt_FileInfoList(list_file_name, 0);      // src.list -> src.info ( FILE_INFO_ST )
    }

    Str3sysdate(prog_info.end_date, 1);
    // AswPrt_ProgInfo(&prog_info);

    return(SUCC);
}

int AswWrt_FileInfoList(char *list_filename, int form_flag)
{
    FILE         *rfp;
    FILE         *wfp;
    FILE_INFO_ST fcinfo;
    char         rptfile[256];
    char         read_buf[4096];

    if(list_filename == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    /* app_20241126_20241126182723.list */
    if ((rfp = fopen(list_filename, "rb")) == NULL ) {
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", list_filename, strerror(errno));
        return(ERR_SYS);
    }
    sprintf(rptfile, "%s.info", list_filename);
    if ((wfp = fopen(rptfile, "wb")) == NULL ) {
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", rptfile, strerror(errno));
        return(ERR_SYS);
    }
    memset(read_buf, 0x00, sizeof(read_buf));
    while(fgets(read_buf, sizeof(read_buf)-1, rfp) != NULL) {
        read_buf[strlen(read_buf)-1] = 0x00;
        //Strn1StrnConv(read_buf, strlen(read_buf), "\t", 1, " ", 1);
        memset(&fcinfo, 0x00, sizeof(fcinfo));
        if(form_flag == 0) { // crc make
            AswGet_FileInfo(read_buf, &fcinfo, 0);
            fprintf(wfp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", fcinfo.name, fcinfo.type, fcinfo.mtime, fcinfo.size, fcinfo.gname
                                                                   , fcinfo.gid, fcinfo.uname, fcinfo.uid, fcinfo.modes, fcinfo.crc);
            printf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", fcinfo.name, fcinfo.type, fcinfo.mtime, fcinfo.size, fcinfo.gname
                                                                   , fcinfo.gid, fcinfo.uname, fcinfo.uid, fcinfo.modes, fcinfo.crc);
        }
        else {
            AswGet_FileInfo(read_buf, &fcinfo, 1);
            fprintf(wfp, "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", fcinfo.name, fcinfo.type, fcinfo.mtime, fcinfo.size, fcinfo.gname
                                                                  , fcinfo.gid, fcinfo.uname, fcinfo.uid, fcinfo.modes, fcinfo.crc);
            printf("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n", fcinfo.name, fcinfo.type, fcinfo.mtime, fcinfo.size, fcinfo.gname
                                                                  , fcinfo.gid, fcinfo.uname, fcinfo.uid, fcinfo.modes, fcinfo.crc);
		}
        fflush(wfp);
        memset(read_buf, 0x00, sizeof(read_buf));
    }
    fclose(wfp);
    fclose(rfp);

    return(SUCC);
}

int AswGet_FileInfo(char *filename, FILE_INFO_ST *fcinfo, int crc_flag)
{
    char          temp_data[128];
    struct stat   statbuf;
    struct group  *group_entry;
    struct passwd *pwd;
    VAR_ST        crc;

    if(filename == NULL || fcinfo == NULL ) {
        if(filename == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(fcinfo == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    memcpy(fcinfo->name, filename, strlen(filename));
    memset(&statbuf, 0x00, sizeof(statbuf));
    if(stat(filename, &statbuf) != 0) {
        strcpy(fcinfo->type ,"E");
        return(ERR_SYS);
    }

    if(S_ISREG(statbuf.st_mode))  {
        strcpy(fcinfo->type ,"F");
        Str2Strs3PathFileSep(filename, fcinfo->dirnm, fcinfo->filenm);
    }
    else if(S_ISDIR(statbuf.st_mode))  {
        strcpy(fcinfo->type ,"D");
        strcpy(fcinfo->dirnm, filename);
    }
    else if(S_ISFIFO(statbuf.st_mode))  {
        strcpy(fcinfo->type ,"O");
        Str2Strs3PathFileSep(filename, fcinfo->dirnm, fcinfo->filenm);
    }
    else if(S_ISSOCK(statbuf.st_mode))  {
        strcpy(fcinfo->type ,"S");
        Str2Strs3PathFileSep(filename, fcinfo->dirnm, fcinfo->filenm);
    }
    else if(S_ISLNK(statbuf.st_mode))  {
        strcpy(fcinfo->type ,"L");
        Str2Strs3PathFileSep(filename, fcinfo->dirnm, fcinfo->filenm);
    }
    else if(S_ISBLK(statbuf.st_mode))  {
        strcpy(fcinfo->type ,"B");
        Str2Strs3PathFileSep(filename, fcinfo->dirnm, fcinfo->filenm);
    }
    else {
        strcpy(fcinfo->type ,"U");
        Str2Strs3PathFileSep(filename, fcinfo->dirnm, fcinfo->filenm);
        //return(ERR_SYS);
    }
    Strn2Str3RcutStr(fcinfo->filenm, strlen(fcinfo->filenm), fcinfo->extnm, ".", 0);

    memset(temp_data, 0x00, sizeof(temp_data));
    strftime(temp_data, 16, "%Y%m%d%H%M%S", localtime(&statbuf.st_ctime));
    memcpy(fcinfo->ctime, temp_data, strlen(temp_data));

    memset(temp_data, 0x00, sizeof(temp_data));
    strftime(temp_data, 16, "%Y%m%d%H%M%S", localtime(&statbuf.st_atime));
    memcpy(fcinfo->atime, temp_data, strlen(temp_data));

    memset(temp_data, 0x00, sizeof(temp_data));
    strftime(temp_data, 16, "%Y%m%d%H%M%S", localtime(&statbuf.st_mtime));
    memcpy(fcinfo->mtime, temp_data, strlen(temp_data));

    sprintf(fcinfo->size , "%lu", statbuf.st_size);

    sprintf(fcinfo->gid  , "%u", statbuf.st_gid);
    if((group_entry = getgrgid(statbuf.st_gid)) == NULL) {
        strcpy(fcinfo->gname, "NONAMEGID");
    }
    else {
        strcpy(fcinfo->gname, group_entry->gr_name);
    }
    sprintf(fcinfo->uid  , "%u", statbuf.st_uid);
    if((pwd = getpwuid(statbuf.st_uid)) == NULL) {
        strcpy(fcinfo->uname, "NONAMEUID");
    }
    else {
        strcpy(fcinfo->uname, pwd->pw_name);
    }
    memset(temp_data, 0x00, sizeof(temp_data));
    sprintf(temp_data, "%o", (unsigned int)statbuf.st_mode);
    Strn2Str3RcutSize(temp_data, strlen(temp_data), fcinfo->modes, 3);

    if(crc_flag == 0 && strcmp(fcinfo->type, "F") == 0) {
        memset(&crc, 0x00, sizeof(VAR_ST));
        File2VAR(filename, &crc);
        Strn2Str3Crc((unsigned char *)crc.var, crc.size-1, fcinfo->crc);
    }
    else {
        strcpy(fcinfo->crc, "0");
    }
    return(SUCC);
}

int AswPrt_FileInfo(FILE_INFO_ST *fcinfo)
{
    if(fcinfo == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    printf("===== FileInfo ====\n");
    printf("TYPE     = [%s]\n", fcinfo->type);
    printf("name     = [%s]\n", fcinfo->name);
    printf("dirnm    = [%s]\n", fcinfo->dirnm);
    printf("filenm   = [%s]\n", fcinfo->filenm);
    printf("extnm    = [%s]\n", fcinfo->extnm);
    printf("ctime    = [%s]\n", fcinfo->ctime);
    printf("atime    = [%s]\n", fcinfo->atime);
    printf("mtime    = [%s]\n", fcinfo->mtime);
    printf("size     = [%s]\n", fcinfo->size);
    printf("gid      = [%s]\n", fcinfo->gid);
    printf("gname    = [%s]\n", fcinfo->gname);
    printf("uid      = [%s]\n", fcinfo->uid);
    printf("uname    = [%s]\n", fcinfo->uname);
    printf("modes    = [%s]\n", fcinfo->modes);
    printf("crc      = [%s]\n", fcinfo->crc);

    return(SUCC);
}

int AswGet_ProgInfo(int argc, char *argv[], PROG_INFO_ST *pinfo)
{
    int  i;

    if(pinfo == NULL) {
        DEBUG_PRINT(1, "argv[3] = NULL \n");
        return(ERR_NULL);
    }
    if(getcwd(pinfo->cwd, sizeof(pinfo->cwd)) == NULL) {
        DEBUG_PRINT(1, "getcwd() : [%s]\n", strerror(errno));
        return(ERR_SYS);
    }
    strcpy(pinfo->exec_nm, argv[0]);
    for(i=1; i<argc; i++) {
        Str2MVAR(argv[i], &pinfo->arg_nm);
    }
    Str3sysdate(pinfo->start_date, 1);
    strcpy(pinfo->func_nm, __func__);
    strcpy(pinfo->message, "Program start");
    /* asw_aml.c : aml file PROG info : OS_VER LOG_LEVEL */
    if(isStrRComp(argv[1], ".aml", 4) == TRUE) {
        if(AswGet_AmlInfo(argv[1], pinfo, "PROG") != SUCC) {
            return(FAIL);
        }
    }
    return(SUCC);
}

int AswPrt_ProgInfo(PROG_INFO_ST *pinfo)
{
    if(pinfo == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    printf("===== ProgInfo ====\n");
    printf("curr_path  = [%s]\n", pinfo->cwd);
    printf("prog_name  = [%s]\n", pinfo->exec_nm);
    for(int i=0; i < pinfo->arg_nm.cnt; i++) {
        printf("argv[%d]    = [%s]\n", i+1, pinfo->arg_nm.mvar[i]->var);
    }
    printf("start_date = [%s]\n", pinfo->start_date);
    printf("end_date   = [%s]\n", pinfo->end_date);
    printf("func_nm    = [%s]\n", pinfo->func_nm);
    printf("mesage     = [%s]\n", pinfo->message);
    printf("os_version = [%s]\n", pinfo->os_ver);                           //aml prog info
    for(int i=0; i < pinfo->log_level.cnt; i++) {
        printf("log_lev[%d] = [%s]\n", i+1, pinfo->log_level.mvar[i]->var); //aml prog info
    }
    return(SUCC);
}

int AswGet_SysInfo(SYS_INFO_ST *sinfo)
{
    char           *addr;
    struct ifaddrs *addrs, *tmp;
    struct sockaddr_in *sa;
    //struct hostent *host_entry;
    struct group   *group_entry;
    struct passwd  *pwd;
    gid_t  gid;
    uid_t  uid;

    /* argument null check */
    if(sinfo == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    /****************/
    /* get hostname */
    /****************/
    if(gethostname(sinfo->hostname, SYS_NAME_LEN-1) != 0) {
        DEBUG_PRINT(1, "gethostname : [%s]\n", strerror(errno));
        return(ERR_SYS);
    }
    /******************/
    /* get IP address */
    /******************/
    getifaddrs(&addrs);
    tmp = addrs;
    while(tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET){
            sa = (struct sockaddr_in*)tmp->ifa_addr;
            addr = inet_ntoa(sa->sin_addr);

            if(strcmp(tmp->ifa_name, "lo")) {
                Strn2MVAR(tmp->ifa_name, strlen(addr), &sinfo->ipname);
                Strn2MVAR(addr, strlen(addr), &sinfo->ipaddr);
            }
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    /**********************/
    /* get gid : group id */
    /**********************/
    gid = getgid();
    sprintf(sinfo->gid, "%u", gid);
    if((group_entry = getgrgid(gid)) == NULL) {
        strcpy(sinfo->gname, "NONAMEGID");
    }
    else {
        strcpy(sinfo->gname, group_entry->gr_name);
    }
    /*********************/
    /* get uid : user id */
    /*********************/
    uid = getuid();
    sprintf(sinfo->uid, "%u", uid);
    if((pwd = getpwuid(uid)) == NULL) {
        if(errno == 0 || errno == ENOENT || errno == ESRCH || errno == EBADF || errno == EPERM) {
            strcpy(sinfo->uname, "NONAMEUID");
        }
        else {
            strcpy(sinfo->uname, "ERRORUID");
        }
    }
    else {
        strcpy(sinfo->uname, pwd->pw_name);
    }

    return(SUCC);
}

int AswPrt_SysInfo(SYS_INFO_ST *sinfo)
{
    if(sinfo == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    printf("===== SysInfo ====\n");
    printf("HOSTNM = [%s]\n", sinfo->hostname);
    for(int i=0; i < sinfo->ipaddr.cnt; i++) {
        printf("IFCONFIG[%d] = [%s:%s]\n", i+1, sinfo->ipname.mvar[i]->var, sinfo->ipaddr.mvar[i]->var);
    }
    printf("GROUP  = [%s:%s]\n", sinfo->gname, sinfo->gid);
    printf("USER   = [%s:%s]\n", sinfo->uname, sinfo->uid);

    return(SUCC);
}

/*********************************************************/

int Str9Command(char *command)
{
    int ret;

    if(command == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    ret=system(command);
    usleep(10);

    return(ret);
}

int Str3sysdate(char *date_buf, int flag)
{
    struct timeval tp;
    struct tm calc_date;

    if(date_buf == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    gettimeofday(&tp, NULL);
    calc_date = *localtime(&tp.tv_sec);
    if(flag == 0){
        sprintf(date_buf, "%04d%02d%02d%02d%02d%02d%06lu",
                       calc_date.tm_year+1900, calc_date.tm_mon+1, calc_date.tm_mday,
                       calc_date.tm_hour, calc_date.tm_min, calc_date.tm_sec,
                       tp.tv_usec);
    }
    else if(flag == 1){ //yyyymmddHHMMSS
        sprintf(date_buf, "%04d%02d%02d%02d%02d%02d",
                       calc_date.tm_year+1900, calc_date.tm_mon+1, calc_date.tm_mday,
                       calc_date.tm_hour, calc_date.tm_min, calc_date.tm_sec);
    }
    else{ // yyyymmdd
        sprintf(date_buf, "%04d%02d%02d",
                       calc_date.tm_year+1900, calc_date.tm_mon+1, calc_date.tm_mday);
    }
    return(SUCC);
}

int Strn2VAR(char *src, size_t src_len, VAR_ST *dst)
{
    size_t  read_len;
    size_t  size_len;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    read_len = src_len;
    size_len = read_len + 1;
    if( (dst->var = (char *)malloc(size_len)) == NULL) {
        DEBUG_PRINT(1, "malloc(%lu) : [%s]\n", size_len, strerror(errno));
        return(ERR_SYS);
    }
    memset(dst->var, 0x00, size_len);
    memcpy(dst->var, src,  read_len);
    dst->len  = read_len;
    dst->size = size_len;
    return(SUCC);
}

int Str2MVAR(char *src, MVAR_ST *dst)
{
    long    cnt=0;
    size_t  src_len;
    size_t  size_len;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    cnt = dst->cnt;

    if(cnt == 0) {
        if( (dst->mvar = malloc( sizeof(VAR_ST **) * (cnt+1))) == NULL ) {
            DEBUG_PRINT(1, "malloc(%lu) : [%s]\n", sizeof(VAR_ST **) * (cnt+1), strerror(errno));
            return(ERR_SYS);
        }
    }
    else {
        if( (dst->mvar = realloc(dst->mvar, sizeof(VAR_ST **) * (cnt+1))) == NULL ) {
            DEBUG_PRINT(1, "realloc(%lu) : [%s]\n", sizeof(VAR_ST **) * (cnt+1), strerror(errno));
            return(ERR_SYS);
        }
    }
    if( (dst->mvar[cnt] = (VAR_ST *)malloc(sizeof(VAR_ST))) == NULL ) {
        DEBUG_PRINT(1, "malloc(%lu) : [%s]\n", sizeof(VAR_ST), strerror(errno));
        if(cnt == 0) {
            free(dst->mvar);
        }
        else {
            if( (dst->mvar = realloc(dst->mvar, sizeof(VAR_ST **) * (cnt))) == NULL ) {
                DEBUG_PRINT(1, "realloc(%lu) : [%s]\n", sizeof(VAR_ST **) * (cnt), strerror(errno));
                return(ERR_SYS);
            }
        }
        return(ERR_SYS);
    }
    src_len  = strlen(src);
    size_len = src_len + 1;
    if( (dst->mvar[cnt]->var = (char *)malloc(size_len)) == NULL ) {
        free(dst->mvar[cnt]);
        if(cnt == 0) {
            free(dst->mvar);
        }
        else {
            if( (dst->mvar = realloc(dst->mvar, sizeof(VAR_ST **) * (cnt))) == NULL ) {
                return(ERR_SYS);
            }
        }
        return(ERR_SYS);
    }
    memset(dst->mvar[cnt]->var, 0x00, size_len);
    dst->mvar[cnt]->len = src_len;
    memcpy(dst->mvar[cnt]->var, src, dst->mvar[cnt]->len);
    dst->mvar[cnt]->size = size_len;
    dst->cnt++;
    return(SUCC);
}

int Strn2MVAR(char *src, size_t src_len, MVAR_ST *dst)
{
    long    cnt=0;
    size_t  size_len;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    cnt = dst->cnt;
    if(cnt == 0) {
        if( (dst->mvar = malloc( sizeof(VAR_ST **) * (cnt+1))) == NULL ) {
            return(ERR_SYS);
        }
    }
    else {
        if( (dst->mvar = realloc(dst->mvar, sizeof(VAR_ST **) * (cnt+1))) == NULL ) {
            return(ERR_SYS);
        }
    }
    if( (dst->mvar[cnt] = (VAR_ST *)malloc(sizeof(VAR_ST))) == NULL ) {
        if(cnt == 0) {
            free(dst->mvar);
        }
        else {
            if( (dst->mvar = realloc(dst->mvar, sizeof(VAR_ST **) * (cnt))) == NULL ) {
                return(ERR_SYS);
            }
        }
        return(ERR_SYS);
    }
    size_len = src_len + 1;
    if( (dst->mvar[cnt]->var = (char *)malloc(size_len)) == NULL ) {
        free(dst->mvar[cnt]);
        if(cnt == 0) {
            free(dst->mvar);
        }
        else {
            if( (dst->mvar = realloc(dst->mvar, sizeof(VAR_ST **) * (cnt))) == NULL ) {
                return(ERR_SYS);
            }
        }
        return(ERR_SYS);
    }
    memset(dst->mvar[cnt]->var, 0x00, size_len);
    dst->mvar[cnt]->len  = src_len;
    memcpy(dst->mvar[cnt]->var, src, dst->mvar[cnt]->len);
    dst->mvar[cnt]->size = size_len;
    dst->cnt++;
    return(SUCC);
}

int MVAR2print(char *cmt, MVAR_ST *src, int no)
{
    int i=0;

    if(src == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }

    if(src->cnt != 0 && src->cnt >= no) {
        if(no == 0) {
            for(i=0; i<src->cnt; i++) {
                printf("MVAR(%s][%d)=[%.*s]\n", cmt, i, (int)src->mvar[i]->len, src->mvar[i]->var);
            }
        }
        else {
            i = no - 1;
            printf("MVAR[%s](%d)=[%.*s]\n", cmt, i, (int)src->mvar[i]->len, src->mvar[i]->var);
        }
    }
    return(SUCC);
}

int VAR2Str(VAR_ST *src, char *dst)
{
    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    memcpy(dst, src->var, src->len);
    *(dst+src->len) = '\0';
    return(SUCC);
}

int PTR2VAR(PTR_ST *src, VAR_ST *dst)
{
    size_t  size_len;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    size_len = src->len + 1;
    if( (dst->var = (char *)malloc(size_len)) == NULL) {
        DEBUG_PRINT(1, "malloc(%lu) : [%s]\n", size_len, strerror(errno));
        return(ERR_SYS);
    }
    memset(dst->var, 0x00, size_len);
    memcpy(dst->var, src->ptr, src->len);
    dst->len  = src->len;
    dst->size = size_len;
    return(SUCC);
}

int File2VAR(char *filename, VAR_ST *dst)
{
    FILE    *rfp;
    struct  stat statbuf;
    size_t  mac_len;

    if(filename == NULL || dst == NULL ) {
        if(filename == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    if((rfp = fopen(filename, "rb") ) == NULL ) {
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", filename, strerror(errno));
        return(ERR_SYS);
    }
    memset(&statbuf, 0x00, sizeof(statbuf));
    if(stat(filename, &statbuf) < 0) {
        DEBUG_PRINT(1, "stat(%s) : [%s]\n", filename, strerror(errno));
        fclose(rfp);
        return(ERR_SYS);
    }
    mac_len = statbuf.st_size + 1;
    if( (dst->var=(char *)malloc(mac_len)) == NULL) {
        DEBUG_PRINT(1, "malloc(%lu) : [%s]\n", mac_len, strerror(errno));
        fclose(rfp);
        return(ERR_SYS);
    }
    memset(dst->var, 0x00, mac_len);
    /* data set */
    dst->len = fread(dst->var, 1, statbuf.st_size, rfp);
    dst->size = mac_len;

    fclose(rfp);
    return(SUCC);
}

int File2print(char *filename, int sline, int eline)
{
    FILE         *rfp;
    char         read_buf[4096];
    int          line_no=0;

    if(filename == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    if ((rfp = fopen(filename, "rb")) == NULL ) {
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", filename, strerror(errno));
        return(ERR_SYS);
    }
    memset(read_buf, 0x00, sizeof(read_buf));
    while(fgets(read_buf, sizeof(read_buf)-1, rfp) != NULL) {
        line_no++;
        if(line_no < sline) {
            continue;
        }
        if(eline != 0 && line_no > eline) {
            break;
        }
        /* printf format */
        printf("%s", read_buf);
    }
    return(line_no);
}


int isStrRComp(char *src, char *dst, size_t dst_len)
{
    size_t src_len;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    src_len = strlen(src);
    if(src_len < dst_len) {
        return(FALSE);
    }
    if(strncmp(src+src_len-dst_len, dst, dst_len) != 0) {
        return(FALSE);
    }
    return(TRUE);
}

int isFilesCompare(char *src_filename, char *dst_filename)
{
    VAR_ST  src;
    VAR_ST  dst;

    if(src_filename == NULL || dst_filename == NULL) {
        if(src_filename == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst_filename == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    memset(&src, 0x00, sizeof(src));
    if(File2VAR(src_filename, &src) != SUCC) {
        return(ERR_SYS);
    }
    memset(&dst, 0x00, sizeof(dst));
    if(File2VAR(dst_filename, &dst) != SUCC) {
        VAR2free(&src);
        return(ERR_SYS);
    }
    if(src.len != dst.len) {
        VAR2free(&src);
        VAR2free(&dst);
        return(FALSE);
    }
    if(strncmp(src.var, dst.var, src.len) != 0) {
        VAR2free(&src);
        VAR2free(&dst);
        return(FALSE);
    }
    VAR2free(&src);
    VAR2free(&dst);

    return(TRUE);
}

int isStrnTimeChk(char *sdate, size_t slen, char *fdate, char *tdate)
{
    size_t clen;

    if(sdate == NULL || fdate == NULL || tdate == NULL) {
        if(sdate == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(fdate == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        if(tdate == NULL) {
            DEBUG_PRINT(1, "argv[4] = NULL \n");
        }
        return(ERR_NULL);
    }
    if(fdate[0] != '\0') {
        clen = strlen(fdate);
        if(slen < clen) clen = slen;
        if(strlen(fdate) > 0 && strncmp(sdate, fdate, clen) < 0) {
            return(FALSE);
        }
    }
    if(tdate[0] != '\0') {
        clen = strlen(tdate);
        if(slen < clen) clen = slen;
        if(strlen(tdate) > 0 && strncmp(sdate, tdate, clen) > 0) {
            return(FALSE);
        }
    }
    return(TRUE);
}

int isFileAccess(char *filename, int flag)
{
    if(filename == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
    }
    if(flag % 2 == 1) {                 // F_OK
        if(access(filename,F_OK) != 0) {
            //DEBUG_PRINT(1, "access[%s] : [%s]\n", filename, strerror(errno));
            return(FALSE);
        }
    }
    if(flag == 2 || flag == 3) {        // R_OK, RF_OK
        if(access(filename,R_OK) != 0) {
            //DEBUG_PRINT(1, "access[%s] : [%s]\n", filename, strerror(errno));
            return(FALSE);
        }
    }
    else if(flag == 4 || flag == 5) {   // W_OK, WF_OK
        if(access(filename,W_OK) != 0) {
            //DEBUG_PRINT(1, "access[%s] : [%s]\n", filename, strerror(errno));
            return(FALSE);
        }
    }
    else if(flag == 6 || flag == 7) {   // RW_OK RWF_OK
        if(access(filename,R_OK) != 0) {
            //DEBUG_PRINT(1, "access[%s] : [%s]\n", filename, strerror(errno));
            return(FALSE);
        }
        if(access(filename,W_OK) != 0) {
            //DEBUG_PRINT(1, "access[%s] : [%s]\n", filename, strerror(errno));
            return(FALSE);
        }
    }
    return(TRUE);
}

int Str2Strs3PathFileSep(char *filename, char *path, char *file)
{
    char   *sptr;
    char   *fptr;
    char   *tptr;

    if(filename == NULL || path == NULL || file == NULL ) {
        if(filename == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(path == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        if(file == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    sptr = filename;
    fptr = filename;
    while(1) {
        if( (tptr=strchr(fptr, '/')) == NULL ) {

            /* dirname : /directory/ */
            strncpy(path, sptr, fptr-sptr);

            /* basename : filename */
            strncpy(file, fptr, strlen(fptr));

            break;
        }
        fptr = tptr + 1;
    }
    return(SUCC);
}

int Strn2Str3RcutStr(char *src, size_t src_len, char *dst, char *cut_str, int cut_flag)
{
    char   *sptr;
    char   *cut_ptr;
    VAR_ST  cut_var;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        if(cut_str == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    memset(&cut_var, 0x00, sizeof(cut_var));
    Strn2VAR(src, src_len, &cut_var);
    sptr = cut_var.var;
    while(1) {
        if((cut_ptr=strstr(sptr, cut_str)) == NULL) {
            if(sptr != cut_var.var) { /* search count > 0 */
                strcpy(dst, sptr);
            }
            break;
        }
        if(cut_flag) { /* last left search */
            strcpy(dst, cut_ptr+strlen(cut_str));
            break;
        }
        sptr = cut_ptr+strlen(cut_str);
    }
    VAR2free(&cut_var);
    return(SUCC);
}

int Strn2Str3RcutSize(char *src, size_t src_len, char *dst, size_t cut_size)
{
    char *cut_ptr;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    if(src_len < cut_size) cut_size = src_len;
    cut_ptr = src + src_len - cut_size;
    strncpy(dst, cut_ptr, cut_size);
    return(SUCC);
}

int Strn2Str3Crc(unsigned char *data, size_t length, char *dst)
{
    unsigned int crcTable[257];
    unsigned int remainder = 0xFFFFFFFF;
    unsigned int gen_remainder;

    for (int dividend = 0; dividend < 256; dividend++) {
        gen_remainder = dividend << 24;
        for (int bit = 0; bit < 256; bit++) {
            if(gen_remainder & 0x80000000) {
                gen_remainder = (gen_remainder << 1) ^ POLINOMIAL;
            }
            else {
                gen_remainder = (gen_remainder << 1);
            }
        }
        crcTable[dividend] = gen_remainder;
    }
    for (int byte = 0; byte < (int)length; byte++) {
        remainder = (remainder << 8) ^ crcTable[(remainder >> 24) ^ data[byte]];
    }
    sprintf(dst, "%u", remainder);
    return(SUCC);
}
int Strn3Replace(char *src, size_t src_len, char *dst, size_t dst_len, char *conv, size_t conv_len)
{
    int     cnt=0;
    long    dlen;
    long    slen=0;
    char   *sptr;
    char   *eptr;
    char   *dptr;
    char   *cptr;

    if(src == NULL || dst == NULL ) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        if(conv == NULL) {
            DEBUG_PRINT(1, "argv[5] = NULL \n");
        }
        return(ERR_NULL);
    }
    if(src_len == 0 && dst_len == 0) {
            memmove(dst+conv_len, dst, strlen(dst));
            memcpy(dst, conv, conv_len);
            return(SUCC);
    }

    dptr = malloc(dst_len+1);
    memset(dptr, 0x00, dst_len+1);
    memcpy(dptr, dst, dst_len);

    cptr = malloc(conv_len+1);
    memset(cptr, 0x00, conv_len+1);
    memcpy(cptr, conv, conv_len);

    sptr = src;
    dlen = dst_len - conv_len;

    while(1) {
        if((eptr=strstr(sptr, dptr)) == NULL) {
            break;
        }
        /* Search length over check */
        slen = slen + (eptr - sptr) + dst_len;
        if(slen > (long)src_len) break;

        if(dlen == 0) {     /* mem equal */
            memcpy(eptr, cptr, conv_len);
        }
        else if(dlen == (long)dst_len) { /* delete : conv_len = 0 */
            memmove(eptr, eptr+dst_len, strlen(eptr+dst_len));
            memset(eptr+strlen(eptr)-dlen, 0x00, dlen);
        }
        else if(dlen > 0) { /* mem less dst_len > conv_len */
            memmove(eptr+conv_len, eptr+dst_len, strlen(eptr+dst_len));
            memcpy(eptr, conv, conv_len);
            memset(eptr+strlen(eptr)-dlen, 0x00, dlen);
        }
        else {              /* mem grate dst_len < conv_len */
            memmove(eptr+conv_len, eptr+dst_len, strlen(eptr+dst_len));
            memcpy(eptr, conv, conv_len);
        }
        /* next search point */
        sptr = eptr + conv_len;
        cnt++;
    }
    return(SUCC);
}

int Strn3AreaReplaceOne(char *src, size_t src_len, char *cmt_sid, char *cmt_tid, char *rep, int rep_id, int lcol, int rcol)
{
    char   *sptr;
    char   *fptr;
    char   *tptr;
    size_t tlen;
    size_t ft_len;

    if(src == NULL || cmt_sid == NULL || cmt_tid == NULL || rep == NULL) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(cmt_sid == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        if(cmt_tid == NULL) {
            DEBUG_PRINT(1, "argv[4] = NULL \n");
        }
        if(rep == NULL) {
            DEBUG_PRINT(1, "argv[5] = NULL \n");
        }
        return(ERR_NULL);
    }
    sptr = src;

    while(1) {
        if( (fptr=strstr(sptr, cmt_sid)) == NULL) {
            break;
        }
        if( fptr+strlen(cmt_sid) > src + src_len ) break;

        if( (tptr=strstr(fptr+strlen(cmt_sid), cmt_tid)) == NULL) {
            break;
        }
        /* src_len over check */
        if( tptr+strlen(cmt_tid) > src + src_len ) break;

        /* cmt_sid ~ cmt_tid -> rep */
        fptr += lcol;
        tptr += (strlen(cmt_tid) + rcol);
        tlen = strlen(tptr);
        ft_len = tptr - fptr;

        /* tptr ~ fptr -> rep : change */
        if(rep_id == 0) {
            memmove(fptr+strlen(rep), tptr, tlen);
            if(ft_len > strlen(rep)) {
                memset(fptr+strlen(rep)+tlen, 0x00, strlen(fptr+strlen(rep)+tlen));
            }
            if(strlen(rep) != 0) {
                memcpy(fptr, rep, strlen(rep));
            }
            sptr = fptr+strlen(rep);
        }
        /* tptr ~ fptr -> rep[0] all change */
        else {
            memset(fptr, rep[0], ft_len);
            sptr = tptr;
        }
        break;
    }
    return(SUCC);
}

int Strn3AreaReplace(char *src, size_t src_len, char *cmt_sid, char *cmt_tid, char *rep, int rep_id, int lcol, int rcol)
{
    char   *sptr;
    char   *fptr;
    char   *tptr;
    size_t tlen;
    size_t ft_len;

    if(src == NULL || cmt_sid == NULL || cmt_tid == NULL || rep == NULL) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(cmt_sid == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        if(cmt_tid == NULL) {
            DEBUG_PRINT(1, "argv[4] = NULL \n");
        }
        if(rep == NULL) {
            DEBUG_PRINT(1, "argv[5] = NULL \n");
        }
        return(ERR_NULL);
    }
    sptr = src;

    while(1) {
        if( (fptr=strstr(sptr, cmt_sid)) == NULL) {
            break;
        }
        if( fptr+strlen(cmt_sid) > src + src_len ) break;

        if( (tptr=strstr(fptr+strlen(cmt_sid), cmt_tid)) == NULL) {
            break;
        }
        /* src_len over check */
        if( tptr+strlen(cmt_tid) > src + src_len ) break;

        /* cmt_sid ~ cmt_tid -> rep */
        fptr += lcol;
        tptr += (strlen(cmt_tid) + rcol);
        tlen = strlen(tptr);
        ft_len = tptr - fptr;

        /* tptr ~ fptr -> rep : change */
        if(rep_id == 0) {
            memmove(fptr+strlen(rep), tptr, tlen);
            if(ft_len > strlen(rep)) {
                memset(fptr+strlen(rep)+tlen, 0x00, strlen(fptr+strlen(rep)+tlen));
            }
            if(strlen(rep) != 0) {
                memcpy(fptr, rep, strlen(rep));
            }
            sptr = fptr+strlen(rep);
        }
        /* tptr ~ fptr -> rep[0] all change */
        else {
            memset(fptr, rep[0], ft_len);
            sptr = tptr;
        }
    }
    return(SUCC);
}

int Strn2PTR3AreaSearch(char *src, size_t src_len, PTR_ST *dst, char *cmt_sid, char *cmt_tid, int lcol, int rcol)
{
    char   *sptr;
    char   *fptr;
    char   *tptr;
    size_t ft_len;

    if(src == NULL || dst == NULL || cmt_sid == NULL || cmt_tid == NULL) {
        if(src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(dst == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        if(cmt_sid == NULL) {
            DEBUG_PRINT(1, "argv[4] = NULL \n");
        }
        if(cmt_tid == NULL) {
            DEBUG_PRINT(1, "argv[5] = NULL \n");
        }
        return(ERR_NULL);
    }

    sptr = src;

    if( (fptr=strstr(sptr, cmt_sid)) == NULL) {
        return(FAIL);
    }
    /* from search area over */
    if( fptr+strlen(cmt_sid) > src + src_len ) return(FAIL);

    if( (tptr=strstr(fptr+strlen(cmt_sid), cmt_tid)) == NULL) {
        return(FAIL);
    }
    /* to search area over */
    if( tptr+strlen(cmt_tid) > src + src_len ) return(FAIL);

    /* cmt_sid ~ cmt_tid -> rep */
    fptr += lcol;
    tptr += (strlen(cmt_tid) + rcol);
    ft_len = tptr - fptr;

    dst->ptr  = fptr;
    dst->len  = ft_len;
    dst->sptr = src;
    dst->slen = src_len;

    return(SUCC);
}

int var2free(void *src)
{
    if(src != NULL) {
        free(src);
        src = NULL;
    }
    return(SUCC);
}

int VAR2free(VAR_ST *src)
{
    if(src == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }
    if(src->var != NULL) {
        free(src->var);
    }
    memset(src, 0x00, sizeof(VAR_ST));
    return(SUCC);
}

int MVAR2free(MVAR_ST *src)
{
    int i=0;

    if(src == NULL) {
        DEBUG_PRINT(1, "argv[1] = NULL \n");
        return(ERR_NULL);
    }

    if(src->cnt != 0) {
        for(i=0; i<src->cnt; i++) {
            VAR2free(src->mvar[i]);
            var2free(src->mvar[i]);
        }
        var2free(src->mvar);
    }
    memset(src, 0x00, sizeof(MVAR_ST));

    return(SUCC);
}

/*===================================================*/


int AswGet_AmlFindProc(AML_FIND_ST *find)
{
    FILE_INFO_ST  finfo;
    VAR_ST        sort_var;
    char   command[4096];
    char   tempname[1096];
    char   sortname[1096];
    char   errname [1280];
    char   read_buf[2048];
    char   depdname[1096];
    FILE   *fp;
    FILE   *wfp;
    FILE   *efp;
    char   *sptr;
    int    except_flag=0;
    int    line_no=0;

    if(find == NULL) {
        return(ERR_NULL);
    }

    printf("===== AmlFindInfo ====\n");
    sprintf(tempname, "%s.temp", find->report_file_body);
    sprintf(errname, "%s.err", tempname);

    sprintf(command, "cat /dev/null > %s ", tempname);
    Str9Command(command);
    sprintf(command, "cat /dev/null > %s ", errname);
    Str9Command(command);

    /* <HOME>/asw /aqt</HOME> <OPTS>"-type f"</OPTS> <NAME>*.c *.pc</NAME> <PIPE>sort uniq</PIPE> */
    if(find->home.cnt != 0 && find->name.cnt != 0) {
        memset(command, 0x00, sizeof(command));
        strcpy(command, "find ");
        for(int i=0; i < find->home.cnt; i++) {
            sprintf(command+strlen(command), "%s ", find->home.mvar[i]->var);
        }
        for(int i=0; i < find->opts.cnt; i++) {
            sprintf(command+strlen(command), "%s ", find->opts.mvar[i]->var);
        }
        for(int i=0; i < find->name.cnt; i++) {
            if(i > 0) {
                sprintf(command+strlen(command), "-o ");
            }
            sprintf(command+strlen(command), "-name \"%s\" ", find->name.mvar[i]->var);
        }
        for(int i=0; i < find->pipe.cnt; i++) {
            sprintf(command+strlen(command), "2>>%s | %s ", errname, find->pipe.mvar[i]->var);
        }
        sprintf(command+strlen(command), " 1>>%s 2>>%s", tempname, errname);
        printf("EXEC : %s\n", command);
        Str9Command(command);
    }

    /* <FULL_FIND>find /usr/include -type f -name '*.h'</FULL_FIND> */
    if(find->full_find.cnt != 0) {
        for(int i=0; i < find->full_find.cnt; i++) {
            memset(command, 0x00, sizeof(command));
            sprintf(command+strlen(command), "%s ", find->full_find.mvar[i]->var);
            sprintf(command+strlen(command), " 1>>%s 2>>%s", tempname, errname);
            printf("EXEC : %s\n", command);
            Str9Command(command);
        }
    }

    /* <LIST_FILE>listfile.lst</LIST_FILE> cat redirection */
    for(int i=0; i < find->list_file.cnt; i++) {
        memset(command, 0x00, sizeof(command));
        sprintf(command, "cat %s 1>>%s 2>>%s", find->list_file.mvar[0]->var, tempname, errname);
        printf("EXEC : %s\n", command);
        Str9Command(command);
    }
    File2print(errname, line_no, 0);

    /* | sort and uniq -> .sort */
    sprintf(sortname, "%s.sort", find->report_file_body);
    sprintf(errname, "%s.err", sortname);
    memset(command, 0x00, sizeof(command));
    sprintf(command, "cat %s | sort | uniq 1>%s 2>%s", tempname, sortname, errname);
    Str9Command(command);
    File2print(errname, 0, 0);

    /* malloc sortname file */
    memset(&sort_var, 0x00, sizeof(sort_var));
    if(File2VAR(sortname, &sort_var) != SUCC) {
        return(ERR_SYS);
    } 
    if ((fp = fopen(sortname, "rb")) == NULL ) {
        VAR2free(&sort_var);
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", sortname, strerror(errno));
        return(ERR_SYS);
    }

    /* list file create */
    sprintf(find->find_list_file, "%s.list", find->report_file_body);
    if ((wfp = fopen(find->find_list_file, "wb")) == NULL ) {
        VAR2free(&sort_var);
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", find->find_list_file, strerror(errno));
        return(ERR_SYS);
    }
    /* list.except file create */
    sprintf(find->find_list_except_file, "%s.list.except", find->report_file_body);
    if ((efp = fopen(find->find_list_except_file, "wb")) == NULL ) {
        VAR2free(&sort_var);
        DEBUG_PRINT(1, "fopen(%s) : [%s]\n", find->find_list_except_file, strerror(errno));
        return(ERR_SYS);
    }

    memset(read_buf, 0x00, sizeof(read_buf));
    while(fgets(read_buf, sizeof(read_buf)-1, fp) != NULL) {
        /* newline delete */
        read_buf[strlen(read_buf)-1] = 0x00;

        /* init flag */
        except_flag = 0;

        /* FILE_INFO_ST finfo set */
        memset(&finfo, 0x00, sizeof(finfo));
        if(AswGet_FileInfo(read_buf, &finfo, 1) != SUCC) {
            memset(read_buf, 0x00, sizeof(read_buf));
            continue;
        }
      
        /* <?aml:FIND>  <FROM_DATE>  <TO_DATE> */  
        if(isStrnTimeChk(finfo.mtime, strlen(finfo.mtime), find->fdate, find->tdate) != TRUE) {
            fprintf(efp, "aml date check except [%s][%s]:[%s][%s]\n", read_buf, finfo.mtime, find->fdate, find->tdate);
            continue;
        }
        for(int i=0; i < find->file_inc.cnt; i++) {
            if(strcmp(read_buf, find->file_inc.mvar[i]->var) == 0) {
                //printf("INC inc [%s][%s]\n", read_buf, find->file_inc.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) {         // <FILE_INC></FILE_INC>
            /***** last listfile *****/
            fprintf(wfp, "%s\n", read_buf);
            fflush(wfp);
            continue;
        }

        for(int i=0; i < find->file_ext.cnt; i++) {
            if(strcmp(read_buf, find->file_ext.mvar[i]->var) == 0) {
                fprintf(efp, "EXT file_ext [%s][%s]\n", read_buf, find->file_ext.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <FILE_EXT></FILE_EXT>

        for(int i=0; i < find->dir_a.cnt; i++) {
            if(strstr(finfo.dirnm, find->dir_a.mvar[i]->var) != NULL) {
                fprintf(efp, "EXT dir_a [%s][%s]\n", read_buf, find->dir_a.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <DIR_A></DIR_A>

        for(int i=0; i < find->dir_l.cnt; i++) {
            if(strncmp(finfo.dirnm, find->dir_l.mvar[i]->var, find->dir_l.mvar[i]->len) == 0) {
                fprintf(efp, "EXT dir_l [%s][%s]\n", read_buf, find->dir_l.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <DIR_L></DIR_L>

        for(int i=0; i < find->dir_r.cnt; i++) {
            if(strncmp(finfo.dirnm+(strlen(finfo.dirnm)-find->dir_r.mvar[i]->len), find->dir_r.mvar[i]->var, find->dir_r.mvar[i]->len) == 0) {
                fprintf(efp, "EXT dir_r [%s][%s]\n", read_buf, find->dir_r.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <DIR_R></DIR_R>

        for(int i=0; i < find->file_a.cnt; i++) {
            if(strstr(finfo.filenm, find->file_a.mvar[i]->var) != NULL) {
                fprintf(efp, "EXT file_a [%s][%s]\n", read_buf, find->file_a.mvar[i]->var); //limlim
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <FILE_A></FILE_A>

        for(int i=0; i < find->file_l.cnt; i++) {
            if(strncmp(finfo.filenm, find->file_l.mvar[i]->var, find->file_l.mvar[i]->len) == 0) {
                fprintf(efp, "EXT file_l [%s][%s]\n", read_buf, find->file_l.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <FILE_L></FILE_L>

        for(int i=0; i < find->file_r.cnt; i++) {
            if(strncmp(finfo.filenm+(strlen(finfo.filenm)-find->file_r.mvar[i]->len), find->file_r.mvar[i]->var, find->file_r.mvar[i]->len) == 0) {
                fprintf(efp, "EXT file_r [%s][%s]\n", read_buf, find->file_r.mvar[i]->var);
                except_flag = 1;
                break;
            }
        }
        if(except_flag) continue; // <FILE_R></FILE_R>

        for(int i=0; i < find->depend_s.cnt; i++) {
            if(strcmp(finfo.extnm, find->depend_s.mvar[i]->var) != 0) {
                continue;    
            }
            memset(depdname, 0x00, sizeof(depdname));
            strncpy(depdname, read_buf, strlen(read_buf)-strlen(finfo.extnm));
            sprintf(depdname+strlen(depdname), "%s\n",  find->depend_d.mvar[i]->var); 
            if((sptr=strstr(sort_var.var, depdname)) != NULL) {
                sptr--;
                if(*sptr == '\0' || *sptr == '\n') { 
                    fprintf(efp, "depend [%s][%s:%s]\n", read_buf, find->depend_s.mvar[i]->var, find->depend_d.mvar[i]->var);
                    except_flag = 1;
                    break;
                }
            }
        }
        if(except_flag) continue; // <DEPEND_?></DEPEND_?>

        /***** last listfile *****/
        fprintf(wfp, "%s\n", read_buf);
        fflush(wfp);

    }
    VAR2free(&sort_var);
    fclose(efp);
    fclose(wfp);
    fclose(fp);

    sprintf(command, "rm -f %s*", sortname);
    Str9Command(command);

    return(SUCC);
}

int AswGet_AmlInfo(char *aml_file, void *aml_st, char *aml_id)
{
    VAR_ST  aml_src;
    VAR_ST  aml_main;

    if(aml_file == NULL || aml_st == NULL || aml_id == NULL) {
        if(aml_file == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(aml_st == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        if(aml_id == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    /* argv[1] : aml file , R_OK */
    if(isFileAccess(aml_file, 2) == FALSE) {
        DEBUG_PRINT(1, "access[%s] : [%s]\n", aml_file, strerror(errno));
        return(ERR_SYS);
    }
    /* AML File var malloc READ */
    if( File2VAR(aml_file, &aml_src) < 0 ) {
        return(ERR_SYS);
    }
    //printf("AML_ALL START\n%.*s\nAML_ALL END\n", (int)aml_src.len, aml_src.var);
    if(strncmp(aml_src.var, "##", 2) == 0) {
        Strn3AreaReplaceOne(aml_src.var, aml_src.len, "##", "\n", "", 0, 0, 0);
    }
    Strn3AreaReplace(aml_src.var, aml_src.len, "\n##", "\n", "", 0, 1, 0);

    memset(&aml_main, 0x00, sizeof(aml_main));
    if(AswGet_AmlMainMarkup(&aml_src, &aml_main, aml_id) != SUCC) {
        return(FAIL);
    }
    //printf("AML_MAIN START\n%.*s\nAML_MAIN END\n", (int)aml_main.len, aml_main.var);

    if(strcmp(aml_id, "FIND") == 0) {
        if(AswGet_AmlFindParse(&aml_main, (AML_FIND_ST *)aml_st) != SUCC) {
            VAR2free(&aml_main);
            VAR2free(&aml_src);
            return(FAIL);
        }
    }
    else if(strcmp(aml_id, "PROG") == 0) {
        if(AswGet_AmlProgParse(&aml_main, (PROG_INFO_ST *)aml_st) != SUCC) {
            VAR2free(&aml_main);
            VAR2free(&aml_src);
            return(FAIL);
        }
    }
    else {
        VAR2free(&aml_main);
        VAR2free(&aml_src);
        return(FAIL);
    }
    VAR2free(&aml_main);
    VAR2free(&aml_src);
    return(SUCC);
}

int AswGet_AmlProgParse(VAR_ST *aml_main, PROG_INFO_ST *prog)
{
    VAR_ST  aml_sub;

    if(aml_main == NULL || prog == NULL) {
        if(aml_main == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(prog == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
    /* OS_VER */
    memset(&aml_sub, 0x00, sizeof(aml_sub));
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "OS_VER") == SUCC) {
        VAR2Str(&aml_sub, prog->os_ver);
    }
    //printf("AML_OS_VER START\n%.*s\nAML_OS_VER END\n", (int)aml_sub.len, aml_sub.var);
    VAR2free(&aml_sub);

    /* LOG_LEVEL */
    memset(&aml_sub, 0x00, sizeof(aml_sub));
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "LOG_LEVEL") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &prog->log_level, "DIV");
    }
    //printf("AML_log_level  START\n%.*s\nAML_log_level END\n", (int)aml_sub.len, aml_sub.var);
    //MVAR2print(&find->home, 0);
    VAR2free(&aml_sub);

    return(SUCC);
}

int AswGet_AmlFindParse(VAR_ST *aml_main, AML_FIND_ST *find)
{
    char    hostname[SYS_NAME_LEN];
    VAR_ST  aml_sub;

    if(aml_main == NULL || find == NULL) {
        if(aml_main == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(find == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        return(ERR_NULL);
    }
/** find date set **/
    //Str3sysdate(find->datetime, 1);
/** find(1) **/
    /* home directory */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "HOME") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->home, "DIV");
    }
    //printf("AML_HOME  START\n%.*s\nAML_HOME END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FIND HOME", &find->home, 0);
    VAR2free(&aml_sub);
    
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "OPTS") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->opts, "DIV");
    }
    //printf("AML_OPTS  START\n%.*s\nAML_OPTS END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FIND OPTS", &find->opts, 0);
    VAR2free(&aml_sub);

    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "NAME") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->name, "DIV");
    }
    //printf("AML_NAME  START\n%.*s\nAML_NAME END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FIND NAME", &find->name, 0);
    VAR2free(&aml_sub);

    memset(&aml_sub, 0x00, sizeof(aml_sub));
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "PIPE") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->pipe, "DIV");
    }
    //printf("AML_PIPE  START\n%.*s\nAML_PIPE END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FIND PIPE", &find->pipe, 0);
    VAR2free(&aml_sub);

    /* from date */
    memset(&aml_sub, 0x00, sizeof(aml_sub));
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FROM_DATE") == SUCC) {
        VAR2Str(&aml_sub, find->fdate);
    }
    printf("AML_FDATE START\n%.*s\nAML_FDATE END\n", (int)aml_sub.len, aml_sub.var);
    VAR2free(&aml_sub);

    /* to date */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "TO_DATE") == SUCC) {
        VAR2Str(&aml_sub, find->tdate);
    }
    printf("AML_TDATE START\n%.*s\nAML_TDATE END\n", (int)aml_sub.len, aml_sub.var);
    VAR2free(&aml_sub);

/** find(2) **/

    /* full find command */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FULL_FIND") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->full_find, "DIV");
    }
    //printf("AML_FULL_FIND  START\n%.*s\nAML_FULL_FIND END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FULL_FIND", &find->full_find, 0);
    VAR2free(&aml_sub);

    /* manual find list file */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "LIST_FILE") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->list_file, "DIV");
    }
    //printf("AML_LIST_FILE  START\n%.*s\nAML_LIST_FILE END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("LIST_FILE", &find->list_file, 0);
    VAR2free(&aml_sub);

/** include condition **/
    /* include file */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FILE_INC") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->file_inc, "DIV");
    }
    //printf("AML_FILE_INC START\n%.*s\nAML_FILE_INC END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FILE_INC", &find->file_inc, 0);
    VAR2free(&aml_sub);

/** except condition **/
    /* execpte file */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FILE_EXT") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->file_ext, "DIV");
    }
    //printf("AML_FILE_EXT START\n%.*s\nAML_FILE_EXT END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FILE_EXT", &find->file_ext, 0);
    VAR2free(&aml_sub);

    /* except dir */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "DIR_A") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->dir_a, "DIV");
    }
    //printf("AML_DIR_A START\n%.*s\nAML_DIR_A END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("DIR_A", &find->dir_a, 0);
    VAR2free(&aml_sub);

    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "DIR_L") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->dir_l, "DIV");
    }
    //printf("AML_DIR_L START\n%.*s\nAML_DIR_L END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("DIR_L", &find->dir_l, 0);
    VAR2free(&aml_sub);

    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "DIR_R") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->dir_r, "DIV");
    }
    //printf("AML_DIR_R START\n%.*s\nAML_DIR_R END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("DIR_R", &find->dir_r, 0);
    VAR2free(&aml_sub);

    /* except FILE */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FILE_A") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->file_a, "DIV");
    }
    //printf("AML_FILE_A START\n%.*s\nAML_FILE_A END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FILE_A", &find->file_a, 0);
    VAR2free(&aml_sub);

    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FILE_L") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->file_l, "DIV");
    }
    //printf("AML_FILE_L START\n%.*s\nAML_FILE_L END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FILE_L", &find->file_l, 0);
    VAR2free(&aml_sub);

    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "FILE_R") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->file_r, "DIV");
    }
    //printf("AML_FILE_R START\n%.*s\nAML_FILE_R END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("FILE_R", &find->file_r, 0);
    VAR2free(&aml_sub);

    /* File extend dependency */
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "DEPEND_S") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->depend_s, "DIV");
    }
    //printf("AML_DEPEND_S START\n%.*s\nAML_DEPEND_S END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("DEPEND_S", &find->depend_s, 0);
    VAR2free(&aml_sub);

    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "DEPEND_D") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->depend_d, "DIV");
    }
    //printf("AML_DEPEND_D START\n%.*s\nAML_DEPEND_D END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("DEPEND_D", &find->depend_d, 0);
    VAR2free(&aml_sub);

    /** report File BODY + OPTS name **/
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "REPORT_OPTS") == SUCC) {
        AswGet_AmlSubDivide(&aml_sub, &find->report_file_opts, "DIV");
    }
    //printf("AML_REPORT_OPTS START\n%.*s\nAML_REPORT_OPTS END\n", (int)aml_sub.len, aml_sub.var);
    MVAR2print("REPORT_OPTS", &find->report_file_opts, 0);
    VAR2free(&aml_sub);

    if(strlen(find->datetime) == 0) {
        Str3sysdate(find->datetime, 1);
    }
    memset(&aml_sub, 0x00, sizeof(aml_sub));
    if(AswGet_AmlSubMarkup(aml_main, &aml_sub, "REPORT_FILE") == SUCC) {
        VAR2Str(&aml_sub, find->report_file_body);
        printf("AML_REPORT_FILE START\n%.*s\nAML_REPORT_FILE END\n", (int)aml_sub.len, aml_sub.var);
        for(int i=0; i < find->report_file_opts.cnt; i++) {
            if(strcmp(find->report_file_opts.mvar[i]->var, "%DATE%") == 0) {
                strncat(find->report_file_body, find->datetime, 8);
            }
            else if(strcmp(find->report_file_opts.mvar[i]->var, "%TIME%") == 0) {
                strncat(find->report_file_body, find->datetime+8, 6);
            }
            else if(strcmp(find->report_file_opts.mvar[i]->var, "%HOSTNAME%") == 0) {
                memset(hostname, 0x00, sizeof(hostname));
                if(gethostname(hostname, SYS_NAME_LEN-1) == 0) {
                    strcat(find->report_file_body, hostname);
                }          
            }
            else {
                strcat(find->report_file_body, find->report_file_opts.mvar[i]->var);
            }
        }
    }
    printf("report_file_body=[%s]\n", find->report_file_body);
    VAR2free(&aml_sub);
    return(SUCC);
}

int AswGet_AmlMainMarkup(VAR_ST *aml_src, VAR_ST *aml_main, char *aml_main_id)
{
    char   fsrch[128];
    char   tsrch[128];
    PTR_ST srch_ptr_st;

    if(aml_src == NULL || aml_main == NULL || aml_main_id == NULL) {
        if(aml_src == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(aml_main == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        if(aml_main_id == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    memset(fsrch, 0x00, sizeof(fsrch));
    memset(tsrch, 0x00, sizeof(tsrch));
    /* MAIN */
    sprintf(fsrch, "<?aml:%s>",  aml_main_id);
    sprintf(tsrch, "<?aml:/%s>", aml_main_id);

    memset(&srch_ptr_st, 0x00, sizeof(srch_ptr_st));
    // <?aml:%s>data<?aml:/%s> : only data 
    Strn2PTR3AreaSearch(aml_src->var, aml_src->len, &srch_ptr_st, fsrch, tsrch, (int)strlen(fsrch), -((int)strlen(tsrch)));
    //printf("%.*s", (int)srch_ptr_st.len, srch_ptr_st.ptr);
    PTR2VAR(&srch_ptr_st, aml_main);

    return(SUCC);
}

int AswGet_AmlSubMarkup(VAR_ST *aml_main, VAR_ST *aml_sub, char *aml_sub_id)
{
    char   fsrch[128];
    char   tsrch[128];
    PTR_ST srch_ptr_st;

    if(aml_main == NULL || aml_sub == NULL || aml_sub_id == NULL) {
        if(aml_main == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(aml_sub == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        if(aml_sub_id == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    memset(fsrch, 0x00, sizeof(fsrch));
    memset(tsrch, 0x00, sizeof(tsrch));
    /* SUB */
    sprintf(fsrch, "<%s>",  aml_sub_id);
    sprintf(tsrch, "</%s>", aml_sub_id);

    memset(&srch_ptr_st, 0x00, sizeof(srch_ptr_st));
    Strn2PTR3AreaSearch(aml_main->var, aml_main->len, &srch_ptr_st, fsrch, tsrch, (int)strlen(fsrch), -((int)strlen(tsrch)));
    //printf("%.*s", (int)srch_ptr_st.len, srch_ptr_st.ptr);
    PTR2VAR(&srch_ptr_st, aml_sub);

    return(SUCC);
}

int AswGet_AmlSubDivide(VAR_ST *aml_sub, MVAR_ST *aml_div, char *aml_div_id)
{
    char   fsrch[128];
    char   tsrch[128];
    char   *sptr;
    char   *fptr; 
    char   *eptr;

    if(aml_sub == NULL || aml_div == NULL || aml_div_id == NULL) {
        if(aml_sub == NULL) {
            DEBUG_PRINT(1, "argv[1] = NULL \n");
        }
        if(aml_div == NULL) {
            DEBUG_PRINT(1, "argv[2] = NULL \n");
        }
        if(aml_div_id == NULL) {
            DEBUG_PRINT(1, "argv[3] = NULL \n");
        }
        return(ERR_NULL);
    }
    memset(fsrch, 0x00, sizeof(fsrch));
    memset(tsrch, 0x00, sizeof(tsrch));
    /* DIV */
    sprintf(fsrch, "<%s>",  aml_div_id);
    sprintf(tsrch, "</%s>", aml_div_id);

    sptr = aml_sub->var;
    while(1) {
        if((fptr=strstr(sptr, fsrch)) == NULL) break;
        fptr += strlen(fsrch);
        sptr = fptr;
        if((eptr=strstr(sptr, tsrch)) == NULL) {
            return(FAIL);
        } 
        if(fptr != eptr) {
	        if(strncmp(fptr, "\\0", 2) == 0) {
                Strn2MVAR("", 0, (MVAR_ST *)aml_div);
            }
	        else {
                Strn2MVAR(fptr, eptr-fptr, (MVAR_ST *)aml_div);
	        }
        }
        sptr = eptr + strlen(tsrch);                
    }
    return(SUCC);
}
