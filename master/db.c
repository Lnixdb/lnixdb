#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/uio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "db.h"
#include "recordlock.h"

void err_dump(const char* msg)
{
	printf("%s\n", msg);
	exit(0);
}

DBHASH _db_hash(DB *db, const char *key)
{
	DBHASH	hval = 0;
	char	c;
	int	i;

	for(i = 1; (c=*key++) != 0; i++ )
		hval += c * i;

	return (hval % db->nhash);
}

DB* _db_alloc(int namelen)
{
	DB	*db;

	if((db =(DB*) calloc(1, sizeof(DB))) == NULL)
		err_dump("_db_alloc: calloc error for DB");
	db->idxfd = -1;
	db->datfd = -1;
	if((db->name =(char *)malloc(namelen+5)) == NULL)
		err_dump("_db_alloc: malloc error for DB");
	if((db->idxbuf = (char *)malloc(IDXLEN_MAX+2)) == NULL)
		err_dump("_db_alloc: malloc error for idxbuf");
	if((db->datbuf = (char *)malloc(DATLEN_MAX+2)) == NULL)
		err_dump("_db_alloc: malloc error for datbuf");
	return db;
}

DBHANDLE db_open(const char *pathname, int oflag, ...)
{
	DB	*db;
	int	len, mode;
	size_t	i, j;
	char	asciiptr[PTR_SZ+1];
	char	hash[(NHASH_DEF+1)*PTR_SZ+2];
	struct stat statbuff;

	len = strlen(pathname);
	db = _db_alloc(len);
	db->nhash = NHASH_DEF;
	db->hashoff = HASH_OFF;
	strcpy(db->name, pathname);
	strcat(db->name, ".idx");

	if(oflag & O_CREAT){
		va_list	ap;
		va_start(ap, oflag);
		mode = va_arg(ap, int);
		va_end(ap);	
		db->idxfd = open(db->name, oflag, mode);
		strcpy(db->name+len, ".dat");
		db->datfd = open(db->name, oflag, mode);
	} else {
		db->idxfd = open(db->name, oflag);
		strcpy(db->name+len, ".dat");
		db->datfd = open(db->name, oflag);
	}


	if(db->idxfd < 0 || db->datfd < 0){
		_db_free(db);
		return NULL;
	}

	if((oflag&(O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC)){
	
		/*avoid two processes opening the same file*/
		
		if(writew_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
			err_dump("db_open: writew_lock error...");

		if(fstat(db->idxfd, &statbuff) < 0)
			err_dump("db_open: fstat error...");
		if(statbuff.st_size == 0){
			sprintf(asciiptr, "%*d", PTR_SZ, 0);
			hash[0] = 0;
			for(j=0; j<NHASH_DEF+1; j++)
				strcat(hash, asciiptr);
			strcat(hash,"\n");
			i = strlen(hash);
			write(db->idxfd, hash, i);
		}
		
		if(un_lock(db->idxfd, 0, SEEK_SET, 0) < 0)
			err_dump("db_open: un_lock error...");
	}
	db_rewind(db);
	return db;
}

void _db_writedat(DB *db, const char *data, off_t offset, int whence)
{
	struct iovec	iov[2];
	static char newline = NEWLINE;

	if(whence == SEEK_END)
		if(writew_lock(db->datfd, 0, SEEK_SET, 0) < 0)
			err_dump("_db_writedat: error writew_lock...");

	db->datoff = lseek(db->datfd, offset, whence);
	db->datlen = strlen(data) + 1;

	iov[0].iov_base = (char*)data;
	iov[0].iov_len = db->datlen-1;
	iov[1].iov_base = &newline;
	iov[1].iov_len = 1;
	if(writev(db->datfd, &iov[0], 2) != db->datlen)
		err_dump("_db_writedat: writev error of data record");
	
	if(whence == SEEK_END)
		if(un_lock(db->datfd, 0, SEEK_SET, 0) < 0)
			err_dump("_db_writedat: error un_lock...");
}

void _db_writeidx(DB *db, const char *key, off_t offset, int whence, off_t ptrval)
{
	char	asciiptr[PTR_SZ + IDXLEN_SZ+1];
	struct 	iovec iov[2];
	int	len;

	if((db->ptrval = ptrval) < 0 || ptrval > PTR_MAX)
		err_dump("_db_writeidx: invalid ptrval");

	sprintf(db->idxbuf,"%s%c%lld%c%ld\n", key, SEP, (long long)db->datoff, SEP, (long)db->datlen);
	if((len = strlen(db->idxbuf)) < IDXLEN_MIN || len > IDXLEN_MAX)
		err_dump("_db_writeidx: invalid length");
	sprintf(asciiptr,"%*lld%*d",PTR_SZ, (long long)ptrval, IDXLEN_SZ, len);

	if(whence == SEEK_END)
		if(writew_lock(db->idxfd, ((db->nhash+1)*PTR_SZ)+1, SEEK_SET, 0) < 0)
			err_dump("_db_writeidx: error writew_lock...");
	
	if((db->idxoff = lseek(db->idxfd, offset, whence)) == -1)
		err_dump("_db_writeidx: lseek error");
	iov[0].iov_base = asciiptr;
	iov[0].iov_len = PTR_SZ + IDXLEN_SZ;
	iov[1].iov_base = db->idxbuf;
	iov[1].iov_len = len;
	
	if(writev(db->idxfd, &iov[0], 2) != (PTR_SZ + IDXLEN_SZ + len))
		err_dump("_db_writeidx: writev error of index record");
	
	if(whence == SEEK_END)
		if(un_lock(db->idxfd, ((db->nhash+1)*PTR_SZ)+1,SEEK_SET, 0) < 0)
			err_dump("_db_writedat: error un_lock...");
}

void _db_writeptr(DB *db, off_t offset, off_t ptrval)
{
	char	asciiptr[PTR_SZ + 1];

	if(ptrval < 0 || ptrval > PTR_MAX)
		err_dump("_db_writeptr: invalid ptrval");

	sprintf(asciiptr, "%*lld", PTR_SZ, (long long)ptrval);
	if(lseek(db->idxfd, offset, SEEK_SET) == -1)
		err_dump("_db_writeptr: lseek error");

	if(write(db->idxfd, asciiptr, PTR_SZ) != PTR_SZ)
		err_dump("_db_writeptr: write error of ptr field");
}

int db_store(DB* db, const char *key, const char *value, int flag)
{
	char	asciiptr[PTR_SZ+1];
	off_t	ptrval;
	int	rc;
	int	keylen = strlen(key);
	int	datlen = strlen(value) + 1; //+1 for newline

	if((datlen < DATLEN_MIN) || (datlen > DATLEN_MAX))
		err_dump("db_store: invalid date length");	
	
	if( _db_find_and_lock(db, key, 1) < 0){	
		if(flag == DB_REPLACE){
			rc = -1;
			db->cnt_storeerr += 1;
			goto doreturn;
		}	

		ptrval = _db_readptr(db, db->chainoff);
		if(_db_findfree(db, keylen, datlen) < 0){ // no free
			_db_writedat(db, value, 0, SEEK_END);
			_db_writeidx(db, key, 0, SEEK_END, ptrval);
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_store1++;
		} else {
			_db_writedat(db, value, db->datoff, SEEK_SET);
			_db_writeidx(db, key, db->idxoff, SEEK_SET, ptrval);
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_store2++;
		}
	}
	else{
		if(flag == DB_INSERT){
			rc = 1;
			db->cnt_storeerr++;
			goto doreturn;
		}

		if(datlen != db->datlen){
			_db_dodelete(db);
			ptrval = _db_readptr(db, db->chainoff);
			_db_writedat(db, value, 0, SEEK_END);
			_db_writeidx(db, key, 0, SEEK_END, ptrval);
			_db_writeptr(db, db->chainoff, db->idxoff);
			db->cnt_store3++;
		} else {
			_db_writedat(db, value, db->datoff, SEEK_SET);
			db->cnt_store4++;
		}
	}
	rc = 0;

	doreturn:
		if(un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
			err_dump("db_store: un_lock error...");
	return rc;
}

off_t _db_readptr(DB *db, off_t offset)
{
	char	asciiptr[PTR_SZ+1];
	int 	len;

	if((lseek(db->idxfd, offset, SEEK_SET)) == -1)
		err_dump("_db_readptr: lseek error to ptr field");
	if((len = read(db->idxfd, asciiptr, PTR_SZ)) != PTR_SZ){
		printf("read %d bytes ", len);
		err_dump("_db_readptr: read error of ptr field...");
	}
	asciiptr[PTR_SZ] = 0;
	
	return (atol(asciiptr));
}


off_t _db_readidx(DB *db, off_t idxoff)
{
	char	asciiptr[PTR_SZ+1], asciilen[IDXLEN_SZ+1];
	struct	iovec iov[2];
	char	*ptr1, *ptr2;
	ssize_t	i;

	if((db->idxoff = lseek(db->idxfd, idxoff, idxoff == 0 ? SEEK_CUR : SEEK_SET)) == -1)
		err_dump("_db_readidx: error lseek");
	iov[0].iov_base = asciiptr;
	iov[0].iov_len = PTR_SZ;
	iov[1].iov_base = asciilen;
	iov[1].iov_len = IDXLEN_SZ;

	if((i = readv(db->idxfd, &iov[0], 2)) != PTR_SZ + IDXLEN_SZ){
		if(i == 0 && idxoff == 0)
			return -1;
		err_dump("_db_readidx: readv error of index record");
	}
	asciiptr[PTR_SZ] = 0;
	db->ptrval = atol(asciiptr);

	asciilen[IDXLEN_SZ] = 0;	
	db->idxlen = atoi(asciilen);

	if((i = read(db->idxfd, db->idxbuf, db->idxlen)) != db->idxlen)
		err_dump("_db_readidx: read error of index record");

	if(db->idxbuf[db->idxlen-1] != NEWLINE)
		err_dump("_db_readidx: missing NEWLINE");
	db->idxbuf[db->idxlen-1] = 0;

	if((ptr1 = strchr(db->idxbuf, SEP)) == NULL)
		err_dump("_db_readidx: missing first SEP");
	*ptr1++ = 0;	

	if((ptr2 = strchr(ptr1, SEP)) == NULL)
		err_dump("db_readidx: missing second SEP");
	*ptr2++ = 0;	
	
	if(strchr(ptr2, SEP) != NULL)
		err_dump("_db_readidx: too many separators");
	db->datoff = atol(ptr1);
	db->datlen = atol(ptr2);
	
	return db->ptrval;
}

char* _db_readdat(DB *db)
{
	if(lseek(db->datfd, db->datoff, SEEK_SET) == -1)
		err_dump("_db_readdat: lssek error");
	if(read(db->datfd, db->datbuf, db->datlen) != db->datlen)
		err_dump("_db_readdat: read error");
	if(db->datbuf[db->datlen-1] != NEWLINE)
		err_dump("_db_readdat: missing newline");
	db->datbuf[db->datlen-1] = 0;
	return db->datbuf;
}
int _db_find_and_lock(DB *db, const char *key, int writelock)
{
	off_t	offset, nextoffset;
	
	db->chainoff = (_db_hash(db, key) * PTR_SZ ) + db->hashoff;
	db->ptroff = db->chainoff;

	if(writelock){
		if(writew_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
			err_dump("_db_find_and_lock: error for writev_lock...");
	} else {	
		if(readw_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
			err_dump("_db_find_and_lock: error for readv_lock...");
	}
	offset = _db_readptr(db, db->ptroff);
	while(offset != 0){
		nextoffset = _db_readidx(db, offset);
		if(strcmp(db->idxbuf, key) == 0)
			break;
		db->ptroff = offset;
		offset = nextoffset;
	}
	return (offset == 0 ? -1 : 0);
}
char* db_fetch(DB *db, const char *key)
{
	char	*ptr;
	
	if(_db_find_and_lock(db, key, READ_LOCK) < 0){
		ptr = NULL;
		db->cnt_fetcherr++;
	}
	else {
		ptr = _db_readdat(db);
		db->cnt_fetchok++;
	}
		
	if(un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)	
		err_dump("db_fetch: un_lock error...");
	return ptr;
}
 void _db_dodelete(DB *db)
{
	int	i;
	char	*ptr;
	off_t	freeptr, saveptr;

	for(ptr=db->datbuf, i=0; i<db->datlen-1; i++)
		*ptr++ = SPACE;
	*ptr = 0;
	ptr = db->idxbuf;
	while(*ptr)
		*ptr++ = SPACE;
	
	if(writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_dodelete: writew_lock error...");
	
	_db_writedat(db, db->datbuf, db->datoff, SEEK_SET);
	freeptr = _db_readptr(db, FREE_OFF);
	saveptr = db->ptrval;
	_db_writeidx(db, db->idxbuf, db->idxoff, SEEK_SET, freeptr);
	_db_writeptr(db, FREE_OFF, db->idxoff);
	_db_writeptr(db, db->ptroff, saveptr);
	
	if(un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_dodelete: un_lock error...");
}
int db_delete(DB *db, const char *key)
{
	int	rc = 0;

	if(_db_find_and_lock(db, key, 1) == 0){
		_db_dodelete(db);
		db->cnt_delok++;
	} else {
		rc = -1;
		db->cnt_delerr++;
	}
	if(un_lock(db->idxfd, db->chainoff, SEEK_SET, 1) < 0)
		err_dump("db_delete: un_lock error...");
	return rc;
}
int _db_findfree(DB* db, int keylen, int datlen)
{
	off_t	offset, saveoffset, nextoffset;
	int	rc;

	if(writew_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_findfree: writew_lock error...");

	saveoffset = FREE_OFF;
	offset = _db_readptr(db, saveoffset); 
	while(offset != 0){
		nextoffset = _db_readidx(db, offset);
		if((db->datlen == datlen) && (strlen(db->idxbuf) == keylen))
			break;
		saveoffset = offset;
		offset = nextoffset;
	}
	if(offset == 0)
		rc = -1;
	else{
		_db_writeptr(db, saveoffset, db->ptrval);
		rc = 0;
	}

	if(un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("_db_findfree: un_lock error...");
	return rc;	
}

void db_close(DB *db)
{
	_db_free(db);
}

void _db_free(DB *db)
{
	if(db->idxfd >= 0)
		close(db->idxfd);
	if(db->datfd)
		close(db->datfd);
	if(db->idxbuf != NULL)
		free(db->idxbuf);
	if(db->datbuf != NULL)
		free(db->datbuf);
	if(db->name != NULL)
		free(db->name);
	free(db);
}

void db_rewind(DB *db)
{
	off_t	offset;

	offset = PTR_SZ*(db->nhash + 1);
	if((db->idxoff = lseek(db->idxfd, offset+1, SEEK_SET)) == -1)
		err_dump("db_rewind: lseek error");

}
char* db_nextrec(DB *db, char *key)
{
	char	c;
	char	*ptr;

	if(readw_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("db_nextrec: readw_lock error");
	
	do{	
		if(_db_readidx(db, 0) < 0){
			ptr = NULL;
			goto doreturn;
		}
		ptr = db->idxbuf;
		while((c = *ptr++) != 0 && c == SPACE)
			;
	}while(c == 0);

	if(key != NULL)
		strcpy(key, db->idxbuf);
	ptr = _db_readdat(db);
	db->cnt_nextrec++;

	doreturn:
	if(un_lock(db->idxfd, FREE_OFF, SEEK_SET, 1) < 0)
		err_dump("db_nextrec: un_lock error");

	return ptr;
}
