# Agbaixo
Instrumento Agbaixo (também chamado de Agbau ou Agbaça), em processo de criação pelo BongarBit

[![Watch the video](https://youtu.be/48ikO1KuL8A)](https://youtu.be/48ikO1KuL8A)

# Para mandar o código para o Arduino:

Baixe e instale o Arduino na versão legado (1.8.x) em https://www.arduino.cc/en/software

Abra o programa Arduino (Arduino IDE)

Instale as bibliotecas colocando no diretório onde o Arduino for instalado, dentro da pasta libraries, todas as pastas que estão em Arduino/libraries deste repositório.

Na IDE do Arduino, vá nas configurações (File > Preferences) e adicione o texto na caixa de texto "Additional Board Manager URLs" o seguinte link: https://raw.githubusercontent.com/sparkfun/Arduino_Boards/main/IDE_Board_Manager/package_sparkfun_index.json

Vá em Tools>Board>Boards Manager e faça uma busca por Sparkfun AVR Boards e clique no botão "Install"

Vá em Tools>Board> e selecione a placa Arduino Pro Micro

Vá em Tools>Processor: e selecione o "ATmega 32u4 (5v, 16MHz)"

Identifique quais opções existem em Tools>Port>

Conecte o arduino no USB do computador

Note qual foi a opção nova que apareceu no submenu Tools>Port> e selecione ela

Abra o arquivo Arduino/agbaixo/agbaixo.ino e clique na barra de ferramentas da IDE do Arduino no botão "Upload" (o segundo da esquerda pra direita)

Se tudo der certo o código será enviado para a memória do Arduino.





