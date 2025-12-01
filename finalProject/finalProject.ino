/*
  Projeto: Escalonamento RM ↔ EDF no ESP32
  Autor: Gabriella Arévalo e Hebert Alan Kubis
  Matéria: Sistemas de Tempo Real 2025.2
  
  Funcionalidades (o que eu tentei fazer né):
  - 3 tarefas periódicas com medições reais
  - 1 tarefa aperiódica acionada por botão (ISR)
  - Alternância dinâmica entre RM e EDF via Serial ("RM" ou "EDF")
  - Medição de:
      * tempo de CPU (execução)
      * jitter
      * ativação real vs período
      * deadline miss
      * utilização total U
  - Logs detalhados com prioridade atual

*/

#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freeRtos/semphr.h>
#include <LiquidCrystal.h>

// Pins
const int BOTAO = 15;
const int statusLedPin = 2; // LED de status do ESP32

// End Pins

// LCD variables
const int rs = 5, en = 4, d4 = 18, d5 = 19, d6 = 23, d7 = 27;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

byte barra_1_linha[] =  {B10000, B10000, B10000, B10000, B10000, B10000, B10000, B10000};
byte barra_2_linhas[] = {B11000, B11000, B11000, B11000, B11000, B11000, B11000, B11000};
byte barra_3_linhas[] = {B11100, B11100, B11100, B11100, B11100, B11100, B11100, B11100};
byte barra_4_linhas[] = {B11110, B11110, B11110, B11110, B11110, B11110, B11110, B11110};

// End LCD variables

// WiFi variables
const char* ssid = "TrabalhoSTR"; 
const char* password = "123456789";
WebServer server(80);

//End WiFi variables

// Sheduler system variable

volatile char currentScheduler[4] = "RM"; // Estado inicial ("RM" ou "EDF")
static SemaphoreHandle_t mtxSheduler;

const int NUM_TASKS = 2;

typedef struct {
    const char *name;
    uint32_t periodo_ms;        // Período da tarefa em milissegundos
    uint32_t carga_us;
    int pin;                   
    UBaseType_t prioridade;     // Prioridade atribuída pelo RM
    uint64_t total_exec_us;     // Soma dos tempos de execução
    uint32_t ativacoes;         // Contador de execuções
    uint32_t misses;            // Deadline misses detectados

    // -------- EDF --------
    uint64_t next_deadline;
    TaskHandle_t handle;
} TarefaPeriodica;

TarefaPeriodica tarefas[NUM_TASKS] = {
    {"CalcLoad", 300, 0, -1, 1, 0, 0, 0, 0, NULL},
    {"Display", 500, 0, 26, 2, 0, 0, 0, 0, NULL}
};

TarefaPeriodica tarefaAperiodica = {"Aperiodica", 9, 0, -1, tskIDLE_PRIORITY, 0, 0, 0, 0, NULL};

static SemaphoreHandle_t mtxTasks;
static SemaphoreHandle_t semAperiodica;

// End sheduler system variables

// Load system variables
static int cpuLoad = 0;
static SemaphoreHandle_t mtxLoad;

// End load system variables

bool ledEnabled = true; 
bool IsStarted = true;

void display(TarefaPeriodica* t);
void calcLoad(TarefaPeriodica* t);
void aplicarEDF();

void setScheduler(String mode) {
    if (mode == "RM" || mode == "EDF") {
        xSemaphoreTake(mtxSheduler, portMAX_DELAY);
        strncpy((char*)currentScheduler, mode.c_str(), 3);
        xSemaphoreGive(mtxSheduler);

        digitalWrite(statusLedPin, (mode == "RM") ? HIGH : LOW);
        Serial.printf("MODO ALTERADO PARA: %s\n", mode.c_str());
    }
}

