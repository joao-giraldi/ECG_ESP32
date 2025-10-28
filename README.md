# Projeto de Sistemas Ubíquos: Eletrocardiograma com ESP32

## Introdução

Este projeto foi desenvolvido como parte da disciplina **Sistemas Ubíquos (DEC0021)** do curso de **Engenharia de Computação da Universidade Federal de Santa Catarina (UFSC)** no segundo semestre de 2025.

O trabalho tem como objetivo aplicar conceitos de sistemas embarcados e computação ubíqua no desenvolvimento de uma solução prática utilizando o microcontrolador **ESP32**. O projeto abrange, desde a integração de sensores e armazenamento local, até a disponibilização de uma interface web para visualização e análise dos dados coletados.

O sistema foi implementado e testado pelos alunos:

- **João Marcos Moço Giraldi** — Matrícula: *21203171*  
- **Gabriel Juliani Segatto** — Matrícula: *21200606*

O desenvolvimento seguiu princípios de modularidade e comunicação ubíqua, com ênfase em eficiência energética, acessibilidade via rede local e integração entre hardware e software. A ideia do projeto surgiu com a necessidade da Profa. Dra. Cristiane A. Moran da Fisioterapia do Campus, em que era necessario coletar e armazenas os dados de batimentos cardiacos de recém-nascidos, porem era inviável entrar com qualquer tipo de tecnologia (celulares, computadores, etc) dentro da sala onde o recém-nascido estaria.

As próximas seções deste documento detalham a estrutura do projeto, o hardware utilizado, a arquitetura de software, os testes realizados e as etapas de instalação e uso do sistema.

## Hardware Necessário (Material Utilizado)

A seguir estão listados os principais componentes utilizados na montagem do sistema, com uma breve descrição de suas funções e especificações básicas:

- **ESP32 (ESP-WROOM-32):** Microcontrolador principal responsável pela leitura dos sinais, controle das tarefas e comunicação Wi-Fi.  
  *Especificações:* Dual-core, 240 MHz, 12-bit ADC, suporte a I²C/SPI/UART.

- **Módulo ECG AD8232:** Captura o sinal cardíaco por meio dos eletrodos conectados ao corpo, realizando a aquisição e o pré-processamento do sinal.  
  *Especificações:* Alimentação 3,3–5V, saída analógica.

- **Conversor ADC ADS1115:** Faz a conversão analógica–digital de 16 bits, garantindo maior precisão na leitura do sinal do AD8232.  
  *Especificações:* 4 canais, resolução de 16 bits, comunicação I²C.

- **Módulo MicroSD Adapter:** Responsável pelo armazenamento local das amostras coletadas no cartão microSD.  
  *Especificações:* Comunicação SPI, suporte a cartões até 32 GB (FAT32).

- **Regulador de Tensão LM2596:** Garante a alimentação estável dos componentes, convertendo tensões mais altas para os 5V necessários pelo ESP e determinados componentes.  
  *Especificações:* Conversor buck DC–DC, saída de tensão ajustável, até 3A.

- **Display LCD 128x64 (I²C):** Exibe informações básicas do sistema como instruções de uso, status e tempo de captura.  
  *Especificações:* 128 colunas, 64 linhas, interface I²C.

- **Dois Botões (Push Buttons):** Usados para iniciar/parar a captura e alternar modos de operação.  
  *Especificações:* Push-button momentâneo, interrupção externa.

- **Switch:** Permite ativar/desativar o sistema.  
  *Especificações:* Chave integrada à fonte.

---

### Esquema de Conexão

A figura a seguir ilustra as conexões entre os principais componentes do sistema (ESP32, ADS1115, AD8232, LCD, microSD, regulador e controles físicos):

<img width="1008" height="777" alt="ecg_pinout" src="https://github.com/user-attachments/assets/6e8a1887-b950-4605-a37f-392a96d70e3e" />

---

## Instalação

