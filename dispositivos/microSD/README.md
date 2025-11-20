# microSD

Visão geral e instruções rápidas para o uso do cartão microSD no projeto.

<p align="center">
  <img width="250" height="auto" alt="image" src="https://github.com/user-attachments/assets/d1b48c46-82de-4582-923d-bcce5869cee8" />  
</p>

**Local do código:** [microSD.c](../../main/microSD.c), [microSD.h](../../main/microSD.h)

## 1. Resumo

O sistema grava amostras de ECG no cartão microSD via interface SPI. Os arquivos são gerados automaticamente durante a coleta e armazenados em formato binário (ex.: `ecg_1.bin`). A interface web do dispositivo disponibiliza download dos dados em `.csv` para análise externa.

## 2. Conexões (mapa de pinos usado no projeto)

| ESP32 | microSD | Função |
|:---:|:---:|:---:|
| D4  | MISO | Dados (MISO) |
| D5  | CS   | Chip Select |
| D15 | SCK  | Clock (SCK) |
| D23 | MOSI | Dados (MOSI) |
| VIN | VCC | Alimentação 5V|
| GND | GND | Terra |

A pinagem utilizada pode ser encontrada em [ports.h](../../main/ports.h).

## 3. Formatação e requisitos do cartão

- Formato recomendado: **FAT32**.
- Cartões de até 32 GB são suportados; cartões maiores podem não ser reconhecidos.
- Se o cartão não montar: formatar em FAT32 e testar em outro leitor USB antes de inserir no dispositivo.

## 4. Comportamento no projeto

- Durante a captura (`SYSTEM_COLLECTING_STATE`) um novo arquivo é criado no microSD.
- Nomeação automática: `ecg_<n>.bin` (incremental). A camada de servidor web fornece conversão/download em `.csv`.
- O sistema mantém um número limitado de arquivos (ex.: 50) antes de sobrescrever os mais antigos — ver implementação em [microSD.c](../../main/microSD.c).

## 5. Exemplo de uso (comportamento esperado)

1. Ligar dispositivo (switch).  
2. Pressionar `START` para iniciar coleta — novo arquivo é criado no SD.  
3. Durante a coleta, amostras são empilhadas em buffer e escritas periodicamente no arquivo.  
4. Pressionar `STOP` — arquivo é fechado; servidor web entra no ar e lista arquivos para visualização ou download.

## 6. Troubleshooting rápido

- Falha ao montar o microSD / Device reinicia: verifique alimentação (aprox. 4.5V), pinos SPI, e formatação FAT32.  
- Arquivos corrompidos: testar cartão em PC; testar com outro cartão; checar se a tarefa de escrita tem prioridade suficiente (evitar preempções).  
- Não há arquivos visíveis na web: confirme que o SD foi montado com sucesso e que o servidor HTTP encontrou o diretório correto (ver logs no monitor serial — tag `SD` ou `MAIN`).
