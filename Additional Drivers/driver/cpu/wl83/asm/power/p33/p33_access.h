#ifndef __P33_ACCESS_H__
#define __P33_ACCESS_H__

//
//
//					for p33 access
//
//
//
/**************************************************************/

#define p33_fast_access(reg, data, en)           \
{ 												 \
    if (en) {                                    \
		p33_or_1byte(reg, (data));               \
    } else {                                     \
		p33_and_1byte(reg, (u8)~(data));         \
    }                                            \
}

#endif