Este projeto usa o **ESP-IDF** integrado ao **VS Code** (extensão *Espressif IDF*). Abaixo estão os passos para preparar o ambiente, configurar o projeto e gravar no ESP32.

### 1) Pré-requisitos

- **VS Code** (atualizado)
- Extensão **Espressif IDF** (Espressif IDF by Espressif Systems)
- **ESP-IDF** (instalado pela própria extensão)
- **Driver USB–UART** do seu kit
- Cartão **microSD** em **FAT32**

---

### 2) Obter o código e abrir no VS Code

1. Clone/baixe este repositório.
2. Abra a pasta do projeto no VS Code.

---

### 3) Selecionar a *Target* e a porta serial

- No Command Palette:
  - `Espressif: Select Port` → selecione a porta do seu ESP32 (ex.: `COMx` no Windows ou `/dev/ttyUSBx` no Linux).
  - `Espressif: Set Espressif Device Target` → **esp32** (ESP-WROOM-32/DevKitC).

---

### 4) Ajustes mínimos de projeto

- **Pinos e periféricos**: revise `ports.h` (I²C, botões, switch, LCD, etc.).
- **Wi-Fi AP**: no `main.c`, a rede padrão é criada por:
  wifi_init_ap("ECG-ESP32", "12345678", 6, 4, false);
  Altere **SSID** e **senha** conforme necessário.
- **microSD**: garanta que o cartão está em **FAT32** e que as linhas SPI/SDMMC e alimentação (5V) estão corretas.

---

### 5) Compilar, gravar e monitorar

Pelo Command Palette:
- `Espressif: Build Project`
- `Espressif: Flash Device`
- `Espressif: Monitor Device`

Ou via terminal do ESP-IDF:
idf.py set-target esp32
idf.py build
idf.py -p <PORTA> flash
idf.py -p <PORTA> monitor

> Dica: Para sair do monitor: `Ctrl+]`.

---

### 6) Bibliotecas/Componentes principais usados
Listamos só os módulos mais relevantes ao funcionamento (o projeto inclui outros headers/componentes internos):

- FreeRTOS: freertos/FreeRTOS.h, freertos/task.h  
  Agendamento de tasks, filas (Queue), sincronização e gerenciamento de recursos.
- GPIO / Interrupções: driver/gpio.h  
  Leitura de botões, debounce via software e ISR para trocar estados do sistema.
- I²C: driver/i2c.h
  Barramento para LCD (I²C) e ADC externo (ADS1115).
- ADC Externo (ADS1115): ads1115.h
  Conversão de alta resolução do sinal do ECG (AD8232).
- LCD (I²C): lvgl.h
  Controle do LCD para exibição de status: inicialização, coleta, rede e servidor.
- microSD / FATFS: esp_vfs_fat.h, sdmmc_cmd.h
  Formatação do microSD, e acesso ao mesmo para o armazenamento de buffers de ECG em arquivos.
- Wi-Fi (AP): esp_wifi, esp_netif, nvs_flash
  Cria um Access Point (ex.: ECG-ESP32).
- Servidor HTTP: web_server.h (sobre esp_http_server)  
  Serve UI/arquivos e expõe endpoints para listar/baixar dados do microSD.
