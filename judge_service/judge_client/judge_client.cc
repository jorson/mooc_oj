//
// File:   main.cc
// Author: sempr
// refacted by zhblue
/*
 * Copyright 2008 sempr <iamsempr@gmail.com>
 *
 * Refacted and modified by zhblue<newsclan@gmail.com>
 * Bug report email newsclan@gmail.com
 *
 *
 * This file is part of HUSTOJ.
 *
 * HUSTOJ is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * HUSTOJ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HUSTOJ. if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/signal.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <mysql/mysql.h>
#include <assert.h>
#include "okcalls.h"

#define STD_MB 1048576
#define STD_T_LIM 2
#define STD_F_LIM (STD_MB<<5)
#define STD_M_LIM (STD_MB<<7)
#define BUFFER_SIZE 512

#define OJ_WT0 0
#define OJ_WT1 1
#define OJ_CI 2
#define OJ_RI 3
#define OJ_AC 4
#define OJ_PE 5
#define OJ_WA 6
#define OJ_TL 7
#define OJ_ML 8
#define OJ_OL 9
#define OJ_RE 10
#define OJ_CE 11
#define OJ_CO 12
#define OJ_TR 13
/*copy from ZOJ
 http://code.google.com/p/zoj/source/browse/trunk/judge_client/client/tracer.cc?spec=svn367&r=367#39
 */
#ifdef __i386
#define REG_SYSCALL orig_eax
#define REG_RET eax
#define REG_ARG0 ebx
#define REG_ARG1 ecx
#else
#define REG_SYSCALL orig_rax
#define REG_RET rax
#define REG_ARG0 rdi
#define REG_ARG1 rsi

#endif

//增加buf存储solution_id
static char sln_id[BUFFER_SIZE];

static int DEBUG = 0;
static char host_name[BUFFER_SIZE];
static char user_name[BUFFER_SIZE];
static char password[BUFFER_SIZE];
static char db_name[BUFFER_SIZE];
static char oj_home[BUFFER_SIZE];
static int port_number;
static int max_running;
static int sleep_time;
static int java_time_bonus = 5;
static int java_memory_bonus = 512;
static char java_xms[BUFFER_SIZE];
static char java_xmx[BUFFER_SIZE];
static int sim_enable = 0;
static int oi_mode = 0;
static int use_max_time = 0;

static int http_judge = 0;
static char http_baseurl[BUFFER_SIZE];

static char http_username[BUFFER_SIZE];
static char http_password[BUFFER_SIZE];
static int shm_run = 0;

static char record_call = 0;

//static int sleep_tmp;
#define ZOJ_COM
MYSQL *conn;

static char lang_ext[13][8] = { "c", "cc", "pas", "java", "rb", "sh", "py",
		"php", "pl", "cs", "m", "bas", "scm" };
//static char buf[BUFFER_SIZE];


long get_file_size(const char * filename) {
	struct stat f_stat;

	if (stat(filename, &f_stat) == -1) {
		return 0;
	}

	return (long) f_stat.st_size;
}

void write_log(const char *fmt, ...) {
	va_list ap;
	char buffer[4096];
	//      time_t          t = time(NULL);
	//int l;
	sprintf(buffer, "%s/log/client.log", oj_home);
	FILE *fp = fopen(buffer, "a+");
	if (fp == NULL) {
		fprintf(stderr, "openfile error!\n");
		system("pwd");
	}
	va_start(ap, fmt);
	//l = 
	vsprintf(buffer, fmt, ap);
	fprintf(fp, "%s\n", buffer);
	if (DEBUG)
		printf("%s\n", buffer);
	va_end(ap);
	fclose(fp);

}
int execute_cmd(const char * fmt, ...) {
	char cmd[BUFFER_SIZE];

	int ret = 0;
	va_list ap;

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	ret = system(cmd);
	va_end(ap);
	return ret;
}






#define FIFO_OUTPUT_CHANNEL_PREFIX "/var/tmp/oj_read_pipe_"  /* 宏定义，fifo路径 */
#define FIFO_INPUT_CHANNEL_PREFIX "/var/tmp/oj_write_pipe_"  /* 宏定义，fifo路径 */

int input_fifo, output_fifo;
char input_fifo_path[BUFFER_SIZE],output_fifo_path[BUFFER_SIZE];
//管道初始化
int init_fifo(char * sln_id) {
	strcpy(output_fifo_path, FIFO_OUTPUT_CHANNEL_PREFIX);
	strcpy(input_fifo_path, FIFO_INPUT_CHANNEL_PREFIX);
	strcat(output_fifo_path, sln_id);
	strcat(input_fifo_path, sln_id);
	if (DEBUG) {
		printf("pipe path: %s  %s\n", output_fifo_path, input_fifo_path);
	}

	if (access(output_fifo_path,F_OK)==-1){//如果不存在FIFO_OUTPUT_CHANNEL，则建立  
		if(mkfifo(output_fifo_path,0777)==-1) /* 创建命名管道，返回-1表示失败 */
		{
			printf("Can't create FIFO channel: %s", output_fifo_path);
			return 1;
		}
	}
	if (access(input_fifo_path,F_OK)==-1){//如果不存在FIFO_OUTPUT_CHANNEL，则建立  
		if(mkfifo(input_fifo_path,0777)==-1) /* 创建命名管道，返回-1表示失败 */
		{
			printf("Can't create FIFO channel: %s", input_fifo_path);
			return 1;
		}
	}
	//打开管道
	if((output_fifo=open(output_fifo_path,O_WRONLY))==-1) {
		printf("Can't open the FIFO: %s", output_fifo_path);
		return 1;
	}
	if((input_fifo=open(input_fifo_path,O_RDONLY))==-1) {
		printf("Can't open the FIFO: %s", input_fifo_path);
		return 1;
	}
	return 0;
}

void close_fifo() {
	close(output_fifo);  /* 关闭管道 */
	close(input_fifo);  /* 关闭管道 */
}

//往输出管道里写报文
void write_into_fifo(char * s) {
	//先写个所需传递的字符串的字节数
	int len = strlen(s);
	write( output_fifo, &len, sizeof(int) );
	//写入字符串
	write( output_fifo, s, len );
	if (DEBUG) {
		//printf("Write into pipe: %s\n",s);
	}
	//sleep(1);  /* sleep 1s */
}

//从输入管道读取报文
char * read_from_fifo() {
	char len_buf[10];
	char * read_buf;
	//先读取主体字符串的字节数
	read( input_fifo, len_buf, sizeof(int) );
	int * p_len = (int *)len_buf;
	if (DEBUG) {
		//printf("Length of Message from pipe: %d\n", *p_len);
	}
	//根据长度申请内存
	read_buf = (char *) malloc((*p_len) + 1);
	//读取主体字符串
	read( input_fifo, read_buf, *p_len );
	if (DEBUG) {
		//printf("Read from pipe: %s\n", read_buf);
	}
	//sleep(1); /* sleep 1s */
	return read_buf;
}
//删除命名管道文件
void delete_fifo_file() {
	execute_cmd("rm %s", output_fifo_path);
	execute_cmd("rm %s", input_fifo_path);
}
//发送交互结束标记
void send_end_flag() {
	char endFlag[BUFFER_SIZE] = "END_PIPE";
	write_into_fifo(endFlag);
}

void send_end_flag_debug(char * ext_debug_info) {
	char endFlag[BUFFER_SIZE] = "END_PIPE";
	strcat(endFlag, ext_debug_info);
	write_into_fifo(endFlag);
}





