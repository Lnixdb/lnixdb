#ifndef _DB_H
#define _DB_H
#include <fcntl.h>

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define IDXLEN_MAX	1024
#define	IDXLEN_MIN	2
#define DATLEN_MAX	1024
#define	DATLEN_MIN	2

#define IDXLEN_SZ	4
#define SEP		':'
#define SPACE		' '
#define NEWLINE		'\n'
#define PTR_SZ		7
#define PTR_MAX		999999
#define NHASH_DEF	137
#define FREE_OFF	0
#define HASH_OFF	PTR_SZ

#define DB_INSERT	1
#define DB_REPLACE	2
#define	DN_STORE	3

#define	WRITE_LOCK	1
#define READ_LOCK	0

typedef void*		DBHANDLE; 
typedef unsigned long	DBHASH;
typedef struct {
	int	idxfd;
	int	datfd;
	char	*idxbuf;
	char	*datbuf;
	char	*name;
	off_t	idxoff;
	size_t	idxlen;
	
	off_t	datoff;
	size_t	datlen;
	
	off_t	ptrval;
	off_t	ptroff;
	off_t	chainoff;
	off_t	hashoff;
	DBHASH	nhash;

	size_t	cnt_delok;
	size_t	cnt_delerr;
	size_t	cnt_fetchok;
	size_t	cnt_fetcherr;
	size_t	cnt_nextrec;
	size_t	cnt_store1;	/*store: DB_INSERT, no empty, appended*/
	size_t	cnt_store2;	/*store: DB_INSERT, found empty, reuse*/
	size_t	cnt_store3;	/*store: DB_REPLACE, diff len, appended*/
	size_t	cnt_store4;	/*store: DB_REPLACE, same len, overwrite*/
	size_t	cnt_storeerr;
}DB;

void	_db_dodelete(DB *db);
int 	_db_findfree(DB* db, int keylen, int datlen);
off_t	_db_readptr(DB *db, off_t offset);
int 	_db_find_and_lock(DB *db, const char *key, int writelock);
void 	_db_free(DB *db);
char*	_db_readdat(DB *db);
off_t	_db_readptr(DB *db, off_t offset);
off_t	_db_readidx(DB *db, off_t idxoff);
void 	_db_writedat(DB *db, const char *data, off_t offset, int whence);
void	_db_writeptr(DB *db, off_t offset, off_t ptrval);
void	_db_writeidx(DB *db, const char *key, off_t offset, int whence, off_t ptrval);
void err_dump(const char* msg);
DBHASH _db_hash(DB *db, const char *key);
DB*	_db_alloc(int namelen);

DBHANDLE db_open(const char *pathname, int oflag, ...);
int	db_store(DB* db, const char *key, const char *value, int flag);
char*	db_fetch(DB* db, const char *key);
void	db_close(DB *db);
char*	db_nextrec(DB *db, char *key);
int 	db_findfree(DB* db, const char *key, const char *value);
int 	db_delete(DB *db, const char *key);
void 	db_rewind(DB *db);

#endif
