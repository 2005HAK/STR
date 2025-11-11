# 🖥 Servidor de Tarefa Aperiódica e Escalonamento Rate Monotonic (RM)

## 📘 Descrição do Projeto

Este projeto implementa, em um **`ESP32`**, um **sistema de tempo real** baseado no **`escalonamento Rate Monotonic (RM)`**, com suporte a uma **tarefa aperiódica** acionada por botão físico.

O sistema simula a execução de três tarefas periódicas e uma tarefa aperiódica, com análise automática de utilização e alarme sonoro caso o tempo de execução da tarefa aperiódica ultrapasse um **orçamento máximo `budget`** definido.


## ⚙️ Funcionalidades

| Componente | Descrição |
|-------------|------------|
| **T1, T2, T3** | Tarefas periódicas com tempos e prioridades distintas. |
| **Tarefa Aperiódica** | Disparada via botão, executa fora do escalonamento regular. |
| **LEDs** | Indicam a execução de cada tarefa e *deadline missed*. |
| **Buzzer** | Toca se a tarefa aperiódica exceder o tempo limite budget. |
| **Cálculo RM** | Análise periódica da utilização do processador e comparação com o limite teórico de Liu & Layland. |


## </> Estrutura do Projeto
Estrutura do Código

- `TarefaPeriodica`: estrutura de dados das tarefas (período, carga, prioridade, etc.)  
- `busyWait()`: simula carga de CPU (espera ocupada)  
- `atribuirPrioridadesRM()`: define prioridades conforme o período (menor período = maior prioridade)  
- `tarefaPeriodica()`: rotina genérica para todas as tarefas periódicas  
- `tarefaAperiodica()`: executa quando o botão é pressionado  
- `isrBotao()`: interrupção que libera o semáforo da tarefa aperiódica  
- `analisarUtilizacao()`: calcula `U_medido` e compara com `U_bound`

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

## 📊 Cálculo e Métricas de Desempenho

### 1️⃣ **Medições Reais**
As funções `esp_timer_get_time()` e `xTaskGetTickCount()` foram utilizadas para medir:
- Tempo de execução real de cada tarefa;
- Jitter entre ativações;
- Latência da tarefa aperiódica.


### 2️⃣ **Utilização Total do Sistema**
## 📐 Cálculo da Utilização e Limite de Liu & Layland

A **utilização real** do processador é dada por:

\[
U = \sum_{i=1}^{n} \frac{C_i}{T_i}
\]

onde:
- \( C_i \) = tempo de computação da tarefa *i*  
- \( T_i \) = período da tarefa *i*  
- \( n \) = número de tarefas periódicas  

O valor obtido é comparado com o **limite teórico de Liu & Layland**:

\[
U_b = n \, (2^{1/n} - 1)
\]

Se \( U \leq U_b \), o sistema é **escalonável** sob a política **Rate Monotonic (RM)**.


A verificação é feita periodicamente (a cada 10 segundos) via função `analisarUtilizacao()`:

```cpp
if (U <= U_bound)
  Serial.println("✅ Sistema escalonável (U <= U_bound)");
else
  Serial.println("⚠️  Sistema NÃO garantido (U > U_bound)");
```

