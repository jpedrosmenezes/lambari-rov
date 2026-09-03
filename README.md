# Lambari-ROV

![Microcontroller](https://img.shields.io/badge/Microcontrolador-ESP32-blue)
![Status](https://img.shields.io/badge/status-em%20desenvolvimento-orange)
![PRs Welcome](https://img.shields.io/badge/PRs-bem%20vindas-brightgreen)

**Um ROV subaquático de baixo custo que você consegue montar sozinho.**

Lambari é um ROV (Remotely Operated Vehicle) open-source, educacional, pensado pra ser barato, replicável e fácil de entender — sem abrir mão de decisões de engenharia bem justificadas. A ideia central é simples: toda a eletrônica cara e sensível fica seca, na superfície, dentro de uma caixa estanque (a *dry box*). Só energia bruta desce pelo cabo até os motores, debaixo d'água. Isso elimina o maior gargalo de custo e risco de um ROV caseiro — vedar componentes eletrônicos sob pressão — sem sacrificar um sistema funcional de verdade.

Além de ser um projeto de robótica em si, o Lambari é pensado como ferramenta de ensino: montar um ensina programação embarcada e eletrônica básica na prática, e o próprio ROV serve de gancho pra falar sobre poluição aquática e seus efeitos na fauna de rios, lagos e mares — é um jeito concreto de aproximar quem tá aprendendo tanto da tecnologia quanto do ambiente que ela vai explorar.

Pra montar um, você vai precisar de noções básicas de solda e fiação, algo em torno de R$1.000-1.800 em componentes (veja o [BOM completo](docs/BOM.md)), um pouco de tubo PVC pra montar o casco, e paciência pra testar tudo numa bacia antes de ir pra água aberta.

---

## O que ele faz

- **Arquitetura dry-box** — toda a inteligência (ESP32, drivers de motor, fonte) fica seca na superfície. Debaixo d'água só tem motor e câmera.
- **Propulsão com 3 motores** — bombas de porão 12V acionadas por pontes H BTS7960, com controle de velocidade e reversão via PWM.
- **Controle Bluetooth** — ESP32 rodando Bluepad32, pareia direto com um controle de videogame sem fio. Sem app, sem PC, sem Raspberry Pi.
- **Câmera de bordo** — uma FPV analógica (CADDX Ant) manda vídeo por um cabo dedicado até um display RCA na superfície, com latência zero.
- **Margens de segurança calculadas, não chutadas** — dimensionamento de fusível, queda de tensão no umbilical e um watchdog que zera os motores se o sinal Bluetooth cair.
- **Pensado pra sala de aula** — cada decisão de projeto é documentada com o raciocínio por trás, então o Lambari serve tanto de robô quanto de material didático de eletrônica e programação embarcada.

---

## Como montar

### 1. Reúna as peças
Confira o [BOM completo](docs/BOM.md) com fornecedores e preços estimados. A lista principal: ESP32, 3x bomba de porão CH8028 (1100 GPH), 3x BTS7960, buck converter LM2596, fonte chaveada 12V, fusível, câmera CADDX Ant e display RCA.

### 2. Monte a dry box
ESP32, as 3 pontes H, o buck converter e o fusível ficam todos na mesma caixa estanque na superfície. Guia de fiação detalhado em [`docs/wiring-guide.md`](docs/wiring-guide.md) *(em construção)*.

### 3. Monte o umbilical
Cabo PP 6 vias (6x1,0mm², ~10m) leva energia bruta até os 3 motores. A câmera usa um cabo manga separado de 3 vias — o PP já está totalmente ocupado pelos motores, então não sobra via pra vídeo.

### 4. Vede tudo
Termorretrátil com adesivo de parede dupla nas emendas dos motores, reforçado com epóxi no ponto de tração. Teste a estanqueidade numa bacia antes de qualquer coisa ir pra água de verdade.

### 5. Grave o firmware
Firmware do ESP32 em [`firmware/`](firmware/) *(em construção)*, baseado em [Bluepad32](https://github.com/ricardoquesada/bluepad32).

### 6. Teste antes de mergulhar
Bacia primeiro, lago depois. Confirme vedação, resposta dos motores e o watchdog de Bluetooth antes de qualquer teste em água aberta.

---

## Como funciona

**Alimentação.** Uma fonte chaveada 12V/15A na dry box passa por um fusível de 15A antes de se distribuir para as 3 pontes H, o buck converter (que alimenta o ESP32) e a câmera. O fusível é dimensionado pro consumo nominal dos 3 motores (~10A) com margem sobre os picos de partida — se algo travar, ele abre de propósito, não é bug.

```
Fonte 12V 15A → Fusível 15A → Bloco de bornes
                                   ├─ BTS7960 x3 → Motores
                                   ├─ Buck 12V→5V → ESP32
                                   └─ Câmera CADDX Ant → Display RCA
```

**Controle.** O ESP32 lê o controle de videogame via Bluepad32 e converte os inputs analógicos em sinais PWM pras 3 pontes H, controlando velocidade e direção de cada motor. Se o Bluetooth cair, um watchdog zera tudo automaticamente — o ROV nunca fica executando o último comando às cegas.

**Vídeo.** O sinal da câmera sobe por um cabo próprio, direto pro display RCA na dry box. É um caminho independente do controle dos motores — não passa pelo ESP32 nem compete por largura de banda com o resto do sistema.

---

## Por que as coisas são do jeito que são

**Por que dry box em vez de eletrônica submersa?** Selar componentes contra água e pressão é a parte mais cara e arriscada de construir um ROV. Tirando isso da equação — só mandando energia bruta pelo cabo — o projeto fica muito mais barato e muito mais fácil de replicar.
 
**Por que BTS7960?** BTS7960 aguenta até 43A, então opera com folga real em vez de no limite. Um L298N, por exemplo, satura em ~2A contínuos, e nossas bombas puxam 2,5-5A em uso normal. Não é que ele "queima na hora" — ele superaquece, entra em proteção térmica, desliga, esfria, tenta de novo, num ciclo instável.
 
**Por que fonte chaveada em vez de bateria?** Uma bateria selada equivalente custa 2-3x mais e ainda te dá só ~20-30 minutos de autonomia real. Como o projeto sempre testa com acesso a tomada por perto, a fonte é a escolha mais barata sem trade-off relevante.
 
**Por que 1,0mm² e não 1,5mm² no umbilical?** Calculando a queda de tensão pros 10 metros do cabo, 1,0mm² já fica dentro de uma margem segura (~10% de perda) — e custa menos.
 
**Por que a câmera precisa de cabo próprio?** O umbilical PP de 6 vias já está 100% ocupado pelos 3 motores (2 fios cada, pra reversão). Não tem via sobrando pra vídeo, então ele viaja por um cabo manga separado. Optar por um cabo PP com mais vias internas prejudicaria a flexibilidade do cabo.

---

## Contribuindo

Encontrou um erro de cálculo, uma peça mais barata, ou quer ajudar com o firmware? Abra uma [issue](../../issues) ou manda um pull request. O projeto é feito pra comunidade maker — toda contribuição que deixe ele mais acessível é bem-vinda.