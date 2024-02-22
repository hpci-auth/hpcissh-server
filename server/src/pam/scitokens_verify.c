/*
 * System includes.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <scitokens/scitokens.h>
/*
 * Local includes.
 */
#include "logger.h"
#include "config.h"
#include "account_map.h"

int scitoken_verify(const char * auth_line, const struct config * config, const char * scitoken_requested_user)
{
    SciToken scitoken;
    char *err_msg;

    if(auth_line == NULL)
    {
        logger(LOG_TYPE_INFO,
        	"Token == NULL");
        return 0;
    }

    if(sizeof(config->issuers) == 0)
    {
        logger(LOG_TYPE_INFO,
		"No issuers in config");
	return 0;
    }

    size_t numberofissuers = sizeof(config->issuers)/sizeof(config->issuers[0]);

    char *null_ended_list[numberofissuers+1];

    //Read list of issuers from configuration and create a null ended list of strings
    for(size_t i = 0; i<numberofissuers; i++) null_ended_list[i] = config->issuers[i];
    null_ended_list[numberofissuers] = NULL;

    if(sizeof(auth_line)>1000*1000)
    {
        logger(LOG_TYPE_INFO,
        	"SciToken too large");
        return 0;
    }

    if(scitoken_deserialize(auth_line, &scitoken, (const char * const*)null_ended_list, &err_msg))
    {
        logger(LOG_TYPE_INFO,
        	"Failed to deserialize scitoken %s \n %s + %s",
		auth_line,err_msg,config->issuers[0]);
        free(err_msg);
        return 0;
    }

    char* issuer_ptr = NULL;
    if(scitoken_get_claim_string(scitoken, "iss", &issuer_ptr, &err_msg))
    {
        logger(LOG_TYPE_INFO,
        	"Failed to get claim \n %s",
        	err_msg);
        free(err_msg);
        return 0;
    }

    //Preparing for enforcer test
    Enforcer enf;
    const char *aud_list[1] = { NULL };
    const char **audp;
    if (config->audiences) {
        audp = (const char **)(config->audiences);
    } else {
        audp = aud_list;
    }

    char audiences[1024] = { 0 };
    size_t len = 0;
    for (int i = 0; *(audp+i); i++)
    {
        strncpy(&audiences[len], audp[i], sizeof(audiences) - 1 - len);
        len = strnlen(audiences, sizeof(audiences));
        if (len >= sizeof(audiences) - 1)
        {
            break;
        }

        audiences[len] = ' ';
        len++;
    }
    audiences[sizeof(audiences) - 1] = '\0';
    logger(LOG_TYPE_INFO, "audiences: %s", audiences);

    if (!(enf = enforcer_create(issuer_ptr, audp, &err_msg)))
    {
        logger(LOG_TYPE_INFO,
        	"Failed to create enforcer\n %s",
        	err_msg);
        free(err_msg);
        return 0;
    }

    Acl acl;
    acl.authz = "";
    acl.resource = "";

    if (enforcer_test(enf, scitoken, &acl, &err_msg))
    {
	logger(LOG_TYPE_INFO,
	       "Failed enforcer test %s %s",
	       err_msg, audiences);
	free(err_msg);
        return 0;
    }

    char* subject_ptr = NULL;
    if(scitoken_get_claim_string(scitoken, "hpci.id", &subject_ptr, &err_msg))
    {
        logger(LOG_TYPE_INFO,
               "Failed to get claim \n %s",
               err_msg);
        free(err_msg);
        return 0;
    }

    struct account_map * account_map;
    account_map = account_map_init_without_globus(config);

    char **local_accounts = acct_to_local_accounts(account_map, subject_ptr);
    if (local_accounts == NULL)
    {
        logger(LOG_TYPE_INFO,
               "subject %s does not exist in account map",
                subject_ptr);
        account_map_fini(account_map);
        return 0;
    }

    logger(LOG_TYPE_INFO, "requested user: %s", scitoken_requested_user);
    for (int i = 0; local_accounts[i]; i++) {
        logger(LOG_TYPE_INFO, "compared to local account: %s", local_accounts[i]);
        if (strcmp(local_accounts[i], scitoken_requested_user) == 0) {
            account_map_fini(account_map);
            return 1;
        }
    }
    logger(LOG_TYPE_INFO, "this user is not authorized as requested user");
    account_map_fini(account_map);

    return 0;
}
