#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004 //SDK antigo do MinGW nao declara essa flag.
#endif

typedef struct{
    uint32_t largura_janela;
    uint32_t altura_janela;
    uint32_t fg_color;
    uint32_t bg_color;
    uint32_t scale_factor;
    uint32_t instructions_per_second; //Velocidade de clock do emulador (instruções executadas por segundo)
}config;

typedef enum{
    QUIT,
    RUNNING,
    PAUSED,
}estados_emulador_t;

typedef struct{
    //Struct que representa os componentes fisicos do chip8, estamos "emulando" esses componentes.
    estados_emulador_t estados;
    uint8_t memory [4096]; //4kb ou 4096 bytes
    bool display[64*32]; //Como o display do chip8 tem somente 64 x 32, criamos um array com todas as opções na tela
    bool draw; //Indica se a tela precisa ser redesenhada nesse frame
    uint16_t pc; //Aponta para a instrução atual na memoria
    uint16_t I; // Usada para apontar para locais na memoria
    uint16_t stack[12]; //which is used to call subroutines/functions and return from them (peguei a definição do guia)
    uint8_t sp; //Usado para apontar para o topo da pilha.
    uint8_t delay_timer; // which is decremented at a rate of 60 Hz (60 times per second) until it reaches 0 ((peguei a definição do guia))
    uint8_t delay_sound;
    uint8_t V[16]; //Registradores de dados V0-VF
    bool keypad[16];
    char *rom_name;
}chip8;

//Mapeamento do teclado fisico (QWERTY) para o teclado hexadecimal do chip8:
// 1 2 3 C        1 2 3 4
// 4 5 6 D   ->   Q W E R
// 7 8 9 E        A S D F
// A 0 B F        Z X C V
static const int chip8_keymap[16] = {
    'X', // 0
    '1', // 1
    '2', // 2
    '3', // 3
    'Q', // 4
    'W', // 5
    'E', // 6
    'A', // 7
    'S', // 8
    'D', // 9
    'Z', // A
    'C', // B
    '4', // C
    'R', // D
    'F', // E
    'V', // F
};

