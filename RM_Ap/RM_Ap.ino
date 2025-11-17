/*
  Projeto: Servidor de Tarefa Aperiódica e Escalonamento Rate Monotonic
  Autor: Gabriella Arévalo Marques e Hebert Alan Kubis
  Curso: EMB5633 - Sistemas de Tempo Real (UFSC)
  Data: Novembro/2025

  Objetivos:
  - Implementar 3 tarefas periódicas com prioridades RM automáticas
  - Ativar uma tarefa aperiódica por botão usando interrupção + semáforo
  - Sinalizar execuções via LEDs
  - Medir tempos usando esp_timer_get_time()
  - Acionar um buzzer quando a tarefa aperiódica exceder o seu orçamento
*/

#include <Arduino.h>

#define NUM_TAREFAS 3

// ================================================================
//  Pinos dos LEDs para visualização das tarefas
// ================================================================
const int LED_T1 = 16;       // LED da tarefa periódica T1
const int LED_T2 = 5;        // LED da tarefa periódica T2
const int LED_T3 = 18;       // LED da tarefa periódica T3
const int LED_AP = 21;       // LED da tarefa aperiódica
const int LED_DEADLINE = 2;  // LED acende quando há deadline miss
const int BOTAO = 15;        // Botão que aciona a tarefa aperiódica

// ================================================================
//  Buzzer e orçamento de execução (budget)
// ================================================================
const int BUZZER = 32;        // Pino do buzzer
const uint32_t D_US = 9000;   // Orçamento de tempo máximo da tarefa aperiódica (9 ms)

// ================================================================
//  Estrutura de dados para tarefas periódicas
// ================================================================
typedef struct {
  const char *nome;
  uint32_t periodo_ms;        // Período da tarefa em milissegundos
  uint32_t carga_us;          // Tempo de execução simulado
  int pino_led;               // LED associado
  UBaseType_t prioridade;     // Prioridade atribuída pelo RM
  TaskHandle_t handle;        // Handle da task no FreeRTOS
  uint64_t total_exec_us;     // Soma dos tempos de execução
  uint32_t ativacoes;         // Contador de execuções
  uint32_t misses;            // Deadline misses detectados
} TarefaPeriodica;

// ================================================================
//  Tarefas periódicas definidas com seus períodos e cargas
// ================================================================
TarefaPeriodica tarefas[NUM_TAREFAS] = {
  {"T1", 200, 8000, LED_T1, 0, NULL, 0, 0, 0},
  {"T2", 400, 15000, LED_T2, 0, NULL, 0, 0, 0},
  {"T3", 600, 25000, LED_T3, 0, NULL, 0, 0, 0}
};

// Semáforo de sincronização da tarefa aperiódica
SemaphoreHandle_t semAperiodica;
TaskHandle_t tarefaAperiodicaHandle = NULL;

// ================================================================
//  busyWait() — Simula execução gastando tempo real de CPU
// ================================================================
void busyWait(uint32_t micros) {
  uint64_t inicio = esp_timer_get_time();
  while ((esp_timer_get_time() - inicio) < micros)
    asm volatile("nop");  // instrução vazia para evitar otimizações
}

// ================================================================
//  Atribuir prioridades RM automaticamente (menor período = maior prioridade)
// ================================================================
void atribuirPrioridadesRM() {
  // Ordena as tarefas pelo período
  for (int i = 0; i < NUM_TAREFAS - 1; i++) {
    for (int j = i + 1; j < NUM_TAREFAS; j++) {
      if (tarefas[j].periodo_ms < tarefas[i].periodo_ms) {
        TarefaPeriodica temp = tarefas[i];
        tarefas[i] = tarefas[j];
        tarefas[j] = temp;
      }
    }
  }

  // Atribui prioridades conforme RM
  for (int i = 0; i < NUM_TAREFAS; i++) {
    tarefas[i].prioridade = tskIDLE_PRIORITY + (NUM_TAREFAS - i);
    Serial.printf("Prioridade atribuída: %s -> %u\n", tarefas[i].nome, (unsigned)tarefas[i].prioridade);
  }
}

// ================================================================
//  Tarefa periódica genérica
// ================================================================
void tarefaPeriodica(void *pvParameters) {
  TarefaPeriodica *t = (TarefaPeriodica *)pvParameters;

  // Controle de periodicidade do FreeRTOS
  TickType_t ultimoTick = xTaskGetTickCount();
  TickType_t periodoTicks = pdMS_TO_TICKS(t->periodo_ms);

  while (1) {
    // Aguarda até o próximo instante exato de execução
    vTaskDelayUntil(&ultimoTick, periodoTicks);

    uint64_t inicio = esp_timer_get_time();
    digitalWrite(t->pino_led, HIGH);

    // Simula execução consumindo CPU
    busyWait(t->carga_us);

    digitalWrite(t->pino_led, LOW);
    uint64_t fim = esp_timer_get_time();
    uint64_t exec_us = fim - inicio;

    // Acumula métricas
    t->total_exec_us += exec_us;
    t->ativacoes++;

    // Verifica deadline miss
    if (exec_us > (t->periodo_ms * 1000)) {
      t->misses++;
      digitalWrite(LED_DEADLINE, HIGH);
      Serial.printf("[MISS] %s excedeu o período (%lluus > %u ms)\n",
                    t->nome, (unsigned long long)exec_us, t->periodo_ms);
      digitalWrite(LED_DEADLINE, LOW);
    }

    // Log
    Serial.printf("%s: exec=%lluus ativ=%u misses=%u\n",
                  t->nome, (unsigned long long)exec_us, t->ativacoes, t->misses);
  }
}

