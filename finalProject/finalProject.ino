/*
  Projeto: Escalonamento RM ↔ EDF no ESP32
  Autor: Gabriella Arévalo e Hebert Alan Kubis
  Matéria: Sistemas de Tempo Real 2025.2
  
  Correção aplicada: Alteração para modo Station (WIFI_STA) para permitir
  que o navegador baixe a biblioteca Chart.js da internet.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h> // Correção: caminho do include (case sensitive em alguns sistemas)
#include <LiquidCrystal.h>
#include <ArduinoJson.h>

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
// ATENÇÃO: Coloque aqui o Wi-Fi da sua casa/laboratório para ter internet
const char* ssid = "Hak"; 
const char* password = "maverick";
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
    // métricas extras
    uint32_t jitter_ms;         // jitter em ms (último ciclo)
    UBaseType_t current_prio;   // prioridade atual (lida em runtime)
} TarefaPeriodica;

TarefaPeriodica tarefas[NUM_TASKS] = {
    {"CalcLoad", 300, 0, -1, 1, 0, 0, 0, 0, NULL, 0, 0},
    {"Display", 500, 0, 26, 2, 0, 0, 0, 0, NULL, 0, 0}
};

TarefaPeriodica tarefaAperiodica = {"Aperiodica", 9, 0, -1, tskIDLE_PRIORITY, 0, 0, 0, 0, NULL, 0, 0};

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

void busyWait(uint32_t micros) {
    uint64_t inicio = esp_timer_get_time();
    while ((esp_timer_get_time() - inicio) < micros) asm volatile("nop");
}

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
            ponteiros[i]->current_prio = novaPrioridade;
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
    else {
        vTaskPrioritySet(t->handle, t->prioridade); // Se voltar para RM, restaura prioridades originais (fixas)
        t->current_prio = t->prioridade;
    }

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

            busyWait(8000); // Simula carga de ~8ms

            uint64_t fim = esp_timer_get_time();
            uint64_t exec_us = fim - inicio;

            t->carga_us = exec_us;
            t->total_exec_us += exec_us;
            t->ativacoes++;

            Serial.printf("[APERIODICA] Duracao=%lluus\n", (unsigned long long)exec_us);
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
        TickType_t expectedTick = ultimoTick + periodoTicks;
        vTaskDelayUntil(&ultimoTick, periodoTicks);

        uint64_t inicio = esp_timer_get_time();
    
        // --- jitter em ticks: diferença entre wake real e expectedTick ---
        TickType_t actualTick = xTaskGetTickCount();
        int32_t jitterTicks = (int32_t)(actualTick - expectedTick);
        uint32_t jitterMs = (uint32_t)( ( (int32_t)jitterTicks ) * portTICK_PERIOD_MS );
        t->jitter_ms = jitterMs;

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
        if(t->handle) t->current_prio = uxTaskPriorityGet(t->handle);
    }
}

void calcLoad(TarefaPeriodica* t){
    TickType_t ultimoTick = xTaskGetTickCount();
    // Ajustado para usar o periodo da própria tarefa para evitar confusão, mas mantive a lógica original se for intencional
    TickType_t periodoTicks = pdMS_TO_TICKS(t->periodo_ms); 

    for(;;){
        TickType_t expectedTick = ultimoTick + periodoTicks;
        vTaskDelayUntil(&ultimoTick, periodoTicks);

        uint64_t inicio = esp_timer_get_time();

        // --- jitter em ticks: diferença entre wake real e expectedTick ---
        TickType_t actualTick = xTaskGetTickCount();
        int32_t jitterTicks = (int32_t)(actualTick - expectedTick);
        uint32_t jitterMs = (uint32_t)( ( (int32_t)jitterTicks ) * portTICK_PERIOD_MS );
        t->jitter_ms = jitterMs;

        checkEDF(t, inicio);

        float totalUtilization = 0.0;

        for(int i = 0; i < NUM_TASKS; i++){
            if(tarefas[i].periodo_ms > 0){
                float Ui = (float)tarefas[i].carga_us / (float)(tarefas[i].periodo_ms * 1000.0f);
                totalUtilization += Ui;
            }
        }
        totalUtilization += (float)tarefaAperiodica.carga_us / (float)(tarefaAperiodica.periodo_ms * 1000.0f);

        tarefaAperiodica.carga_us = 0;

        xSemaphoreTake(mtxLoad, portMAX_DELAY);
        cpuLoad = (int)(totalUtilization * 100.0f);
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
        if(t->handle) t->current_prio = uxTaskPriorityGet(t->handle);
    }
}

void IRAM_ATTR isrBotao() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(semAperiodica, &xHigherPriorityTaskWoken);   // Libera a tarefa aperiódica
    if(xHigherPriorityTaskWoken) portYIELD_FROM_ISR();                // Força troca de contexto se necessário
}

// -------------------- ROTA /metrics --------------------
// Retorna JSON com arrays de métricas para uso pelo Chart.js
void handleMetrics() {
    // Aumentado um pouco o buffer por segurança (512 -> 768)
    StaticJsonDocument<768> doc;
    JsonArray tasks = doc.createNestedArray("tasks");

    xSemaphoreTake(mtxTasks, portMAX_DELAY);
    for (int i = 0; i < NUM_TASKS; i++) {
        JsonObject obj = tasks.createNestedObject();
        obj["name"] = tarefas[i].name;
        obj["exec_us"] = tarefas[i].carga_us;
        obj["misses"] = tarefas[i].misses;
        obj["jitter_ms"] = tarefas[i].jitter_ms;
        obj["priority"] = tarefas[i].current_prio;
        obj["ativacoes"] = tarefas[i].ativacoes;
    }
    xSemaphoreGive(mtxTasks);

    xSemaphoreTake(mtxLoad, portMAX_DELAY);
    doc["cpu"] = cpuLoad;
    xSemaphoreGive(mtxLoad);

    xSemaphoreTake(mtxSheduler, portMAX_DELAY);
    doc["mode"] = (const char*) currentScheduler;
    xSemaphoreGive(mtxSheduler);

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

// -------------------- HTML (page) --------------------
String htmlPage() {
    xSemaphoreTake(mtxSheduler, portMAX_DELAY);
    String currentModeString = (const char*)currentScheduler;
    xSemaphoreGive(mtxSheduler);

    String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>";
    page += "<title>Controle Dinâmico</title>";
    page += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    
    // CSS
    page += "<style>";
    page += "body{font-family: 'Segoe UI', sans-serif; background:#181818; color:#eee; text-align:center; margin:0; padding:10px;}";
    page += "h1{font-size: 1.5rem; margin-bottom: 5px;}";
    
    // Botões de Modo
    page += ".button-container{margin:10px auto; padding: 10px; width: fit-content; background: #222; border-radius: 8px;}";
    page += "button{padding:8px 16px; margin:0 5px; border:none; border-radius:4px; cursor:pointer; font-weight:600; transition:all 0.2s;}";
    page += ".btn-mode{background: #444; color: #ccc;}";
    page += ".btn-mode:hover{background: #666; color: #fff;}";
    page += ".btn-mode.active{background: #2ecc71; color: #fff; box-shadow: 0 0 8px rgba(46,204,113,0.4);}";
    
    // Abas
    page += ".tabs{display:flex; gap:5px; justify-content:center; margin-top:15px; flex-wrap: wrap;}";
    page += ".tab{padding:6px 12px; background:#333; border-radius:4px; cursor:pointer; font-size: 0.9rem; color: #aaa;}";
    page += ".tab:hover{background:#444; color:#fff;}";
    page += ".tab.active{background:#3498db; color:#fff;}";
    
    // Gráficos
    page += ".chart-box{background:#fff; margin:15px auto; padding:10px; border-radius:8px; max-width: 800px; height: 250px;}";
    page += ".status{font-size:1.1rem; font-weight:bold; margin-bottom:10px; display:block;}";
    page += "</style>";
    
    page += "</head><body>";
    page += "<h1>Monitoramento RTOS (ESP32)</h1>";

    page += "<div class='status'>Modo Atual: <span id='currentMode' style='color:#2ecc71'>" + currentModeString + "</span></div>";
    
    // Botões
    page += "<div class='button-container'>";
    page += "<button id='btnRM' class='btn-mode' onclick='setMode(\"RM\")'>RM (Fixo)</button>";
    page += "<button id='btnEDF' class='btn-mode' onclick='setMode(\"EDF\")'>EDF (Dinâmico)</button>";
    page += "</div>";

    // Abas
    page += "<div class='tabs'>";
    page += "<div class='tab active' id='tabCpu' onclick='showTab(\"cpu\")'>CPU Util</div>";
    page += "<div class='tab' id='tabExec' onclick='showTab(\"exec\")'>Execução & Misses</div>";
    page += "<div class='tab' id='tabJitter' onclick='showTab(\"jitter\")'>Jitter (ms)</div>";
    page += "<div class='tab' id='tabPrio' onclick='showTab(\"prio\")'>Prioridade</div>";
    page += "</div>";

    // Paineis (Canvases)
    page += "<div id='panelCpu' class='chart-box'><canvas id='cpuChart'></canvas></div>";
    page += "<div id='panelExec' class='chart-box' style='display:none;'><canvas id='execChart'></canvas><div style='height:10px'></div><canvas id='missChart'></canvas></div>";
    page += "<div id='panelJitter' class='chart-box' style='display:none;'><canvas id='jitterChart'></canvas></div>";
    page += "<div id='panelPrio' class='chart-box' style='display:none;'><canvas id='prioChart'></canvas></div>";

    page += R"(
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <script>
    
    // --- Configurações ---
    const UPDATE_INTERVAL_MS = 250; // Atualiza a cada 250ms (4x mais rápido que antes)
    const MAX_DATA_POINTS = 50;     // Mantém histórico maior no gráfico

    // --- Dados Globais ---
    let labels = [];
    
    // Helpers para datasets dinâmicos
    const datasets = { exec: {}, miss: {}, jitter: {}, prio: {} };

    // --- Opções Comuns ---
    const commonOptions = {
        responsive: true,
        maintainAspectRatio: false,
        animation: false, // Desativa animação suave para ficar "instantâneo"
        interaction: { mode: 'index', intersect: false },
        elements: { point: { radius: 0 } }, // Remove bolinhas para performance
        scales: { x: { display: false } }   // Esconde labels X para limpar visual
    };

    // 1. Chart CPU
    const cpuChart = new Chart(document.getElementById('cpuChart'), {
        type: 'line',
        data: { labels: labels, datasets: [{ label: 'CPU Load (%)', data: [], borderColor: '#e74c3c', backgroundColor: 'rgba(231,76,60,0.2)', fill: true, tension: 0.3 }] },
        options: { 
            ...commonOptions, 
            scales: { y: { min: 0, max: 100 } } 
        }
    });

    // 2. Chart Exec / Misses
    const execChart = new Chart(document.getElementById('execChart'), {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { ...commonOptions, plugins: { title: { display: true, text: 'Tempo de Execução (us)' } } }
    });
    const missChart = new Chart(document.getElementById('missChart'), {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { ...commonOptions, plugins: { title: { display: true, text: 'Deadline Misses (Acumulado)' } }, borderColor: '#e67e22' }
    });

    // 3. Chart Jitter (Auto-scale no Y para ver valores pequenos)
    const jitterChart = new Chart(document.getElementById('jitterChart'), {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { 
            ...commonOptions, 
            scales: { y: { suggestedMin: 0, suggestedMax: 5 } } // Escala pequena para ver 0, 1, 2ms
        }
    });

    // 4. Chart Prioridade (Stepped line para ver degraus)
    const prioChart = new Chart(document.getElementById('prioChart'), {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { 
            ...commonOptions,
            scales: { y: { ticks: { stepSize: 1 } } } // Força números inteiros no eixo Y
        }
    });

    // --- Lógica de Abas ---
    function showTab(name){
        // Oculta tudo
        document.querySelectorAll('.chart-box').forEach(el => el.style.display = 'none');
        document.querySelectorAll('.tab').forEach(el => el.classList.remove('active'));

        // Mostra o atual
        document.getElementById('panel'+(name.charAt(0).toUpperCase()+name.slice(1))).style.display = 'block';
        document.getElementById('tab'+(name.charAt(0).toUpperCase()+name.slice(1))).classList.add('active');

        // IMPORTANTE: Resize manual para corrigir bug do Chart.js em divs ocultas
        if(name === 'cpu') cpuChart.resize();
        if(name === 'exec') { execChart.resize(); missChart.resize(); }
        if(name === 'jitter') jitterChart.resize();
        if(name === 'prio') prioChart.resize();
    }

    // --- Comandos do Servidor ---
    async function setMode(mode){
        try { await fetch('/setScheduler?mode=' + mode); } catch(e){}
    }

    function updateInterface(mode){
        document.getElementById('currentMode').innerText = mode;
        document.getElementById('btnRM').className = (mode === 'RM') ? 'btn-mode active' : 'btn-mode';
        document.getElementById('btnEDF').className = (mode === 'EDF') ? 'btn-mode active' : 'btn-mode';
    }

    // --- Loop Principal ---
    async function updateMetrics(){
        try {
            const res = await fetch('/metrics');
            const data = await res.json();

            updateInterface(data.mode);

            // Gerencia Labels (eixo X)
            labels.push('');
            if(labels.length > MAX_DATA_POINTS) labels.shift();

            // Atualiza CPU
            cpuChart.data.datasets[0].data.push(data.cpu);
            if(cpuChart.data.datasets[0].data.length > MAX_DATA_POINTS) cpuChart.data.datasets[0].data.shift();

            // Atualiza Tarefas
            data.tasks.forEach((t, idx) => {
                const color = `hsl(${(idx * 137) % 360}, 70%, 50%)`;

                // Função auxiliar para garantir que o dataset existe
                const ensureDataset = (chart, dict, key, label, isStepped = false) => {
                    if(!dict[key]) {
                        // Preenche histórico com null para não riscar o passado
                        const hist = new Array(labels.length - 1).fill(null);
                        dict[key] = {
                            label: label,
                            data: [...hist],
                            borderColor: color,
                            fill: false,
                            stepped: isStepped, // Importante para prioridade
                            borderWidth: 2
                        };
                        chart.data.datasets.push(dict[key]);
                    }
                    return dict[key];
                };

                // EXEC
                const dsExec = ensureDataset(execChart, datasets.exec, t.name, t.name);
                dsExec.data.push(t.exec_us);
                if(dsExec.data.length > MAX_DATA_POINTS) dsExec.data.shift();

                // MISS
                const dsMiss = ensureDataset(missChart, datasets.miss, t.name, t.name);
                dsMiss.data.push(t.misses);
                if(dsMiss.data.length > MAX_DATA_POINTS) dsMiss.data.shift();

                // JITTER
                const dsJitter = ensureDataset(jitterChart, datasets.jitter, t.name, t.name);
                dsJitter.data.push(t.jitter_ms);
                if(dsJitter.data.length > MAX_DATA_POINTS) dsJitter.data.shift();

                // PRIO (Stepped = true para ver degraus)
                const dsPrio = ensureDataset(prioChart, datasets.prio, t.name, t.name, true);
                dsPrio.data.push(t.priority);
                if(dsPrio.data.length > MAX_DATA_POINTS) dsPrio.data.shift();
            });

            // Renderiza apenas se a aba estiver visível (economiza CPU do navegador)
            const activeTab = document.querySelector('.tab.active').id;
            if(activeTab === 'tabCpu') cpuChart.update();
            if(activeTab === 'tabExec') { execChart.update(); missChart.update(); }
            if(activeTab === 'tabJitter') jitterChart.update();
            if(activeTab === 'tabPrio') prioChart.update();

        } catch(e){
            console.error(e);
        }
    }

    // Inicia loop rápido
    setInterval(updateMetrics, UPDATE_INTERVAL_MS);

    </script>
    )";

    page += "</body></html>";
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

// rota metrics
void handleMetricsWrapper() { handleMetrics(); }

void setup() {
    Serial.begin(115200);

    // Configuração dos pinos
    pinMode(BOTAO, INPUT_PULLDOWN);
    pinMode(statusLedPin, OUTPUT);

    // Fim configuração dos pinos
    
    // ------------------------------------------------------------------------
    // CONFIGURAÇÃO WIFI (MODO STATION)
    // Motivo: Para que a biblioteca Chart.js possa ser baixada da internet,
    // o ESP32 e o computador precisam estar numa rede COM internet.
    // ------------------------------------------------------------------------
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("Conectando ao WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.print("Conectado! IP do Servidor: "); 
    Serial.println(WiFi.localIP()); 
    // ------------------------------------------------------------------------

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
    server.on("/metrics", handleMetricsWrapper); // rota para gráficos

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

    Serial.println("Sistema iniciado!");
}

void loop() {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
}