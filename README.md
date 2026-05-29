# Sistema de Iluminação Pública Inteligente para Cidades Sustentáveis

> **Objetos Inteligentes Conectados | Universidade Presbiteriana Mackenzie**  
> **Autor:** Gustavo Gonçalves dos Santos Neves  
> **Alinhamento:** ODS 11 da ONU — Cidades e Comunidades Sustentáveis

---

## 📋 Descrição do Projeto

Sistema de controle de iluminação pública que utiliza um **sensor de luminosidade LDR** para acionar automaticamente um **relé** (lâmpada pública) com base nas condições ambientais. O sistema permite também **controle remoto via internet** através do protocolo **MQTT**, usando o broker público HiveMQ.

O protótipo foi integralmente desenvolvido e validado no simulador **Wokwi**, eliminando custos de hardware e viabilizando testes com conectividade real com a internet.

### Funcionalidades

- ✅ Controle automático da lâmpada com base na luminosidade (LDR)
- ✅ Publicação de dados do sensor a cada 5 segundos via MQTT
- ✅ Controle remoto manual (LIGAR / DESLIGAR / AUTO) via MQTT
- ✅ Reconexão automática ao broker em caso de queda de conexão
- ✅ Medição e log de latência de sensor e atuador no monitor serial

---

## 🔧 Hardware Utilizado

| Componente | Especificação | Função |
|---|---|---|
| ESP32 DevKit V1 | Dual-core 240 MHz, 520 KB SRAM, Wi-Fi 802.11 b/g/n | Microcontrolador principal |
| Sensor LDR 5mm | Fotorresistor, resistência varia com luz | Sensor de luminosidade |
| Resistor 10 kΩ | Tolerância 5% | Divisor de tensão (LDR) |
| Módulo Relé 5V | 1 canal, isolamento galvânico | Controle da lâmpada |
| LED Amarelo 5mm | — | Representação da lâmpada pública |
| Resistor 220 Ω | Tolerância 5% | Limitação de corrente do LED |

### Pinagem

| Pino ESP32 | Componente | Função |
|---|---|---|
| GPIO 34 (ADC1) | LDR (saída AO) | Leitura analógica da luminosidade |
| GPIO 23 | Relé (IN) | Controle do atuador |
| 3V3 | LDR (VCC) | Alimentação do sensor |
| 5V (VIN) | Relé (VCC) | Alimentação do módulo relé |
| GND | LDR, Relé, LED | Referência de terra |

> **Observação:** O GPIO 34 pertence ao grupo ADC1 do ESP32, que permanece funcional mesmo com o Wi-Fi ativo. Os pinos ADC2 são desabilitados quando o Wi-Fi está em uso.

---

## 📡 Interfaces e Protocolos de Comunicação

### Protocolo MQTT

| Parâmetro | Valor |
|---|---|
| Protocolo | MQTT v3.1.1 |
| Broker | HiveMQ Cloud Público |
| Endereço | `broker.hivemq.com` |
| Porta | `1883` (TCP) |
| Client ID | `esp32_iluminacao_pub_ggsn` |
| QoS | 0 (At most once) |

### Tópicos

| Tópico | Direção | Descrição |
|---|---|---|
| `cidade/iluminacao/sensor` | Publicação (5s) | Estado atual do sistema em JSON |
| `cidade/iluminacao/comando` | Assinatura | Comandos remotos de controle |

### Formato do Payload (Publicação)

```json
{
  "luminosidade_adc": 1001,
  "percentual_luz": 24,
  "lampada": "LIGADA",
  "modo": "AUTOMATICO",
  "uptime_ms": 5148
}
```

### Comandos suportados (Assinatura)

| Comando | Efeito |
|---|---|
| `LIGAR` | Aciona o relé e ativa modo manual |
| `DESLIGAR` | Desativa o relé e ativa modo manual |
| `AUTO` | Retorna ao controle automático pelo LDR |

---

## 💻 Software — Documentação do Código

### Estrutura do Firmware (`sketch.ino`)

```
sketch.ino
├── Configurações (WiFi, MQTT, pinos, limiar)
├── setup()
│   ├── Inicializa GPIO e Serial
│   ├── conectarWifi()
│   └── Configura cliente MQTT + callback
└── loop()
    ├── Verifica/reconecta MQTT
    ├── A cada 5s:
    │   ├── analogRead(GPIO 34) → ldrValor
    │   ├── Controle automático (se !modoManual)
    │   └── publicarDados() → MQTT publish
    └── mqttClient.loop() → processa callbacks
```

### Funções principais

| Função | Descrição |
|---|---|
| `conectarWifi()` | Conecta ao Wi-Fi Wokwi-GUEST com retry |
| `conectarMQTT()` | Conecta ao broker e subscreve ao tópico de comandos |
| `callbackMQTT()` | Processa mensagens recebidas (LIGAR/DESLIGAR/AUTO) e mede latência |
| `controlarRele(bool)` | Aciona ou desativa o GPIO 23 e atualiza o estado |
| `publicarDados(int)` | Monta JSON e publica no tópico sensor via MQTT |

### Bibliotecas

