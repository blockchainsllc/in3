/** @file 
 * storage handler storing cache in the home-dir/.in3
 */

#include "../core/client/client.h"

bytes_t* storage_get_item(void* cptr, char* key);

void storage_set_item(void* cptr, char* key, bytes_t* content);