## 🔌 Ligação Esquematica de Pinos
![Esquema de Ligações](https://github.com/2005HAK/STR/blob/master/RM_Ap/Esquema%20RM_Ap.png?raw=true)



| Componente | Pino ESP32 | Função |
|-------------|-------------|--------|
| LED T1 | 16 | Tarefa periódica 1 |
| LED T2 | 5 | Tarefa periódica 2 |
| LED T3 | 18 | Tarefa periódica 3 |
| LED Aperiódica | 21 | Execução da tarefa aperiódica |
| LED Deadline | 2 | Sinaliza *deadline missed* |
| Botão | 15 | Aciona a tarefa aperiódica |
| Buzzer | 32 | Sinal de aviso de budget excedido |


## 🕹️ Como Usar

1. Carregue o código no **ESP32 Dev Module**.  
2. Abra o **Monitor Serial** (115200 baud).  
3. Observe os LEDs piscando de acordo com o período de cada tarefa.  
4. Pressione o **botão (pino 15)** para acionar a tarefa aperiódica.  
   - Se ela ultrapassar o **tempo limite de 8000 µs**, o **buzzer será acionado**.  
5. A cada 10 segundos, o sistema exibe uma **análise de utilização e escalonabilidade**.


## 🧠 Conceitos Aplicados

| Conceito | Implementação |
|-----------|----------------|
| **Rate Monotonic (RM)** | menor período → maior prioridade |
| **Tarefas periódicas** | `vTaskDelayUntil()` |
| **Tarefa aperiódica** | ISR + semáforo binário `xSemaphoreGiveFromISR` |
| **Medição de tempo** | `esp_timer_get_time()` (µs) |
|**Escalonabilidade** | comparação  `U`<sub>`medido`</sub> ≤ `U`<sub>`bound`</sub>
| **Jitter e deadline miss** | Verificados com diferença entre execuções |
| **Orçamento (budget)** | Tempo máximo de execução da tarefa aperiódica |
| **Buzzer sonoro** | Indica estouro do orçamento |
| **FreeRTOS** | usado para tarefas e semáforos| 


## 📊 Exemplo de Saída Serial

```
Prioridade atribuída: T1 -> 4
Prioridade atribuída: T2 -> 3
Prioridade atribuída: T3 -> 2
Sistema iniciado com sucesso!

T1: exec=8021us ativ=1 misses=0
T2: exec=14987us ativ=1 misses=0
T3: exec=24870us ativ=1 misses=0

[APERIODICA] Iniciou em 785602us
[APERIODICA] Terminou (Duração=9044us)
[BUDGET] Orçamento excedido (9044us > 8000us)
  🔔 Buzzer ativo por 200ms

========== ANÁLISE ==========
T1 -> T=200ms, C_médio=8021us, ativ=50, misses=0
T2 -> T=400ms, C_médio=14987us, ativ=25, misses=0
T3 -> T=600ms, C_médio=24870us, ativ=17, misses=0
U_medido = 0.217 (21.7%)
U_bound = 0.779 (77.9%)
  ✅ Sistema escalonável (U <= U_bound)
=============================
```


## 📈 Diagrama de Tempo (Timeline)

Representação simplificada da execução das tarefas sob o escalonador **Rate Monotonic**:

```
Tempo →
|----200ms----|----400ms----|----600ms----|----800ms----|

T1: ███ ███ ███ ███ ███ ███ ███ ███ ███ ███   (período = 200ms)
T2: ██████████       ██████████       █████   (período = 400ms)
T3: ██████████████████               ███████   (período = 600ms)
AP:           *---Execução on-demand---*        (acionada por botão)
```

🔹 **Símbolos:**
- `███` → Execução de tarefa  
- `*` → Início da tarefa aperiódica  
- O escalonador **preempte** tarefas de menor prioridade conforme RM
```
## 🔄 Fluxo de Execução


                ┌────────────────────────────┐
                │     Sistema Inicializa     │
                └────────────┬───────────────┘
                             │
          ┌──────────────────┴──────────────────┐
          │                                     │
 ┌────────▼────────┐                  ┌─────────▼─────────┐
 │  Criação das    │                  │  ISR do Botão     │
 │  Tarefas RM     │                  │(Tarefa Aperiódica)│
 └────────┬────────┘                  └─────────┬─────────┘
          │                                     │
 ┌────────▼────────┐                  ┌─────────▼─────────┐
 │ Execução RM     │                  │  Executa tarefa   │
 │ (T1, T2, T3)    │                  │  aperiódica       │
 └────────┬────────┘                  └─────────┬─────────┘
          │                                     │
 ┌────────▼───────────────────────────┐         │
 │  Análise periódica de utilização   │         │
 │  (U_medido vs U_bound)             │         │
 └────────────────────────────────────┘         │
          │                                     │
          └─────────────────────────────────────┘
```

## 📎 Requisitos

- **Placa:** ESP32 Dev Module  
- **IDE:** Arduino IDE (versão 2.x)  
- **Bibliotecas:** incluídas no pacote ESP32 (FreeRTOS e esp_timer)



## 🔔 Observações

- Ajuste o **budget** em:
  ```c
  const uint32_t D_US = 9000;
  ```
- Modifique as cargas das tarefas em:
  ```c
  {"T1", 200, 8000, LED_T1, ...}
  ```
- O código pode ser expandido para incluir servidores de tarefas aperiódicas (ex.: *Deferrable Server*, *Sporadic Server*).

---

**Autores:** Gabriella Arévalo Marques e Hebert Alan Kubis  
**Curso:** EMB5633 – Sistemas de Tempo Real (UFSC)  
**Data:** Novembro de 2025  

---
