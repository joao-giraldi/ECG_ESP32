# LCD

Visão geral e instruções rápidas para o uso do display LCD no projeto.

**Local do código:** [lcd.c](../../main/lcd.c), [lcd.h](../../main/lcd.h)

## 1. Resumo

O display LCD 128x64 I²C é usado para exibir informações básicas do sistema, como logo inicial, status de coleta, tempo decorrido e mensagens de rede. Utiliza a biblioteca LVGL para renderização gráfica e interface I²C para comunicação com o ESP32.

## 2. Conexões (mapa de pinos usado no projeto)

| ESP32 | LCD | Função |
|:---:|:---:|:---:|
| D21 | SDA | Dados I²C (SDA) |
| D22 | SCL | Clock I²C (SCL) |
| 3.3V | VCC | Alimentação |
| GND | GND | Terra |

A pinagem utilizada pode ser encontrada em [ports.h](../../main/ports.h). Compartilha o barramento I²C com o ADS1115.

## 3. Configuração e requisitos

- Resolução: 128x64 pixels.
- Interface: I²C.
- Biblioteca: LVGL (integrada via ESP-IDF).
- Alimentação: 3.3V.
- Se o display não inicializar: verifique conexões I²C, endereço padrão (geralmente 0x3C), e alimentação estável.

## 4. Comportamento no projeto

- **Estado inicial (SYSTEM_BOOT_STATE):** Exibe logo do coração e mensagem de boot.
- **Durante coleta (SYSTEM_COLLECTING_STATE):** Mostra contador de tempo decorrido e status "Coletando".
- **Estado parado (SYSTEM_WEB_STATE):** Exibe informações da coleta como, tempo decorrido, nome arquivo salvo, rede wifi e URL da página web.
- Atualizações são feitas via tarefas FreeRTOS para evitar bloqueios.

## 5. Exemplo de uso (comportamento esperado)

1. Ligar dispositivo — LCD acende e mostra logo inicial.
2. Pressionar START — transita para tela de coleta com timer.
3. Durante coleta — atualiza tempo em tempo real.
4. Pressionar STOP — muda para tela de rede/servidor ativo.
5. Desligar — LCD apaga automaticamente.

## 6. Troubleshooting rápido

- Display não acende: verifique alimentação (3.3V), conexões I²C, e endereço I²C correto.
- Tela congelada: possível problema de tarefa FreeRTOS; verifique logs por erros de LVGL ou I²C.
- Texto distorcido: confirmar resolução e configuração LVGL; testar com outro display I²C.