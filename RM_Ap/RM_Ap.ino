/*
  Projeto: Servidor de Tarefa Aperiódica e Escalonamento Rate Monotonic
  Autor: Gabriella Arévalo Marques e Hebert Alan Kubis
  Curso: EMB5633 - Sistemas de Tempo Real (UFSC)
  Data: Novembro/2025

  Objetivos:
  - 3 tarefas periódicas com períodos distintos (RM automático)
  - 1 tarefa aperiódica acionada por botão
  - Visualização prática via GPIOs (LEDs)
  - Registro e análise de tempos com esp_timer_get_time()
  - Buzzer avisa quando a tarefa aperiódica excede o orçamento (budget)
*/

#include <Arduino.h>

#define NUM_TAREFAS 3

// ==== Pinos dos LEDs ====
const int LED_T1 = 16;
const int LED_T2 = 5;
const int LED_T3 = 18;
const int LED_AP = 21;
const int LED_DEADLINE = 2; // pisca em caso de deadline missed
const int BOTAO = 15;       // botão para tarefa aperiódica

// ==== Pino do buzzer e orçamento ====
const int BUZZER = 32;
const uint32_t D_US = 8000; 

// ==== Estrutura de uma tarefa periódica ====
typedef struct {
  const char *nome;
  uint32_t periodo_ms;
  uint32_t carga_us;
  int pino_led;
  UBaseType_t prioridade;
  TaskHandle_t handle;
  uint64_t total_exec_us;
  uint32_t ativacoes;
  uint32_t misses;
} TarefaPeriodica;

// ==== Tarefas periódicas ====
TarefaPeriodica tarefas[NUM_TAREFAS] = {
  {"T1", 200, 8000, LED_T1, 0, NULL, 0, 0, 0},
  {"T2", 400, 15000, LED_T2, 0, NULL, 0, 0, 0},
  {"T3", 600, 25000, LED_T3, 0, NULL, 0, 0, 0}
};

// ==== Semáforo para tarefa aperiódica ====
SemaphoreHandle_t semAperiodica;
TaskHandle_t tarefaAperiodicaHandle = NULL;

// ==== Função utilitária: espera ocupada (simula execução CPU) ====
void busyWait(uint32_t micros) {
  uint64_t inicio = esp_timer_get_time();
  while ((esp_timer_get_time() - inicio) < micros) asm volatile("nop");
}

// ==== Função: atribui prioridades RM automaticamente ====
void atribuirPrioridadesRM() {
  for (int i = 0; i < NUM_TAREFAS - 1; i++) {
    for (int j = i + 1; j < NUM_TAREFAS; j++) {
      if (tarefas[j].periodo_ms < tarefas[i].periodo_ms) {
        TarefaPeriodica temp = tarefas[i];
        tarefas[i] = tarefas[j];
        tarefas[j] = temp;
      }
    }
  }
  for (int i = 0; i < NUM_TAREFAS; i++) {
    tarefas[i].prioridade = tskIDLE_PRIORITY + (NUM_TAREFAS - i);
    Serial.printf("Prioridade atribuída: %s -> %u\n", tarefas[i].nome, (unsigned)tarefas[i].prioridade);
  }
}

// ==== Tarefa periódica genérica ====
void tarefaPeriodica(void *pvParameters) {
  TarefaPeriodica *t = (TarefaPeriodica *)pvParameters;
  TickType_t ultimoTick = xTaskGetTickCount();
  TickType_t periodoTicks = pdMS_TO_TICKS(t->periodo_ms);

  while (1) {
    vTaskDelayUntil(&ultimoTick, periodoTicks);

    uint64_t inicio = esp_timer_get_time();
    digitalWrite(t->pino_led, HIGH);
    busyWait(t->carga_us);
    digitalWrite(t->pino_led, LOW);
    uint64_t fim = esp_timer_get_time();

    uint64_t exec_us = fim - inicio;
    t->total_exec_us += exec_us;
    t->ativacoes++;

    // Verifica deadline missed
    if (exec_us > (t->periodo_ms * 1000)) {
      t->misses++;
      digitalWrite(LED_DEADLINE, HIGH);
      Serial.printf("[MISS] %s excedeu o período (%lluus > %u ms)\n",
                    t->nome, (unsigned long long)exec_us, t->periodo_ms);
      digitalWrite(LED_DEADLINE, LOW);
    }

    Serial.printf("%s: exec=%lluus ativ=%u misses=%u\n",
                  t->nome, (unsigned long long)exec_us, t->ativacoes, t->misses);
  }
}

