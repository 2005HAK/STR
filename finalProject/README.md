# 📊 Escalonamento em Tempo Real: RM vs. EDF no ESP32

<div style="text-align: justify">
Este projeto implementa um sistema comparativo de escalonamento de tarefas em tempo real utilizando <strong>FreeRTOS</strong> no ESP32. O sistema permite alternar dinamicamente entre os algoritmos <strong>Rate Monotonic (RM)</strong> e <strong>Earliest Deadline First (EDF)</strong>, oferecendo visualização de métricas via interface Web.
</div>

## 📌 Descrição do Projeto

Este projeto implementa, em um ESP32, um sistema de escalonamento comutável entre:

* **RM (Rate Monotonic)** – prioridades fixas baseadas no período (menor período = maior prioridade).
* **EDF (Earliest Deadline First)** – prioridades dinâmicas baseadas no prazo (deadline) mais próximo.
* **Dashboard Web:** Interface gráfica hospedada no ESP32 exibindo gráficos em tempo real de:
    * Carga da CPU (%).
    * Tempos de Execução e *Deadline Misses*.
    * Jitter (variação temporal).
    * Prioridades dinâmicas das tarefas.
* **Tarefas Simuladas:**
    * *CalcLoad:* Tarefa de cálculo de carga.
    * *Display:* Atualização de LCD físico.
    * *Aperiódica:* Tarefa pesada disparada por botão para teste de estresse.
* **Feedback Físico:** 
    * Exibe métricas em LCD 16×2
    * Coleta e envia dados em formato JSON
    * Possui interface web com gráficos em Chart.js
    * Monitora uso de CPU, jitter, tempos de execução, misses e prioridades
    * Inclui tarefa aperiódica acionada por botão
    * Inclui cálculo dinâmico de carga da CPU
    * Opera em modo Wi-Fi Station (WIFI_STA) para usar Chart.js online

## 🧩 Funcionalidades do Sistema

### ✔️ 1. Alternância dinâmica RM ↔ EDF

* Interruptor por software através da interface web
* Atualização visual no LCD (“RM → EDF” e vice-versa)
* Prioridades reconfiguradas em tempo real

### ✔️ 2. Tarefas Periódicas

| Tarefa   | Período | Função                      |
| -------- | ------- | --------------------------- |
| CalcLoad | 300 ms  | Calcula carga total da CPU  |
| Display  | 500 ms  | Atualiza LCD com CPU e modo |

### ✔️ 3. Tarefa Aperiódica

* Acionada por interrupção do botão (GPIO 15)
* Executa carga simulada (busy wait 8,5 ms)
* Reporta deadline misses com buzzer

### ✔️ 4. Interface Web Moderna (Chart.js)

**Gráficos disponíveis:**

* CPU Load (%)
* Execução (µs)
* Deadline Misses
* Jitter (ms)
* Prioridade (dinâmica – EDF)

**Interface inclui:**

* Botões RM / EDF
* Abas de visualização
* Atualização a cada 250 ms


## 🌐 Interface Web

A página HTML é enviada com `server.send()` e contém:

* Controle para selecionar **RM** ou **EDF**
* Gráfico de barras dos `tempos de execução`
* Gráfico de linhas dos `atrasos (jitter)`
* Indicadores de `deadline misses`
* Requisições periódicas para `/metrics`
* Biblioteca carregada via:

```html
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
```


## 🔌 Pinos Utilizados

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

## ⚙️ Configuração e Execução

### 1. Configurar Credenciais Wi-Fi
Para que o dashboard funcione corretamente (baixando a biblioteca `Chart.js` da internet), é necessário configurar sua rede local. Edite as seguintes linhas no início do código:

```cpp
const char* ssid = "NOME_DA_SUA_REDE"; 
const char* password = "SUA_SENHA";
```

## 📊 Comparativo de Escalonamento em Tempo Real: RM vs. EDF (ESP32)

<div style="text-align: justify">
Este projeto implementa e compara dois algoritmos clássicos de escalonamento de tempo real — <strong>Rate Monotonic (RM)</strong> e <strong>Earliest Deadline First (EDF)</strong> — utilizando o <strong>FreeRTOS</strong> em um microcontrolador ESP32.

O sistema permite a alternância dinâmica entre os modos de escalonamento e oferece um dashboard web para visualização de métricas em tempo real (Carga da CPU, Jitter, Misses e Prioridades).
</div>

## 🚀 Funcionalidades

### Troca de Escalonador em Tempo Real
* **RM (Rate Monotonic):** Prioridades fixas baseadas no período (menor período = maior prioridade).
* **EDF (Earliest Deadline First):** Prioridades dinâmicas reatribuídas em tempo de execução com base no prazo (deadline) mais próximo.

### Tarefas do Sistema
* **Display** (Periódica, 500ms): Atualiza o LCD físico.
* **CalcLoad** (Periódica, 300ms): Calcula a carga da CPU.
* **Aperiodica** (Esporádica): Disparada por botão, simula uma carga pesada (~8.5ms) para testar a robustez do sistema.

