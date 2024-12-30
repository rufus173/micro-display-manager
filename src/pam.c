#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>

#define PAM_ERR(fname,pam_handle,err_code) (fprintf(stderr,"%s(): %s() error: %s\n",__FUNCTION__,fname,pam_strerror(pam_handle,err_code)))

static pam_handle_t *default_pam_handle;

int pam_conversation_func(int num_msg, const struct pam_message **msg, struct pam_response **resp,void *appdata_ptr); //callback function
int pam_login(char *username, char *password,pam_handle_t **pam_handle);
int pam_logout(pam_handle_t *pam_handle);
int pam_conversation_func(int num_msg, const struct pam_message **msg, struct pam_response **response,void *appdata_ptr){
	int pam_result = PAM_SUCCESS;
	//appdata_ptr = conversation_data from previous struct
	char **conversation_data = (char**)appdata_ptr;
	*response = calloc(num_msg,sizeof(struct pam_response)); //alloc num_messages structs for us to fill in with a response
	if (response == NULL){
		return PAM_BUF_ERR;
	}
	
	//deal with messages
	for (int i = 0; i < num_msg; i++){ //its almost like an argc and argv of messages
		switch( msg[i]->msg_style){
		case PAM_TEXT_INFO:
			printf("%s\n",msg[i]->msg);
			break;
		case PAM_ERROR_MSG:
			fprintf(stderr,"pam error: %s\n",msg[i]->msg);
			pam_result = PAM_CONV_ERR;
			break;
		case PAM_PROMPT_ECHO_ON: //it wants the username (redundant as provided in pam_start)
			(*response)[i].resp = strdup(conversation_data[0]); //username
			//printf("sending username: %s\n",conversation_data[0]);
			break;
		case PAM_PROMPT_ECHO_OFF:
			(*response)[i].resp = strdup(conversation_data[1]); //password
			//printf("sending password: %s\n",conversation_data[1]);
			break;
		}
		//stop when an error has occured
		if (pam_result != PAM_SUCCESS){
			break;
		}
	}

	if (pam_result != PAM_SUCCESS){
		free(*response);
		*response = 0; //return an error
	}

	return PAM_SUCCESS;
}
int pam_login(char *username, char *password,pam_handle_t **pam_handle){
	int return_status = -1; //returned when there is an error
	int pam_result; //needs to be passed to pam_end at the end
	int result = 0; //for other glibc functions
	char *conversation_data[2] = {username,password};
	struct pam_conv conversation = {pam_conversation_func,conversation_data};

	if (pam_handle == NULL) pam_handle = &default_pam_handle; //use a global handle


	//start pam
	pam_result = pam_start("micro-display-manager",NULL,&conversation,pam_handle);
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_start",*pam_handle,pam_result);
		goto end;
	}

	//set PAM_TTY
	pam_result = pam_set_item(*pam_handle,PAM_TTY,"/dev/tty1");
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_result",*pam_handle,pam_result);
		goto end;
	}

	//authenitcate
	pam_result = pam_authenticate(*pam_handle,0);
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_authenticate",*pam_handle,pam_result);
		goto end;
	}

	//idk what this does but you need to do it
	pam_result = pam_acct_mgmt(*pam_handle, 0);
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_acct_mgmt",*pam_handle,pam_result);
		goto end;
	}

	pam_result = pam_setcred(*pam_handle, PAM_ESTABLISH_CRED);
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_setcred",*pam_handle,pam_result);
		goto end;
	}

	pam_result = pam_open_session(*pam_handle, 0);
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_open_session",*pam_handle,pam_result);
		goto end;
	}

	//set the list of env variables the pam modules give us to set
	char **pam_env_variables = pam_getenvlist(*pam_handle);
	for (int i = 0; pam_env_variables[i] != NULL; i++){
		printf("%s\n",pam_env_variables[i]);
		//split string
		char *var = pam_env_variables[i];
		char *value = strchr(pam_env_variables[i],'=');
		value[0] = '\0';
		value++;
		setenv(var,value,0);
		free(pam_env_variables[i]);
	}
	free(pam_env_variables);
	//-----successfull exit-------
	return 0;
	//---only comes here on failure---
	end:
	result = pam_end(*pam_handle,result);
	if (pam_result != PAM_SUCCESS){
		fprintf(stderr,"logout(): pam_end() error: %s\n",pam_strerror(*pam_handle,pam_result));
	}
	return return_status;
}
int pam_logout(pam_handle_t *pam_handle){
	int pam_result = PAM_SUCCESS; //placeholder so pam_end doesnt bug
	int return_status = 0;

	if (pam_handle == NULL) pam_handle = default_pam_handle; //use a global handle
	
	pam_result = pam_close_session(pam_handle, 0);
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_close_session",pam_handle,pam_result);
		return_status = -1;
		goto end;
	}

	pam_result = pam_setcred(pam_handle, PAM_DELETE_CRED); //delete credential
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_setcred",pam_handle,pam_result);
		return_status = -1;
		goto end;
	}

	end:
	pam_result = pam_end(pam_handle,pam_result);//always called even if an error occurs
	if (pam_result != PAM_SUCCESS){
		PAM_ERR("pam_end",pam_handle,pam_result);
		return EXIT_FAILURE;
	}
	return return_status;
}
