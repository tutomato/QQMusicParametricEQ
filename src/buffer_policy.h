#ifndef QQ_BUFFER_POLICY_H
#define QQ_BUFFER_POLICY_H
#include <stdint.h>
/* QMP can reject a valid driver buffer solely because it exceeds 2x its hint.
 * Accept only the native exclusive request, validated rate, and <= 500 ms.
 * Returned device size and PCM are never modified. */
static unsigned qq_buffer_hint(unsigned actual,unsigned requested,unsigned rate,unsigned mode){
 if(mode!=1||rate<8000||rate>768000||requested<128||requested>rate/2||actual>rate/2||actual==0)return requested;
 return actual>2*requested?actual:requested;
}
#endif
