<h1>Objetivo do projeto</h1>
<p>O principal objetivo desse projeto é o de entrar no mundo da emulação, onde eu começo desenvolvendo meu primeiro emulador, o CHIP8.</p>

<h1>Sobre o CHIP8</h1>
<p>Sobre o CHIP8, O CHIP-8 foi mais comumente implementado em sistemas de 4K, como o Cosmac VIP e o Telmac 1800. Essas máquinas possuíam 4096 (0x1000) posições de memória, todas de 8 bits (um byte), de onde se originou o termo CHIP-8. No entanto, o próprio interpretador do CHIP-8 ocupa os primeiros 512 bytes do espaço de memória nessas máquinas. Por esse motivo, a maioria dos programas escritos para o sistema original começa na posição de memória 512 (0x200) e não acessa nenhuma parte da memória abaixo da posição 512 (0x200). 
Os 256 bytes mais altos (0xF00-0xFFF) são reservados para a 
atualização da tela, e os 96 bytes abaixo deles (0xEA0-0xEFF) eram reservados para a pilha de chamadas, uso interno e outras variáveis.</p>