void aplicarEDF() {
    xSemaphoreTake(mtxTasks, portMAX_DELAY);

    TarefaPeriodica* ponteiros[NUM_TASKS];
    for(int i=0; i<NUM_TASKS; i++) ponteiros[i] = &tarefas[i];

    // Bubble sort simples nos ponteiros baseado no deadline
    for (int i = 0; i < NUM_TASKS - 1; i++) {
        for (int j = i + 1; j < NUM_TASKS; j++) {
            if (ponteiros[j]->next_deadline < ponteiros[i]->next_deadline) {
                TarefaPeriodica* temp = ponteiros[i];
                ponteiros[i] = ponteiros[j];
                ponteiros[j] = temp;
            }
        }
    }

    for (int i = 0; i < NUM_TASKS; i++) {
        UBaseType_t novaPrioridade = (NUM_TASKS - i) + 1; 
        if(ponteiros[i]->handle != NULL){
            vTaskPrioritySet(ponteiros[i]->handle, novaPrioridade);
        }
    }

    xSemaphoreGive(mtxTasks);
}

String checkEDF(TarefaPeriodica* t, uint64_t inicio){
    t->next_deadline = inicio + (t->periodo_ms * 1000ULL);

    xSemaphoreTake(mtxSheduler, portMAX_DELAY);
    String mode = (const char*) currentScheduler;
    xSemaphoreGive(mtxSheduler);

    if(mode == "EDF") aplicarEDF();
    else vTaskPrioritySet(t->handle, t->prioridade); // Se voltar para RM, restaura prioridades originais (fixas)

    return mode;
}

void tarefaPeriodica(void* pvParameters){
    TarefaPeriodica *t = (TarefaPeriodica*) pvParameters;

    if (strcmp(t->name, "Display") == 0) display(t);
    else if (strcmp(t->name, "CalcLoad") == 0) calcLoad(t);
    else for(;;) vTaskDelay(pdMS_TO_TICKS(1000));
}

void taskAperiodica(void* pvParameters){
    TarefaPeriodica *t = (TarefaPeriodica*) pvParameters;

    for(;;){
        if(xSemaphoreTake(semAperiodica, portMAX_DELAY) == pdTRUE){
            uint64_t inicio = esp_timer_get_time();

            Serial.printf("[APERIODICA] Iniciou em %lluus\n", (unsigned long long)inicio);

            while(esp_timer_get_time() - inicio < 8000){
                // Simulação de execução por 8 ms
            }

            uint64_t fim = esp_timer_get_time();
            uint64_t exec_us = fim - inicio;

            t->carga_us = exec_us;
            t->total_exec_us += exec_us;
            t->ativacoes++;
        }
    }
}

void display(TarefaPeriodica* t){
    String pastMode = "RM";
    String msg = "";
    int count = 0;

    TickType_t ultimoTick = xTaskGetTickCount();
    TickType_t periodoTicks = pdMS_TO_TICKS(t->periodo_ms);

    for(;;){
        vTaskDelayUntil(&ultimoTick, periodoTicks);

        uint64_t inicio = esp_timer_get_time();

        String mode = checkEDF(t, inicio);

        lcd.setCursor(0, 0);
        lcd.print("CPU:");

        xSemaphoreTake(mtxLoad, portMAX_DELAY);
        int load = cpuLoad;
        xSemaphoreGive(mtxLoad);

        int intLoad = load / 10;
        int fracLoad = (load % 10) / 2;

        for(int i = 0; i < intLoad; i++) lcd.print((char) 255);
        if(fracLoad == 1) lcd.write((uint8_t) 0);
        else if(fracLoad == 2) lcd.write((uint8_t) 1);
        else if(fracLoad == 3) lcd.write((uint8_t) 2);
        else if(fracLoad == 4) lcd.write((uint8_t) 3);

        for (int i = 0; i < 10 - (intLoad + ((fracLoad > 0) ? 1 : 0)); i++) lcd.print(' ');

        lcd.setCursor(14, 0);
        lcd.print(" %");

        lcd.setCursor(0, 1); 
        if(mode != pastMode){
            msg = pastMode + " -> " + mode;
            pastMode = mode;
            count = 1;
        } else if(count > 0){
            count++;    
            if(count >= 5) count = 0;     
        } else msg = "Sheduler: " + mode;

        lcd.print(msg);

        int len = msg.length();
        for (int i = len; i < 16; i++) lcd.print(' ');

        uint64_t fim = esp_timer_get_time();
        uint64_t exec_us = fim - inicio;

        t->total_exec_us += exec_us;
        t->ativacoes++;
        t->carga_us = exec_us;

        if(exec_us > (t->periodo_ms * 1000)){
            t->misses++;
            Serial.printf("[MISS] %s excedeu o período (%lluus > %u ms)\n", t->name, (unsigned long long)exec_us, t->periodo_ms);
        }
    }
}