### Dashboard Web
* Interface HTML hospedada no próprio ESP32.
* Gráficos via **Chart.js** para monitoramento de Jitter, execução e deadlines perdidos.
* Controle remoto para alternar entre RM e EDF.

### Feedback Físico
* Display LCD 16x2 para status local.
* Buzzer para alerta sonoro de perda de prazo (*Deadline Miss*).

## 🛠️ Hardware Necessário e Pinagem

| Componente | Pino ESP32 (GPIO) | Função |
| :--- | :--- | :--- |
| **Botão** | GPIO 15 | Dispara tarefa aperiódica (Interrupção) |
| **Buzzer** | GPIO 22 | Alerta de Deadline Miss |
| **LED Status** | GPIO 2 | Indica modo de operação (RM = Ligado) |
| **LCD - RS** | GPIO 5 | Controle do LCD |
| **LCD - EN** | GPIO 4 | Controle do LCD |
| **LCD - D4** | GPIO 18 | Dados LCD |
| **LCD - D5** | GPIO 19 | Dados LCD |
| **LCD - D6** | GPIO 23 | Dados LCD |
| **LCD - D7** | GPIO 27 | Dados LCD |

## 📦 Dependências de Software

Certifique-se de instalar as seguintes bibliotecas na IDE do Arduino ou PlatformIO:

* **LiquidCrystal** (para controle do LCD paralelo)
* **ArduinoJson** (versão 6 ou superior, para serialização dos dados do dashboard)
* **WiFi** & **WebServer** (Nativas do core ESP32)

## ⚙️ Configuração e Instalação

### 1. Configurar Wi-Fi
<div style="text-align: justify">
Abra o código e localize as seguintes linhas para inserir as credenciais da sua rede (necessário para baixar a biblioteca Chart.js no navegador):
</div>

```cpp
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SUA_SENHA";
```
### 2. Upload
Compile e carregue o código para o seu ESP32.

### 3. Acessar o Dashboard
* Abra o Monitor Serial (Baud Rate: 115200).

* Reinicie o ESP32.

* Copie o endereço IP exibido (ex: 192.168.1).

* Cole o endereço IP no navegador do seu computador ou celular (conectado à mesma rede).

## 🧠 Como Funciona o EDF no FreeRTOS

<div style="text-align: justify">
O FreeRTOS nativamente é um sistema de prioridade fixa (preemptivo). Para simular o EDF, este projeto utiliza uma técnica de reatribuição dinâmica:

* As tarefas calculam seus próprios deadlines absolutos.

* A função aplicarEDF() é chamada periodicamente.

* Ela ordena as tarefas com base no next_deadline.

* A função utiliza vTaskPrioritySet() para alterar a prioridade das tarefas no kernel do FreeRTOS, garantindo que a tarefa com o prazo mais curto tenha a maior prioridade numérica naquele instante.
</div>

## 📊 Comparativo de Escalonamento em Tempo Real: RM vs. EDF (ESP32)

<div style="text-align: justify">
Este projeto implementa e compara dois algoritmos clássicos de escalonamento de tempo real — <strong>Rate Monotonic (RM)</strong> e <strong>Earliest Deadline First (EDF)</strong> — utilizando o <strong>FreeRTOS</strong> em um microcontrolador ESP32.
</div>

<br>

<div style="text-align: justify">
O sistema permite a alternância dinâmica entre os modos de escalonamento e oferece um dashboard web para visualização de métricas em tempo real (Carga da CPU, Jitter, Misses e Prioridades).
</div>

## 📝 Informações do Projeto

* **Autores:** Gabriella Arévalo e Hebert Alan Kubis
* **Matéria:** Sistemas de Tempo Real (2025.2)
* **Plataforma:** ESP32 (Arduino Framework)

## 🚀 Funcionalidades

### Troca de Escalonador em Tempo Real
* **RM (Rate Monotonic):** Prioridades fixas baseadas no período (menor período = maior prioridade).
* **EDF (Earliest Deadline First):** Prioridades dinâmicas reatribuídas em tempo de execução com base no prazo (deadline) mais próximo.

### Tarefas do Sistema
* **Display** (Periódica, 500ms): Atualiza o LCD físico.
* **CalcLoad** (Periódica, 300ms): Calcula a carga da CPU.
* **Aperiodica** (Esporádica): Disparada por botão, simula uma carga pesada (~8.5ms) para testar a robustez do sistema.

### Dashboard Web
* Interface HTML hospedada no próprio ESP32.
* Gráficos via **Chart.js** para monitoramento de Jitter, execução e deadlines perdidos.
* Controle remoto para alternar entre RM e EDF.

### Feedback Físico
* Display LCD 16x2 para status local.
* Buzzer para alerta sonoro de perda de prazo (*Deadline Miss*).

## 🛠️ Hardware Necessário e Pinagem

