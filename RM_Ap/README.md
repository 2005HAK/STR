# 🚀 Servidor de Tarefa Aperiódica com Rate Monotonic (RM)

Este projeto implementa um sistema de escalonamento em tempo real (RTS) em um ESP32, combinando tarefas periódicas com prioridade fixa (Rate Monotonic) e uma tarefa aperiódica acionada por evento (botão). O sistema usa LEDs para visualização e um buzzer para sinalizar a ultrapassagem do deadline da tarefa aperiódica.

## 📋 Configuração e Objetivos

* **Algoritmo Base:** Rate Monotonic (RM). As prioridades das tarefas periódicas são atribuídas automaticamente (menor período = maior prioridade).
* **Servidor Aperiódico:** Tarefa de background acionada via Semáforo (ISR de botão).
* **Monitoramento:** Detecção de *Deadline Missed* (LED de alarme) e *Budget Overrun* (Buzzer).

## ⚙️ Tarefas e Parâmetros

| Nome | Período (T) | Carga (C) | Prioridade | Pino LED |
| :---: | :---: | :---: | :---: | :---: |
| **T1** | 200 ms | 8000 µs | Mais Alta | 16 |
| **T2** | 400 ms | 15000 µs | Média | 5 |
| **T3** | 600 ms | 25000 µs | Mais Baixa | 18 |
| **APERIODICA** | - | 7967 µs (Simulada) | Prioridade 1 (Background) | 21 |

**Deadline T. Aperiódica:** D_US = 8000 µs.

---

## 💾 Hardware e Diagrama de Conexão

Este projeto requer a seguinte montagem no ESP32:

![Esquema de Conexão RM_Ap](https://raw.githubusercontent.com/2005HAK/STR/master/RM_Ap/Esquema%20RM_Ap.png)

| Componente | Função | Pino (GPIO) |
| :---: | :---: | :---: |
| LED_T1 | Tarefa T1 (200ms) | 16 |
| LED_T2 | Tarefa T2 (400ms) | 5 |
| LED_T3 | Tarefa T3 (600ms) | 18 |
| LED_AP | Tarefa Aperiódica | 21 |
| LED_DEADLINE | Alarme (Miss) | 2 |
| BUZZER | Alerta de Budget | 32 |
| BOTÃO | Aciona T. Aperiódica | 15 |

---
## 💡 Como Usar

1.  Conecte os componentes ao ESP32 (verifique as conexões GND e VIN no diagrama).
2.  Carregue o código para o ESP32.
3.  Monitore a **saída serial (115200)** para ver os tempos de execução (`exec=...`) e a análise de utilização (`U_medido`).
4.  Pressione o **botão (GPIO 15)** para acionar a tarefa aperiódica. O **Buzzer** soará se o tempo de execução exceder o deadline de 8000 µs da T. Aperiódica.