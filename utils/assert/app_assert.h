/*
 * app_assert.h
 *
 *  Created on: 20-Sep-2023
 *      Author: Mahesh Murty
 */

#ifndef __APP_ASSERT_H__
#define __APP_ASSERT_H__				1

#include "utils/trice/trice.h"

#define STR_HELPER(x)					#x
#define STR(x)							STR_HELPER(x)

#ifndef TEST

#define APP_ASSERT(expr)										\
    if (!(expr)) {												\
      TRICE_S(ID(7868), "err: Assert triggered %s ", __FILE__);\
      trice(iD(4544), "err::%u \n", __LINE__);					\
      APP_ASSERT_DUMP_LOGS();									\
    }
#else
	#define APP_ASSERT(expr)
#endif

void APP_ASSERT_DUMP_LOGS(void);

#endif /* __APP_ASSERT_H__ */
