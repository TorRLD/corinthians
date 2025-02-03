## Animação Corinthians com Hino 

Este projeto em C/C++ para Raspberry Pi Pico (RP2040) demonstra:

1. **Reprodução de uma melodia** usando **buzzer passivo** via PWM, onde as notas são definidas como  `A, B, C, D, E, F, G` e suas variações sustenidas `AS, CS, DS, FS, GS`, além de `"-"` para silêncio.
2. **Exibição de imagens** numa matriz de 5×5 LEDs WS2812, gerenciada em paralelo no **core 1** (multicore).

## Sumário

* [Componentes e Requisitos](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#componentes-e-requisitos)
* [Visão Geral do Projeto](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#vis%C3%A3o-geral-do-projeto)
* [Como Compilar e Carregar](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#como-compilar-e-carregar)
* [Arquitetura do Código](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#arquitetura-do-c%C3%B3digo)
  * [Estrutura de Pastas (Sugestão)](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#estrutura-de-pastas-sugest%C3%A3o)
  * [Descrição dos Principais Arquivos](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#descri%C3%A7%C3%A3o-dos-principais-arquivos)
* [Funcionamento em Detalhes](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#funcionamento-em-detalhes)
  * [Melodia com Notas sem Números](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#melodia-com-notas-sem-n%C3%BAmeros)
  * [Exibição das Imagens (WS2812)](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#exibi%C3%A7%C3%A3o-das-imagens-ws2812)
  * [Uso do Multicore (RP2040)](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#uso-do-multicore-rp2040)
* [Ajustes e Personalizações](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#ajustes-e-personaliza%C3%A7%C3%B5es)
* [Licença](https://chatgpt.com/c/67a14930-b688-800b-8062-fe8ddc2a6c53#licen%C3%A7a)

---

## Componentes e Requisitos

1. **Placa** : Raspberry Pi Pico ou similar (RP2040), eu utilizei a BitDogLab.
2. **Buzzer Passivo** conectado ao pino definido em `#define BUZZER_PIN` (no código, pino 10).
3. **Matriz WS2812 5×5** (ou 25 LEDs WS2812 dispostos em grade).
   * Conectada ao pino definido em `#define WS2812_PIN` (no código, pino 7).
4. **SDK do Raspberry Pi Pico** configurado (CMake, toolchain ARM, etc.).
5. Opcional: cabo USB para alimentação e programação.

---

## Visão Geral do Projeto

* **Melodia** : um array de 119 elementos com strings como `"C"`, `"E"`, `"GS"` e `"-"` (silêncio).
* **PWM** : O PWM do RP2040 gera a frequência definida pela função `note_to_freq()`.
* **Multicore** :
* **Core 0** : gerencia a música (troca de notas, silêncios, etc.).
* **Core 1** : atualiza, a cada 1 segundo, as imagens do **brasão** e variações na matriz 5×5 de LEDs WS2812.

---

## Como Compilar e Carregar

1. **Instale** o [Pico SDK](https://github.com/raspberrypi/pico-sdk) e o [CMake](https://cmake.org/).
2. **Crie** uma pasta de projeto, por exemplo `pico_melodia_ws2812/`.
3. **Coloque** dentro dessa pasta:
   * O arquivo principal `main.c` (com o código fornecido).
   * O arquivo `ws2812.pio` (programa PIO para controlar WS2812).
   * Um `CMakeLists.txt` básico para compilar com o SDK do Pico (ver exemplo abaixo).
4. **Abra** um terminal na pasta do projeto e execute:
   ```bash
   mkdir build
   cd build
   cmake ..
   make
   ```
5. Será gerado um arquivo `.uf2` na pasta `build`.
6. **Conecte** seu Pico em modo bootloader (segurando o botão BOOTSEL ao encaixar no USB).
7. **Arraste** o `.uf2` para a unidade de disco que aparecerá (RPI-RP2).

### Exemplo de CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)

project(melodia_ws2812 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(melodia_ws2812
    main.c
    ws2812.pio
)

pico_generate_pio_header(melodia_ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

target_link_libraries(melodia_ws2812 pico_stdlib hardware_pio hardware_pwm)

pico_enable_stdio_usb(melodia_ws2812 1)
pico_enable_stdio_uart(melodia_ws2812 0)

pico_add_extra_outputs(melodia_ws2812)
```

---

## Arquitetura do Código

### Estrutura de Pastas (Sugestão)

```
pico_melodia_ws2812/
├── CMakeLists.txt
├── main.c
├── ws2812.pio
└── build/      (gerado após cmake .. && make)
```

### Descrição dos Principais Arquivos

* **`main.c`** :
* Contém toda a lógica para tocar a melodia (notas em formato `char *`), configurar o PWM, tratar cada “nota” ou “silêncio”.
* Inicializa e lança `core1_entry()` para a atualização da matriz WS2812.
* Mantém as imagens 5×5 definidas em array `padroes_imagens[6][5][5]`.
* **`ws2812.pio`** :
* Arquivo PIO contendo o programa assembly do RP2040 (PIO state machine) para acionar LEDs WS2812 (protocolo one-wire).

---

## Funcionamento em Detalhes

### Melodia com Notas sem Números

* Cada nota, como `"C"`, `"D"`, `"E"`, `"F"`, `"G"`, `"A"`, `"B"`, `"CS"`, `"DS"`, `"FS"`, `"GS"`, `"AS"`, é convertida na função `note_to_freq()` para **uma frequência** (por exemplo, `"E"` → 330 Hz).
* Não há distinção de oitava nos nomes: `"C"` corresponde a ~262 Hz (C4), `"E"` ~330 Hz (E4), etc. Assim, a melodia fica toda “na mesma oitava” do ponto de vista do código.

### Exibição das Imagens (WS2812)

* Há 6 “imagens” em `padroes_imagens[][][]`, cada qual definindo cores (RGB + brilho) para 25 LEDs (5×5).
* No `core1_entry()`, a cada 1 segundo troca para a próxima imagem e a envia para os LEDs via PIO.

### Uso do Multicore (RP2040)

* **Core 0** :
* Inicializa PWM para o buzzer (`BUZZER_PIN`), define a primeira nota e gerencia uma pequena máquina de estados (`STATE_TONE` e `STATE_GAP`) para controlar quando toca e quando faz pausa.
* **Core 1** :
* Executa a função `core1_entry()`, que atualiza a matriz WS2812 a cada 1s, trocando entre as imagens.

---

## Ajustes e Personalizações

1. **Alterar a frequência de cada nota** :

* Na função `note_to_freq()`, você pode trocar o valor (por exemplo, `"C"` de 262 Hz para 523 Hz, se quiser tudo na 5ª oitava).

1. **Acrescentar/Remover Notas** :

* Se quiser `"BS"` (B sustenido) ou outro semitom, basta adicionar a condição no `note_to_freq()`.

1. **Mudar o Tempo** :

* Ajuste `compasso` (p.ex. 1500 para 2000) ou as subdivisões em `tempoNotas[]` para ficar mais lento ou mais rápido.

1. **Mudar as Imagens** :

* Edite as matrizes `padroes_imagens[6][5][5]` para desenhar novos padrões, alterando cor e brilho.

1. **Duty Cycle** :

* Se algumas notas soarem distorcidas, teste *duty cycles* menores (ex.: 30%) ajustando `pwm_set_chan_level(..., wrap * 0.3f)`.

---

## Licença

Este projeto é oferecido sob licença livre (ex.: [MIT License](https://opensource.org/licenses/MIT)) – sinta-se à vontade para usar, modificar e distribuir. Ajuste conforme a sua necessidade ou política do seu repositório/projeto.
