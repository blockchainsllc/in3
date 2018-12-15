
#include "rlp.h"

static ref(bytes_t* b, size_t l, uint8_t* s, int r) {
    b->len  = l;
    b->data = s;
    return r;
}

int rlp_decode(bytes_t* b, size_t index, bytes_t* dst) {
    size_t p,i,l,n;
    uint8_t c;
    for (p=0,i=0;i<b->len;i++,p++) {
        c=b->data[i];
        if (c<0x80)      // single byte-item
            if (p==index) return ref(dst,1,b->data+i,1);
        else if (c<0xb8) {  // 0-55 length-item
            if (p==index) {
                dst->len  = c -0x80;
                dst->data = b->data + i+1;
                return 1;
            }
            i+=c -0x80;
        } 
        else if (c<0xc0) { // very long item
            for (l=0,n=0;n<c-0xb7;n++) l|= (*(b->data+i+1+n)) << ( 8*((c-0xb7)-n-1) );
            if (p==index) {
                dst->len  = l;
                dst->data = b->data + i + c-0xb7 + 1;
                return 1;
            }
            i+= l + c-0xb7;
        }
        else if (c<0xf8) { // 0-55 byte long list
            l = c- 0xc0;
            if (p==index) {
                dst->len  = l;
                dst->data = b->data + i +1;
                return 2;
            }
            i+=l+1;
        }
        else { // very long list
            for (l=0,n=0;n<c-0xf7;n++) l|= (*(b->data+i+1+n)) << ( 8*((c-0xf7)-n-1) );
            if (p==index) {
                dst->len  = l;
                dst->data = b->data + i + c-0xf7 + 1;
                return 2;
            }
            i+= l + c-0xf7;
        }
    }
    return 0;

}