| Componente | Pino ESP32 (GPIO) | Função |
| :--- | :--- | :--- |
| **Botão** | GPIO 15 | Dispara tarefa aperiódica (Interrupção) |
| **Buzzer** | GPIO 22 | Alerta de Deadline Miss |
| **LED Status** | GPIO 2 | Indica modo de operação (RM = Ligado) |
| **LCD - RS** | GPIO 5 | Controle do LCD |
| **LCD - EN** | GPIO 4 | Controle do LCD |
| **LCD - D4** | GPIO 18 | Dados LCD |
| **LCD - D5** | GPIO 19 | Dados LCD |
| **LCD - D6** | GPIO 23 | Dados LCD |
| **LCD - D7** | GPIO 27 | Dados LCD |

## 📦 Dependências de Software

Certifique-se de instalar as seguintes bibliotecas na IDE do Arduino ou PlatformIO:

* **LiquidCrystal** (para controle do LCD paralelo)
* **ArduinoJson** (versão 6 ou superior, para serialização dos dados do dashboard)
* **WiFi** & **WebServer** (Nativas do core ESP32)

## ⚙️ Configuração e Instalação

### 1. Configurar Wi-Fi
<div style="text-align: justify">
Abra o código e localize as seguintes linhas para inserir as credenciais da sua rede (necessário para baixar a biblioteca Chart.js no navegador):
</div>

```cpp
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SUA_SENHA";
```

### 2. Upload
Compile e carregue o código para o seu ESP32.

3. Acessar o Dashboard
Abra o Monitor Serial (Baud Rate: 115200).

Reinicie o ESP32.

Copie o endereço IP exibido (ex: 192.168.1.105).

Cole o endereço IP no navegador do seu computador ou celular (conectado à mesma rede).

## 🧠 Como Funciona o EDF no FreeRTOS
 O **FreeRTOS** nativamente é um sistema de prioridade fixa (preemptivo). Para simular o **EDF**, este projeto utiliza uma técnica de reatribuição dinâmica:

* As tarefas calculam seus próprios deadlines absolutos.

* A função aplicarEDF() é chamada periodicamente.

* Ela ordena as tarefas com base no `next_deadline`.

A função utiliza `vTaskPrioritySet()` para alterar a prioridade das tarefas no kernel do **FreeRTOS**, garantindo que a tarefa com o prazo mais curto tenha a maior prioridade numérica naquele instante.

Usado pelos gráficos da interface.

## 🕒 Diagrama de Escalonamento

* Cada tarefa recebe `next_deadline = now + periodo`
* Tarefas são ordenadas por deadline
* Prioridades são atribuídas dinamicamente: **tarefa mais urgente → prioridade mais alta**
 
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


## 📊 Métricas Calculadas

* `exec_us` → tempo de execução real da tarefa
* `misses` → contagem de deadline misses
* `jitter_ms` → jitter medido usando `vTaskDelayUntil`
* `prio` → prioridade real no FreeRTOS
* `cpuLoad (%)` → cálculo baseado na soma das cargas

## 📦 Dependências de Software

Para compilar este projeto, certifique-se de ter as seguintes bibliotecas instaladas na sua IDE (Arduino IDE ou PlatformIO):

1.  **ArduinoJson** (v6 ou superior)
2.  **LiquidCrystal** (Biblioteca padrão para LCDs paralelos)
3.  **WiFi.h** & **WebServer** (Nativas do core ESP32)
4. **FreeRTOS** (nativo no ESP32)
## ▶️ Execução

1. Altere SSID e senha Wi-Fi
2. Faça upload do código
3. Abra o Serial Monitor para ver o IP (“Connected at: …”)
4. Entre no **navegador e acesse**:

```
http://<seu-esp32>
```

5. Use os botões para alternar entre **RM ↔ EDF**
6. Observe gráficos em tempo real

## 👩‍💻 Autores

  - **Gabriella Arévalo Marques**  
    📧 [gabriellaarevalomarques@gmail.com](mailto:gabriellaarevalomarques@gmail.com)

  - **Hebert Allan Kubis**  
    📧 [herbertkubis15@gmail.com](mailto:herbertkubis15@gmail.com)

## 🔗 Repositório

👉 [Acesse no GitHub](https://github.com/2005HAK/STR.git) 
<p align="center">

**Autores:** Gabriella Arévalo Marques e Hebert Alan Kubis  
**Curso:** EMB5633 – Sistemas de Tempo Real (UFSC)  
**Data:** Novembro de 2025  
</p>



<p align="center">
  <!-- ESP32 -->
  <img src="https://avatars.githubusercontent.com/u/64278475?s=280&v=4" alt="ESP32" width=35"/>
  &nbsp;&nbsp;&nbsp;
    <!-- FreeRTOS -->
  <img src="https://miro.medium.com/v2/resize:fit:1400/1*kKOI5rbDyooILE3yL1ipkA.png" alt="FreeRTOS" width="70"/>
  &nbsp;&nbsp;&nbsp;
  <!-- Arduino -->
  <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/7/73/Arduino_IDE_logo.svg/2048px-Arduino_IDE_logo.svg.png" alt="C" width="35"/>
</p>
