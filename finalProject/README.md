# 💻 Sistema Reativo de Tempo Real com Gerenciamento Dinâmico de Carga: RM 🆚 EDF no ESP32

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
    * *Random:* Tarefa com carga de processamento aleatória.
    * *Aperiódica:* Tarefa pesada disparada por botão para teste de estresse.
* **Feedback Físico:** 
  * **LCD 16×2:** Exibe métricas de uso de CPU e modo atual.
  * **Servo Motor:** Atua como indicador visual de carga (movimenta-se durante tarefas aperiódicas ou falhas).
  * **Buzzer:** Alarme sonoro para *Deadline Misses*.
  * **LED:** Indicador de status de carga alta.

## 🧩 Funcionalidades do Sistema

### ✔️ 1. Alternância dinâmica RM ↔ EDF

* Interruptor por software através da interface web
* Atualização visual no LCD (“RM → EDF” e vice-versa)
* O algoritmo EDF utiliza uma ordenação (*Bubble Sort*) dos ponteiros das tarefas para redefinir prioridades dinamicamente a cada ciclo.

### ✔️ 2. Tarefas Periódicas

| Tarefa   | Período | Prioridade (RM) | Função                     |
| -------- | ------- | --------------- |----------------------------|
| CalcLoad | 300 ms  | Alta            | Calcula carga total da CPU |
| Display  | 500 ms  | Média           | Atualiza LCD com CPU e modo|
| Random   | 700 ms  | Baixa           | Simula processamento variável (Wait aleatório) |
### ✔️ 3. Tarefa Aperiódica

* Acionada por interrupção do botão (GPIO 15).
* Executa carga simulada (busy wait ~8,5 ms).
* Aciona o **Servo Motor** para 90° durante a execução.
* Reporta deadline misses com buzzer e reseta o servo para 0°.

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
## ⏳ Fluxograma do Sistema
<div style="text-align: justify">

```
             ┌─────────────────────┐
             │        Início       │
             └──────────┬──────────┘
                        │
        ┌───────────────▼────────────────┐
        │   Inicializa WiFi (modo STA)   │
        │     Inicializa WebServer       │
        │    Cria tarefas periódicas     │
        └───────────────┬────────────────┘
                        │
              ┌─────────▼─────────┐
              │   Loop Principal  │
              └─────────┬─────────┘
                        │
       ┌────────────────▼────────────────┐
       │      Scheduler (RM ou EDF)      │
       │  Ordena tarefas por prioridade  │
       └────────────────┬────────────────┘
                        │
       ┌────────────────▼───────────────────┐
       │        Execução das Tarefas        │
       │ - Mede tempo real                  │
       │ - Aciona Servo/Buzzer se necessário│
       │ - Envia dados para interface web   │
       └────────────────┬───────────────────┘
                        │
                 ┌──────▼───────┐
                 │Web Dashboard │
                 └──────────────┘
```

## 🖼️ Diagrama de conexões

<p align="center">
  <img src="https://github.com/2005HAK/STR/blob/master/finalProject/finalProject.png" alt="Esquema eletrico de conexões" width="600"/>
</p>

## 🔌 Pinos Utilizados

| Componente | Pino ESP32 (GPIO) | Função |
| :--- | :--- | :--- |
| **Botão** | GPIO 15 | Dispara tarefa aperiódica (Interrupção) |
| **Buzzer** | GPIO 22 | Alerta de Deadline Miss |
| **Server Motor** | GPIO 32 |Indicador físico (0° = Idle/Miss, 90° = Ativo)|
| **LED Status** | GPIO 2 | Indica sobrecarga de CPU (>80%)|
| **LCD - RS** | GPIO 5 | Controle do LCD |
| **LCD - EN** | GPIO 4 | Controle do LCD |
| **LCD - D4** | GPIO 18 | Dados LCD |
| **LCD - D5** | GPIO 19 | Dados LCD |
| **LCD - D6** | GPIO 23 | Dados LCD |
| **LCD - D7** | GPIO 27 | Dados LCD |


## 🧮 Diagrama de Escalonamento - Exemplo

* Cada tarefa recebe `next_deadline = now + periodo`
* Tarefas são ordenadas por deadline
* Prioridades são atribuídas dinamicamente: **tarefa mais urgente → prioridade mais alta**
 
### RM — Prioridade fixa (menor período = maior prioridade)

**Linha do tempo →**\
T1 (CalcLoad): `|■■|      |■■|        |■■|     |■■|` \
T2 (Display): `      |■■■■|               |■■■■|`\
T3 (Random): `              |■■■■■■■■|`


### EDF — Prioridade dinâmica (menor deadline primeiro)

**Linha do tempo →**\
T1 (CalcLoad): `|■■|       |■■|    |■■|     |■■|`\
T2 (Display): `       |■■■■|           |■■■■|`\
T3 (Random):               `|■■■■|`


## 📝 Comparativo de Escalonamento em Tempo Real: RM vs. EDF (ESP32)
<div style="text-align: justify">
Este projeto implementa e compara dois algoritmos clássicos de escalonamento de tempo real — <strong>Rate Monotonic (RM)</strong> e <strong>Earliest Deadline First (EDF)</strong> — utilizando o <strong>FreeRTOS</strong> em um microcontrolador ESP32.

O sistema permite a alternância dinâmica entre os modos de escalonamento e oferece um dashboard web para visualização de métricas em tempo real (Carga da CPU, Jitter, Misses e Prioridades).
</div>

## 📊 Métricas Calculadas

* `exec_us` → tempo de execução real da tarefa com `esp_timer_get_time`
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

## ▶️ Instalação Execução

### Configurar Wi-Fi

Abra o código e localize as seguintes linhas para inserir as credenciais da sua rede (necessário para baixar a biblioteca Chart.js no navegador):

```cpp
const char* ssid = "NOME_DA_SUA_REDE";
const char* password = "SUA_SENHA";
```
### Upload
Compile e carregue o código para o seu ESP32.

### Acessar o Dashboard
1. Abra o Monitor Serial (Baud Rate: 115200).

2. Reinicie o ESP32.

3. Abra o Serial Monitor para ver o IP (“Connected at: …”) e copie o endereço IP exibido (ex: 192.168.1).

4. Cole o endereço IP no navegador do seu computador ou celular (conectado à mesma rede)e acesse:

```
http://<IP do seu esp32>
```
5. Use os botões para alternar entre **RM ↔ EDF**
6. Observe gráficos em tempo real

## 👩‍💻 Autores

  - **Gabriella Arévalo Marques**  
    📧 [gabriellaarevalomarques@gmail.com](mailto:gabriellaarevalomarques@gmail.com)

  - **Hebert Alan Kubis**  
    📧 [herbertkubis15@gmail.com](mailto:herbertkubis15@gmail.com)

## 🔗 Repositório

👉 [Acesse no GitHub](https://github.com/2005HAK/STR.git) 
<p align="center">
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
