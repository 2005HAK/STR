#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <LiquidCrystal.h>

// Definição dos pinos GPIO no ESP32 Dev Module (Pinos Seguros)
// (RS, Enable, D4, D5, D6, D7)
const int rs = 2;   // Pino de controle 
const int en = 4;   // Pino de controle
const int d4 = 18;  // Pino de dados
const int d5 = 19;  // Pino de dados
const int d6 = 23;  // Pino de dados
const int d7 = 27;  // Pino de dados

// Inicializa o objeto LCD
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const char* ssid = "TrabalhoSTR"; 
const char* password = "123456789";

WebServer server(80);

volatile char currentScheduler[4] = "RM"; // Estado inicial ("RM" ou "EDF")
static portMUX_TYPE scheduler_spinlock = portMUX_INITIALIZER_UNLOCKED;

const int statusLedPin = 48; // LED de status do ESP32
bool ledEnabled = true; 
bool IsStarted = true;

TaskHandle_t rtosTaskHandle = NULL; 

static SemaphoreHandle_t mtx;

#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

void setupPWM() {
    pinMode(statusLedPin, OUTPUT);
    digitalWrite(statusLedPin, HIGH);
}

void setScheduler(String mode) {
    if (mode == "RM" || mode == "EDF") {

        xSemaphoreTake(mtx, portMAX_DELAY);
        strncpy((char*)currentScheduler, mode.c_str(), 3);
        xSemaphoreGive(mtx);

        if(mode == "RM"){
            digitalWrite(statusLedPin, HIGH);
        }else{
            digitalWrite(statusLedPin, LOW);
        }

        Serial.printf("MODO ALTERADO PARA: %s\n", mode.c_str());
        
        if (rtosTaskHandle != NULL) {
        }
    }
}

// Rota Principal (HTML)
String htmlPage() {
    // LEITURA SEGURA: Copia o estado volatile para uma String local (não volatile)
    xSemaphoreTake(mtx, portMAX_DELAY);
    // O cast (const char*) remove o qualificador volatile para o construtor String
    String currentModeString = (const char*)currentScheduler;
    xSemaphoreGive(mtx);

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
    xSemaphoreTake(mtx, portMAX_DELAY);
    String schedulerMode = (const char*)currentScheduler;
    xSemaphoreGive(mtx);
    
    server.send(200, "text/plain", schedulerMode);
}

void handleGetEnabled() { 
    server.send(200, "text/plain", ledEnabled ? "true" : "false"); 
}

void handleGetStatus() { 
    server.send(200, "text/plain", IsStarted ? "true" : "false"); 
}

void handleToggle() {
    ledEnabled = !ledEnabled;
    digitalWrite(statusLedPin, ledEnabled ? HIGH : LOW);
    server.send(200, "text/plain", "OK");
}

void display(void* pvParameters){
    for(;;){
        xSemaphoreTake(mtx, portMAX_DELAY);
        String mode = (const char*) currentScheduler;
        lcd.setCursor(0, 0);
        lcd.print("TRABALHO");

        lcd.setCursor(0, 1); 
        lcd.print(mode);

        int len = mode.length();
        for (int i = len; i < 16; i++) { // 16 é o número de colunas
            lcd.print(' ');
        }
        xSemaphoreGive(mtx);

        vTaskDelay(pdMS_TO_TICKS(1000));
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

        // IP (geralmente 192.168.4.1).
        Serial.print("IP do Ponto de Acesso: "); 
        Serial.println(WiFi.softAPIP()); 
    } else Serial.println("ERRO: Falha ao iniciar o Ponto de Acesso (AP)!");

    mtx = xSemaphoreCreateMutex();

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

    Serial.println("LCD Paralelo inicializado.");

    xTaskCreatePinnedToCore(display, "Display", 1024, NULL, 1, NULL, app_cpu);
}

void loop() {
    server.handleClient();
}