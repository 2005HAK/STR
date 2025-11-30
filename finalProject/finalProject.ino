#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal.h>

// Definição dos pinos GPIO no ESP32 Dev Module (Pinos Seguros)
// (RS, Enable, D4, D5, D6, D7)
const int rs = 5;   // Pino de controle 
const int en = 4;   // Pino de controle
const int d4 = 18;  // Pino de dados
const int d5 = 19;  // Pino de dados
const int d6 = 23;  // Pino de dados
const int d7 = 27;  // Pino de dados

// LCD variables

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
    int pin;                    // LED associado
    UBaseType_t prioridade;     // Prioridade atribuída pelo RM
    uint64_t total_exec_us;     // Soma dos tempos de execução
    uint32_t ativacoes;         // Contador de execuções
    uint32_t misses;            // Deadline misses detectados
} TarefaPeriodica;

TarefaPeriodica tarefas[NUM_TASKS] = {
    {"CalcLoad", 500, 0, -1, 0, 0, 0, 0},
    {"Display", 1000, 0, 26, 1, 0, 0, 0}
};

static SemaphoreHandle_t mtxTasks;

// End sheduler system variables

// Load system variables

static int cpuLoad = 0;
static SemaphoreHandle_t mtxLoad;

// End load system variables

const int statusLedPin = 2; // LED de status do ESP32
bool ledEnabled = true; 
bool IsStarted = true;

void setupPWM() {
    pinMode(statusLedPin, OUTPUT);
    digitalWrite(statusLedPin, HIGH);
}

void setScheduler(String mode) {
    if (mode == "RM" || mode == "EDF") {
        xSemaphoreTake(mtxSheduler, portMAX_DELAY);
        strncpy((char*)currentScheduler, mode.c_str(), 3);
        xSemaphoreGive(mtxSheduler);

        if(mode == "RM") digitalWrite(statusLedPin, HIGH);
        else digitalWrite(statusLedPin, LOW);

        Serial.printf("MODO ALTERADO PARA: %s\n", mode.c_str());
    }
}

// Rota Principal (HTML)
String htmlPage() {
    // LEITURA SEGURA: Copia o estado volatile para uma String local (não volatile)
    xSemaphoreTake(mtxSheduler, portMAX_DELAY);
    // O cast (const char*) remove o qualificador volatile para o construtor String
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

void calcLoad(void* pvParameters){
    TickType_t ultimoTick = xTaskGetTickCount();
    TickType_t periodoTicks = pdMS_TO_TICKS(tarefas[1].periodo_ms);

    int i = 0;

    for(;;){
        vTaskDelayUntil(&ultimoTick, periodoTicks);

        cpuLoad = i;
        i++;
        if(i >= 100) i = 0;
/*
        xSemaphoreTake(mtxLoad, portMAX_DELAY);
        for(int i = 1; i < NUM_TASKS; i++) {
            cpuLoad += (tarefas[i].carga_us * 1000) / tarefas[i].periodo_ms;
        }
        xSemaphoreGive(mtxLoad);
*/
    }
}

void display(void* pvParameters){
    TarefaPeriodica* t = (TarefaPeriodica*) pvParameters;

    String pastMode = "RM";
    String msg = "";
    int count = 0;

    TickType_t ultimoTick = xTaskGetTickCount();
    TickType_t periodoTicks = pdMS_TO_TICKS(t->periodo_ms);

    for(;;){
        vTaskDelayUntil(&ultimoTick, periodoTicks);

        uint64_t inicio = esp_timer_get_time();
    
        xSemaphoreTake(mtxSheduler, portMAX_DELAY);
        String mode = (const char*) currentScheduler;
        xSemaphoreGive(mtxSheduler);

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

void setup() {
    Serial.begin(115200);
    setupPWM();
    
    // CONFIGURAÇÃO WIFI (Ponto de Acesso)
    WiFi.mode(WIFI_AP);
    bool result = WiFi.softAP(ssid, password);

    if (result) {
        Serial.println("AP iniciado com sucesso!");
        delay(100); 

        // IP (192.168.4.1)
        Serial.print("IP do Ponto de Acesso: "); 
        Serial.println(WiFi.softAPIP()); 
    } else Serial.println("ERRO: Falha ao iniciar o Ponto de Acesso (AP)!");

    mtxSheduler = xSemaphoreCreateMutex();
    mtxLoad = xSemaphoreCreateMutex();
    mtxTasks = xSemaphoreCreateMutex();

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

    // Cria a tarefa de display no LCD
    xTaskCreate(calcLoad, tarefas[0].name, 1024, NULL, tarefas[0].prioridade, NULL);
    xTaskCreate(display, tarefas[1].name, 1024, (void*) &tarefas[1], tarefas[1].prioridade, NULL);
}

void loop() {
    server.handleClient();
}