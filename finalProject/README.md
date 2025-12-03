# 📊 Escalonamento em Tempo Real: RM vs. EDF no ESP32

<div style="text-align: justify">
Este projeto implementa um sistema comparativo de escalonamento de tarefas em tempo real utilizando <strong>FreeRTOS</strong> no ESP32. O sistema permite alternar dinamicamente entre os algoritmos <strong>Rate Monotonic (RM)</strong> e <strong>Earliest Deadline First (EDF)</strong>, oferecendo visualização de métricas via interface Web.
</div>

## 🚀 Funcionalidades

* **Alternância de Escalonadores:**
    * **RM (Rate Monotonic):** Prioridade fixa baseada no período (menor período = maior prioridade).
    * **EDF (Earliest Deadline First):** Prioridade dinâmica baseada no prazo (deadline) mais próximo.
* **Dashboard Web:** Interface gráfica hospedada no ESP32 exibindo gráficos em tempo real de:
    * Carga da CPU (%).
    * Tempos de Execução e *Deadline Misses*.
    * Jitter (variação temporal).
    * Prioridades dinâmicas das tarefas.
* **Tarefas Simuladas:**
    * *CalcLoad:* Tarefa de cálculo de carga.
    * *Display:* Atualização de LCD físico.
    * *Aperiódica:* Tarefa pesada disparada por botão para teste de estresse.
* **Feedback Físico:** LCD 16x2, LED de status e Buzzer para alertas de *miss* (perda de prazo).

<div align="justify">

# 🕒 Diagrama de Escalonamento (Exemplo)

## RM — Prioridade fixa (menor período = maior prioridade)

**Linha do tempo →**\
T1: `|■■|    |■■|    |■■|    |■■|` \
T2: `    |■■■■|      |■■■■|`\
T3: `         |■■■■■■■■|`


## EDF — Prioridade dinâmica (menor deadline primeiro)

**Linha do tempo →**\
T1: `|■■| |■■| |■■| |■■|`\
T3: `     |■■■■|     |■■■■|`\
T2: `         |■■■■|`


# 🌐 Interface Web

A página HTML é enviada com `server.send()` e contém:

* Controle para selecionar **RM** ou **EDF**
* Gráfico de barras dos **tempos de execução**
* Gráfico de linhas dos **atrasos (jitter)**
* Indicadores de **deadline misses**
* Biblioteca carregada via:

```html
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
```

---

# 📡 Configuração do WiFi

O código usa modo **Station (WIFI_STA)** para permitir a importação online do Chart.js.

```cpp
WiFi.mode(WIFI_STA);
WiFi.begin(ssid, password);
```

Acessar no navegador:

```
http://<IP do ESP32>/
```

</div>


## 🛠️ Hardware e Pinagem

O projeto utiliza os seguintes pinos do ESP32:

| Componente | GPIO | Descrição |
| :--- | :--- | :--- |
| **Botão** | 15 | Dispara a tarefa aperiódica (Input Pull-down) |
| **LED Status** | 2 | Indica o estado do sistema |
| **Buzzer** | 22 | Alerta sonoro de *Deadline Miss* |
| **LCD - RS** | 5 | Sinal Register Select do LCD |
| **LCD - EN** | 4 | Sinal Enable do LCD |
| **LCD - D4** | 18 | Barramento de dados LCD |
| **LCD - D5** | 19 | Barramento de dados LCD |
| **LCD - D6** | 23 | Barramento de dados LCD |
| **LCD - D7** | 27 | Barramento de dados LCD |

## 📦 Dependências de Software

Para compilar este projeto, certifique-se de ter as seguintes bibliotecas instaladas na sua IDE (Arduino IDE ou PlatformIO):

1.  **ArduinoJson** (v6 ou superior)
2.  **LiquidCrystal** (Biblioteca padrão para LCDs paralelos)
3.  **WiFi** & **WebServer** (Nativas do core ESP32)

## ⚙️ Configuração e Execução

### 1. Configurar Credenciais Wi-Fi
Para que o dashboard funcione corretamente (baixando a biblioteca `Chart.js` da internet), é necessário configurar sua rede local. Edite as seguintes linhas no início do código:

```cpp
const char* ssid = "NOME_DA_SUA_REDE"; 
const char* password = "SUA_SENHA";
```
## 📝 Informações do Projeto

* **Autores:** Gabriella Arévalo e Hebert Alan Kubis
* **Matéria:** Sistemas de Tempo Real (2025.2)
* **Plataforma:** ESP32 (Arduino Framework)

