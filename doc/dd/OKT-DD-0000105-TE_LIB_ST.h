/*
 * Test Environment: Storage Library
 * API definition
 *
 * Copyright (C) 2003 Test Environment authors (see file AUTHORS in the
 * root directory of the distribution).
 *
 * Test Environment is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * Test Environment is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA  02111-1307  USA
 *
 * Author: Andrey G. Ivanov  <andron@oktetlabs.ru>
 *
 * @(#) $Id$
 */

#ifndef __OKT_DD_0000105_TE_LIB_ST_H__
#define __OKT_DD_0000105_TE_LIB_ST_H__

/*
 * COMMENTS ABOUT const char *storage
 *
 * Three functions (strg_fopen, strg_fopendir and strg_mkdir) have
 * a parameter const char *storage. This parameter describes the storage
 * be used.
 *
 * For the file system-bases storage it is just a prefix (path to the
 * storage). Something like: "/home/tester/storage".
 *
 * For cvs-based storage this string should be in the format
 * cvs://<cvs root>\n<cvs password>\n<cvs module>
 *
 * The function strg_set_default_storage defines default storage. After it
 * called NULL can be passed to the strg_fopen/strg_opendir/strg_mkdir as
 * storage parameter. The default storage will be used.
 */

/* strg_set_default_storage -
 *     Set default storage to use in strg_fopen/strg_opendir/strg_mkdir.
 *
 *     This function defines the default storage. Default storage will be
 *     used by strg_fopen/strg_opendir/strg_mkdir functions if parameter
 *     storage is NULL. Error will be generated if
 *     strg_fopen/strg_opendir/strg_mkdir called with storage=NULL w/o
 *     previously calling of the strg_set_default_storage is an error.
 *
 * PARAMETERS:
 *     default_storage - String with default storage description
 *
 * RETURNS:
 *     none
 * 
 * SIDE EFFECT:
 *     This function just copies passed string to the internals of storage.
 * This function does not tries to access the storage in any way.
 */
void strg_set_default_storage(const char *default_storage);

/* 
 * File access functions.
 * Prototypes of the following function's are taken from the stdio library.
 *
 * For the file system-based storage those functions will be mapped to the
 * stdio's functions, except for the strg_open and strg_remove functions.
 * For strg_open/strg_remove functions filenames will be modified before
 * calling functions from stdio.h.
 *
 * For other storages (CVS-base, for example) those functions must be
 * implemented. It seems to be possible to use stdio's functions. When the
 * file is opened/created, the temporary file in the local file system will
 * be created (copied from the storage). The strg_fseek/strg_fput/etc
 * functions will call stdio's functions for local file. When strg_close is
 * called, the file will be written to the real storage.
 */


/* strg_fopen --
 *     Open the file.
 *
 *     For simple file system-based storage the function just opens the
 *     file.  For other storages (CVS, archive, etc) this function obtains
 *     specified file (takes it from the CVS or extracts it from the
 *     archive) and copies it to the local file system. Then fopen function
 *     called for this local file.
 *
 * PARAMETERS:
 *     storage   - The string with information about the storage to be
 *                 used. 
 *     path/mode - See fopen man page.
 *
 * RETURNS:
 *     See fopen man page.
 */
FILE *strg_fopen(const char *storage, const char *path, const char *mode);

/* strg_fclose --
 *     Close the file.
 *
 *     For simple file system-based storage the function just closes the
 *     file.  For other storages (CVS, archive, etc) this function first
 *     closes the local file. Then it writes specified file to the storage
 *     (puts it into the CVS or archive) and deletes local copy. 
 
 * PARAMETERS:
 *     stream - Pointer to the stream returned by strg_fopen.
 *
 * RETURNS:
 *     0  - success
 *     -1 - fail
 */
int strg_fclose(FILE *stream);

/* strg_fflush --
 *     Flush a stream.
 *
 *     This function forces a write of all user-space buffered data for
 * the given stream. 
 *
 *     For simple file system-based storage the function just flushes the
 *     file.  For other storages (CVS, archive, etc) this function first
 *     flushes the local file. Then it writes specified file to the storage
 *     (puts it into the CVS or archive). The local copy of the file is not
 *     deleted (in contrast to the strg_close).
 *
 * PARAMETERS:
 *     stream - Pointer to the stream returned by strg_fopen.
 *
 * RETURNS:
 *     0  - success
 *     -1 - fail
 */
int strg_fflush(FILE *stream);

/*
 * The following functions will be mapped to the corresponding stdio's 
 * functions.
 */
