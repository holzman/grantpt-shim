#define _LARGEFILE64_SOURCE
#define __USE_LARGEFILE64
#define _LARGEFILE_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ptsname-internal.h"
#include "pty-private.h"

static char buffer[sizeof (_PATH_DEVPTS) + 20];

static int
pts_name (int fd, char **pts, size_t buf_len, struct stat64 *stp)
{
  int rv;
  char *buf = *pts;

  for (;;)
    {
      char *new_buf;

      if (buf_len)
	{
	  rv = __ptsname_internal (fd, buf, buf_len, stp);

	  if (rv != 0)
	    {
	      if (rv == ENOTTY)
		/* ptsname_r returns with ENOTTY to indicate
		   a descriptor not referring to a pty master.
		   For this condition, grantpt must return EINVAL.  */
		rv = EINVAL;
	      errno = rv;	/* Not necessarily set by __ptsname_r.  */
	      break;
	    }

	  if (memchr (buf, '\0', buf_len))
	    /* We succeeded and the returned name fit in the buffer.  */
	    break;

	  /* Try again with a longer buffer.  */
	  buf_len += buf_len;	/* Double it */
	}
      else
	/* No initial buffer; start out by mallocing one.  */
	buf_len = 128;		/* First time guess.  */

      if (buf != *pts)
	/* We've already malloced another buffer at least once.  */
	new_buf = (char *) realloc (buf, buf_len);
      else
	new_buf = (char *) malloc (buf_len);
      if (! new_buf)
	{
	  rv = -1;
	  __set_errno (ENOMEM);
	  break;
	}
      buf = new_buf;
    }

  if (rv == 0)
    *pts = buf;		/* Return buffer to the user.  */
  else if (buf != *pts)
    free (buf);		/* Free what we malloced when returning an error.  */

  return rv;
}

int
grantpt (int fd)
{
  int retval = -1;
#ifdef PATH_MAX
  char _buf[PATH_MAX];
#else
  char _buf[512];
#endif
  char *buf = _buf;
  struct stat64 st;
  if (__glibc_unlikely (pts_name (fd, &buf, sizeof (_buf), &st)))
    {
      int save_errno = errno;

      /* Check, if the file descriptor is valid.  pts_name returns the
	 wrong errno number, so we cannot use that.  */
      if (fcntl (fd, F_GETFD) == -1 && errno == EBADF)
	return -1;

       /* If the filedescriptor is no TTY, grantpt has to set errno
	  to EINVAL.  */
       if (save_errno == ENOTTY)
	 __set_errno (EINVAL);
       else
	 __set_errno (save_errno);

       return -1;
    }

  /* Make sure that we own the device.  */
  uid_t uid = getuid ();
  if (st.st_uid != uid)
    {
      if (chown (buf, uid, st.st_gid) < 0)
	goto helper;
    }

  static int tty_gid = -1;
  if (__glibc_unlikely (tty_gid == -1))
    {
      char *grtmpbuf;
      struct group grbuf;
      size_t grbuflen = __sysconf (_SC_GETGR_R_SIZE_MAX);
      struct group *p;

      /* Get the group ID of the special `tty' group.  */
      if (grbuflen == (size_t) -1L)
	/* `sysconf' does not support _SC_GETGR_R_SIZE_MAX.
	   Try a moderate value.  */
	grbuflen = 1024;
      grtmpbuf = (char *) alloca (grbuflen);
      getgrnam_r (TTY_GROUP, &grbuf, grtmpbuf, grbuflen, &p);
      if (p != NULL)
	tty_gid = p->gr_gid;
    }
  gid_t gid = tty_gid == -1 ? getgid () : tty_gid;

  mode_t mode = S_IRUSR|S_IWUSR|
	        ((st.st_gid == gid) ? (st.st_mode & S_IWGRP) : 0);

  if ((st.st_mode & ACCESSPERMS) != mode)
    {
      if (chmod (buf, mode) < 0)
	goto helper;
    }

  retval = 0;
  goto cleanup;

  /* We have to use the helper program if it is available.  */
 helper:;

 cleanup:
  if (buf != _buf)
    free (buf);

  return retval;
}
