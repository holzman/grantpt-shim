#include "itoa.h"

const char _itoa_lower_digits[36]
= "0123456789abcdefghijklmnopqrstuvwxyz";

const char _itoa_upper_digits[36]
= "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

char * 
_itoa_word (_ITOA_WORD_TYPE value, char *buflim,
	    unsigned int base, int upper_case)
{
  const char *digits = (upper_case
			? _itoa_upper_digits
			: _itoa_lower_digits);

  switch (base)
    {
# define SPECIAL(Base)							      \
    case Base:								      \
      do								      \
	*--buflim = digits[value % Base];				      \
      while ((value /= Base) != 0);					      \
      break

      SPECIAL (10);
      SPECIAL (16);
      SPECIAL (8);
    default:
      do
	*--buflim = digits[value % base];
      while ((value /= base) != 0);
    }
  return buflim;
}
# undef SPECIAL