void calcLoad(TarefaPeriodica* t){
    TickType_t ultimoTick = xTaskGetTickCount();
    TickType_t periodoTicks = pdMS_TO_TICKS(tarefas[1].periodo_ms);

    int i = 0;

    for(;;){
        vTaskDelayUntil(&ultimoTick, periodoTicks);

        uint64_t inicio = esp_timer_get_time();

        checkEDF(t, inicio);

        float totalUtilization = 0.0;

        for(int i = 0; i < NUM_TASKS; i++){
            if(tarefas[i].periodo_ms > 0){
                float Ui = (float)tarefas[i].carga_us / (float)(tarefas[i].periodo_ms * 1000);
                totalUtilization += Ui;
            }
        }
        totalUtilization += (float)tarefaAperiodica.carga_us / (float)(tarefaAperiodica.periodo_ms * 1000);

        tarefaAperiodica.carga_us = 0;

        xSemaphoreTake(mtxLoad, portMAX_DELAY);
        cpuLoad = (int)(totalUtilization * 100);
        if(cpuLoad > 100) cpuLoad = 100;
        xSemaphoreGive(mtxLoad);

        uint64_t fim = esp_timer_get_time();
        uint64_t exec_us = fim - inicio;

        t->carga_us = exec_us;
        t->total_exec_us += exec_us;
        t->ativacoes++;

        if(exec_us > (t->periodo_ms * 1000ULL)){
            t->misses++;
            Serial.printf("[MISS] %s excedeu o período (%llu us)\n", t->name, (unsigned long long)exec_us);
        }
    }
}

void IRAM_ATTR isrBotao() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreGiveFromISR(semAperiodica, &xHigherPriorityTaskWoken);   // Libera a tarefa aperiódica

    if(xHigherPriorityTaskWoken) portYIELD_FROM_ISR();                // Força troca de contexto se necessário
}

// Rota Principal (HTML)
String htmlPage() {
    xSemaphoreTake(mtxSheduler, portMAX_DELAY);
    String currentModeString = (const char*)currentScheduler;
    xSemaphoreGive(mtxSheduler);

    String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    page += "<title>Controle de Escalonamento RTOS</title>";
    page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    page += "<style>";
    page += "body{font-family:sans-serif;background:#111;color:#eee;text-align:center;}";
    page += ".button-container{margin:20px 0; padding: 15px; border-radius: 8px; background: #222;}";
    page += "button{padding:12px 25px;margin:10px;border:none;border-radius:5px;cursor:pointer;font-size:16px; transition: background 0.3s;}";
    page += ".scheduler-btn{background:#555;color:white;}";
    page += ".scheduler-btn.active{background:#4CAF50; font-weight: bold;}";
    page += ".status{font-size:20px;font-weight:bold;margin:15px 0;padding:10px;border-radius:5px; background: #333;}";
    page += "</style></head><body>";
    page += "<h1>Controle Dinâmico de Escalonamento</h1>";
    
    // Status do Escalonador Atual
    page += "<div class='status'>";
    page += "Modo RTOS: <span id='currentMode'>" + currentModeString + "</span>";
    page += "</div>";
    
    // Controle do Escalonador
    page += "<div class='button-container'>";
    page += "<h2>Alterar Algoritmo</h2>";
    page += "<button class='scheduler-btn' id='btnRM' onclick='setMode(\"RM\")'>RATE MONOTONIC (RM)</button>";
    page += "<button class='scheduler-btn' id='btnEDF' onclick='setMode(\"EDF\")'>EARLIEST DEADLINE FIRST (EDF)</button>";
    page += "</div>";

    // SCRIPT JAVASCRIPT
    page += R"(
    <script>
    // Leitura inicial do estado
    let currentMode = document.getElementById('currentMode').innerText;

    function updateModeDisplay(mode) {
    // Atualiza o texto na tela
    document.getElementById('currentMode').innerText = mode;
    
    // Atualiza os botões (RM ou EDF)
    document.getElementById('btnRM').classList.remove('active');
    document.getElementById('btnEDF').classList.remove('active');
    
    let btn = document.getElementById('btn' + mode);
    if(btn) btn.classList.add('active');
    }

    async function setMode(mode){
    let res = await fetch('/setScheduler?mode=' + mode);
    if (res.ok) {
        updateModeDisplay(mode);
    }
    }

    // Inicializa a exibição correta na carga da página
    function initDisplay(){
        let mode = document.getElementById('currentMode').innerText;
        updateModeDisplay(mode);
    }

    document.addEventListener('DOMContentLoaded', initDisplay);

    </script></body></html>
    )";
    return page;
}

