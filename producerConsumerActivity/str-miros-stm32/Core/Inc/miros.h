/*
 * miros.h
 *
 *  Created on: Feb 6, 2025
 *      Author: guilh
 */

#ifndef INC_MIROS_H_
#define INC_MIROS_H_

namespace rtos {
	/* Thread Control Block (TCB) */
	typedef struct {
		void *sp; /* stack pointer */
		uint32_t timeout; /* timeout delay down-counter */
		/* ... other attributes associated with a thread */
	} OSThread;

	const uint16_t TICKS_PER_SEC = 100U;

	typedef void (*OSThreadHandler)();

	void OS_init(void *stkSto, uint32_t stkSize);

	/* callback to handle the idle condition */
	void OS_onIdle(void);

	/* this function must be called with interrupts DISABLED */
	void OS_sched(void);

	/* transfer control to the RTOS to run the threads */
	void OS_run(void);

	/* blocking delay */
	void OS_delay(uint32_t ticks);

	/* process all timeouts */
	void OS_tick(void);

	/* callback to configure and start interrupts */
	void OS_onStartup(void);

	void OSThread_start(OSThread *me, OSThreadHandler threadHandler, void *stkSto, uint32_t stkSize);

	typedef struct {
		uint8_t value;
		uint32_t waitingSet;
	} OSSem;

	void OSSem_init(OSSem *me, uint8_t initialValue);

	void OSSem_pend(OSSem *me);

	void OSSem_post(OSSem *me);
}

#endif /* INC_MIROS_H_ */
