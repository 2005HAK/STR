#include "main.h"
#include "miros.h"

// Stack da thread blink
uint32_t pilha_blink[80];
rtos::OSThread thread_blink;

// Função da thread
void funcao_blink() {
    while (1) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);  // LED LD2
        rtos::OS_delay(500);                    // espera 500 ms
    }
}

// Idle stack
uint32_t pilha_idle[40];

int main(void) {
    HAL_Init();

    // Clock de GPIOA
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Configura LED PA5 como saída
    GPIO_InitTypeDef gpio = { 0 };
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    // Inicializa o RTOS e idle
    rtos::OS_init(pilha_idle, sizeof(pilha_idle));

    // Cria a thread blink
    rtos::OSThread_start(&thread_blink, &funcao_blink, pilha_blink, sizeof(pilha_blink));

    // Inicia o escalonador (não retorna)
    rtos::OS_run();
}