// ================================================================
//  Tarefa aperiódica — ativada via botão (ISR → semáforo)
// ================================================================
void tarefaAperiodica(void *pvParameters) {
  (void)pvParameters;

  while (1) {
    // Espera o semáforo liberado pela interrupção
    if (xSemaphoreTake(semAperiodica, portMAX_DELAY) == pdTRUE) {
      uint64_t inicio = esp_timer_get_time();
      Serial.printf("[APERIODICA] Iniciou em %lluus\n", (unsigned long long)inicio);

      // Simulação de execução (aprox. 8 ms)
      digitalWrite(LED_AP, HIGH);
      busyWait(7967);
      digitalWrite(LED_AP, LOW);

      uint64_t fim = esp_timer_get_time();
      uint64_t duracao = fim - inicio;

      Serial.printf("[APERIODICA] Terminou (Duração=%lluus)\n", (unsigned long long)duracao);

      // Verifica estouro do budget
      if (duracao > D_US) {
        Serial.printf("[BUDGET] Orçamento excedido (%lluus > %luus)\n",
                      (unsigned long long)duracao, D_US);

        // Alerta sonoro
        digitalWrite(BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200)); // buzzer ativo por 200 ms
        digitalWrite(BUZZER, LOW);
      }
    }
  }
}

// ================================================================
//  ISR do botão — libera a tarefa aperiódica via semáforo
// ================================================================
void IRAM_ATTR isrBotao() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Libera a tarefa aperiódica
  xSemaphoreGiveFromISR(semAperiodica, &xHigherPriorityTaskWoken);

  // Força troca de contexto se necessário
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// ================================================================
//  Análise de utilização do processador (U)
// ================================================================
void analisarUtilizacao() {
  double U = 0.0;

  // Cálculo da utilização real
  for (int i = 0; i < NUM_TAREFAS; i++) {
    if (tarefas[i].ativacoes == 0) continue;

    double Ci = (double)tarefas[i].total_exec_us / tarefas[i].ativacoes;
    double Ti = (double)tarefas[i].periodo_ms * 1000.0;
    U += Ci / Ti;
  }

  // Limite teórico (Liu & Layland)
  int n = NUM_TAREFAS;
  double U_bound = n * (pow(2.0, 1.0 / n) - 1.0);

  Serial.printf("\n========== ANÁLISE ==========\n");

  // Impressão das métricas de cada tarefa
  for (int i = 0; i < NUM_TAREFAS; i++) {
    double media = tarefas[i].ativacoes ?
                   (double)tarefas[i].total_exec_us / tarefas[i].ativacoes : 0.0;

    Serial.printf("%s -> T=%ums, C_médio=%.0fus, ativ=%u, misses=%u\n",
                  tarefas[i].nome, tarefas[i].periodo_ms, media,
                  tarefas[i].ativacoes, tarefas[i].misses);
  }

  Serial.printf("U_medido = %.3f (%.1f%%)\n", U, U * 100.0);
  Serial.printf("U_bound = %.3f (%.1f%%)\n", U_bound, U_bound * 100.0);

  if (U <= U_bound)
    Serial.println("✅ Sistema escalonável (U <= U_bound)");
  else
    Serial.println("⚠️ Sistema NÃO garantido (U > U_bound)");

  Serial.println("=============================\n");
}

// ================================================================
//  Setup do sistema
// ================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Sistema RM + Tarefa Aperiódica + Buzzer ===");

  // Configuração dos GPIOs
  pinMode(LED_T1, OUTPUT);
  pinMode(LED_T2, OUTPUT);
  pinMode(LED_T3, OUTPUT);
  pinMode(LED_AP, OUTPUT);
  pinMode(LED_DEADLINE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO, INPUT_PULLUP);

  // Criação do semáforo da tarefa aperiódica
  semAperiodica = xSemaphoreCreateBinary();

  // Interrupção do botão
  attachInterrupt(digitalPinToInterrupt(BOTAO), isrBotao, FALLING);

  // Atribui prioridades RM às tarefas periódicas
  atribuirPrioridadesRM();

  // Cria as tarefas periódicas
  for (int i = 0; i < NUM_TAREFAS; i++) {
    xTaskCreate(
      tarefaPeriodica,
      tarefas[i].nome,
      4096,
      (void *)&tarefas[i],
      tarefas[i].prioridade,
      &tarefas[i].handle
    );
  }

  // Cria a tarefa aperiódica
  xTaskCreate(
    tarefaAperiodica,
    "APERIODICA",
    4096,
    NULL,
    1,
    &tarefaAperiodicaHandle
  );

  Serial.println("Sistema iniciado com sucesso!");
}

// ================================================================
//  Loop principal — apenas roda a análise periódica
// ================================================================
void loop() {
  static uint64_t ultimo = 0;

  // Executa a análise a cada 10 segundos
  if (millis() - ultimo > 10000) {
    ultimo = millis();
    analisarUtilizacao();
  }

  vTaskDelay(pdMS_TO_TICKS(200));
}
