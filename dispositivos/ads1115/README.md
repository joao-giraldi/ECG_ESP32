# ADS1115

Visão geral e instruções rápidas para o uso do conversor ADC ADS1115 no projeto.

<p align="center">
  <img width="250" height="auto" alt="image" src="https://github.com/user-attachments/assets/17839e62-0656-4cf7-a0a4-2be97b2ab68b" />
</p>

**Local do código:** [ads1115.c](../../main/ads1115.c), [ads1115.h](../../main/ads1115.h)

## 1. Resumo

O ADS1115 é um conversor analógico-digital de 16 bits com 4 canais, usado para converter o sinal analógico do AD8232 em dados digitais com alta precisão. Comunica via I²C com o ESP32.

## 2. Conexões (mapa de pinos usado no projeto)

| ESP32 | ADS1115 | Função |
|:---:|:---:|:---:|
| D21 | SDA | Dados I²C (SDA) |
| D22 | SCL | Clock I²C (SCL) |
| GND | ADDR | Endereço I²C (GND para 0x48) |
| 3.3V | VCC | Alimentação |
| GND | GND | Terra |

A pinagem utilizada pode ser encontrada em [ports.h](../../main/ports.h). Compartilha o barramento I²C com o LCD.

## 3. Configuração e requisitos

- Resolução: 16 bits.
- Canais: 4 (usado A0 para sinal do AD8232).
- Interface: I²C.
- Taxa de amostragem: Configurável (padrão 100 Hz no projeto) por interupção de clock.
- Se a conversão falhar: verifique conexões I²C, endereço correto (0x48), e alimentação 3.3V.

## 4. Comportamento no projeto

- Durante a coleta (`SYSTEM_COLLECTING_STATE`), o ADS1115 amostra o sinal analógico do AD8232 em intervalos regulares.
- Dados convertidos são armazenados em buffer e escritos no microSD via task dedicada.
- Fornece precisão superior ao ADC interno do ESP32 para sinais de baixa amplitude como ECG.

## 5. Exemplo de uso (comportamento esperado)

1. Ligar dispositivo — ADS1115 inicializa via I²C.
2. Pressionar START — começa amostragem no canal A0.
3. Durante coleta — conversões ocorrem em 100 Hz, dados bufferizados.
4. Pressionar STOP — amostragem para; buffer final escrito no microSD.
5. Dados disponíveis para consulta via web.

## 6. Troubleshooting rápido

- Conversões incorretas: verifique conexão do OUTPUT do AD8232 ao A0 do ADS1115, e configurações de ganho/amostragem.
- Erro I²C: confirme pinos SDA/SCL, endereço 0x48 (ADDR a GND), e ausência de conflitos no barramento.
- Resolução baixa: ajustar taxa de amostragem ou ganho; sinais fracos podem precisar de amplificação externa.
- Alimentação: 3.3V estável; flutuações afetam precisão.
