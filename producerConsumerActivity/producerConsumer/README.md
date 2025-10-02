# Produtor/Consumidor - FreeRTOS ⛓️‍💥 (Versão com 4 Botões e 4 LEDs)

 Este projeto demonstra o uso de **semáforos de contagem** e **mutex** do `FreeRTOS` no `Arduino/ESP32` para gerenciar concorrência entre múltiplas tarefas **produtoras** (controladas por botões) e múltiplas tarefas **consumidoras** (que acendem LEDs).
 
 A lógica central implementa um **buffer circular protegido**, onde os produtores escrevem valores e os consumidores retiram valores, garantindo que o acesso ao recurso compartilhado seja **seguro contra condições de corrida**.

## 📌 Objetivo
- Utilizar um **mutex** e **semáforos de contagem** para proteger o **buffer compartilhado**.  
- Cada produtor insere números no buffer (quando um botão é pressionado).  
- Cada consumidor retira números do buffer e acende um LED correspondente.  
- **Sem uso de filas (queues)** do FreeRTOS.


## 🚀 Como Executar

1. Instale a `Arduino IDE (ou PlatformIO)`.

2. Conecte a placa `ESP32`.

3. Monte os botões e LEDs conforme o diagrama elétrico.

4. Compile e envie o código.

5. Abra o Serial Monitor `115200 baud`.

## 📂 Estrutura do Código

- *producer(void *parameters)** → Lê o botão e escreve no buffer.

- *consumer(void *parameters)** → Lê do buffer e acende o LED.

- *setup()** → Configura GPIOs, semáforos e cria tarefas.

- *loop()** → Apenas libera CPU (vTaskDelay).

## ⚙️ Conceitos Utilizados
- **Buffer Circular (`buf`)**  
  Estrutura que armazena dados de forma contínua, reiniciando no índice zero ao atingir o limite.  
  - `head` → índice de escrita  
  - `tail` → índice de leitura  

- **Mutex (`mtx`)**  
  Garante que apenas uma tarefa por vez acesse o buffer.  

- **Semáforo de Contagem**  
  - `noEmptyStaces`: controla quantos espaços vazios ainda existem no buffer.  
  - `noItemsAvailable`: controla quantos itens já estão disponíveis para leitura.  

- **Produtores (`producer`)**  
  Ativados quando um **botão** é pressionado.  
  - Escrevem no buffer.  
  - Liberam `noItemsAvailable`.  

- **Consumidores (`consumer`)**  
  - Esperam até que haja dados (`noItemsAvailable`).  
  - Leem do buffer e liberam espaço (`noEmptyStaces`).  
  - Acendem o LED correspondente.  


## 🔧 Hardware Necessário
- **Placa**: ESP32 (ou Arduino compatível com FreeRTOS).  
- **Botões (x4)** com **pull-down** interno habilitado.  
- **LEDs (x4)** com resistores de **220 Ω a 330 Ω**.  

### 🖲️ Mapeamento de Pinos

| Componente | Pino ESP32 |
|------------|------------|
| Botão 1    | GPIO 15    |
| Botão 2    | GPIO 4     |
| Botão 3    | GPIO 17    |
| Botão 4    | GPIO 19    |
| LED 1      | GPIO 2     |
| LED 2      | GPIO 16    |
| LED 3      | GPIO 5     |
| LED 4      | GPIO 18    |


## 🔌 Circuito

<p align="center">
  <img src="https://github.com/2005HAK/STR/blob/master/producerConsumerActivity/producerConsumer/EsquematicoProdutorComsumidor.png" alt="Esquemático produtor consumidor" width="720"/>
  &nbsp;&nbsp;&nbsp;
</p>

## 📜 Fluxo Produtor → Consumidor (Mermaid)

```mermaid
flowchart LR
    BTN1[Botão 1] --> P1[Produtor 1]
    BTN2[Botão 2] --> P2[Produtor 2]
    BTN3[Botão 3] --> P3[Produtor 3]
    BTN4[Botão 4] --> P4[Produtor 4]

    P1 --> BUF[Buffer Circular]
    P2 --> BUF
    P3 --> BUF
    P4 --> BUF

    BUF --> C1[Consumidor 1 → LED1]
    BUF --> C2[Consumidor 2 → LED2]
    BUF --> C3[Consumidor 3 → LED3]
    BUF --> C4[Consumidor 4 → LED4]
```
## 🔄 Diagrama de Estados (Semáforos + Mutex)

```mermaid
stateDiagram-v2
    [*] --> Produtor
    Produtor --> EsperaEspaco: xSemaphoreTake(noEmptyStaces)
    EsperaEspaco --> RegiaoCriticaProdutor: xSemaphoreTake(mtx)
    RegiaoCriticaProdutor --> EscreveBuffer: escreve valor
    EscreveBuffer --> LiberaMutexProdutor: xSemaphoreGive(mtx)
    LiberaMutexProdutor --> SinalizaItem: xSemaphoreGive(noItemsAvailable)
    SinalizaItem --> [*]

    [*] --> Consumidor
    Consumidor --> EsperaItem: xSemaphoreTake(noItemsAvailable)
    EsperaItem --> RegiaoCriticaConsumidor: xSemaphoreTake(mtx)
    RegiaoCriticaConsumidor --> LeBuffer: lê valor
    LeBuffer --> LiberaMutexConsumidor: xSemaphoreGive(mtx)
    LiberaMutexConsumidor --> SinalizaEspaco: xSemaphoreGive(noEmptyStaces)
    SinalizaEspaco --> AcionaLED: LED acende 1s
    AcionaLED --> [*]
```
## 🖥️ Saída Esperada (Serial Monitor)
```
--- FreeRTOS Semaphore Alternate Solution ---
Produces: 0
Produces: 1
Consume: 0
Consume: 1
```
## 👩‍💻 Autores

  - **Gabriella Arévalo Marques**  
    📧 [gabriellaarevalomarques@gmail.com](mailto:gabriellaarevalomarques@gmail.com)

  - **Hebert Allan Kubis**  
    📧 [herbertkubis15@gmail.com](mailto:herbertkubis15@gmail.com)

## 🔗 Repositório

👉 [Acesse no GitHub](https://github.com/2005HAK/STR.git) 
<p align="center">
  <!-- ESP32 -->
  <img src="https://avatars.githubusercontent.com/u/64278475?s=280&v=4" alt="ESP32" width="35"/>
  &nbsp;&nbsp;&nbsp;
    <!-- FreeRTOS -->
  <img src="https://miro.medium.com/v2/resize:fit:1400/1*kKOI5rbDyooILE3yL1ipkA.png" alt="FreeRTOS" width="70"/>
  &nbsp;&nbsp;&nbsp;
  <!-- C Language -->
  <img src="https://upload.wikimedia.org/wikipedia/commons/1/19/C_Logo.png" alt="C" width="30"/>
</p>


