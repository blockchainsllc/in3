/** @file 
 * storage handler storing cache in the home-dir/.in3
 */ 


#include "../core/client/client.h"



bytes_t* storage_get_item(char* key);


void storage_set_item(char* key, bytes_t* content) ;