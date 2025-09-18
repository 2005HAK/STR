#include <cstdint>
#include "miros.h"
#include "qassert.h"
#include "stm32g4xx.h"

Q_DEFINE_THIS_FILE

namespace rtos {

// Ponteiros para a thread atual e a próxima
OSThread * volatile OS_curr;
OSThread * volatile OS_next;

// Array com todas as threads criadas (max 32 + idle)
OSThread *OS_thread[32 + 1];

// Bitmask indicando quais threads estão prontas para executar
uint32_t OS_readySet;

// Contadores de threads
uint8_t OS_threadNum;
uint8_t OS_currIdx;

// Thread ociosa (executa quando nenhuma outra está pronta)
OSThread idleThread;
void main_idleThread() {
    while (1) {
        OS_onIdle(); // Aguarda interrupção (economia de energia)
    }
}

// Inicializa o RTOS e cria a thread idle
void OS_init(void *stkSto, uint32_t stkSize) {
    *(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16); // Prioridade mais baixa para PendSV
    OSThread_start(&idleThread, &main_idleThread, stkSto, stkSize);
}

// Função de escalonamento: escolhe a próxima thread a executar
void OS_sched(void) {
    if (OS_readySet == 0U) {
        OS_currIdx = 0U; // Idle thread
    } else {
        do {
            OS_currIdx++;
            if(OS_currIdx == OS_threadNum){
                OS_currIdx = 1;
            }
            OS_next = OS_thread[OS_currIdx];
        } while ((OS_readySet & (1U << (OS_currIdx - 1U))) == 0);
    }
    OS_next = OS_thread[OS_currIdx];
    if(OS_next != OS_curr){
        *(uint32_t volatile *)0xE000ED04 = (1U << 28); // Aciona PendSV
    }
}

// Começa a execução do RTOS
void OS_run(void) {
    OS_onStartup();
    __disable_irq();
    OS_sched();
    __enable_irq();
    Q_ERROR(); // Nunca deve ser alcançado
}

// Handler do SysTick: decrementa delays das threads
void OS_tick(void) {
    for(uint8_t n = 1U; n < OS_threadNum; n++) {
        if(OS_thread[n]->timeout != 0U){
            OS_thread[n]->timeout--;
            if(OS_thread[n]->timeout == 0U){
                OS_readySet |= (1U << (n-1U));
            }
        }
    }
}

// Bloqueia a thread atual por um tempo
void OS_delay(uint32_t ticks) {
    __asm volatile ("cpsid i");
    Q_REQUIRE(OS_curr != OS_thread[0]); // Não pode ser chamada pela idle
    OS_curr->timeout = ticks;
    OS_readySet &= ~(1U << (OS_currIdx - 1U));
    OS_sched();
    __asm volatile ("cpsie i");
}

// Inicia uma nova thread
void OSThread_start(OSThread *me, OSThreadHandler threadHandler, void *stkSto, uint32_t stkSize) {
    uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
    uint32_t *stk_limit;

    Q_REQUIRE((OS_threadNum < Q_DIM(OS_thread)) && (OS_thread[OS_threadNum] == nullptr));

    *(--sp) = (1U << 24);            // xPSR
    *(--sp) = (uint32_t)threadHandler; // PC
    *(--sp) = 0x0000000EU;           // LR
    *(--sp) = 0x0000000CU;           // R12
    *(--sp) = 0x00000003U;           // R3
    *(--sp) = 0x00000002U;           // R2
    *(--sp) = 0x00000001U;           // R1
    *(--sp) = 0x00000000U;           // R0
    *(--sp) = 0x0000000BU;           // R11
    *(--sp) = 0x0000000AU;           // R10
    *(--sp) = 0x00000009U;           // R9
    *(--sp) = 0x00000008U;           // R8
    *(--sp) = 0x00000007U;           // R7
    *(--sp) = 0x00000006U;           // R6
    *(--sp) = 0x00000005U;           // R5
    *(--sp) = 0x00000004U;           // R4

    me->sp = sp;
    stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);

    for (sp = sp - 1U; sp >= stk_limit; --sp) {
        *sp = 0xDEADBEEF; // Marca espaço não usado
    }

    OS_thread[OS_threadNum] = me;
    if (OS_threadNum > 0U) {
        OS_readySet |= (1U << (OS_threadNum - 1U));
    }
    OS_threadNum++;
}

// Configura clock do sistema (HSI + PLL)
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
}

// Função chamada ao iniciar o RTOS (configura SysTick)
void OS_onStartup(void) {
    SystemClock_Config();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / TICKS_PER_SEC);
    NVIC_SetPriority(SysTick_IRQn, 0U);
}

// Função executada pela thread idle
void OS_onIdle(void) {
#ifdef NDEBUG
    __WFI(); // Espera por interrupção (baixo consumo)
#endif
}

} // Fim do namespace rtos

// Funções de semáforo
void rtos::OSSem_init(OSSem *s, int16_t count) {
    s->count = count;
}

void rtos::OSSem_wait(OSSem *s) {
    __disable_irq();
    while (s->count == 0) {
        __enable_irq();
        OS_delay(1);
        __disable_irq();
    }
    s->count--;
    __enable_irq();
}

void rtos::OSSem_signal(OSSem *s) {
    __disable_irq();
    s->count++;
    __enable_irq();
}

// Assert customizado: reinicia o sistema se algo inesperado acontecer
void Q_onAssert(char const *module, int loc) {
    (void)module;
    (void)loc;
    NVIC_SystemReset();
}

// Handler PendSV: realiza troca de contexto entre threads
__attribute__ ((naked, optimize("-fno-stack-protector")))
void PendSV_Handler(void) {
__asm volatile (
    "  CPSID         I                 \n"
    "  LDR           r1,=_ZN4rtos7OS_currE       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CBZ           r1,PendSV_restore \n"
    "  PUSH          {r4-r11}          \n"
    "  LDR           r1,=_ZN4rtos7OS_currE       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  STR           sp,[r1,#0x00]     \n"
    "PendSV_restore:                   \n"
    "  LDR           r1,=_ZN4rtos7OS_nextE       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           sp,[r1,#0x00]     \n"
    "  LDR           r1,=_ZN4rtos7OS_nextE       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=_ZN4rtos7OS_currE       \n"
    "  STR           r1,[r2,#0x00]     \n"
    "  POP           {r4-r11}          \n"
    "  CPSIE         I                 \n"
    "  BX            lr                \n"
    );
}
