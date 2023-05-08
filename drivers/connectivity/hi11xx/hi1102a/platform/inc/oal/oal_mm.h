

#ifndef __OAL_MM_H__
#define __OAL_MM_H__

/* 其他头文件包含 */
#include "oal_types.h"
#include "arch/oal_mm.h"

/* 宏定义 */
/* 内存清零 */
#define OAL_MEMZERO(_p_buf, _ul_size) oal_memset((_p_buf), 0, (_ul_size))

#endif /* end of oal_mm.h */