// ==== Tarefa aperiódica com medição e buzzer ====
void tarefaAperiodica(void *pvParameters) {
  (void)pvParameters;
  while (1) {
    if (xSemaphoreTake(semAperiodica, portMAX_DELAY) == pdTRUE) {
      uint64_t inicio = esp_timer_get_time();
      Serial.printf("[APERIODICA] Iniciou em %lluus\n", (unsigned long long)inicio);

      // Simula execução (6 a 10ms)
      digitalWrite(LED_AP, HIGH);
      busyWait(7967); // teste: 9ms (maior que o budget)
      digitalWrite(LED_AP, LOW);

      uint64_t fim = esp_timer_get_time();
      uint64_t duracao = fim - inicio;

      Serial.printf("[APERIODICA] Terminou (Duração=%lluus)\n", (unsigned long long)duracao);

      // Verifica se o orçamento foi ultrapassado
      if (duracao > D_US) {
        Serial.printf("[BUDGET] Orçamento excedido (%lluus > %luus)\n",
                      (unsigned long long)duracao, D_US);
        digitalWrite(BUZZER, HIGH); // liga o buzzer
        vTaskDelay(pdMS_TO_TICKS(200)); // 200 ms de aviso sonoro
        digitalWrite(BUZZER, LOW);  // desliga o buzzer
      }
    }
  }
}

// ==== ISR do botão ====
void IRAM_ATTR isrBotao() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(semAperiodica, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

// ==== Cálculo da utilização e comparação com U_bound ====
void analisarUtilizacao() {
  double U = 0.0;
  for (int i = 0; i < NUM_TAREFAS; i++) {
    if (tarefas[i].ativacoes == 0) continue;
    double Ci = (double)tarefas[i].total_exec_us / tarefas[i].ativacoes;
    double Ti = (double)tarefas[i].periodo_ms * 1000.0;
    U += Ci / Ti;
  }

  int n = NUM_TAREFAS;
  double U_bound = n * (pow(2.0, 1.0 / n) - 1.0);

  Serial.printf("\n========== ANÁLISE ==========\n");
  for (int i = 0; i < NUM_TAREFAS; i++) {
    double media = tarefas[i].ativacoes ? (double)tarefas[i].total_exec_us / tarefas[i].ativacoes : 0.0;
    Serial.printf("%s -> T=%ums, C_médio=%.0fus, ativ=%u, misses=%u\n",
                  tarefas[i].nome, tarefas[i].periodo_ms, media, tarefas[i].ativacoes, tarefas[i].misses);
  }
  Serial.printf("U_medido = %.3f (%.1f%%)\n", U, U * 100.0);
  Serial.printf("U_bound = %.3f (%.1f%%)\n", U_bound, U_bound * 100.0);
  if (U <= U_bound) Serial.println("✅ Sistema escalonável (U <= U_bound)");
  else Serial.println("⚠️  Sistema NÃO garantido (U > U_bound)");
  Serial.println("=============================\n");
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Sistema RM + Tarefa Aperiódica + Buzzer ===");

  pinMode(LED_T1, OUTPUT);
  pinMode(LED_T2, OUTPUT);
  pinMode(LED_T3, OUTPUT);
  pinMode(LED_AP, OUTPUT);
  pinMode(LED_DEADLINE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO, INPUT_PULLUP);

  semAperiodica = xSemaphoreCreateBinary();
  attachInterrupt(digitalPinToInterrupt(BOTAO), isrBotao, FALLING);

  atribuirPrioridadesRM();

  for (int i = 0; i < NUM_TAREFAS; i++)
    xTaskCreate(tarefaPeriodica, tarefas[i].nome, 4096, (void *)&tarefas[i], tarefas[i].prioridade, &tarefas[i].handle);

  xTaskCreate(tarefaAperiodica, "APERIODICA", 4096, NULL, 1, &tarefaAperiodicaHandle);

  Serial.println("Sistema iniciado com sucesso!");
}

// ==== Loop ====
void loop() {
  static uint64_t ultimo = 0;
  if (millis() - ultimo > 10000) {
    ultimo = millis();
    analisarUtilizacao();
  }
  vTaskDelay(pdMS_TO_TICKS(200));
}
