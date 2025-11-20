# AD8232

Visão geral e instruções rápidas para o uso do módulo ECG AD8232 no projeto.

<p align="center">
  <img width="250" height="600" alt="image" src="https://github.com/user-attachments/assets/44cf8931-2c1f-4392-be6a-1960d761864b" />  
</p>

**Local do código:** [ecg.c](../../main/ecg.c), [ecg.h](../../main/ecg.h)

## 1. Resumo

O módulo ECG AD8232 capta o sinal cardíaco por meio de eletrodos adesivos conectados ao corpo, realizando aquisição e pré-processamento do sinal. Fornece saída analógica que é convertida pelo ADS1115 para armazenamento e análise.

## 2. Conexões (mapa de pinos usado no projeto)

| ESP32 | AD8232 | Função |
|:---:|:---:|:---:|
| D18 | LO- | Lead-off negativo |
| D19 | LO+ | Lead-off positivo |
| - | OUTPUT | Saída analógica (conectada ao ADS1115 A0) |
| 3.3V | VCC | Alimentação |
| GND | GND | Terra |

A pinagem utilizada pode ser encontrada em [ports.h](../../main/ports.h).

## 3. Configuração e requisitos

- Alimentação: 3.3V a 5V.
- Saída: Analógica (0-3.3V).
- Eletrodos: 3 fios (RA, LA, RL).
- Se o sinal não for detectado: verifique conexões dos eletrodos, contato com a pele, e alimentação estável.

## 4. Comportamento no projeto

- Durante a coleta (`SYSTEM_COLLECTING_STATE`), o AD8232 capta o sinal cardíaco em tempo real por meio de uma task do freeRTOS.
- Os pinos LO- e LO+ indicam se os eletrodos estão conectados corretamente.
- A saída analógica é amostrada pelo ADS1115 e armazenada no microSD.
- Pré-processa o sinal para reduzir ruídos e artefatos.

## 5. Exemplo de uso (comportamento esperado)

1. Conectar os 3 eletrodos adesivos no corpo: RA (braço direito), LA (braço esquerdo), RL (perna direita).
2. Ligar dispositivo — AD8232 inicializa e verifica lead-off.
3. Pressionar START — começa a captura do sinal cardíaco.
4. Durante coleta — sinal é enviado ao ADS1115.
5. Pressionar STOP — task é interrompida e coleta finaliza; dados ficam disponíveis para consulta.

## 6. Troubleshooting rápido

- Sinal fraco ou ausente: verifique contato dos eletrodos com a pele, posições corretas (RA, LA, RL), e ausência de suor excessivo.
- Indicadores LO ativos: eletrodos desconectados ou mal posicionados; reconecte e aguarde estabilização.
- Ruído no sinal: mover o dispositivo ou pessoa pode causar interferências; mantenha imóvel durante coleta.
- Alimentação instável: confirme tensão de 3.3V-5V; flutuações podem afetar o pré-processamento.
