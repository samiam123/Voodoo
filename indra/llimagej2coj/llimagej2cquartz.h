// This #deine turns off inclusion of llstring.h. It's not needed in this
//  module and causes compile errors. -- TS
#define  LL_LLSTRING_H
#include "llimagej2c.h"

#ifdef __OBJC__
  #ifdef BOOL
    #undef BOOL
  #endif
#endif // __OBJC__

BOOL decodeJ2CQuartz(LLImageJ2C &base, LLImageRaw &raw_image, F32 decode_time, S32 first_channel, S32 max_channel_count);