bool init_chip8(chip8 *chip8, char *rom_name){
        uint8_t fonts[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F

        //Essas fontes foram pré-disponibilizadas no guia de implementação, é possível de alteralas, mas no momento eu vou fazer o feijão com arroz.
    };

    memcpy(&chip8->memory[0],fonts,sizeof(fonts)); //Carregando as fontes para a memoria.
    chip8->rom_name = rom_name;

    FILE *rom = fopen(chip8->rom_name, "rb");
    if(!rom){
        printf("Houve um erro ao tentar abrir o arquivo da ROM, ou ele não existe");
        return false;
    }
    fseek(rom,0,SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t rom_max_size = sizeof(chip8->memory) - 0x200;
    rewind(rom);

    if(rom_size > rom_max_size){
        printf("O arquivo da rom %s e muito grande \nTamanho da ROM: %lu \nTamanho maximo permitido: %lu\n",
        chip8->rom_name, (unsigned long)rom_size, (unsigned long)rom_max_size);
        fclose(rom);
        return false;
    }

    if(fread(&chip8->memory[0x200], rom_size, 1, rom) != 1){
        printf("Não foi possível ler o arquivo da rom %s\nTamanho da ROM: %lu \nTamanho maximo permitido: %lu\n",
        chip8->rom_name, (unsigned long)rom_size, (unsigned long)rom_max_size);
        fclose(rom);
        return false;
    }
    fclose(rom);

    chip8->estados = RUNNING;
    chip8->pc = 0x200; //Iniciando o programing counter no inicio da rom, no local 0x200 (512).

    return true;
}

void init_config(config *config){
    config->largura_janela = 64;
    config->altura_janela = 32;
    config->fg_color = 0xFFFFFFFF;
    config->bg_color = 0x000000FF;
    config->scale_factor = 20;
    config->instructions_per_second = 700; //Valor tipico usado pela maioria dos emuladores de chip8
}

//Habilita o processamento de sequencias ANSI no console do Windows, para permitir reposicionar o cursor sem "piscar" a tela.
void enable_ansi_console(void){
    HANDLE console_saida = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo_console = 0;
    GetConsoleMode(console_saida, &modo_console);
    modo_console |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(console_saida, modo_console);
}

void handle_input(chip8 *chip8){
    if(GetAsyncKeyState(VK_ESCAPE) & 0x8000){
        chip8->estados = QUIT;
        return;
    }

    if(GetAsyncKeyState('P') & 0x8000){
        chip8->estados = (chip8->estados == PAUSED) ? RUNNING : PAUSED;
    }

    for(int i = 0; i < 16; i++){
        chip8->keypad[i] = (GetAsyncKeyState(chip8_keymap[i]) & 0x8000) != 0;
    }
}

//Desenha o display 64x32 do chip8 no terminal, usando dois caracteres por pixel para compensar a proporção das celulas do console.
void render_screen(chip8 *chip8){
    printf("\x1b[H");
    for(uint32_t y = 0; y < 32; y++){
        for(uint32_t x = 0; x < 64; x++){
            fputs(chip8->display[y * 64 + x] ? "##" : "  ", stdout);
        }
        fputc('\n', stdout);
    }
    fflush(stdout);
}

void update_timers(chip8 *chip8){
    if(chip8->delay_timer > 0) chip8->delay_timer--;

    if(chip8->delay_sound > 0){
        chip8->delay_sound--;
        if(chip8->delay_sound == 0) putchar('\a'); //Beep simples no terminal, ainda sem audio de verdade.
    }
}

void emulate_instruction(chip8 *chip8){
    const uint16_t opcode = (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
    chip8->pc += 2;

    const uint16_t NNN = opcode & 0x0FFF;
    const uint8_t NN = opcode & 0x00FF;
    const uint8_t N = opcode & 0x000F;
    const uint8_t X = (opcode >> 8) & 0x000F;
    const uint8_t Y = (opcode >> 4) & 0x000F;

    switch(opcode >> 12){
        case 0x0:
            if(opcode == 0x00E0){ //CLS: Limpa a tela
                memset(chip8->display, false, sizeof(chip8->display));
                chip8->draw = true;
            } else if(opcode == 0x00EE){ //RET: Retorna de uma sub-rotina
                chip8->sp--;
                chip8->pc = chip8->stack[chip8->sp];
            }
            break;

        case 0x1: //JP NNN: Pula para o endereço NNN
            chip8->pc = NNN;
            break;

        case 0x2: //CALL NNN: Chama a sub-rotina em NNN
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;
            chip8->pc = NNN;
            break;

        case 0x3: //SE VX, NN: Pula a proxima instrução se VX == NN
            if(chip8->V[X] == NN) chip8->pc += 2;
            break;

        case 0x4: //SNE VX, NN: Pula a proxima instrução se VX != NN
            if(chip8->V[X] != NN) chip8->pc += 2;
            break;

        case 0x5: //SE VX, VY: Pula a proxima instrução se VX == VY
            if(chip8->V[X] == chip8->V[Y]) chip8->pc += 2;
            break;

        case 0x6: //LD VX, NN: VX = NN
            chip8->V[X] = NN;
            break;

        case 0x7: //ADD VX, NN: VX += NN (sem afetar VF)
            chip8->V[X] += NN;
            break;

        case 0x8:
            switch(N){
                case 0x0: chip8->V[X] = chip8->V[Y]; break;
                case 0x1: chip8->V[X] |= chip8->V[Y]; break;
                case 0x2: chip8->V[X] &= chip8->V[Y]; break;
                case 0x3: chip8->V[X] ^= chip8->V[Y]; break;
                case 0x4: { //ADD com carry
                    const uint16_t soma = chip8->V[X] + chip8->V[Y];
                    chip8->V[0xF] = (soma > 0xFF);
                    chip8->V[X] = soma & 0xFF;
                    break;
                }
                case 0x5: { //SUB VX - VY
                    const uint8_t sem_borrow = chip8->V[X] >= chip8->V[Y];
                    chip8->V[X] -= chip8->V[Y];
                    chip8->V[0xF] = sem_borrow;
                    break;
                }
                case 0x6: { //SHR: desloca VY para a direita e guarda em VX (comportamento original)
                    const uint8_t bit_perdido = chip8->V[Y] & 0x1;
                    chip8->V[X] = chip8->V[Y] >> 1;
                    chip8->V[0xF] = bit_perdido;
                    break;
                }
                case 0x7: { //SUBN VY - VX
                    const uint8_t sem_borrow = chip8->V[Y] >= chip8->V[X];
                    chip8->V[X] = chip8->V[Y] - chip8->V[X];
                    chip8->V[0xF] = sem_borrow;
                    break;
                }
                case 0xE: { //SHL: desloca VY para a esquerda e guarda em VX (comportamento original)
                    const uint8_t bit_perdido = (chip8->V[Y] & 0x80) >> 7;
                    chip8->V[X] = chip8->V[Y] << 1;
                    chip8->V[0xF] = bit_perdido;
                    break;
                }
            }
            break;

        case 0x9: //SNE VX, VY: Pula a proxima instrução se VX != VY
            if(chip8->V[X] != chip8->V[Y]) chip8->pc += 2;
            break;

        case 0xA: //LD I, NNN
            chip8->I = NNN;
            break;

        case 0xB: //JP V0, NNN
            chip8->pc = NNN + chip8->V[0];
            break;

        case 0xC: //RND VX, NN
            chip8->V[X] = (rand() % 256) & NN;
            break;

        case 0xD: { //DRW VX, VY, N: Desenha um sprite de N linhas na posição (VX, VY)
            const uint8_t x_pos = chip8->V[X] % 64;
            const uint8_t y_pos = chip8->V[Y] % 32;
            chip8->V[0xF] = 0;

            for(uint8_t linha = 0; linha < N; linha++){
                if(y_pos + linha >= 32) break; //Nao ha wrap vertical, o sprite e cortado.
                const uint8_t sprite_byte = chip8->memory[chip8->I + linha];

                for(uint8_t coluna = 0; coluna < 8; coluna++){
                    if(x_pos + coluna >= 64) break; //Nao ha wrap horizontal, o sprite e cortado.
                    const bool sprite_bit = (sprite_byte & (0x80 >> coluna)) != 0;
                    bool *pixel_tela = &chip8->display[(y_pos + linha) * 64 + (x_pos + coluna)];

                    if(sprite_bit){
                        if(*pixel_tela) chip8->V[0xF] = 1; //Colisao: um pixel ligado foi apagado.
                        *pixel_tela = !(*pixel_tela);
                    }
                }
            }
            chip8->draw = true;
            break;
        }

        case 0xE:
            if(NN == 0x9E){ //SKP VX: Pula se a tecla VX estiver pressionada
                if(chip8->keypad[chip8->V[X]]) chip8->pc += 2;
            } else if(NN == 0xA1){ //SKNP VX: Pula se a tecla VX NAO estiver pressionada
                if(!chip8->keypad[chip8->V[X]]) chip8->pc += 2;
            }
            break;

        case 0xF:
            switch(NN){
                case 0x07: chip8->V[X] = chip8->delay_timer; break;
                case 0x0A: { //LD VX, K: Espera uma tecla ser pressionada (bloqueante)
                    bool tecla_pressionada = false;
                    for(uint8_t i = 0; i < 16; i++){
                        if(chip8->keypad[i]){
                            chip8->V[X] = i;
                            tecla_pressionada = true;
                            break;
                        }
                    }
                    if(!tecla_pressionada) chip8->pc -= 2; //Repete essa mesma instrução no proximo ciclo.
                    break;
                }
                case 0x15: chip8->delay_timer = chip8->V[X]; break;
                case 0x18: chip8->delay_sound = chip8->V[X]; break;
                case 0x1E: chip8->I += chip8->V[X]; break;
                case 0x29: chip8->I = chip8->V[X] * 5; break; //Cada caractere da fonte ocupa 5 bytes.
                case 0x33: { //Armazena o BCD de VX em I, I+1, I+2
                    const uint8_t valor = chip8->V[X];
                    chip8->memory[chip8->I] = valor / 100;
                    chip8->memory[chip8->I + 1] = (valor / 10) % 10;
                    chip8->memory[chip8->I + 2] = valor % 10;
                    break;
                }
                case 0x55: //Salva V0..VX na memoria a partir de I (comportamento original: I avança junto)
                    for(uint8_t i = 0; i <= X; i++) chip8->memory[chip8->I + i] = chip8->V[i];
                    chip8->I += X + 1;
                    break;
                case 0x65: //Carrega V0..VX da memoria a partir de I (comportamento original: I avança junto)
                    for(uint8_t i = 0; i <= X; i++) chip8->V[i] = chip8->memory[chip8->I + i];
                    chip8->I += X + 1;
                    break;
            }
            break;

        default:
            fprintf(stderr, "Opcode desconhecido: 0x%04X\n", opcode);
            break;
    }
}

int main(int argc, char **argv){
    if(argc < 2){
        printf("Uso: %s <rom>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    chip8 chip8 = {0};
    config config = {0};
    init_config(&config);

    if(!init_chip8(&chip8,argv[1])) exit(EXIT_FAILURE);

    srand((unsigned int)time(NULL));
    enable_ansi_console();
    printf("\x1b[2J"); //Limpa a tela uma unica vez, os proximos frames so reposicionam o cursor.
    printf("Pressione ESC para sair, P para pausar.\n");

    const double fps_alvo = 60.0;
    const uint32_t instrucoes_por_frame = config.instructions_per_second / (uint32_t)fps_alvo;

    while(chip8.estados != QUIT){
        handle_input(&chip8);

        if(chip8.estados == PAUSED){
            Sleep(100);
            continue;
        }

        for(uint32_t i = 0; i < instrucoes_por_frame; i++){
            emulate_instruction(&chip8);
        }

        update_timers(&chip8);

        if(chip8.draw){
            render_screen(&chip8);
            chip8.draw = false;
        }

        Sleep((DWORD)(1000.0 / fps_alvo));
    }

    return 0;
}