| Biblioteca | Versão | Fonte |
|---|---|---|
| `WiFi.h` | Nativa ESP32 | Pacote ESP32 para Arduino |
| `PubSubClient.h` | ≥ 2.8 | Nick O'Leary / Arduino Library Manager |

---

## 🚀 Como Reproduzir

### Pré-requisitos

- Conta gratuita em [wokwi.com](https://wokwi.com)
- Acesso ao [HiveMQ WebClient](https://www.hivemq.com/demos/websocket-client/) para monitorar

### Passo a passo

1. **Criar projeto no Wokwi**
   - Acesse [wokwi.com](https://wokwi.com) → **New Project** → **ESP32**

2. **Substituir o circuito**
   - Abra o arquivo `diagram.json` no editor do Wokwi
   - Apague o conteúdo e cole o arquivo `diagram.json` deste repositório

3. **Substituir o código**
   - Abra o arquivo `sketch.ino` no editor do Wokwi
   - Apague o conteúdo e cole o arquivo `sketch.ino` deste repositório

4. **Adicionar a biblioteca MQTT**
   - Crie um novo arquivo chamado `libraries.txt`
   - Adicione o conteúdo: `PubSubClient`

5. **Executar**
   - Clique em **▶ Play**
   - Acompanhe o Monitor Serial — a conexão Wi-Fi e MQTT serão estabelecidas automaticamente

6. **Monitorar via MQTT**
   - Acesse o [HiveMQ WebClient](https://www.hivemq.com/demos/websocket-client/)
   - Clique em **Connect**
   - Adicione subscription: `cidade/iluminacao/sensor`
   - Para enviar comandos, publique `LIGAR`, `DESLIGAR` ou `AUTO` em `cidade/iluminacao/comando`

---

## 📁 Estrutura do Repositório

```
iluminacao-publica-iot/
├── sketch.ino          # Firmware ESP32 (código principal)
├── diagram.json        # Circuito Wokwi (importar no simulador)
├── libraries.txt       # Dependências de bibliotecas (Wokwi)
└── README.md           # Este arquivo
```

---

## 📊 Resultados de Latência

Medições realizadas no ambiente Wokwi com broker HiveMQ público:

| Dispositivo | Operação | Latência Média |
|---|---|---|
| LDR (Sensor) | Leitura ADC (analogRead) | < 1 ms |
| Relé (Atuador) | Recepção cmd MQTT → GPIO | ~2–3 ms |
| MQTT Publish | Montagem JSON + publish | ~2–3 ms |

> Em uma instalação física com Wi-Fi real e broker remoto, a latência de publicação e recepção MQTT tende a ser de 50–200 ms, dependendo da qualidade da rede.

---

## 🔗 Links

- 🎬 **Vídeo-demonstração:** [YouTube](https://youtu.be/0lvL_WsK8Uw)

---

## 📚 Referências

- ARDUINO. Project Hub. 2024. Disponível em: https://www.hackster.io/arduino/projects. Acesso em: 09 mar. 2026.
- AYAZ, M. et al. Internet-of-Things (IoT)-Based Smart Agriculture: toward making the fields talk. IEEE Access, v. 7, 2019. Acesso em: 24 mar. 2026.
- BOULIC, R.; RENAULT, O. 3D Hierarchies for Animation. In: MAGNENAT-THALMANN, N.; THALMANN, D. (Eds.). New Trends in Animation and Visualization. [s.l.] John Wiley & Sons ltd., 1991. Acesso em: 24 mar. 2026.
- ESPRESSIF SYSTEMS. ESP32 Series Datasheet. v. 4.1, 2024. Acesso em: 25 mar. 2026.
- HEATH, S. Embedded Systems Design. Disponível em: https://www.google.com/search?q=https://fdocuments.in/document/embedded-systems-design.html. Acesso em: 24 mar. 2026.
- HIVEMQ. MQTT Essentials. 2026. Disponível em: https://www.hivemq.com/mqtt-essentials/. Acesso em: 25 mar. 2026.
- IBM. MQTT: um protocolo de transporte de mensagens leve. 2021. Disponível em: https://www.google.com/search?q=https://www.ibm.com/developerworks/br/library/iot-mqtt-why-good-for-iot/index.html. Acesso em: 24 mar. 2026.
- KNUTH, D. E. The TeX Book. 15th. ed. [s.l.] Addison-Wesley, 1984. Acesso em: 24 mar. 2026.
- SANTOS, B. et al. Internet das Coisas: da teoria à prática. Disponível em: https://homepages.dcc.ufmg.br/~mmvieira/cc/papers/internet-das-coisas.pdf. Acesso em: 24 mar. 2026.
- STANFORD-CLARK, A.; LIGHT, R. A. MQTT Version 3.1.1. OASIS Standard. 2014. Disponível em: https://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html. Acesso em: 23 mai. 2026.
- WOKWI. Docs: Welcome to Wokwi!. 2026. Disponível em: https://docs.wokwi.com/. Acesso em: 24 mar. 2026.

---

*Universidade Presbiteriana Mackenzie — Faculdade de Computação e Informática*