void handleRoot() { server.send(200, "text/html", htmlPage()); }

// Recebe a seleção do escalonador
void handleSetScheduler() {
    if (server.hasArg("mode")) {
        String mode = server.arg("mode");
        if (mode == "RM" || mode == "EDF") {
            setScheduler(mode);
            server.send(200, "text/plain", "Scheduler set to " + mode);
            return;
        }
    }
    server.send(400, "text/plain", "Argumento 'mode' inválido.");
}

void handleGetScheduler() {
    xSemaphoreTake(mtxSheduler, portMAX_DELAY);
    String schedulerMode = (const char*)currentScheduler;
    xSemaphoreGive(mtxSheduler);
    
    server.send(200, "text/plain", schedulerMode);
}

void handleGetEnabled() {server.send(200, "text/plain", ledEnabled ? "true" : "false");} 

void handleGetStatus() {server.send(200, "text/plain", IsStarted ? "true" : "false");}

void handleToggle() {
    ledEnabled = !ledEnabled;
    digitalWrite(statusLedPin, ledEnabled ? HIGH : LOW);
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);

    // Configuração dos pinos
    pinMode(BOTAO, INPUT_PULLDOWN);
    pinMode(statusLedPin, OUTPUT);

    // Fim configuração dos pinos
    
    // CONFIGURAÇÃO WIFI (Ponto de Acesso)
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(ssid, password);

    if(result){
        Serial.println("AP iniciado com sucesso!");
        delay(100); 

        // IP (192.168.4.1)
        Serial.print("IP do Ponto de Acesso: "); 
        Serial.println(WiFi.softAPIP()); 
    } else Serial.println("ERRO: Falha ao iniciar o Ponto de Acesso (AP)!");

    mtxSheduler = xSemaphoreCreateMutex();
    mtxLoad = xSemaphoreCreateMutex();
    mtxTasks = xSemaphoreCreateMutex();
    semAperiodica = xSemaphoreCreateBinary();

    attachInterrupt(digitalPinToInterrupt(BOTAO), isrBotao, FALLING); // Botão no GPIO0

    // CONFIGURAÇÃO DAS ROTAS
    server.on("/", handleRoot);
    server.on("/setScheduler", handleSetScheduler); 
    server.on("/getScheduler", handleGetScheduler); 
    server.on("/toggle", handleToggle);
    server.on("/getEnabled", handleGetEnabled);
    server.on("/getStatus", handleGetStatus); 

    server.begin();
    Serial.println("Servidor iniciado!");

    lcd.begin(16, 2);
    lcd.createChar(0, barra_1_linha);
    lcd.createChar(1, barra_2_linhas);
    lcd.createChar(2, barra_3_linhas);
    lcd.createChar(3, barra_4_linhas);

    Serial.println("LCD Paralelo inicializado.");

    for(int i = 0; i < NUM_TASKS; i++) xTaskCreate(tarefaPeriodica, tarefas[i].name, 2048, &tarefas[i], tarefas[i].prioridade, &tarefas[i].handle);

    xTaskCreate(taskAperiodica, "Aperiodica", 2048, &tarefaAperiodica, tarefaAperiodica.prioridade, &tarefaAperiodica.handle);
}

void loop() {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
}