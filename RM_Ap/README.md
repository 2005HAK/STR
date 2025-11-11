
# 📘 README — Servidor de Tarefa Aperiódica e Escalonamento Rate Monotonic (ESP32 + FreeRTOS)

### 👩‍💻 Autores
**Gabriella Arévalo Marques**  
**Hebert Alan Kubis**

**Disciplina:** EMB5633 - Sistemas de Tempo Real  
**Instituição:** Universidade Federal de Santa Catarina (UFSC)  
**Data:** Novembro de 2025  

---

## 🎯 Objetivo do Projeto

Implementar no **ESP32** (utilizando **FreeRTOS**) um sistema de escalonamento baseado em **Rate Monotonic (RM)** contendo:

- **3 tarefas periódicas** com períodos e tempos de execução distintos;  
- **Prioridades automáticas**, determinadas de acordo com o período (menor período → maior prioridade);  
- **1 tarefa aperiódica** acionada por um **evento real (botão físico)**;  
- **Registro e visualização prática** do escalonamento via **LEDs (GPIO)** e **Serial Monitor**;  
- **Medição de tempos reais de execução** e **análise de utilização U**;  
- **Buzzer sonoro** que sinaliza quando a tarefa aperiódica **excede seu orçamento de execução (budget)**.

---

## ⚙️ Descrição Geral do Sistema

O sistema implementa um **escalonador preemptivo** baseado na política **Rate Monotonic (RM)**, onde:
- Cada tarefa periódica \( \tau_i \) possui um **período Ti** e um **tempo de execução Ci**.  
- A **prioridade** é **inversamente proporcional** ao período.  
- As tarefas utilizam a função **`vTaskDelayUntil()`** para manter periodicidade estável.  

A **tarefa aperiódica** é ativada por uma **interrupção de botão (ISR)** e executa apenas quando o semáforo é liberado.  
Caso o **tempo de execução** da tarefa aperiódica **ultrapasse o limite de orçamento (budget_us)**, um **buzzer é acionado** para indicar a violação.

---

## 🧩 Estrutura do Projeto

### 🧱 Tarefas Periódicas
- **T1:** Período = 200 ms, Carga ≈ 8 ms  
- **T2:** Período = 400 ms, Carga ≈ 15 ms  
- **T3:** Período = 600 ms, Carga ≈ 25 ms  

Cada tarefa:
- Pisca um LED durante sua execução (GPIOs distintos).  
- Mede tempo real de execução usando `esp_timer_get_time()`.  
- Detecta **deadline misses** (quando tempo de execução > período).  
- Armazena estatísticas: número de ativações, tempo médio e misses.

### 🔘 Tarefa Aperiódica
- Ativada por **botão físico (GPIO 15)** via **interrupção (ISR)**.
- Sinaliza execução em um LED dedicado (**LED_AP**).  
- Mede o tempo total de execução.  
- Caso **duração > D_US (orçamento)**, o **buzzer (GPIO 32)** é acionado por 200 ms.  
- O orçamento está definido em:
  ```cpp
  const uint32_t D_US = 8000; // 8 milissegundos
  ```

### 🔔 Buzzer (Budget Overflow)
- **Pino:** GPIO 32  
- **Função:** Sinalizar quando a tarefa aperiódica ultrapassa seu tempo limite de execução.  
- O buzzer emite som durante **200 ms**.

---

## 📊 Cálculo e Métricas de Desempenho

### 1️⃣ **Medições Reais**
As funções `esp_timer_get_time()` e `xTaskGetTickCount()` foram utilizadas para medir:
- Tempo de execução real de cada tarefa;
- Jitter entre ativações;
- Latência da tarefa aperiódica.

### 2️⃣ **Utilização Total do Sistema**
A utilização real \( U \) é calculada conforme:

\[ U = \sum_{i=1}^{n} \frac{C_i}{T_i} \]

E comparada com o limite teórico de Liu & Layland:

\[ U_b = n(2^{1/n} - 1) \]

A verificação é feita periodicamente (a cada 10 segundos) via função `analisarUtilizacao()`:

```cpp
if (U <= U_bound)
  Serial.println("✅ Sistema escalonável (U <= U_bound)");
else
  Serial.println("⚠️  Sistema NÃO garantido (U > U_bound)");
```

---

## 🧠 Conceitos Aplicados

| Conceito | Implementação |
|-----------|----------------|
| **Rate Monotonic (RM)** | Prioridade inversa ao período |
| **Tarefas periódicas** | `vTaskDelayUntil()` |
| **Tarefa aperiódica** | ISR + semáforo binário (`xSemaphoreGiveFromISR`) |
| **Medição de tempo** | `esp_timer_get_time()` (µs) |
| **Jitter e deadline miss** | Verificados com diferença entre execuções |
| **Orçamento (budget)** | Tempo máximo de execução da tarefa aperiódica |
| **Buzzer sonoro** | Indica estouro do orçamento |

---

## 🔌 Ligações de Hardware

| Componente | GPIO | Função |
|-------------|------|--------|
| LED T1 | 16 | Indica execução da tarefa T1 |
| LED T2 | 5 | Indica execução da tarefa T2 |
| LED T3 | 18 | Indica execução da tarefa T3 |
| LED Aperiódica | 21 | Pisca durante execução da tarefa aperiódica |
| LED Deadline Miss | 2 | Pisca quando há deadline excedido |
| Botão | 15 | Ativa a tarefa aperiódica |
| Buzzer | 32 | Emite som quando o orçamento é excedido |

---

## 🧮 Resultados Esperados (exemplo de saída Serial)

```
=== Sistema RM + Tarefa Aperiódica + Buzzer ===
Prioridade atribuída: T1 -> 4
Prioridade atribuída: T2 -> 3
Prioridade atribuída: T3 -> 2
Sistema iniciado com sucesso!

T1: exec=7900us ativ=10 misses=0
T2: exec=15000us ativ=5 misses=0
T3: exec=25000us ativ=3 misses=0
[APERIODICA] Iniciou em 56780000us
[APERIODICA] Terminou (Duração=9100us)
[BUDGET] Orçamento excedido (9100us > 8000us)
🔔 Buzzer ativo por 200ms

========== ANÁLISE ==========
T1 -> T=200ms, C_médio=7900us, ativ=100, misses=0
T2 -> T=400ms, C_médio=15000us, ativ=50, misses=0
T3 -> T=600ms, C_médio=25000us, ativ=30, misses=0
U_medido = 0.475 (47.5%)
U_bound = 0.779 (77.9%)
✅ Sistema escalonável (U <= U_bound)
=============================
```

---

## 🧩 Conclusão

O projeto cumpre **integralmente** os requisitos do trabalho prático definido no Moodle UFSC:

✅ 3 tarefas periódicas com tempos distintos  
✅ Priorização automática (RM)  
✅ 1 tarefa aperiódica acionada por botão  
✅ Visualização real via LEDs e Serial  
✅ Medição de execução com `esp_timer_get_time()`  
✅ Cálculo e comparação de U e U_b  
✅ Buzzer sinalizando estouro de orçamento  

O sistema demonstra na prática o funcionamento do **escalonamento Rate Monotonic**, o comportamento **preemptivo do FreeRTOS**, e os efeitos do **budget excedido** em tarefas aperiódicas.



## 🧪 Ferramentas e Ambiente

- **Placa:** ESP32 DevKit v1  
- **IDE:** Arduino IDE / PlatformIO  
- **Framework:** FreeRTOS  
- **Linguagem:** C++ (Arduino core)  
- **Baud Rate Serial:** 115200  