const int call_array_size = 512;
int call_counter[call_array_size] = { 0 };
static char LANG_NAME[BUFFER_SIZE];
void init_syscalls_limits(int lang) {
	int i;
	memset(call_counter, 0, sizeof(call_counter));
	if (DEBUG)
		write_log("init_call_counter:%d", lang);
	if (record_call) { // C & C++
		for (i = 0; i < call_array_size; i++) {
			call_counter[i] = 0;
		}
	} else if (lang <= 1) { // C & C++
		for (i = 0; i==0||LANG_CV[i]; i++) {
			call_counter[LANG_CV[i]] = HOJ_MAX_LIMIT;
		}
	} else if (lang == 2) { // Pascal
		for (i = 0; i==0||LANG_PV[i]; i++)
			call_counter[LANG_PV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 3) { // Java
		for (i = 0; i==0||LANG_JV[i]; i++)
			call_counter[LANG_JV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 4) { // Ruby
		for (i = 0; i==0||LANG_RV[i]; i++)
			call_counter[LANG_RV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 5) { // Bash
		for (i = 0; i==0||LANG_BV[i]; i++)
			call_counter[LANG_BV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 6) { // Python
		for (i = 0; i==0||LANG_YV[i]; i++)
			call_counter[LANG_YV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 7) { // php
		for (i = 0; i==0||LANG_PHV[i]; i++)
			call_counter[LANG_PHV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 8) { // perl
		for (i = 0; i==0||LANG_PLV[i]; i++)
			call_counter[LANG_PLV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 9) { // mono c#
		for (i = 0; i==0||LANG_CSV[i]; i++)
			call_counter[LANG_CSV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 10) { //objective c
		for (i = 0; i==0||LANG_OV[i]; i++)
			call_counter[LANG_OV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 11) { //free basic
		for (i = 0; i==0||LANG_BASICV[i]; i++)
			call_counter[LANG_BASICV[i]] = HOJ_MAX_LIMIT;
	} else if (lang == 12) { //scheme guile
		for (i = 0; i==0||LANG_SV[i]; i++)
			call_counter[LANG_SV[i]] = HOJ_MAX_LIMIT;
	}

}

int after_equal(char * c) {
	int i = 0;
	for (; c[i] != '\0' && c[i] != '='; i++)
		;
	return ++i;
}
void trim(char * c) {
	char buf[BUFFER_SIZE];
	char * start, *end;
	strcpy(buf, c);
	start = buf;
	while (isspace(*start))
		start++;
	end = start;
	while (!isspace(*end))
		end++;
	*end = '\0';
	strcpy(c, start);
}
bool read_buf(char * buf, const char * key, char * value) {
	if (strncmp(buf, key, strlen(key)) == 0) {
		strcpy(value, buf + after_equal(buf));
		trim(value);
		if (DEBUG)
			printf("%s\n", value);
		return 1;
	}
	return 0;
}
void read_int(char * buf, const char * key, int * value) {
	char buf2[BUFFER_SIZE];
	if (read_buf(buf, key, buf2))
		sscanf(buf2, "%d", value);

}
// read the configue file
void init_judge_conf() {
	FILE *fp = NULL;
	char buf[BUFFER_SIZE];
	host_name[0] = 0;
	user_name[0] = 0;
	password[0] = 0;
	db_name[0] = 0;
	port_number = 3306;
	max_running = 3;
	sleep_time = 3;
	strcpy(java_xms, "-Xms32m");
	strcpy(java_xmx, "-Xmx256m");
	sprintf(buf, "%s/etc/judge.conf", oj_home);
	fp = fopen("./etc/judge.conf", "r");
	if (fp != NULL) {
		while (fgets(buf, BUFFER_SIZE - 1, fp)) {
			read_buf(buf, "OJ_HOST_NAME", host_name);
			read_buf(buf, "OJ_USER_NAME", user_name);
			read_buf(buf, "OJ_PASSWORD", password);
			read_buf(buf, "OJ_DB_NAME", db_name);
			read_int(buf, "OJ_PORT_NUMBER", &port_number);
			read_int(buf, "OJ_JAVA_TIME_BONUS", &java_time_bonus);
			read_int(buf, "OJ_JAVA_MEMORY_BONUS", &java_memory_bonus);
			read_int(buf, "OJ_SIM_ENABLE", &sim_enable);
			read_buf(buf, "OJ_JAVA_XMS", java_xms);
			read_buf(buf, "OJ_JAVA_XMX", java_xmx);
			read_int(buf, "OJ_HTTP_JUDGE", &http_judge);
			read_buf(buf, "OJ_HTTP_BASEURL", http_baseurl);
			read_buf(buf, "OJ_HTTP_USERNAME", http_username);
			read_buf(buf, "OJ_HTTP_PASSWORD", http_password);
			read_int(buf, "OJ_OI_MODE", &oi_mode);
			read_int(buf, "OJ_SHM_RUN", &shm_run);
			read_int(buf, "OJ_USE_MAX_TIME", &use_max_time);

		}
		//fclose(fp);
	}
}

int isInFile(const char fname[]) {
	int l = strlen(fname);
	if (l <= 3 || strcmp(fname + l - 3, ".in") != 0)
		return 0;
	else
		return l - 3;
}

void find_next_nonspace(int & c1, int & c2, FILE *& f1, FILE *& f2, int & ret) {
	// Find the next non-space character or \n.
	while ((isspace(c1)) || (isspace(c2))) {
		if (c1 != c2) {
			if (c2 == EOF) {
				do {
					c1 = fgetc(f1);
				} while (isspace(c1));
				continue;
			} else if (c1 == EOF) {
				do {
					c2 = fgetc(f2);
				} while (isspace(c2));
				continue;
			} else if ((c1 == '\r' && c2 == '\n')) {
				c1 = fgetc(f1);
			} else if ((c2 == '\r' && c1 == '\n')) {
				c2 = fgetc(f2);
			} else {
				if (DEBUG)
					printf("%d=%c\t%d=%c", c1, c1, c2, c2);
				;
				ret = OJ_PE;
			}
		}
		if (isspace(c1)) {
			c1 = fgetc(f1);
		}
		if (isspace(c2)) {
			c2 = fgetc(f2);
		}
	}
}

/***
 int compare_diff(const char *file1,const char *file2){
 char diff[1024];
 sprintf(diff,"diff -q -B -b -w --strip-trailing-cr %s %s",file1,file2);
 int d=system(diff);
 if (d) return OJ_WA;
 sprintf(diff,"diff -q -B --strip-trailing-cr %s %s",file1,file2);
 int p=system(diff);
 if (p) return OJ_PE;
 else return OJ_AC;

 }
 */
const char * getFileNameFromPath(const char * path) {
	for (int i = strlen(path); i >= 0; i--) {
		if (path[i] == '/')
			return &path[i];
	}
	return path;
}
void make_diff_out(FILE *f1, FILE *f2, int c1, int c2, const char * path) {
	FILE *out;
	char buf[45];
	out = fopen("diff.out", "a+");
	fprintf(out, "=================%s\n", getFileNameFromPath(path));
	fprintf(out, "Right:\n%c", c1);
	if (fgets(buf, 44, f1)) {
		fprintf(out, "%s", buf);
	}
	fprintf(out, "\n-----------------\n");
	fprintf(out, "Your:\n%c", c2);
	if (fgets(buf, 44, f2)) {
		fprintf(out, "%s", buf);
	}
	fprintf(out, "\n=================\n");
	fclose(out);
}

/*
 * translated from ZOJ judger r367
 * http://code.google.com/p/zoj/source/browse/trunk/judge_client/client/text_checker.cc#25
 *
 */
int compare_zoj(const char *file1, const char *file2) {
	int ret = OJ_AC;
	int c1, c2;
	FILE * f1, *f2;
	f1 = fopen(file1, "r");
	f2 = fopen(file2, "r");
	if (!f1 || !f2) {
		ret = OJ_RE;
	} else
		for (;;) {
			// Find the first non-space character at the beginning of line.
			// Blank lines are skipped.
			c1 = fgetc(f1);
			c2 = fgetc(f2);
			find_next_nonspace(c1, c2, f1, f2, ret);
			// Compare the current line.
			for (;;) {
				// Read until 2 files return a space or 0 together.
				while ((!isspace(c1) && c1) || (!isspace(c2) && c2)) {
					if (c1 == EOF && c2 == EOF) {
						goto end;
					}
					if (c1 == EOF || c2 == EOF) {
						break;
					}
					if (c1 != c2) {
						// Consecutive non-space characters should be all exactly the same
						ret = OJ_WA;
						goto end;
					}
					c1 = fgetc(f1);
					c2 = fgetc(f2);
				}
				find_next_nonspace(c1, c2, f1, f2, ret);
				if (c1 == EOF && c2 == EOF) {
					goto end;
				}
				if (c1 == EOF || c2 == EOF) {
					ret = OJ_WA;
					goto end;
				}

				if ((c1 == '\n' || !c1) && (c2 == '\n' || !c2)) {
					break;
				}
			}
		}
	end: if (ret == OJ_WA)
		make_diff_out(f1, f2, c1, c2, file1);
	if (f1)
		fclose(f1);
	if (f2)
		fclose(f2);
	return ret;
}

void delnextline(char s[]) {
	int L;
	L = strlen(s);
	while (L > 0 && (s[L - 1] == '\n' || s[L - 1] == '\r'))
		s[--L] = 0;
}

int compare(const char *file1, const char *file2) {
#ifdef ZOJ_COM
	//compare ported and improved from zoj don't limit file size
	return compare_zoj(file1, file2);
#endif
#ifndef ZOJ_COM
	//the original compare from the first version of hustoj has file size limit
	//and waste memory
	FILE *f1,*f2;
	char *s1,*s2,*p1,*p2;
	int PEflg;
	s1=new char[STD_F_LIM+512];
	s2=new char[STD_F_LIM+512];
	if (!(f1=fopen(file1,"r")))
	return OJ_AC;
	for (p1=s1;EOF!=fscanf(f1,"%s",p1);)
	while (*p1) p1++;
	fclose(f1);
	if (!(f2=fopen(file2,"r")))
	return OJ_RE;
	for (p2=s2;EOF!=fscanf(f2,"%s",p2);)
	while (*p2) p2++;
	fclose(f2);
	if (strcmp(s1,s2)!=0) {
		//              printf("A:%s\nB:%s\n",s1,s2);
		delete[] s1;
		delete[] s2;

		return OJ_WA;
	} else {
		f1=fopen(file1,"r");
		f2=fopen(file2,"r");
		PEflg=0;
		while (PEflg==0 && fgets(s1,STD_F_LIM,f1) && fgets(s2,STD_F_LIM,f2)) {
			delnextline(s1);
			delnextline(s2);
			if (strcmp(s1,s2)==0) continue;
			else PEflg=1;
		}
		delete [] s1;
		delete [] s2;
		fclose(f1);fclose(f2);
		if (PEflg) return OJ_PE;
		else return OJ_AC;
	}
#endif
}

FILE * read_cmd_output(const char * fmt, ...) {
	char cmd[BUFFER_SIZE];

	FILE * ret = NULL;
	va_list ap;

	va_start(ap, fmt);
	vsprintf(cmd, fmt, ap);
	va_end(ap);
	if (DEBUG)
		printf("%s\n", cmd);
	ret = popen(cmd, "r");

	return ret;
}
bool check_login() {
	const char * cmd =
			" wget --post-data=\"checklogin=1\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	int ret = 0;
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);
	fscanf(fjobs, "%d", &ret);
	pclose(fjobs);

	return ret;
}
void login() {
	if (!check_login()) {
		const char * cmd =
				"wget --post-data=\"user_id=%s&password=%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/login.php\"";
		FILE * fjobs = read_cmd_output(cmd, http_username, http_password,
				http_baseurl);
		//fscanf(fjobs,"%d",&ret);
		pclose(fjobs);
	}

}
/* write result back to database */
void _update_solution_mysql(int solution_id, int result, int time, int memory,
		int sim, int sim_s_id, double pass_rate) {
	char sql[BUFFER_SIZE];
	if (oi_mode) {
		sprintf(sql,
				"UPDATE solution SET result=%d,time=%d,memory=%d,judgetime=NOW(),pass_rate=%f WHERE solution_id=%d LIMIT 1%c",
				result, time, memory, pass_rate, solution_id, 0);
	} else {
		sprintf(sql,
				"UPDATE solution SET result=%d,time=%d,memory=%d,judgetime=NOW() WHERE solution_id=%d LIMIT 1%c",
				result, time, memory, solution_id, 0);
	}
	//      printf("sql= %s\n",sql);
	if (mysql_real_query(conn, sql, strlen(sql))) {
		//              printf("..update failed! %s\n",mysql_error(conn));
	}
	if (sim) {
		sprintf(sql,
				"insert into sim(s_id,sim_s_id,sim) values(%d,%d,%d) on duplicate key update  sim_s_id=%d,sim=%d",
				solution_id, sim_s_id, sim, sim_s_id, sim);
		//      printf("sql= %s\n",sql);
		if (mysql_real_query(conn, sql, strlen(sql))) {
			//              printf("..update failed! %s\n",mysql_error(conn));
		}

	}

}
void _update_solution_http(int solution_id, int result, int time, int memory,
		int sim, int sim_s_id, double pass_rate) {
	const char * cmd =
			" wget --post-data=\"update_solution=1&sid=%d&result=%d&time=%d&memory=%d&sim=%d&simid=%d&pass_rate=%f\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, solution_id, result, time, memory, sim,
			sim_s_id, pass_rate, http_baseurl);
	//fscanf(fjobs,"%d",&ret);
	pclose(fjobs);
}

void _update_solution_named_pipe(int solution_id, int result, int time, int memory,int sim, int sim_s_id,double pass_rate) {
	char query[200];
	memset(query, 0, sizeof(query));
	char query_tpl[]="updateSolution|||result=%d&time=%d&memory=%d&sim=%d&simid=%d&pass_rate=%f";
	sprintf(query, query_tpl, result,  time,  memory, sim, sim_s_id, pass_rate);
	
	write_into_fifo(query);
	char * res = read_from_fifo();
	free(res);

}

void update_solution(int solution_id, int result, int time, int memory, int sim,
		int sim_s_id, double pass_rate) {
	if (result == OJ_TL && memory == 0)
		result = OJ_ML;
	_update_solution_named_pipe( solution_id,  result,  time,  memory, sim, sim_s_id,pass_rate);
	/*
	if (http_judge) {
		_update_solution_http(solution_id, result, time, memory, sim, sim_s_id,
				pass_rate);
	} else {
		_update_solution_mysql(solution_id, result, time, memory, sim, sim_s_id,
				pass_rate);
	}
	*/
}
/* write compile error message back to database */
void _addceinfo_mysql(int solution_id) {
	char sql[(1 << 16)], *end;
	char ceinfo[(1 << 16)], *cend;
	FILE *fp = fopen("ce.txt", "r");
	snprintf(sql, (1 << 16) - 1, "DELETE FROM compileinfo WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	cend = ceinfo;
	while (fgets(cend, 1024, fp)) {
		cend += strlen(cend);
		if (cend - ceinfo > 40000)
			break;
	}
	cend = 0;
	end = sql;
	strcpy(end, "INSERT INTO compileinfo VALUES(");
	end += strlen(sql);
	*end++ = '\'';
	end += sprintf(end, "%d", solution_id);
	*end++ = '\'';
	*end++ = ',';
	*end++ = '\'';
	end += mysql_real_escape_string(conn, end, ceinfo, strlen(ceinfo));
	*end++ = '\'';
	*end++ = ')';
	*end = 0;
	//      printf("%s\n",ceinfo);
	if (mysql_real_query(conn, sql, end - sql))
		printf("%s\n", mysql_error(conn));
	fclose(fp);
}
// urlencoded function copied from http://www.geekhideout.com/urlcode.shtml
/* Converts a hex character to its integer value */
char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
	char *pstr = str, *buf = (char *) malloc(strlen(str) * 3 + 1), *pbuf = buf;
	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.'
				|| *pstr == '~')
			*pbuf++ = *pstr;
		else if (*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(
					*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

void _addceinfo_http(int solution_id) {

	char ceinfo[(1 << 16)], *cend;
	char * ceinfo_encode;
	FILE *fp = fopen("ce.txt", "r");

	cend = ceinfo;
	while (fgets(cend, 1024, fp)) {
		cend += strlen(cend);
		if (cend - ceinfo > 40000)
			break;
	}
	fclose(fp);
	ceinfo_encode = url_encode(ceinfo);
	FILE * ce = fopen("ce.post", "w");
	fprintf(ce, "addceinfo=1&sid=%d&ceinfo=%s", solution_id, ceinfo_encode);
	fclose(ce);
	free(ceinfo_encode);

	const char * cmd =
			" wget --post-file=\"ce.post\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);
	//fscanf(fjobs,"%d",&ret);
	pclose(fjobs);

}

void _addceinfo_named_pipe(int solution_id) {
    char ceinfo[(1 << 16)], *cend;
    FILE *fp = fopen("ce.txt", "r");
    cend = ceinfo;
	strcpy(cend, "addCeInfo|||");
	cend += strlen(cend);
    while (fgets(cend, 1024, fp))
    {
        cend += strlen(cend);
        if (cend - ceinfo > 40000)
            break;
    }
	
	fclose(fp);

	write_into_fifo(ceinfo);
	char * ans = read_from_fifo();
	free(ans);

    
}

void addceinfo(int solution_id) {
	_addceinfo_named_pipe(solution_id);
	/*
	if (http_judge) {
		_addceinfo_http(solution_id);
	} else {
		_addceinfo_mysql(solution_id);
	}
	*/
}
/* write runtime error message back to database */
void _addreinfo_mysql(int solution_id, const char * filename) {
	char sql[(1 << 16)], *end;
	char reinfo[(1 << 16)], *rend;
	FILE *fp = fopen(filename, "r");
	snprintf(sql, (1 << 16) - 1, "DELETE FROM runtimeinfo WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	rend = reinfo;
	while (fgets(rend, 1024, fp)) {
		rend += strlen(rend);
		if (rend - reinfo > 40000)
			break;
	}
	rend = 0;
	end = sql;
	strcpy(end, "INSERT INTO runtimeinfo VALUES(");
	end += strlen(sql);
	*end++ = '\'';
	end += sprintf(end, "%d", solution_id);
	*end++ = '\'';
	*end++ = ',';
	*end++ = '\'';
	end += mysql_real_escape_string(conn, end, reinfo, strlen(reinfo));
	*end++ = '\'';
	*end++ = ')';
	*end = 0;
	//      printf("%s\n",ceinfo);
	if (mysql_real_query(conn, sql, end - sql))
		printf("%s\n", mysql_error(conn));
	fclose(fp);
}

void _addreinfo_http(int solution_id, const char * filename) {

	char reinfo[(1 << 16)], *rend;
	char * reinfo_encode;
	FILE *fp = fopen(filename, "r");

	rend = reinfo;
	while (fgets(rend, 1024, fp)) {
		rend += strlen(rend);
		if (rend - reinfo > 40000)
			break;
	}
	fclose(fp);
	reinfo_encode = url_encode(reinfo);
	FILE * re = fopen("re.post", "w");
	fprintf(re, "addreinfo=1&sid=%d&reinfo=%s", solution_id, reinfo_encode);
	fclose(re);
	free(reinfo_encode);

	const char * cmd =
			" wget --post-file=\"re.post\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, http_baseurl);
	//fscanf(fjobs,"%d",&ret);
	pclose(fjobs);

}

void _addreinfo_named_pipe(int solution_id,const char * filename) {

	char reinfo[(1 << 16)], *rend;
    FILE *fp = fopen(filename, "r");

    rend = reinfo;
	strcpy(rend, "addReInfo|||");
	rend += strlen(rend);
    while (fgets(rend, 1024, fp))
    {
        rend += strlen(rend);
        if (rend - reinfo > 40000)
            break;
    }
    fclose(fp);

	write_into_fifo(reinfo);
	char * ans = read_from_fifo();
	free(ans);

}

void addreinfo(int solution_id) {
	_addreinfo_named_pipe(solution_id,"error.out");
	/*
	if (http_judge) {
		_addreinfo_http(solution_id, "error.out");
	} else {
		_addreinfo_mysql(solution_id, "error.out");
	}
	*/
}

void adddiffinfo(int solution_id) {
	_addreinfo_named_pipe(solution_id,"diff.out");
	/*
	if (http_judge) {
		_addreinfo_http(solution_id, "diff.out");
	} else {
		_addreinfo_mysql(solution_id, "diff.out");
	}
	*/
}
void addcustomout(int solution_id) {
	_addreinfo_named_pipe(solution_id, "user.out");
	/*
	if (http_judge) {
		_addreinfo_http(solution_id, "user.out");
	} else {
		_addreinfo_mysql(solution_id, "user.out");
	}
	*/
}

void _update_user_mysql(char * user_id) {
	char sql[BUFFER_SIZE];
	sprintf(sql,
			"UPDATE `users` SET `solved`=(SELECT count(DISTINCT `problem_id`) FROM `solution` WHERE `user_id`=\'%s\' AND `result`=\'4\') WHERE `user_id`=\'%s\'",
			user_id, user_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
	sprintf(sql,
			"UPDATE `users` SET `submit`=(SELECT count(*) FROM `solution` WHERE `user_id`=\'%s\') WHERE `user_id`=\'%s\'",
			user_id, user_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
}
void _update_user_http(char * user_id) {

	const char * cmd =
			" wget --post-data=\"updateuser=1&user_id=%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, user_id, http_baseurl);
	//fscanf(fjobs,"%d",&ret);
	pclose(fjobs);
}
void update_user(char * user_id) {
	if (http_judge) {
		_update_user_http(user_id);
	} else {
		_update_user_mysql(user_id);
	}
}

void _update_problem_http(int pid) {
	const char * cmd =
			" wget --post-data=\"updateproblem=1&pid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, pid, http_baseurl);
	//fscanf(fjobs,"%d",&ret);
	pclose(fjobs);
}
void _update_problem_mysql(int p_id) {
	char sql[BUFFER_SIZE];
	sprintf(sql,
			"UPDATE `problem` SET `accepted`=(SELECT count(*) FROM `solution` WHERE `problem_id`=\'%d\' AND `result`=\'4\') WHERE `problem_id`=\'%d\'",
			p_id, p_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
	sprintf(sql,
			"UPDATE `problem` SET `submit`=(SELECT count(*) FROM `solution` WHERE `problem_id`=\'%d\') WHERE `problem_id`=\'%d\'",
			p_id, p_id);
	if (mysql_real_query(conn, sql, strlen(sql)))
		write_log(mysql_error(conn));
}
void update_problem(int pid) {
	if (http_judge) {
		_update_problem_http(pid);
	} else {
		_update_problem_mysql(pid);
	}
}
int compile(int lang) {
	int pid;

	const char * CP_C[] = { "gcc", "Main.c", "-o", "Main", "-fno-asm", "-Wall",
			"-lm", "--static", "-std=c99", "-DONLINE_JUDGE", NULL };
	const char * CP_X[] = { "g++", "Main.cc", "-o", "Main", "-fno-asm", "-Wall",
			"-lm", "--static", "-std=c++0x", "-DONLINE_JUDGE", NULL };
	const char * CP_P[] =
			{ "fpc", "Main.pas", "-O2", "-Co", "-Ct", "-Ci", NULL };
//      const char * CP_J[] = { "javac", "-J-Xms32m", "-J-Xmx256m","-encoding","UTF-8", "Main.java",NULL };

	const char * CP_R[] = { "ruby", "-c", "Main.rb", NULL };
	const char * CP_B[] = { "chmod", "+rx", "Main.sh", NULL };
	const char * CP_Y[] = { "python", "-c",
			"import py_compile; py_compile.compile(r'Main.py')", NULL };
	const char * CP_PH[] = { "php", "-l", "Main.php", NULL };
	const char * CP_PL[] = { "perl", "-c", "Main.pl", NULL };
	const char * CP_CS[] = { "gmcs", "-warn:0", "Main.cs", NULL };
	const char * CP_OC[] = { "gcc", "-o", "Main", "Main.m",
			"-fconstant-string-class=NSConstantString", "-I",
			"/usr/include/GNUstep/", "-L", "/usr/lib/GNUstep/Libraries/",
			"-lobjc", "-lgnustep-base", NULL };
	const char * CP_BS[] = { "fbc", "Main.bas", NULL };
	char javac_buf[7][16];
	char *CP_J[7];

	for (int i = 0; i < 7; i++)
		CP_J[i] = javac_buf[i];

	sprintf(CP_J[0], "javac");
	sprintf(CP_J[1], "-J%s", java_xms);
	sprintf(CP_J[2], "-J%s", java_xmx);
	sprintf(CP_J[3], "-encoding");
	sprintf(CP_J[4], "UTF-8");
	sprintf(CP_J[5], "Main.java");
	CP_J[6] = (char *) NULL;

	pid = fork();
	if (pid == 0) {
		struct rlimit LIM;
		LIM.rlim_max = 60;
		LIM.rlim_cur = 60;
		setrlimit(RLIMIT_CPU, &LIM);
		alarm(60);
		LIM.rlim_max = 100 * STD_MB;
		LIM.rlim_cur = 100 * STD_MB;
		setrlimit(RLIMIT_FSIZE, &LIM);

		if(lang==3){
		   LIM.rlim_max = STD_MB << 11;
		   LIM.rlim_cur = STD_MB << 11;	
                }else{
		   LIM.rlim_max = STD_MB << 10;
		   LIM.rlim_cur = STD_MB << 10;
		}
		setrlimit(RLIMIT_AS, &LIM);
		if (lang != 2 && lang != 11) {
			freopen("ce.txt", "w", stderr);
			//freopen("/dev/null", "w", stdout);
		} else {
			freopen("ce.txt", "w", stdout);
		}
		execute_cmd("chown judge *");
		while(setgid(1536)!=0) sleep(1);
        while(setuid(1536)!=0) sleep(1);
        while(setresuid(1536, 1536, 1536)!=0) sleep(1);

		switch (lang) {
		case 0:
			execvp(CP_C[0], (char * const *) CP_C);
			break;
		case 1:
			execvp(CP_X[0], (char * const *) CP_X);
			break;
		case 2:
			execvp(CP_P[0], (char * const *) CP_P);
			break;
		case 3:
			execvp(CP_J[0], (char * const *) CP_J);
			break;
		case 4:
			execvp(CP_R[0], (char * const *) CP_R);
			break;
		case 5:
			execvp(CP_B[0], (char * const *) CP_B);
			break;
		case 6:
			execvp(CP_Y[0], (char * const *) CP_Y);
			break;
		case 7:
			execvp(CP_PH[0], (char * const *) CP_PH);
			break;
		case 8:
			execvp(CP_PL[0], (char * const *) CP_PL);
			break;
		case 9:
			execvp(CP_CS[0], (char * const *) CP_CS);
			break;

		case 10:
			execvp(CP_OC[0], (char * const *) CP_OC);
			break;
		case 11:
			execvp(CP_BS[0], (char * const *) CP_BS);
			break;
		default:
			printf("nothing to do!\n");
		}
		if (DEBUG)
			printf("compile end!\n");
		//exit(!system("cat ce.txt"));
		exit(0);
	} else {
		int status = 0;

		waitpid(pid, &status, 0);
		if (lang > 3 && lang < 7)
			status = get_file_size("ce.txt");
		if (DEBUG)
			printf("status=%d\n", status);
		return status;
	}

}
/*
 int read_proc_statm(int pid){
 FILE * pf;
 char fn[4096];
 int ret;
 sprintf(fn,"/proc/%d/statm",pid);
 pf=fopen(fn,"r");
 fscanf(pf,"%d",&ret);
 fclose(pf);
 return ret;
 }
 */
int get_proc_status(int pid, const char * mark) {
	FILE * pf;
	char fn[BUFFER_SIZE], buf[BUFFER_SIZE];
	int ret = 0;
	sprintf(fn, "/proc/%d/status", pid);
	pf = fopen(fn, "r");
	int m = strlen(mark);
	while (pf && fgets(buf, BUFFER_SIZE - 1, pf)) {

		buf[strlen(buf) - 1] = 0;
		if (strncmp(buf, mark, m) == 0) {
			sscanf(buf + m + 1, "%d", &ret);
		}
	}
	if (pf)
		fclose(pf);
	return ret;
}
int init_mysql_conn() {

	conn = mysql_init(NULL);
	//mysql_real_connect(conn,host_name,user_name,password,db_name,port_number,0,0);
	const char timeout = 30;
	mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

	if (!mysql_real_connect(conn, host_name, user_name, password, db_name,
			port_number, 0, 0)) {
		write_log("%s", mysql_error(conn));
		return 0;
	}
	const char * utf8sql = "set names utf8";
	if (mysql_real_query(conn, utf8sql, strlen(utf8sql))) {
		write_log("%s", mysql_error(conn));
		return 0;
	}
	return 1;
}
void _get_solution_mysql(int solution_id, char * work_dir, int lang) {
	char sql[BUFFER_SIZE], src_pth[BUFFER_SIZE];
	// get the source code
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(sql, "SELECT source FROM source_code WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);

	// create the src file
	sprintf(src_pth, "Main.%s", lang_ext[lang]);
	if (DEBUG)
		printf("Main=%s", src_pth);
	FILE *fp_src = fopen(src_pth, "w");
	fprintf(fp_src, "%s", row[0]);
	mysql_free_result(res);
	fclose(fp_src);
}
void _get_solution_http(int solution_id, char * work_dir, int lang) {
	char src_pth[BUFFER_SIZE];

	// create the src file
	sprintf(src_pth, "Main.%s", lang_ext[lang]);
	if (DEBUG)
		printf("Main=%s", src_pth);

	//login();

	const char * cmd2 =
			"wget --post-data=\"getsolution=1&sid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O %s \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd2, solution_id, src_pth, http_baseurl);

	pclose(pout);

}
void _get_solution_named_pipe(int solution_id, char * work_dir, int lang) {
	
	char flagSourceCode[]="sourceCode";
	write_into_fifo(flagSourceCode);
	char * ansSourceCode = read_from_fifo();
	if (DEBUG) {
		printf("ansSourceCode: %s\n", ansSourceCode);
	}
	
	char  src_pth[BUFFER_SIZE];
    // create the src file
    sprintf(src_pth, "Main.%s", lang_ext[lang]);
    if (DEBUG)
        printf("Main=%s", src_pth);
	FILE *fp_src = fopen(src_pth, "w");
    fprintf(fp_src, "%s", ansSourceCode);
    fclose(fp_src);

	free(ansSourceCode);
}
void get_solution(int solution_id, char * work_dir, int lang) {
	_get_solution_named_pipe(solution_id,  work_dir, lang);
	/*
	if (http_judge) {
		_get_solution_http(solution_id, work_dir, lang);
	} else {
		_get_solution_mysql(solution_id, work_dir, lang);
	}
	*/
}

void _get_custominput_mysql(int solution_id, char * work_dir) {
	char sql[BUFFER_SIZE], src_pth[BUFFER_SIZE];
	// get the source code
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(sql, "SELECT input_text FROM custominput WHERE solution_id=%d",
			solution_id);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	if (row != NULL) {

		// create the src file
		sprintf(src_pth, "data.in");
		FILE *fp_src = fopen(src_pth, "w");
		fprintf(fp_src, "%s", row[0]);
		fclose(fp_src);

	}
	mysql_free_result(res);
}
void _get_custominput_http(int solution_id, char * work_dir) {
	char src_pth[BUFFER_SIZE];

	// create the src file
	sprintf(src_pth, "data.in");

	//login();

	const char * cmd2 =
			"wget --post-data=\"getcustominput=1&sid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O %s \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd2, solution_id, src_pth, http_baseurl);

	pclose(pout);

}
void _get_custominput_named_pipe(int solution_id, char * work_dir) {

	char flagCustomInput[]="customInput";
	write_into_fifo(flagCustomInput);
	char * ansCustomInput = read_from_fifo();
	if (DEBUG) {
		printf("ansCustomInput: %s\n", ansCustomInput);
	}

	char  src_pth[BUFFER_SIZE];
    // create the src file
    sprintf(src_pth, "data.in");
	FILE *fp_src = fopen(src_pth, "w");

	fprintf(fp_src, "%s", ansCustomInput);
	fclose(fp_src);

	free(ansCustomInput);
}
void get_custominput(int solution_id, char * work_dir) {
	_get_custominput_named_pipe(solution_id,  work_dir);
	/*
	if (http_judge) {
		_get_custominput_http(solution_id, work_dir);
	} else {
		_get_custominput_mysql(solution_id, work_dir);
	}
	*/
}

void _get_solution_info_mysql(int solution_id, int & p_id, char * user_id,
		int & lang) {

	MYSQL_RES *res;
	MYSQL_ROW row;

	char sql[BUFFER_SIZE];
	// get the problem id and user id from Table:solution
	sprintf(sql,
			"SELECT problem_id, user_id, language FROM solution where solution_id=%d",
			solution_id);
	//printf("%s\n",sql);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	p_id = atoi(row[0]);
	strcpy(user_id, row[1]);
	lang = atoi(row[2]);
	mysql_free_result(res);
}

void _get_solution_info_http(int solution_id, int & p_id, char * user_id,
		int & lang) {

	login();

	const char * cmd =
			"wget --post-data=\"getsolutioninfo=1&sid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd, solution_id, http_baseurl);
	fscanf(pout, "%d", &p_id);
	fscanf(pout, "%s", user_id);
	fscanf(pout, "%d", &lang);
	pclose(pout);

}

void _get_solution_info_named_pipe(int solution_id, int & p_id, char * user_id, int & lang) {
	char flagProblemId[]="problemId";
	char flagUserId[]="userId";
	char flagLang[]="lang";
	
	write_into_fifo(flagProblemId);
	char * ansProblemId = read_from_fifo();
	write_into_fifo(flagUserId);
	char * ansUserId = read_from_fifo();
	write_into_fifo(flagLang);
	char * ansLang = read_from_fifo();
	if (DEBUG) {
		printf("ansProblemId: %s\n", ansProblemId);
		printf("ansUserId: %s\n", ansUserId);
		printf("ansLang: %s\n", ansLang);
	}

	p_id = atoi(ansProblemId);
	strcpy(user_id, ansUserId);
	lang = atoi(ansLang);
	
	free(ansProblemId);
	free(ansUserId);
	free(ansLang);
}

void get_solution_info(int solution_id, int & p_id, char * user_id,
		int & lang) {
	_get_solution_info_named_pipe(solution_id,p_id,user_id,lang);
	/*
	if (http_judge) {
		_get_solution_info_http(solution_id, p_id, user_id, lang);
	} else {
		_get_solution_info_mysql(solution_id, p_id, user_id, lang);
	}
	*/
}

void _get_problem_info_mysql(int p_id, int & time_lmt, int & mem_lmt,
		int & isspj) {
	// get the problem info from Table:problem
	char sql[BUFFER_SIZE];
	MYSQL_RES *res;
	MYSQL_ROW row;
	sprintf(sql,
			"SELECT time_limit,memory_limit,spj FROM problem where problem_id=%d",
			p_id);
	mysql_real_query(conn, sql, strlen(sql));
	res = mysql_store_result(conn);
	row = mysql_fetch_row(res);
	time_lmt = atoi(row[0]);
	mem_lmt = atoi(row[1]);
	isspj = (row[2][0] == '1');
	mysql_free_result(res);
}

void _get_problem_info_http(int p_id, int & time_lmt, int & mem_lmt,
		int & isspj) {
	//login();

	const char * cmd =
			"wget --post-data=\"getprobleminfo=1&pid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * pout = read_cmd_output(cmd, p_id, http_baseurl);
	fscanf(pout, "%d", &time_lmt);
	fscanf(pout, "%d", &mem_lmt);
	fscanf(pout, "%d", &isspj);
	pclose(pout);
}

void _get_problem_info_named_pipe(int p_id, int & time_lmt, int & mem_lmt, int & isspj) {
	char flagTimeLimit[]="timeLimit";
	char flagMemLimit[]="memLimit";
	char flagSpj[]="spj";
	
	write_into_fifo(flagTimeLimit);
	char * ansTimeLimit = read_from_fifo();
	write_into_fifo(flagMemLimit);
	char * ansMemLimit = read_from_fifo();
	write_into_fifo(flagSpj);
	char * ansSpj = read_from_fifo();
	if (DEBUG) {
		printf("ansTimeLimit: %s\n", ansTimeLimit);
		printf("ansMemLimit: %s\n", ansMemLimit);
		printf("ansSpj: %s\n", ansSpj);
	}

	time_lmt = atoi(ansTimeLimit);
	mem_lmt = atoi(ansMemLimit);
	isspj = atoi(ansSpj);
	
	free(ansTimeLimit);
	free(ansMemLimit);
	free(ansSpj);
}

void get_problem_info(int p_id, int & time_lmt, int & mem_lmt, int & isspj) {
	_get_problem_info_named_pipe(p_id,time_lmt,mem_lmt,isspj);
	/*
	if (http_judge) {
		_get_problem_info_http(p_id, time_lmt, mem_lmt, isspj);
	} else {
		_get_problem_info_mysql(p_id, time_lmt, mem_lmt, isspj);
	}
	*/
}

void prepare_files(char * filename, int namelen, char * infile, int & p_id,
		char * work_dir, char * outfile, char * userfile, int solution_id) {
	//              printf("ACflg=%d %d check a file!\n",ACflg,solution_id);

	char fname[BUFFER_SIZE];
	strncpy(fname, filename, namelen);
	fname[namelen] = 0;
	sprintf(infile, "%s/data/%d/%s.in", oj_home, p_id, fname);
	execute_cmd("/bin/cp %s %s/data.in", infile, work_dir);
	execute_cmd("/bin/cp %s/data/%d/*.dic %s/", oj_home, p_id, work_dir);

	sprintf(outfile, "%s/data/%d/%s.out", oj_home, p_id, fname);
	sprintf(userfile, "%s/run%d/user.out", oj_home, solution_id);
}

void copy_shell_runtime(char * work_dir) {

	execute_cmd("/bin/mkdir %s/lib", work_dir);
	execute_cmd("/bin/mkdir %s/lib64", work_dir);
	execute_cmd("/bin/mkdir %s/bin", work_dir);
	execute_cmd("/bin/cp /lib/* %s/lib/", work_dir);
	execute_cmd("/bin/cp -a /lib/i386-linux-gnu %s/lib/", work_dir);
	execute_cmd("/bin/cp -a /lib/x86_64-linux-gnu %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib64/* %s/lib64/", work_dir);
	execute_cmd("/bin/cp -a /lib32 %s/", work_dir);
	execute_cmd("/bin/cp /bin/busybox %s/bin/", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/sh", work_dir);
	execute_cmd("/bin/cp /bin/bash %s/bin/bash", work_dir);

}
void copy_objc_runtime(char * work_dir) {
	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/proc", work_dir);
	execute_cmd("/bin/mount -o bind /proc %s/proc", work_dir);
	execute_cmd("/bin/mkdir -p %s/lib/", work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libdbus-1.so.3                          %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libgcc_s.so.1                           %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libgcrypt.so.11                         %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libgpg-error.so.0                       %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/libz.so.1                               %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libc.so.6                 %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libdl.so.2                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libm.so.6                 %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libnsl.so.1               %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/libpthread.so.0           %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /lib/tls/i686/cmov/librt.so.1                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libavahi-client.so.3                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libavahi-common.so.3                %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libdns_sd.so.1                      %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libffi.so.5                         %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libgnustep-base.so.1.19             %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libgnutls.so.26                     %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libobjc.so.2                        %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libtasn1.so.3                       %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libxml2.so.2                        %s/lib/ ",
			work_dir);
	execute_cmd(
			"/bin/cp -aL /usr/lib/libxslt.so.1                        %s/lib/ ",
			work_dir);

}
void copy_bash_runtime(char * work_dir) {
	//char cmd[BUFFER_SIZE];
	//const char * ruby_run="/usr/bin/ruby";
	copy_shell_runtime(work_dir);
	execute_cmd("/bin/cp `which bc`  %s/bin/", work_dir);
	execute_cmd("busybox dos2unix Main.sh", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/grep", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/awk", work_dir);
	execute_cmd("/bin/cp /bin/sed %s/bin/sed", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/cut", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/sort", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/join", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/wc", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/tr", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/dc", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/dd", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/cat", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/tail", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/head", work_dir);
	execute_cmd("/bin/ln -s /bin/busybox %s/bin/xargs", work_dir);
	execute_cmd("chmod +rx %s/Main.sh", work_dir);

}
void copy_ruby_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/usr/lib", work_dir);
	execute_cmd("/bin/cp /usr/lib/libruby* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp -a /usr/lib/ruby %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/bin/ruby* %s/", work_dir);

}
void copy_guile_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/proc", work_dir);
	execute_cmd("/bin/mount -o bind /proc %s/proc", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/share", work_dir);
	execute_cmd("/bin/cp -a /usr/share/guile %s/usr/share/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libguile* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libgc* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libffi* %s/usr/lib/",
			work_dir);
	execute_cmd("/bin/cp /usr/lib/i386-linux-gnu/libunistring* %s/usr/lib/",
			work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libgmp* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libgmp* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libltdl* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libltdl* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/bin/guile* %s/", work_dir);

}

void copy_python_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/include", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib", work_dir);
	execute_cmd("/bin/cp /usr/bin/python* %s/", work_dir);
	execute_cmd("/bin/cp -a /usr/lib/python* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp -a /usr/include/python* %s/usr/include/", work_dir);
	execute_cmd("/bin/cp -a /usr/lib/libpython* %s/usr/lib/", work_dir);

}
void copy_php_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/usr/lib", work_dir);
	execute_cmd("/bin/cp /usr/lib/libedit* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libdb* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libgssapi_krb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libkrb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libk5crypto* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libedit* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libdb* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libgssapi_krb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libkrb5* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/*/libk5crypto* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/libxml2* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/lib/x86_64-linux-gnu/libxml2.so* %s/usr/lib/",
			work_dir);
	execute_cmd("/bin/cp /usr/bin/php* %s/", work_dir);
	execute_cmd("chmod +rx %s/Main.php", work_dir);

}
void copy_perl_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/usr/lib", work_dir);
	execute_cmd("/bin/cp /usr/lib/libperl* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /usr/bin/perl* %s/", work_dir);

}
void copy_freebasic_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/local/lib", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/local/bin", work_dir);
	execute_cmd("/bin/cp /usr/local/lib/freebasic %s/usr/local/lib/", work_dir);
	execute_cmd("/bin/cp /usr/local/bin/fbc %s/", work_dir);
	execute_cmd("/bin/cp -a /lib32/* %s/lib/", work_dir);

}
void copy_mono_runtime(char * work_dir) {

	copy_shell_runtime(work_dir);
	execute_cmd("/bin/mkdir %s/usr", work_dir);
	execute_cmd("/bin/mkdir %s/proc", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib/mono/2.0", work_dir);
	execute_cmd("/bin/cp -a /usr/lib/mono %s/usr/lib/", work_dir);
	execute_cmd("/bin/mkdir -p %s/usr/lib64/mono/2.0", work_dir);
	execute_cmd("/bin/cp -a /usr/lib64/mono %s/usr/lib64/", work_dir);

	execute_cmd("/bin/cp /usr/lib/libgthread* %s/usr/lib/", work_dir);

	execute_cmd("/bin/mount -o bind /proc %s/proc", work_dir);
	execute_cmd("/bin/cp /usr/bin/mono* %s/", work_dir);

	execute_cmd("/bin/cp /usr/lib/libgthread* %s/usr/lib/", work_dir);
	execute_cmd("/bin/cp /lib/libglib* %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib/tls/i686/cmov/lib* %s/lib/tls/i686/cmov/",
			work_dir);
	execute_cmd("/bin/cp /lib/libpcre* %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib/ld-linux* %s/lib/", work_dir);
	execute_cmd("/bin/cp /lib64/ld-linux* %s/lib64/", work_dir);
	execute_cmd("/bin/mkdir -p %s/home/judge", work_dir);
	execute_cmd("/bin/chown judge %s/home/judge", work_dir);
	execute_cmd("/bin/mkdir -p %s/etc", work_dir);
	execute_cmd("/bin/grep judge /etc/passwd>%s/etc/passwd", work_dir);

}

void run_solution(int & lang, char * work_dir, int & time_lmt, int & usedtime,
		int & mem_lmt) {
	nice(19);
	// now the user is "judger"
	chdir(work_dir);
	// open the files
	freopen("data.in", "r", stdin);
	freopen("user.out", "w", stdout);
	freopen("error.out", "a+", stderr);
	// trace me
	ptrace(PTRACE_TRACEME, 0, NULL, NULL);
	// run me
	if (lang != 3)
		chroot(work_dir);

	while (setgid(1536) != 0)
		sleep(1);
	while (setuid(1536) != 0)
		sleep(1);
	while (setresuid(1536, 1536, 1536) != 0)
		sleep(1);

//      char java_p1[BUFFER_SIZE], java_p2[BUFFER_SIZE];
	// child
	// set the limit
	struct rlimit LIM; // time limit, file limit& memory limit
	// time limit
	if (oi_mode)
		LIM.rlim_cur = time_lmt + 1;
	else
		LIM.rlim_cur = (time_lmt - usedtime / 1000) + 1;
	LIM.rlim_max = LIM.rlim_cur;
	//if(DEBUG) printf("LIM_CPU=%d",(int)(LIM.rlim_cur));
	setrlimit(RLIMIT_CPU, &LIM);
	alarm(0);
	alarm(time_lmt * 10);

	// file limit
	LIM.rlim_max = STD_F_LIM + STD_MB;
	LIM.rlim_cur = STD_F_LIM;
	setrlimit(RLIMIT_FSIZE, &LIM);
	// proc limit
	switch (lang) {
	case 3:  //java
	case 12:
		LIM.rlim_cur = LIM.rlim_max = 50;
		break;
	case 5: //bash
		LIM.rlim_cur = LIM.rlim_max = 3;
		break;
	case 9: //C#
		LIM.rlim_cur = LIM.rlim_max = 50;
		break;
	default:
		LIM.rlim_cur = LIM.rlim_max = 1;
	}

	setrlimit(RLIMIT_NPROC, &LIM);

	// set the stack
	LIM.rlim_cur = STD_MB << 6;
	LIM.rlim_max = STD_MB << 6;
	setrlimit(RLIMIT_STACK, &LIM);
	// set the memory
	LIM.rlim_cur = STD_MB * mem_lmt / 2 * 3;
	LIM.rlim_max = STD_MB * mem_lmt * 2;
	if (lang < 3)
		setrlimit(RLIMIT_AS, &LIM);

	switch (lang) {
	case 0:
	case 1:
	case 2:
	case 10:
	case 11:
		execl("./Main", "./Main", (char *) NULL);
		break;
	case 3:
//              sprintf(java_p1, "-Xms%dM", mem_lmt / 2);
//              sprintf(java_p2, "-Xmx%dM", mem_lmt);

		execl("/usr/bin/java", "/usr/bin/java", java_xms, java_xmx,
				"-Djava.security.manager",
				"-Djava.security.policy=./java.policy", "Main", (char *) NULL);
		break;
	case 4:
		//system("/ruby Main.rb<data.in");
		execl("/ruby", "/ruby", "Main.rb", (char *) NULL);
		break;
	case 5: //bash
		execl("/bin/bash", "/bin/bash", "Main.sh", (char *) NULL);
		break;
	case 6: //Python
		execl("/python", "/python", "Main.py", (char *) NULL);
		break;
	case 7: //php
		execl("/php", "/php", "Main.php", (char *) NULL);
		break;
	case 8: //perl
		execl("/perl", "/perl", "Main.pl", (char *) NULL);
		break;
	case 9: //Mono C#
		execl("/mono", "/mono", "--debug", "Main.exe", (char *) NULL);
		break;
	case 12: //guile
		execl("/guile", "/guile", "Main.scm", (char *) NULL);
		break;

	}
	//sleep(1);
	exit(0);
}
int fix_java_mis_judge(char *work_dir, int & ACflg, int & topmemory,
		int mem_lmt) {
	int comp_res = OJ_AC;
	if (DEBUG)
		execute_cmd("cat %s/error.out", work_dir);
	comp_res = execute_cmd("/bin/grep 'Exception'  %s/error.out", work_dir);
	if (!comp_res) {
		printf("Exception reported\n");
		ACflg = OJ_RE;
	}

	comp_res = execute_cmd(
			"/bin/grep 'java.lang.OutOfMemoryError'  %s/error.out", work_dir);

	if (!comp_res) {
		printf("JVM need more Memory!");
		ACflg = OJ_ML;
		topmemory = mem_lmt * STD_MB;
	}
	comp_res = execute_cmd(
			"/bin/grep 'java.lang.OutOfMemoryError'  %s/user.out", work_dir);

	if (!comp_res) {
		printf("JVM need more Memory or Threads!");
		ACflg = OJ_ML;
		topmemory = mem_lmt * STD_MB;
	}
	comp_res = execute_cmd("/bin/grep 'Could not create'  %s/error.out",
			work_dir);
	if (!comp_res) {
		printf("jvm need more resource,tweak -Xmx(OJ_JAVA_BONUS) Settings");
		ACflg = OJ_RE;
		//topmemory=0;
	}
	return comp_res;
}
int special_judge(char* oj_home, int problem_id, char *infile, char *outfile,
		char *userfile) {

	pid_t pid;
	printf("pid=%d\n", problem_id);
	pid = fork();
	int ret = 0;
	if (pid == 0) {

		while (setgid(1536) != 0)
			sleep(1);
		while (setuid(1536) != 0)
			sleep(1);
		while (setresuid(1536, 1536, 1536) != 0)
			sleep(1);

		struct rlimit LIM; // time limit, file limit& memory limit

		LIM.rlim_cur = 5;
		LIM.rlim_max = LIM.rlim_cur;
		setrlimit(RLIMIT_CPU, &LIM);
		alarm(0);
		alarm(10);

		// file limit
		LIM.rlim_max = STD_F_LIM + STD_MB;
		LIM.rlim_cur = STD_F_LIM;
		setrlimit(RLIMIT_FSIZE, &LIM);

		ret = execute_cmd("%s/data/%d/spj %s %s %s", oj_home, problem_id,
				infile, outfile, userfile);
		if (DEBUG)
			printf("spj1=%d\n", ret);
		if (ret)
			exit(1);
		else
			exit(0);
	} else {
		int status;

		waitpid(pid, &status, 0);
		ret = WEXITSTATUS(status);
		if (DEBUG)
			printf("spj2=%d\n", ret);
	}
	return ret;

}
void judge_solution(int & ACflg, int & usedtime, int time_lmt, int isspj,
		int p_id, char * infile, char * outfile, char * userfile, int & PEflg,
		int lang, char * work_dir, int & topmemory, int mem_lmt,
		int solution_id, double num_of_test) {
	//usedtime-=1000;
	int comp_res;
	if (!oi_mode)
		num_of_test = 1.0;
	if (ACflg == OJ_AC
			&& usedtime > time_lmt * 1000 * (use_max_time ? 1 : num_of_test))
		ACflg = OJ_TL;
	if (topmemory > mem_lmt * STD_MB)
		ACflg = OJ_ML; //issues79
	// compare
	if (ACflg == OJ_AC) {
		if (isspj) {
			comp_res = special_judge(oj_home, p_id, infile, outfile, userfile);

			if (comp_res == 0)
				comp_res = OJ_AC;
			else {
				if (DEBUG)
					printf("fail test %s\n", infile);
				comp_res = OJ_WA;
			}
		} else {
			comp_res = compare(outfile, userfile);
		}
		if (comp_res == OJ_WA) {
			ACflg = OJ_WA;
			if (DEBUG)
				printf("fail test %s\n", infile);
		} else if (comp_res == OJ_PE)
			PEflg = OJ_PE;
		ACflg = comp_res;
	}
	//jvm popup messages, if don't consider them will get miss-WrongAnswer
	if (lang == 3) {
		comp_res = fix_java_mis_judge(work_dir, ACflg, topmemory, mem_lmt);
	}
}

int get_page_fault_mem(struct rusage & ruse, pid_t & pidApp) {
	//java use pagefault
	int m_vmpeak, m_vmdata, m_minflt;
	m_minflt = ruse.ru_minflt * getpagesize();
	if (0 && DEBUG) {
		m_vmpeak = get_proc_status(pidApp, "VmPeak:");
		m_vmdata = get_proc_status(pidApp, "VmData:");
		printf("VmPeak:%d KB VmData:%d KB minflt:%d KB\n", m_vmpeak, m_vmdata,
				m_minflt >> 10);
	}
	return m_minflt;
}
void print_runtimeerror(char * err) {
	FILE *ferr = fopen("error.out", "a+");
	fprintf(ferr, "Runtime Error:%s\n", err);
	fclose(ferr);
}
void clean_session(pid_t p) {
	//char cmd[BUFFER_SIZE];
	const char *pre = "ps awx -o \"\%p \%P\"|grep -w ";
	const char *post = " | awk \'{ print $1  }\'|xargs kill -9";
	execute_cmd("%s %d %s", pre, p, post);
	execute_cmd("ps aux |grep \\^judge|awk '{print $2}'|xargs kill");
}

void watch_solution(pid_t pidApp, char * infile, int & ACflg, int isspj,
		char * userfile, char * outfile, int solution_id, int lang,
		int & topmemory, int mem_lmt, int & usedtime, int time_lmt, int & p_id,
		int & PEflg, char * work_dir) {
	// parent
	int tempmemory;

	if (DEBUG)
		printf("pid=%d judging %s\n", pidApp, infile);

	int status, sig, exitcode;
	struct user_regs_struct reg;
	struct rusage ruse;
	while (1) {
		// check the usage

		wait4(pidApp, &status, 0, &ruse);

//jvm gc ask VM before need,so used kernel page fault times and page size
		if (lang == 3) {
			tempmemory = get_page_fault_mem(ruse, pidApp);
		} else {        //other use VmPeak
			tempmemory = get_proc_status(pidApp, "VmPeak:") << 10;
		}
		if (tempmemory > topmemory)
			topmemory = tempmemory;
		if (topmemory > mem_lmt * STD_MB) {
			if (DEBUG)
				printf("out of memory %d\n", topmemory);
			if (ACflg == OJ_AC)
				ACflg = OJ_ML;
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}
		//sig = status >> 8;/*status >> 8 Ã¥Â·Â®Ã¤Â¸ÂÃ¥Â¤Å¡Ã¦ËÂ¯EXITCODE*/

		if (WIFEXITED(status))
			break;
		if ((lang < 4 || lang == 9) && get_file_size("error.out") && !oi_mode) {
			ACflg = OJ_RE;
			//addreinfo(solution_id);
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}

		if (!isspj
				&& get_file_size(userfile)
						> get_file_size(outfile) * 2 + 1024) {
			ACflg = OJ_OL;
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
			break;
		}

		exitcode = WEXITSTATUS(status);
		/*exitcode == 5 waiting for next CPU allocation          * ruby using system to run,exit 17 ok
		 *  */
		if ((lang >= 3 && exitcode == 17) || exitcode == 0x05 || exitcode == 0)
			//go on and on
			;
		else {

			if (DEBUG) {
				printf("status>>8=%d\n", exitcode);

			}
			//psignal(exitcode, NULL);

			if (ACflg == OJ_AC) {
				switch (exitcode) {
				case SIGCHLD:
				case SIGALRM:
					alarm(0);
				case SIGKILL:
				case SIGXCPU:
					ACflg = OJ_TL;
					break;
				case SIGXFSZ:
					ACflg = OJ_OL;
					break;
				default:
					ACflg = OJ_RE;
				}
				print_runtimeerror(strsignal(exitcode));
			}
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);

			break;
		}
		if (WIFSIGNALED(status)) {
			/*  WIFSIGNALED: if the process is terminated by signal
			 *
			 *  psignal(int sig, char *s)，like perror(char *s)，print out s, with error msg from system of sig  
			 * sig = 5 means Trace/breakpoint trap
			 * sig = 11 means Segmentation fault
			 * sig = 25 means File size limit exceeded
			 */
			sig = WTERMSIG(status);

			if (DEBUG) {
				printf("WTERMSIG=%d\n", sig);
				psignal(sig, NULL);
			}
			if (ACflg == OJ_AC) {
				switch (sig) {
				case SIGCHLD:
				case SIGALRM:
					alarm(0);
				case SIGKILL:
				case SIGXCPU:
					ACflg = OJ_TL;
					break;
				case SIGXFSZ:
					ACflg = OJ_OL;
					break;

				default:
					ACflg = OJ_RE;
				}
				print_runtimeerror(strsignal(sig));
			}
			break;
		}
		/*     comment from http://www.felix021.com/blog/read.php?1662

		 WIFSTOPPED: return true if the process is paused or stopped while ptrace is watching on it
		 WSTOPSIG: get the signal if it was stopped by signal
		 */

		// check the system calls
		ptrace(PTRACE_GETREGS, pidApp, NULL, &reg);
		if (call_counter[reg.REG_SYSCALL] ){
			//call_counter[reg.REG_SYSCALL]--;
		}else if (record_call) {
			call_counter[reg.REG_SYSCALL] = 1;
		
		}else { //do not limit JVM syscall for using different JVM
			ACflg = OJ_RE;
			char error[BUFFER_SIZE];
			sprintf(error,
					"[ERROR] A Not allowed system call: runid:%d callid:%ld\n TO FIX THIS , ask admin to add the CALLID into corresponding LANG_XXV[] located at okcalls32/64.h ,and recompile judge_client",
					solution_id, (long)reg.REG_SYSCALL);
			write_log(error);
			print_runtimeerror(error);
			ptrace(PTRACE_KILL, pidApp, NULL, NULL);
		}
		

		ptrace(PTRACE_SYSCALL, pidApp, NULL, NULL);
	}
	usedtime += (ruse.ru_utime.tv_sec * 1000 + ruse.ru_utime.tv_usec / 1000);
	usedtime += (ruse.ru_stime.tv_sec * 1000 + ruse.ru_stime.tv_usec / 1000);

	//clean_session(pidApp);
}
void clean_workdir(char * work_dir) {
	execute_cmd("/bin/umount %s/proc", work_dir);
	if (DEBUG) {
		execute_cmd("/bin/mv %s/* %slog/", work_dir, work_dir);
	} else {
		execute_cmd("/bin/rm -Rf %s/*", work_dir);

	}

}

//删除工作目录 run + solution_id
void delete_workdir(char * work_dir) {
	execute_cmd("rm -r %s", work_dir);
}

//获取命令行传来的参数
void get_params_from_argv(int argc, char** argv, char * sln_id, char * oj_home_path, int & debug) {
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-solutionId") == 0 && i != argc - 1) {
			strcpy(sln_id, argv[i + 1]);
		} else if (strcmp(argv[i], "-ojHome") == 0 && i != argc - 1) {
			strcpy(oj_home_path, argv[i + 1]);
		} if (strcmp(argv[i], "-debug") == 0) {
			debug = 1;
		}
	}
}

void init_parameters(int argc, char ** argv, int & solution_id)
{
	memset(sln_id, 0, sizeof(sln_id));
	strcpy(oj_home, "/home/judge");
	
	get_params_from_argv(argc, argv, sln_id, oj_home, DEBUG);

    if (sln_id[0] == '\0')
    {
        fprintf(stderr, "Fail to get solution_id.\n");
        exit(1);
    }

    chdir(oj_home); // change the dir// init our work

    solution_id = atoi(sln_id);

	if (DEBUG) {
		printf("init_parameters sln_id: %s\n", sln_id);
		printf("init_parameters oj_home: %s\n", oj_home);
	}
}
int get_sim(int solution_id, int lang, int pid, int &sim_s_id) {
	char src_pth[BUFFER_SIZE];
	//char cmd[BUFFER_SIZE];
	sprintf(src_pth, "Main.%s", lang_ext[lang]);

	int sim = execute_cmd("/usr/bin/sim.sh %s %d", src_pth, pid);
	if (!sim) {
		execute_cmd("/bin/mkdir ../data/%d/ac/", pid);

		execute_cmd("/bin/cp %s ../data/%d/ac/%d.%s", src_pth, pid, solution_id,
				lang_ext[lang]);
		//c cpp will
		if (lang == 0)
			execute_cmd("/bin/ln ../data/%d/ac/%d.%s ../data/%d/ac/%d.%s", pid,
					solution_id, lang_ext[lang], pid, solution_id,
					lang_ext[lang + 1]);
		if (lang == 1)
			execute_cmd("/bin/ln ../data/%d/ac/%d.%s ../data/%d/ac/%d.%s", pid,
					solution_id, lang_ext[lang], pid, solution_id,
					lang_ext[lang - 1]);

	} else {

		FILE * pf;
		pf = fopen("sim", "r");
		if (pf) {
			fscanf(pf, "%d%d", &sim, &sim_s_id);
			fclose(pf);
		}

	}
	if (solution_id <= sim_s_id)
		sim = 0;
	return sim;
}
void mk_shm_workdir(char * work_dir) {
	char shm_path[BUFFER_SIZE];
	sprintf(shm_path, "/dev/shm/hustoj/%s", work_dir);
	execute_cmd("/bin/mkdir -p %s", shm_path);
	execute_cmd("/bin/rm -rf %s", work_dir);
	execute_cmd("/bin/ln -s %s %s/", shm_path, oj_home);
	execute_cmd("/bin/chown judge %s ", shm_path);
	execute_cmd("chmod 755 %s ", shm_path);
	//sim need a soft link in shm_dir to work correctly
	sprintf(shm_path, "/dev/shm/hustoj/%s/", oj_home);
	execute_cmd("/bin/ln -s %s/data %s", oj_home, shm_path);

}
int count_in_files(char * dirpath) {
	const char * cmd = "ls -l %s/*.in|wc -l";
	int ret = 0;
	FILE * fjobs = read_cmd_output(cmd, dirpath);
	fscanf(fjobs, "%d", &ret);
	pclose(fjobs);

	return ret;
}

int get_test_file(char* work_dir, int p_id) {
	char filename[BUFFER_SIZE];
	char localfile[BUFFER_SIZE];
	int ret = 0;
	const char * cmd =
			" wget --post-data=\"gettestdatalist=1&pid=%d\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O - \"%s/admin/problem_judge.php\"";
	FILE * fjobs = read_cmd_output(cmd, p_id, http_baseurl);
	while (fgets(filename, BUFFER_SIZE - 1, fjobs) != NULL) {
		sscanf(filename, "%s", filename);
		sprintf(localfile, "%s/data/%d/%s", oj_home, p_id, filename);
		if (DEBUG)
			printf("localfile[%s]\n", localfile);

		const char * check_file_cmd =
				" wget --post-data=\"gettestdatadate=1&filename=%d/%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O -  \"%s/admin/problem_judge.php\"";
		FILE * rcop = read_cmd_output(check_file_cmd, p_id, filename,
				http_baseurl);
		time_t remote_date, local_date;
		fscanf(rcop, "%ld", &remote_date);
		fclose(rcop);
		struct stat fst;
		stat(localfile, &fst);
		local_date = fst.st_mtime;

		if (access(localfile, 0) == -1 || local_date < remote_date) {

			if (strcmp(filename, "spj") == 0)
				continue;
			execute_cmd("/bin/mkdir -p %s/data/%d", oj_home, p_id);
			const char * cmd2 =
					" wget --post-data=\"gettestdata=1&filename=%d/%s\" --load-cookies=cookie --save-cookies=cookie --keep-session-cookies -q -O \"%s\"  \"%s/admin/problem_judge.php\"";
			execute_cmd(cmd2, p_id, filename, localfile, http_baseurl);
			ret++;

			if (strcmp(filename, "spj.c") == 0) {
				//   sprintf(localfile,"%s/data/%d/spj.c",oj_home,p_id);
				if (access(localfile, 0) == 0) {
					const char * cmd3 = "gcc -o %s/data/%d/spj %s/data/%d/spj.c";
					execute_cmd(cmd3, oj_home, p_id, oj_home, p_id);
				}

			}
			if (strcmp(filename, "spj.cc") == 0) {
				//     sprintf(localfile,"%s/data/%d/spj.cc",oj_home,p_id);
				if (access(localfile, 0) == 0) {
					const char * cmd4 =
							"g++ -o %s/data/%d/spj %s/data/%d/spj.cc";
					execute_cmd(cmd4, oj_home, p_id, oj_home, p_id);
				}
			}
		}

	}
	pclose(fjobs);

	return ret;
}
void print_call_array() {
	printf("int LANG_%sV[256]={", LANG_NAME);
	int i = 0;
	for (i = 0; i < call_array_size; i++) {
		if (call_counter[i]) {
			printf("%d,", i);
		}
	}
	printf("0};\n");

	printf("int LANG_%sC[256]={", LANG_NAME);
	for (i = 0; i < call_array_size; i++) {
		if (call_counter[i]) {
			printf("HOJ_MAX_LIMIT,");
		}
	}
	printf("0};\n");

}
int main(int argc, char** argv) {

	struct rlimit LIM;
	LIM.rlim_max = 800;
	LIM.rlim_cur = 800;
	setrlimit(RLIMIT_CPU, &LIM);

	LIM.rlim_max = 80 * STD_MB;
	LIM.rlim_cur = 80 * STD_MB;
	setrlimit(RLIMIT_FSIZE, &LIM);

	LIM.rlim_max = STD_MB << 11;
	LIM.rlim_cur = STD_MB << 11;
	setrlimit(RLIMIT_AS, &LIM);

	LIM.rlim_cur = LIM.rlim_max = 200;
	setrlimit(RLIMIT_NPROC, &LIM);


	char work_dir[BUFFER_SIZE];
	//char cmd[BUFFER_SIZE];
	char user_id[BUFFER_SIZE];
	int solution_id = 1000;
	//int runner_id = 0;
	int p_id, time_lmt, mem_lmt, lang, isspj, sim, sim_s_id, max_case_time = 0;

	//从命令行初始化参数
    init_parameters(argc, argv, solution_id);
	
	//从配置文件读取配置
    init_judge_conf();
	/*
	if (!http_judge && !init_mysql_conn()) {
		exit(0); //exit if mysql is down
	}
	*/
	//set work directory to start running & judging
    sprintf(work_dir, "%s/run%d/", oj_home, solution_id);
	//sprintf(work_dir, "%s/run%s/", oj_home, argv[2]);
	//如果之前这个工作目录不存在。则创建
	if (opendir(work_dir) == NULL) {
        execute_cmd("mkdir %s",work_dir);
		execute_cmd("chmod 775 %s",work_dir);
    }
	if (DEBUG) {
		printf("work_dir: %s\n", work_dir);
	}
	
	
	//关于sim的代码，现在目前还没有用到
	if (shm_run)
		mk_shm_workdir(work_dir);

	chdir(work_dir);
	if (!DEBUG)
		clean_workdir(work_dir);

	/*
	if (http_judge)
		system("/bin/ln -s ../cookie ./");
	*/
	
	//初始化命名管道
	if (init_fifo(sln_id)) {
		return 1;
	}
	
	get_solution_info(solution_id, p_id, user_id, lang);
	//get the limit

	if (p_id == 0) {
		time_lmt = 5;
		mem_lmt = 128;
		isspj = 0;
	} else {
		get_problem_info(p_id, time_lmt, mem_lmt, isspj);
	}
	//copy source file

	get_solution(solution_id, work_dir, lang);

	//java is lucky
	if (lang >= 3) {
		// the limit for java
		time_lmt = time_lmt + java_time_bonus;
		mem_lmt = mem_lmt + java_memory_bonus;
		// copy java.policy
		execute_cmd("/bin/cp %s/etc/java0.policy %s/java.policy", oj_home,
				work_dir);

	}

	//never bigger than judged set value;
	if (time_lmt > 300 || time_lmt < 1)
		time_lmt = 300;
	if (mem_lmt > 1024 || mem_lmt < 1)
		mem_lmt = 1024;

	if (DEBUG)
		printf("time: %d mem: %d\n", time_lmt, mem_lmt);

	// compile
	//      printf("%s\n",cmd);
	// set the result to compiling
	int Compile_OK;

	Compile_OK = compile(lang);
	if (Compile_OK != 0) {
		addceinfo(solution_id);
		update_solution(solution_id, OJ_CE, 0, 0, 0, 0, 0.0);
		//update_user(user_id);
		//update_problem(p_id);
		/*
		if (!http_judge)
			mysql_close(conn);
		*/
		if (!DEBUG)
			clean_workdir(work_dir);
		else
			write_log("compile error");
		exit(0);
	} else {
		update_solution(solution_id, OJ_RI, 0, 0, 0, 0, 0.0);
	}
	//exit(0);
	// run
	char fullpath[BUFFER_SIZE];
	char infile[BUFFER_SIZE];
	char outfile[BUFFER_SIZE];
	char userfile[BUFFER_SIZE];
	sprintf(fullpath, "%s/data/%d", oj_home, p_id); // the fullpath of data dir

	// open DIRs
	DIR *dp;
	dirent *dirp;
	//这块看不懂，可以先不考虑
	// using http to get remote test data files
	//if (p_id > 0 && http_judge)
	//	get_test_file(work_dir, p_id);
	if (p_id > 0 && (dp = opendir(fullpath)) == NULL) {

		write_log("No such dir:%s!\n", fullpath);
		/*
		if (!http_judge)
			mysql_close(conn);
		*/
		exit(-1);
	}

	int ACflg, PEflg;
	ACflg = PEflg = OJ_AC;
	int namelen;
	int usedtime = 0, topmemory = 0;

	//create chroot for ruby bash python
	if (lang == 4)
		copy_ruby_runtime(work_dir);
	if (lang == 5)
		copy_bash_runtime(work_dir);
	if (lang == 6)
		copy_python_runtime(work_dir);
	if (lang == 7)
		copy_php_runtime(work_dir);
	if (lang == 8)
		copy_perl_runtime(work_dir);
	if (lang == 9)
		copy_mono_runtime(work_dir);
	if (lang == 10)
		copy_objc_runtime(work_dir);
	if (lang == 11)
		copy_freebasic_runtime(work_dir);
	if (lang == 12)
		copy_guile_runtime(work_dir);
	// read files and run
	// read files and run
	// read files and run
	double pass_rate = 0.0;
	int num_of_test = 0;
	int finalACflg = ACflg;
	if (p_id == 0) {  //custom input running
		printf("running a custom input...\n");
		get_custominput(solution_id, work_dir);
		init_syscalls_limits(lang);
		pid_t pidApp = fork();

		if (pidApp == 0) {
			run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
		} else {
			watch_solution(pidApp, infile, ACflg, isspj, userfile, outfile,
					solution_id, lang, topmemory, mem_lmt, usedtime, time_lmt,
					p_id, PEflg, work_dir);

		}
		if (ACflg == OJ_TL) {
			usedtime = time_lmt * 1000;
		}
		if (ACflg == OJ_RE) {
			if (DEBUG)
				printf("add RE info of %d..... \n", solution_id);
			addreinfo(solution_id);
		} else {
			addcustomout(solution_id);
		}
		update_solution(solution_id, OJ_TR, usedtime, topmemory >> 10, 0, 0, 0);

		exit(0);
	}

	for (; (oi_mode || ACflg == OJ_AC) && (dirp = readdir(dp)) != NULL;) {

		namelen = isInFile(dirp->d_name); // check if the file is *.in or not
		if (namelen == 0)
			continue;

		prepare_files(dirp->d_name, namelen, infile, p_id, work_dir, outfile,
				userfile, solution_id);
		init_syscalls_limits(lang);

		pid_t pidApp = fork();

		if (pidApp == 0) {

			run_solution(lang, work_dir, time_lmt, usedtime, mem_lmt);
		} else {

			num_of_test++;

			watch_solution(pidApp, infile, ACflg, isspj, userfile, outfile,
					solution_id, lang, topmemory, mem_lmt, usedtime, time_lmt,
					p_id, PEflg, work_dir);

			judge_solution(ACflg, usedtime, time_lmt, isspj, p_id, infile,
					outfile, userfile, PEflg, lang, work_dir, topmemory,
					mem_lmt, solution_id, num_of_test);
			if (use_max_time) {
				max_case_time =
						usedtime > max_case_time ? usedtime : max_case_time;
				usedtime = 0;
			}
			//clean_session(pidApp);
		}
		if (oi_mode) {
			if (ACflg == OJ_AC) {
				++pass_rate;
			}
			if (finalACflg < ACflg) {
				finalACflg = ACflg;
			}

			ACflg = OJ_AC;
		}
	}
	if (ACflg == OJ_AC && PEflg == OJ_PE)
		ACflg = OJ_PE;
	if (sim_enable && ACflg == OJ_AC && (!oi_mode || finalACflg == OJ_AC)
			&& lang < 5) { //bash don't supported
		sim = get_sim(solution_id, lang, p_id, sim_s_id);
	} else {
		sim = 0;
	}
	//if(ACflg == OJ_RE)addreinfo(solution_id);

	if ((oi_mode && finalACflg == OJ_RE) || ACflg == OJ_RE) {
		if (DEBUG)
			printf("add RE info of %d..... \n", solution_id);
		addreinfo(solution_id);
	}
	if (use_max_time) {
		usedtime = max_case_time;
	}
	if (ACflg == OJ_TL) {
		usedtime = time_lmt * 1000;
	}
	if (oi_mode) {
		if (num_of_test > 0)
			pass_rate /= num_of_test;
		update_solution(solution_id, finalACflg, usedtime, topmemory >> 10, sim,
				sim_s_id, pass_rate);
	} else {
		update_solution(solution_id, ACflg, usedtime, topmemory >> 10, sim,
				sim_s_id, 0);
	}
	if ((oi_mode && finalACflg == OJ_WA) || ACflg == OJ_WA) {
		if (DEBUG)
			printf("add diff info of %d..... \n", solution_id);
		if (!isspj)
			adddiffinfo(solution_id);
	}
	/*
	update_user(user_id);
	update_problem(p_id);
	*/
	clean_workdir(work_dir);

	if (DEBUG)
		write_log("result=%d", oi_mode ? finalACflg : ACflg);
	/*
	if (!http_judge)
		mysql_close(conn);
	*/
	if (record_call) {
		print_call_array();
	}
	closedir(dp);
	
	//////////////////////////////////////////////////////debug
	/*
	char debugBuf[10000];
	memset(debugBuf, '\0', sizeof(debugBuf));
	sprintf(debugBuf, "aaaa %d\n", Compile_OK);
	send_end_flag_debug(debugBuf);
	while (1){;}
	*/



	send_end_flag();
	close_fifo();
	delete_workdir(work_dir);
	sleep(1);
	delete_fifo_file();
	
	return 0;
}