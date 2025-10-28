# TCC de Engenharia de Computação - Copiador de controles remotos infravermelhos com interface WEB

O artigo final do TCC encontra-se em [artigo.pdf]("artigo.pdf")
O artigo enuncia os requisitos de hardware, bem como a circuitaria.

## Requisitos de software

Foram utilizados os seguintes softares:

- Arduino IDE v2.3.6
- Driver para comunicação serial com chip CH340
- Sistema de placas ESP8266 (instalável diretamente pela Arduino IDE) v3.1.2
- Bibliotecas instaláveis diretamente pela Arduino IDE
    - Arduinojson v7.4.2
    - IRemoteESP8266 v2.8.6
    - Outras bibliotecas presentes no código são nativas da Arduino IDE ou vêm juntamente com o sistema de placas ESP8266

## Envio do código para a placa

Basta selecionar a porta onde a placa está conectada ao computador (ex: COM3 no windows) e configurá-la como `Generic ESP8266 Module`. A compilação e o upload devem funcionar sem nenhuma configuração adicional.


