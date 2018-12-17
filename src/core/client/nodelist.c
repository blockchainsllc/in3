/** @file 
 * handles nodelists.
 * 
 * */ 


#include "client.h"
#include "nodelist.h"

/** removes all nodes and their weights from the nodelist */
int in3_nodelist_clear(in3_chain_t* chain) {
    int i;
    for (i=0;i<chain->nodeListLength;i++) {
        if (chain->nodeList[i].url)       free(chain->nodeList[i].url);
        if (chain->nodeList[i].address) b_free(chain->nodeList[i].address);
    }
    free(chain->nodeList);
    free(chain->weights);
    return 0;
}





