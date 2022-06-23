#define _LARGEFILE64_SOURCE

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include "itoa.h"
#include "ptsname-internal.h"

#define _PATH_TTY "/dev/tty"

const char __libc_ptyname1[] = "pqrstuvwxyzabcde";
const char __libc_ptyname2[] = "0123456789abcdef";

#define MASTER_P(Dev)							\
  (major ((Dev)) == 2						\
   || (major ((Dev)) == 4					\
       && minor ((Dev)) >= 128 && minor ((Dev)) < 192) \
   || (major ((Dev)) >= 128 && major ((Dev)) < 136))


#define SLAVE_P(Dev)							\
  (major ((Dev)) == 3						\
   || (major ((Dev)) == 4					\
       && minor ((Dev)) >= 192 && minor ((Dev)) < 256) \
   || (major ((Dev)) >= 136 && major ((Dev)) < 144))

int
__ptsname_internal (int fd, char *buf, size_t buflen, struct stat64 *stp)
{

  int save_errno = errno;
  unsigned int ptyno;

  if (!isatty (fd))
    {
      __set_errno (ENOTTY);
      return ENOTTY;
    }

#ifdef TIOCGPTN
  if (ioctl (fd, TIOCGPTN, &ptyno) == 0)
    {
      /* Buffer we use to print the number in.  For a maximum size for
	 `int' of 8 bytes we never need more than 20 digits.  */
      char numbuf[21];
      const char *devpts = _PATH_DEVPTS;
      const size_t devptslen = strlen (_PATH_DEVPTS);
      char *p;

      numbuf[sizeof (numbuf) - 1] = '\0';
      p = _itoa_word (ptyno, &numbuf[sizeof (numbuf) - 1], 10, 0);

      if (buflen < devptslen + (&numbuf[sizeof (numbuf)] - p))
	{
	  __set_errno (ERANGE);
	  return ERANGE;
	}

      memcpy (__stpcpy (buf, devpts), p, &numbuf[sizeof (numbuf)] - p);
    }
  else if (errno != EINVAL)
    return errno;
  else
#endif
    {
      char *p;

      if (buflen < strlen (_PATH_TTY) + 3)
	{
	  __set_errno (ERANGE);
	  return ERANGE;
	}
      if (__fxstat64 (_STAT_VER, fd, stp) < 0)
	return errno;
      /* Check if FD really is a master pseudo terminal.  */

      if (! MASTER_P (stp->st_rdev))
	{
	  __set_errno (ENOTTY);
	  return ENOTTY;
	}

      ptyno = minor (stp->st_rdev);

      if (ptyno / 16 >= strlen (__libc_ptyname1))
	{
	  __set_errno (ENOTTY);
	  return ENOTTY;
	}

      p = __stpcpy (buf, _PATH_TTY);
      p[0] = __libc_ptyname1[ptyno / 16];
      p[1] = __libc_ptyname2[ptyno % 16];
      p[2] = '\0';
    }

  if (__xstat64 (_STAT_VER, buf, stp) < 0)
    return errno;

  /* Check if the name we're about to return really corresponds to a
     slave pseudo terminal.  */
  if (! S_ISCHR (stp->st_mode) || ! SLAVE_P (stp->st_rdev))
    {
      /* This really is a configuration problem.  */
      __set_errno (ENOTTY);
      return ENOTTY;
    }

  __set_errno (save_errno);

  return 0;
}

