// simples helper para alternar abas
function showTab(name){
    document.getElementById('panelCpu').style.display = (name=='cpu')?'block':'none';
    document.getElementById('panelExec').style.display = (name=='exec')?'block':'none';
    document.getElementById('panelJitter').style.display = (name=='jitter')?'block':'none';
    document.getElementById('panelPrio').style.display = (name=='prio')?'block':'none';
    // botões
    document.getElementById('tabCpu').classList.remove('active');
    document.getElementById('tabExec').classList.remove('active');
    document.getElementById('tabJitter').classList.remove('active');
    document.getElementById('tabPrio').classList.remove('active');
    document.getElementById('tab'+(name.charAt(0).toUpperCase()+name.slice(1))).classList.add('active');
}

async function setMode(mode){
    await fetch('/setScheduler?mode=' + mode);
    document.getElementById('currentMode').innerText = mode;
}

// Aguarda o carregamento do DOM e do Chart.js
document.addEventListener('DOMContentLoaded', function() {
    // dados e gráficos
    const maxPoints = 30; // quantos pontos manter no gráfico

    let labels = [];
    let cpuData = [];

    // Chart CPU
    const cpuCtx = document.getElementById('cpuChart').getContext('2d');
    const cpuChart = new Chart(cpuCtx, {
        type: 'line',
        data: { labels: labels, datasets: [{ label: 'CPU (%)', data: cpuData, fill:false }] },
        options: { scales: { y: { suggestedMin: 0, suggestedMax: 100 } } }
    });

    // Exec chart (uma linha por task)
    const execCtx = document.getElementById('execChart').getContext('2d');
    const execChart = new Chart(execCtx, {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { scales: { y: { suggestedMin: 0 } } }
    });

    // Miss chart
    const missCtx = document.getElementById('missChart').getContext('2d');
    const missChart = new Chart(missCtx, {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { scales: { y: { suggestedMin: 0 } } }
    });

    // Jitter chart
    const jitterCtx = document.getElementById('jitterChart').getContext('2d');
    const jitterChart = new Chart(jitterCtx, {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { scales: { y: { suggestedMin: 0 } } }
    });

    // Priority chart
    const prioCtx = document.getElementById('prioChart').getContext('2d');
    const prioChart = new Chart(prioCtx, {
        type: 'line',
        data: { labels: labels, datasets: [] },
        options: { scales: { y: { suggestedMin: 0 } } }
    });

    // mantém datasets por tarefa (por nome)
    const execDatasets = {};
    const missDatasets = {};
    const jitterDatasets = {};
    const prioDatasets = {};

    async function updateMetrics(){
        try {
            const res = await fetch('/metrics');
            const data = await res.json();

            const now = new Date().toLocaleTimeString();
            labels.push(now);
            if(labels.length > maxPoints) labels.shift();

            // CPU
            cpuData.push(data.cpu);
            if(cpuData.length > maxPoints) cpuData.shift();

            // tasks
            data.tasks.forEach((t, idx)=>{
                // exec datasets
                if(!execDatasets[t.name]){
                    const color = `hsl(${(idx*80)%360} 70% 50%)`;
                    execDatasets[t.name] = { label: t.name + ' (us)', data: [], borderColor: color, fill:false };
                    execChart.data.datasets.push(execDatasets[t.name]);

                    missDatasets[t.name] = { label: t.name + ' misses', data: [], borderColor: color, fill:false };
                    missChart.data.datasets.push(missDatasets[t.name]);

                    jitterDatasets[t.name] = { label: t.name + ' jitter(ms)', data: [], borderColor: color, fill:false };
                    jitterChart.data.datasets.push(jitterDatasets[t.name]);

                    prioDatasets[t.name] = { label: t.name + ' prio', data: [], borderColor: color, fill:false };
                    prioChart.data.datasets.push(prioDatasets[t.name]);
                }

                execDatasets[t.name].data.push(t.exec_us);
                missDatasets[t.name].data.push(t.misses);
                jitterDatasets[t.name].data.push(t.jitter_ms);
                prioDatasets[t.name].data.push(t.priority);

                // trim
                if(execDatasets[t.name].data.length > maxPoints) execDatasets[t.name].data.shift();
                if(missDatasets[t.name].data.length > maxPoints) missDatasets[t.name].data.shift();
                if(jitterDatasets[t.name].data.length > maxPoints) jitterDatasets[t.name].data.shift();
                if(prioDatasets[t.name].data.length > maxPoints) prioDatasets[t.name].data.shift();
            });

            // trim labels
            cpuChart.update();
            execChart.update();
            missChart.update();
            jitterChart.update();
            prioChart.update();

        } catch(e){
            console.log("Erro /metrics:", e);
        }
    }

    // atualiza cada 1s
    setInterval(updateMetrics, 1000);
    // chamada inicial
    updateMetrics();

}); // Fim do DOMContentLoaded
