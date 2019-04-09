# ifndef NULL
# define NULL 0
# endif

extern "C"
{
#include "openssl/ssl.h"
}

int SSL_library_init(void){ return 0; }

void SSL_load_error_strings(void){}

const SSL_METHOD* SSLv23_method(void){ return NULL; }

unsigned long SSLeay(void){ return 0L; }