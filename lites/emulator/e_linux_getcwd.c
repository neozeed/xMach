/*      $NetBSD: getcwd.c,v 1.5 1995/06/16 07:05:30 jtc Exp $   */

/*
 * Copyright (c) 1989, 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <e_defs.h>
#include <sys/dirent.h>

#define ISDOT(dp) \
        (dp->d_name[0] == '.' && (dp->d_name[1] == '\0' || \
            dp->d_name[1] == '.' && dp->d_name[2] == '\0'))

static errno_t search_dir(int dir_fd, 
			  struct dirent* dp, 
			  unsigned int ino,
			  struct dirent** rde)
{
   errno_t err;
   unsigned int offset;
   unsigned int nread;
   struct dirent* de;
   unsigned int j;
   
   for (;;)
     {
	err = e_lite_getdirentries(dir_fd, 
				   dp, 
				   sizeof(struct dirent), 
				   &offset, 
				   &nread);
	
	if (err)
	  {
	     return(err);
	  }
	
	if (nread <= 0)
	  {
	     return(ENOENT);
	  }
	
	j = 0;
	while (j < nread)
	  {
	     de = (struct dirent *)(((char *)dp) + j);
	     
	     if (de->d_fileno == ino)
	       {
		  *rde = de;
		  return(ESUCCESS);
	       }

	     j = j + de->d_reclen;
	  }
     }
   return(ENOENT);
}

errno_t e_linux_getcwd(char* pt, size_t size, int* retval)
{  
   struct dirent dp;
   struct dirent* de;
   dev_t dev;
   ino_t ino;
   int first;
   char *bpt, *bup;
   struct stat s;
   dev_t root_dev;
   ino_t root_ino;
   size_t ptsize, upsize;
   int save_errno;
   char *ept, *eup, *up;
   int dir_fd;
   errno_t err;
   
#ifdef GETCWD_DEBUG   
   e_emulator_error("e_linux_getcwd(pt %x, size %d)", pt, size);
#endif   
   
   /*
    * If no buffer specified by the user, allocate one as necessary.
    * If a buffer is specified, the size has to be non-zero.  The path
    * is built from the end of the buffer backwards.
    */
   ptsize = 0;
   ept = pt + size;
   bpt = ept - 1;
   *bpt = '\0';
   
   /*
    * Only allocate as much as the path length
    */
   upsize = 1024 - 4;
   up = alloca(size);
   if (up  == NULL)
     {
#ifdef GETCWD_DEBUG
	e_emulator_error("alloca failed at %s:%d", __FILE__, __LINE__);
#endif	
	return(ENOMEM);
     }
   eup = up + MAXPATHLEN;
   bup = up;
   up[0] = '.';
   up[1] = '\0';
   
   /* Save root values, so know when to stop. */
   if ((err = e_lite_stat("/", &s)))
     {
#ifdef GETCWD_DEBUG
	e_emulator_error("stat failed at %s:%d", __FILE__, __LINE__);
#endif
	return(err);
     }
   root_dev = s.st_dev;
   root_ino = s.st_ino;

   for (first = 1;; first = 0) {
      /* Stat the current level. */
      if ((err = e_lite_lstat(up, &s)))
	{
#ifdef GETCWD_DEBUG
	   e_emulator_error("lstat failed at %s:%d\n", 
			    __FILE__, __LINE__);
#endif
	   return(err);
	}
      
      /* Save current node values. */
      ino = s.st_ino;
      dev = s.st_dev;
      
      /* Check for reaching root. */
      if (root_dev == dev && root_ino == ino) {
	 *--bpt = '/';
	 /*
	  * It's unclear that it's a requirement to copy the
	  * path to the beginning of the buffer, but it's always
	  * been that way and stuff would probably break.
	  */
	 memcpy(pt, bpt, ept - bpt);
#ifdef GETCWD_DEBUG
	 e_emulator_error("Found root: returning success <%s>\n",
			  pt);
#endif
	 *retval = strlen(pt);
	 return (ESUCCESS);
      }
      
      /*
       * Build pointer to the parent directory, allocating memory
       * as necessary.  Max length is 3 for "../", the largest
       * possible component name, plus a trailing NULL.
       */
      if (bup + 3  + MAXNAMLEN + 1 >= eup) {
#ifdef GETCWD_DEBUG
	 e_emulator_error("Failed with not enough space at %s:%d\n",
			  __FILE__, __LINE__);
#endif
	 return(ERANGE);
      }
      *bup++ = '.';
      *bup++ = '.';
      *bup = '\0';
      
//      if (!(err = e_open(up, O_DIRECTORY, 0, &dir_fd)))
      if ((err = e_open(up, O_RDONLY, 0, &dir_fd)))
	{
#ifdef GETCWD_DEBUG
	   e_emulator_error("open failed at %s:%d", __FILE__, __LINE__);
#endif
	   return(err);
	}
      if ((err = e_lite_fstat(dir_fd, &s))) 
	{
#ifdef GETCWD_DEBUG
	   e_emulator_error("fstat failed at %s:%d", __FILE__, __LINE__);
#endif
	   return(err);
	}
      
      /* Add trailing slash for next directory. */
      *bup++ = '/';
      
      /*
       * If it's a mount point, have to stat each element because
       * the inode number in the directory is for the entry in the
       * parent directory, not the inode number of the mounted file.
       */
      save_errno = 0;
      if (s.st_dev == dev) 
	{
	   err = search_dir(dir_fd, &dp, ino, &de);
	   if (err)
	     {
#ifdef GETCWD_DEBUG
		e_emulator_error("search_dir failed at %s:%d\n",
				 __FILE__, __LINE__);
#endif
		return(err);
	     }
	} 
      else
	{
#if 0
	   for (;;) 
	     {
		if (!(dp = readdir(dir)))
		  goto notfound;
		if (ISDOT(dp))
		  continue;
		memcpy(bup, dp->d_name, dp->d_namlen + 1);
		
		/* Save the first error for later. */
		if (lstat(up, &s)) 
		  {
		     if (!save_errno)
		       save_errno = errno;
		     errno = 0;
		     continue;
		  }
		if (s.st_dev == dev && s.st_ino == ino)
		  break;
	     }
#else
	   e_emulator_error("FIXME: Reached mountpoint\n");
#endif
	}
      
      /*
       * Check for length of the current name, preceding slash,
       * leading slash.
       */
      if (bpt - pt <= de->d_namlen + (first ? 1 : 2)) {
#ifdef GETCWD_DEBUG
	 e_emulator_error("Out of space at %s:%d\n",
			  __FILE__, __LINE__);
#endif
	    return(ERANGE);
      }
      if (!first)
	*--bpt = '/';
      bpt -= de->d_namlen;
      memcpy(bpt, de->d_name, de->d_namlen);
      e_close(dir_fd);
      
      /* Truncate any file name. */
      *bup = '\0';
   }
   
#ifdef GETCWD_DEBUG
   e_emulator_error("File not found at %s:%d\n", __FILE__, __LINE__);
#endif
   return(ENOENT);
}
