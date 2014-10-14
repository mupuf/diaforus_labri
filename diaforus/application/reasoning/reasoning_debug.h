#ifndef REASONING_DEBUG_H_
#define REASONING_DEBUG_H_

#include "common_config.h"

#include "sim_common.h"

#if IS_SIMU && DEBUG_REASONING
	#define DEBUG debug
        #define PRINTF(...) debug("REASONING", LOG_INFO, __VA_ARGS__)
#else
	#define DEBUG(...)
        #define PRINTF(...)
#endif

#endif /* REASONING_DEBUG_H_ */
