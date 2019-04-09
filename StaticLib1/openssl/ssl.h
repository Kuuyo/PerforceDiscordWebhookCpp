#ifndef HEADER_SSL_H 
#define HEADER_SSL_H 

#define SSL_METHOD void

int SSL_library_init (void);
void SSL_load_error_strings (void);
const SSL_METHOD *SSLv23_method(void);
unsigned long SSLeay(void);
#endif // HEADER_SSL_H 
