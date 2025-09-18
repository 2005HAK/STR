# Projeto RTOS Led Blink

## 🎯 Objetivo
Demonstrar o funcionamento de um sistema multitarefa usando um RTOS simples (MiROS) com as funções clássicas de **Produtor e Consumidor**, utilizando semáforos para sincronização.

---

## 🛠️ Plataforma
- **Microcontrolador:** STM32G474RE
- **IDE:** STM32CubeIDE
- **RTOS:** MiROS (Minimal Real-Time Operating System)
- **Linguagem:** C++

---

## ⚙️ Funcionamento

### 🧠 Conceito:
-


---

## 📊 Variáveis de Debug


---

## 🖥️ Como usar

### 1. Compile o projeto
Abra no STM32CubeIDE e clique no botão **martelo (Build)**.

### 2. Debug
- Clique com o botão direito no projeto → **Debug As → STM32 C/C++ Application**
- Rode o código e observe as variáveis em tempo real.

### 3. Sem hardware?
Você pode:
- Simular com Renode
- Ou compilar uma versão em C++ puro no PC (peça ajuda se quiser essa versão)

---

## 👨‍🔧 Threads disponíveis

- `funcao_produtor` → Produz dados
- `funcao_consumidor` → Consome dados
- `main_blinkyX` → Threads de teste com delays diferentes (opcional)

---

## ✅ Conclusão

Este projeto mostra como criar uma thread simples em Miros