int strg_feof(FILE *stream);
int strg_fseek(FILE *stream, long offset, int whence);
long strg_ftell(FILE *stream);
void strg_rewind(FILE *stream);
int strg_fgetc(FILE *stream);
char *strg_fgets(char *s, int size, FILE *stream);
int strg_fputc(int c, FILE *stream);
int strg_fputs(const char *s, FILE *stream);
int strg_fprintf(FILE *stream, const char *format, ...);
int strg_remove(const char *pathname);

/*
 * Directory access functions.
 *
 * Those functions are analogous to dirent.h functions.
 *
 */

/* strg_opendir --
 *     Open the directory stream.
 *
 * PARAMETERS:
 *     storage - The string with information about the storage to be used. 
 *     name    - String with path to the directory.
 *
 * RETURNS:
 *     pointer to the directory stream on success
 *     NULL on failure
 */
DIR *strg_opendir(const char *storage, const char *name);

/* strg_closedir --
 *     Close the directory stream.
 *
 * PARAMETERS:
 *     dir - Pointer to the directory stream returned by strg_opendir.
 *
 * RETURNS:
 *     0  on success
 *     -1 on failure
 */
int strg_closedir(DIR *dir);

/* strg_readdir --
 *     Read next entry in the directory structure.
 *
 *     Returns a pointer to a dirent structure representing the next
 *     directory entry in the directory stream pointed to  by dir. Only the
 *     field char d_name[] can be used in the returned structure. Returned
 *     structure may be overwritten by  subsequent calls to readdir.
 *
 * PARAMETERS:
 *     dir - Pointer to the directory stream returned by strg_opendir.
 *
 * RETURNS:
 *     Pointer to the dirent structure on success
 *     NULL on failure.
 */
struct dirent *strg_readdir(DIR *dir);

/* strg_seekdir --
 *     Set the location in the directory  stream.
 *
 * PARAMETERS:
 *     dir    - Pointer to the directory stream returned by strg_opendir.
 *     offset - Offset returned by telldir().
 *
 * RETURNS:
 *     none
 */
void strg_seekdir(DIR *dir, off_t offset);

/* strg_telldir --
 *     Return current location in directory stream.
 *
 * PARAMETERS:
 *     dir - Pointer to the directory stream returned by strg_opendir.
 *
 * RETURNS:
 *     the current location  in  the  directory stream on success
 *     -1 on error
 */
off_t strg_telldir(DIR *dir);

/* strg_rewind --
 *     Reset directory stream.
 *
 *     Resets the position of the directory stream to the beginning of the 
 * directory.
 *
 * PARAMETERS:
 *     dir  - Pointer to the directory stream returned by strg_opendir.
 *     
 * RETURNS:
 *     none
 */
void strg_rewinddir(DIR *dir);

/* 
 * mkdir is not a function from stdio.h or dirent.h.
 * It is from sys/stat.h
 */

/* strg_mkdir --
 *     Create directory.
 *
 * PARAMETERS:
 *     storage  - The string with information about the storage to be used.
 *     pathname - String with path to directory to be created.
 *     mode     - Access permissions.
 *
 * RETURNS
 *     0  - success
 *     -1 - failure
 */
int strg_mkdir(const char *storage, const char *pathname, mode_t mode);

/* strg_storage_to_local --
 *     copies the files, directory (possibly with subdirectories) from
 *     storage into specified directory in the local file system. 
 *
 * PARAMETERS:
 *     storage       - The string with information about the storage to be 
 *                     used.
 *     st_obj_name   - String with filename or directory to be copied.
 *     local_dirname - String with path to local destination directory.
 *     recursive     - Recursive flag: 0 - non-recursive mode, 
 *                                     1 - recursive mode.
 *                         
 * RETURNS:
 *     0 (success) or errno value
 */
int strg_storage_to_local(const char *storage, const char *st_obj_name, 
                          const char *local_dirname, int recursive);

/* strg_local_to_storage --
 *     copies the files, directory (possibly with subdirectories) specified 
 *     directory in the local file system into the storage
 *
 * PARAMETERS:
 *     storage       - The string with information about the storage to be 
 *                     used.
 *     local_dirname - String with path to local destination directory.
 *     st_obj_name   - String with filename or directory to be copied.
 *     recursive     - Recursive flag: 0 - non-recursive mode, 
 *                                     1 - recursive mode.
 *                         
 * RETURNS:
 *     0 (success) or errno value
 */
int strg_local_to_storage(const char *storage, const char *local_dirname, 
                          const char *st_obj_name, int recursive);

#endif /* __OKT_DD_0000105_TE_LIB_ST_H__ */

