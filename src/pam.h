#ifndef _PAM_H
#define _PAM_H
#include <security/pam_misc.h>
#include <security/pam_appl.h>
int login(char *username, char *password,pam_handle_t **pam_handle);
int logout(pam_handle_t *pam_handle);
#endif
