# Fundamentos de Sistemas Embarcados (FSE)
O objetivo deste trabalho é criar sensores e atuadores distribuídos baseados nos microcontroladores ESP32 interconectados via Wifi através do protocolo MQTT, podendo ser aplicada em diversos contextos de automação a exemplo das áreas de Automação Residencial, Predial ou Industrial.
Os microcontroladores ESP32 irão controlar a aquisição de dados de sensores, botões e chaves e acionar saídas como leds, dentre outras. Haverão dois modos de operação dos dispositivos: modo energia que representa o dispositivo ligado à tomada e modo bateria que representa o dispositivo que deverá operar em modo de baixo consumo.

Toda a comunicação será feita via rede Wifi com o protocolo MQTT e será provido um servidor central para cadastro e controle dos dispositivos através da plataforma Thingsboard. Veja mais [Trabalho Final - 2022/2](https://gitlab.com/fse_fga/trabalhos-2022_2/trabalho-final-2022-2).
</br>

## Alunos

| Nome | Matrícula  |
| :- | :- |
| Antonio Ruan Moura Barreto | 180030272 |
| Thiago Luiz de Souza Gomes | 180028324 |
| Victor Samuel dos Santos Lucas | 180028685 |


## Sensores 
- DHT11: Leitura de Temperatura e Humidade
- LED RGB: Output de cores
- SOM: Detector de Som
- PRESENÇA: Detector de Presença
- LED BICOLOR: Output de cor
- JOYSTIC: Controle
- Buzzer: Alarme

## O que foi implementado
- Leitura e envio de Temperatura e Humidade para a Dashboard (Telimetria e Atributo);
- Led de Acionamento do Sensor DHT11 e de Intervalo de Risco na Dashboard a partir da leitura dos valores de Temperatura e Humidade;
- Acionar e Desativar o Sensor DHT11 e LED RGB pela Dashboard;
- Acionamento de Leds a partir da Deteção de Som e Presença;
- Acionamento do Buzzer a partir da botão do Joystic;
- Clique longo no Joystic para ativar os Leds;
- Manipulação dos Led pelo Joystic;
- Aumento e diminuição do DUTY do Buzzer pelo Joystic;


## Screenshots da Dashboard!
<img width="1440" alt="Captura de Tela 2023-02-14 às 16 30 19" src="https://user-images.githubusercontent.com/52058094/218845298-4032117c-54a4-4eec-b951-e8996fa2ca93.png">

https://user-images.githubusercontent.com/52058094/218844855-8f2fb398-c82d-445b-927e-bec79342cf00.mov


## Apresentação
[Link do vídeo de apresentação]()
