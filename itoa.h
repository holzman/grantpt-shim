#ifndef _ITOA_H_
#define _ITOA_H_

#define _ITOA_WORD_TYPE	unsigned long int

char *_itoa_word (_ITOA_WORD_TYPE value, char *buflim,
	    unsigned int base, int upper_case);

#endif
