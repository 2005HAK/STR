/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "main.h"
#include <cstdint>
#include "miros.h"

uint32_t bufferSize = 10, occupiedPositions  = 0, head = 0, tail = 0;
uint32_t buffer[bufferSize];
rtos::OSSem mtx;
rtos::OSSem noEmptySpaces;
rtos::OSSem noItemsAvailable;

bool insert(uint32_t code){
	if(occupiedPositions < bufferSize){
		buffer[tail] = code;
		tail = (tail + 1) % bufferSize;
		occupiedPositions++;
		return true;
	} else{
		return false;
	}
}

uint32_t remove(){
	if(occupiedPositions > 0){
		uint32_t code = buffer[head];
		head = (head + 1) % bufferSize;
		occupiedPositions--;
		return code;
	}
	return 0;
}

uint32_t stackProd[40];
rtos::OSThread prod;
void producer(){
	uint32_t code = 1;

	while(1){
		rtos::OSSem_pend(&noEmptySpaces);

		rtos::OSSem_pend(&mtx);
		insert(code);
		rtos::OSSem_post(&mtx);

		rtos::OSSem_post(&noItemsAvailable);

		code++;
		rtos::OS_delay(rtos::TICKS_PER_SEC);
	}
}

uint32_t stackCons[40];
rtos::OSThread cons;
void consumer(){
	while(1){
		rtos::OSSem_pend(&noItemsAvailable);

		rtos::OSSem_pend(&mtx);
		remove();
		rtos::OSSem_post(&mtx);

		rtos::OSSem_post(&noEmptySpaces);

		rtos::OS_delay(rtos::TICKS_PER_SEC);
	}
}

uint32_t stack_idleThread[40];

int main(void){
	rtos::OS_init(stack_idleThread, sizeof(stack_idleThread));

	rtos::OSSem_init(&mtx, 1);
	rtos::OSSem_init(&noEmptySpaces, bufferSize);
	rtos::OSSem_init(&noItemsAvailable, 0);

	/* start blinky1 thread */
	rtos::OSThread_start(&prod, &producer, stackProd, sizeof(stackProd));

	/* start blinky2 thread */
	rtos::OSThread_start(&cons, &consumer, stackCons, sizeof(stackCons));

	/* transfer control to the RTOS to run the threads */
	rtos::OS_run();
}


