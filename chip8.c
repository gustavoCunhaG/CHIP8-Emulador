#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum{
    QUIT,
    RUNNING,
    PAUSED, 
}estados_emulador_t;

typedef struct chip8{
    //Struct que representa os componentes fisicos do chip8, estamos "emulando" esses componentes.
    estados_emulador_t estados;
    uint8_t memory [4096]; //4kb ou 4096 bytes
    bool display[64*32]; //Como o display do chip8 tem somente 64 x 32, criamos um array com todas as opções na tela
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


bool init_chip8(chip8 *chip8){
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
    FILE *rom = fopen(chip8->rom_name, "rb");
    if(!rom){
        printf("Houve um erro ao tentar abrir o arquivo da ROM, ou ele não existe");
        return false;
    }
    fseek(rom,0,SEEK_END);
    const size_t rom_size = ftell(rom);
    const size_t rom_max_size = sizeof(chip8->memory - 0x200);
    rewind(rom);
    
    if(rom_size > rom_max_size){
        printf("O arquivo da rom %s e muito grande \n, Tamanho da ROM: %zu \n, Tamanho maximo permitido: %zu",
        chip8->rom_name, rom_size, rom_max_size);
    }

    if(fread(&chip8->memory[0x200], rom_size, 1, rom) != 1){
        printf("Não foi possível ler o arquivo da rom %s\n, Tamanho da ROM: %zu \n, Tamanho maximo permitido: %zu",
        chip8->rom_name, rom_size, rom_max_size);
    }
    fclose(rom); 
    
    chip8->estados = RUNNING;
    chip8->pc = 0x200; //Iniciando o programing counter no inicio da rom, no local 0x200 (512).

    return true;
}

int main(){
    
    return 0;
}