- mDNS: mdns.h (sobre mdns)  
  Máscara de IP para acesso por nome local (ex.: http://ecg.local).
- Timer de amostragem: gptimer  
  Controle de tempo de amostragem do sinal ECG.
- Log: esp_log.h  
  Depuração organizada por tag (ex.: "MAIN", "ISR").

---

### 7) Fluxo de execução (resumo prático)

1. Inicializa Wi-Fi AP e mDNS → acesso local: http://ecg.local.
2. Configura I²C, LCD (logo/estado), ECG/ADS1115, microSD.
3. Sobe servidor HTTP e registra rotas do SD.
4. Cria as tasks: ecg_task (amostragem/buffer) e sd_task (persistência em microSD).
5. Botões (ISR) alternam estados:
   - SYSTEM_BOOT_STATE → estado inicial padrão do sistema (display do logo)
   - SYSTEM_COLLECTING_STATE → inicia amostragem e gravação
   - SYSTEM_WEB_STATE → encerra captura, fecha arquivo, exibe telas (stopped/network/server) e mantém o servidor acessível.

---

### 8) Problemas comuns & soluções rápidas

- Falha ao montar o microSD: o sistema fica reiniciando em *loop*. Desligue a alimentação e tente novamente. Caso não resolva, formate em FAT32 e verifique alimentação (deve ser aproximadamente 4.5V) e pinos.

---

## Funcionalidades Principais

O sistema desenvolvido consiste em **um dispositivo portátil de monitoramento cardíaco** baseado no **ESP32**, montado em uma **caixa com placa universal**, todos os componentes devidamente soldados e alimentado por **fonte externa com pilhas**.  
O projeto integra hardware embarcado e interface web, permitindo tanto a coleta de sinais de ECG quanto a análise posterior via navegador.

---

### Estrutura e operação do dispositivo

- **Alimentação por pilhas** — o sistema é totalmente alimentado por fonte externa acoplada, garantindo portabilidade.  
- **Interruptor (Switch)** — responsável por **ligar e desligar** o dispositivo.  
- **Dois botões físicos:**
  - **Botão Iniciar:** inicia a medição e gravação do sinal de ECG.
  - **Botão Parar:** finaliza a captura e fecha o arquivo no cartão microSD.
- **Display LCD 128x64:** exibe em tempo real o estado do sistema, alternando entre telas de:
  - Inicialização (logo)
  - Coleta de dados (com contador de tempo)
  - Estado parado / rede / servidor ativo

Durante a coleta, o **ECG AD8232** capta o sinal cardíaco, o **ADS1115** realiza a conversão analógica–digital e o **ESP32** coordena o armazenamento no microSD e a atualização do LCD.

---

### Servidor Web Integrado

Enquanto o dispositivo está ligado, o **ESP32 hospeda um servidor web local**, acessível via rede criada pelo próprio microcontrolador.  
O acesso pode ser feito digitando no navegador após conectar na rede:

> Nome padrão: **http://ecg.local**

No navegador, o usuário é redirecionado para o **dashboard do sistema**, desenvolvido em HTML/JavaScript embarcado no próprio ESP32.

---

### Dashboard e funcionalidades online

O painel web permite visualizar, selecionar e exportar os dados de ECG coletados:

- **Seleção de arquivos**: lista automaticamente os arquivos gravados no microSD.
- **Visualização gráfica**: mostra em tempo real a forma de onda do sinal cardíaco (ECG).
- **Frequência de amostragem (Fs)**: exibida na interface, com valor padrão de **100 Hz**, mas ajustável conforme necessidade.
- **Cálculo automático de BPM médio**: o sistema identifica os picos do sinal e calcula a frequência cardíaca média.
- **Download dos dados**: os registros podem ser exportados em **formato `.txt`**, contendo todas as medições cruas (amostras do ECG).

<img width="1375" height="744" alt="image" src="https://github.com/user-attachments/assets/7494f18e-6082-4196-b8b9-cb43ac5f0dd7" />

---

### Resumo de fluxo

1. Usuário **liga o dispositivo** pelo switch.
2. O LCD mostra o **logo e status de inicialização**.
3. Ao pressionar **Iniciar**, a coleta começa:
   - ECG é amostrado pelo ADS1115.
   - Dados são armazenados em tempo real no microSD.
   - O LCD exibe o tempo de coleta.
4. Ao pressionar **Parar**, a coleta é finalizada:
   - Arquivo é fechado no microSD.
   - O ESP32 entra no modo web (exibe IP e estado do servidor).
5. Ao acessar **ecg.local**, o usuário vê o **dashboard** com:
   - Sinal capturado.
   - BPM médio calculado.
   - Frequência de amostragem.
   - Opção de **baixar o arquivo em `.txt`**.

---
## Tutorial de Uso do Sistema e dos Sensores

A seguir está o passo a passo completo para operar o sistema de monitoramento de ECG, desde a ligação do dispositivo até a visualização dos batimentos cardíacos no dashboard.

---

### 1) Ligar o dispositivo

- Certifique-se de que **as pilhas estão corretamente instaladas** na caixa.  
- Acione o **switch lateral** para ligar o sistema.
- O **display LCD** acenderá e exibirá a **tela de inicialização** com o logotipo e status de boot.

---

### 2) Conectar os sensores ECG (AD8232)

- Encaixe corretamente o **conector de 3 fios** dos eletrodos no módulo AD8232.  
- Fixe os **eletrodos adesivos** no corpo do usuário, seguindo a posição recomendada:
  - **Vermelho** – braço direito (RA)  
  - **Amarelo** – braço esquerdo (LA)  
  - **Verde** – perna direita (RL, referência)  
- Espere alguns segundos para estabilização do contato com a pele antes de iniciar a medição.

<p align="center">
  <img width="434" height="399" alt="image" src="https://github.com/user-attachments/assets/a4128ccc-e1b2-4059-88ea-43dff5baf3c6" />
</p>
---

### 3) Iniciar a medição

- Pressione o **botão de início** na caixa (marcado como *START*).  
- O LCD mudará para o estado **"Coletando dados"**, mostrando o **tempo decorrido** da captura.  
- A coleta será salva em um novo arquivo no **cartão microSD**, com nome gerado automaticamente (ex.: `ecg_1.bin`, `ecg_2.bin`, assim por diante.).

**IMPORTANTE**! O sistema mantém apenas 50 arquivos por vez no SD, até passar a sobrescrever os arquivos mais antigos. Caso queira mantê-los por mais tempo, utilize a função de **DOWNLOAD**.

---

### 4) Finalizar a medição

- Quando desejar encerrar a coleta, pressione o **botão de parada** (*STOP*).  
- O LCD exibirá o estado **"Coleta finalizada"**, seguido pelo **nome do arquivo salvo**.  
- O sistema automaticamente suspende a tarefa de coleta e ativa o modo **Web Server**, ficando pronto para acesso via rede.

---

### 5) Conectar-se à rede do dispositivo

- No computador ou celular, abra as configurações de Wi-Fi.  
- Conecte-se à rede criada pelo ESP32:
  - **SSID:** `ECG-ESP32`
  - **Senha:** `12345678`  
- Aguarde alguns segundos até a conexão ser estabelecida.

---

### 6) Acessar o dashboard web

- Após conectado, abra o navegador e digite o endereço:  
  **http://ecg.local**  

- A página carregará o **dashboard de visualização**, que permite:
  - Selecionar o arquivo desejado entre os armazenados no **microSD**.
  - Visualizar o **sinal do ECG** em forma de gráfico.
  - Observar a **frequência de amostragem (Fs)** — padrão de 100 Hz.
  - Ver o **BPM médio calculado automaticamente**.
  - Fazer **download dos dados** em **formato `.txt`**.

---

### 7) Interpretar os resultados

- O gráfico exibido representa a **variação do sinal elétrico cardíaco** (ECG) em função do tempo.  
- O sistema realiza uma análise simples de picos para estimar o **BPM médio**.  
- O arquivo `.txt` baixado contém **todas as amostras brutas** (em sequência temporal), permitindo análises mais detalhadas posteriormente em ferramentas como Python, MATLAB ou Excel.

---

### 8) Desligar o dispositivo

- Após finalizar e verificar os dados, desligue o sistema movendo o **switch** para a posição *OFF*.  
- Os dados permanecerão gravados no microSD até serem manualmente removidos ou substituídos.

---
