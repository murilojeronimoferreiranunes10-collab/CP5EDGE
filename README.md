# CP5 — Edge Computing & IoT com FIWARE

> Checkpoint 5 — Monitoramento Ambiental Inteligente com ESP32, MQTT e Plataforma FIWARE

---

## 📌 Visão Geral

Este projeto consiste no desenvolvimento de um sistema de **monitoramento ambiental em tempo real** utilizando conceitos de **Edge Computing** e **IoT (Internet of Things)**. O dispositivo de borda — um microcontrolador **ESP32** — coleta dados ambientais de temperatura, umidade e luminosidade, e os transmite via protocolo **MQTT** para a plataforma **FIWARE**, onde os dados são armazenados, contextualizados e disponibilizados para visualização em dashboard.

O sistema também permite o **controle remoto de atuadores** (LED e Buzzer), demonstrando comunicação bidirecional entre borda e nuvem.

---

## 🎯 Objetivos

- Implementar um nó de **edge computing** com ESP32 capaz de coletar e transmitir dados ambientais.
- Utilizar o protocolo **MQTT** como camada de transporte leve e eficiente para IoT.
- Integrar a solução à plataforma **FIWARE** para gerenciamento de contexto via **NGSIv2**.
- Armazenar séries históricas de dados com **STH-Comet** e **MongoDB**.
- Visualizar métricas em tempo real por meio de um **dashboard** interativo.
- Controlar atuadores remotamente a partir da plataforma, demonstrando o ciclo completo de uma arquitetura IoT.

---

## 🏗️ Arquitetura do Sistema

O projeto é estruturado em quatro camadas:

```
┌─────────────────────────────────────────────┐
│         Camada de Aplicação                 │
│   Dashboard · Grafana / Freeboard           │
└────────────────────┬────────────────────────┘
                     │ NGSIv2 / REST
┌────────────────────▼────────────────────────┐
│         Camada FIWARE — Back-end IoT         │
│  IoT Agent MQTT → Orion Context Broker      │
│  STH-Comet → MongoDB (histórico + contexto) │
└────────────────────┬────────────────────────┘
                     │ MQTT subscribe
┌────────────────────▼────────────────────────┐
│     Camada de Transporte — MQTT Broker      │
│           Mosquitto · porta 1883            │
└────────────────────┬────────────────────────┘
                     │ MQTT publish
┌────────────────────▼────────────────────────┐
│      Camada IoT — Dispositivo de Borda      │
│  ESP32 + DHT22 + LDR + LED + Buzzer         │
└─────────────────────────────────────────────┘
```

---

## 🔧 Componentes de Hardware

| Componente | Função | Interface |
|---|---|---|
| ESP32 | Microcontrolador principal com Wi-Fi | — |
| DHT22 | Sensor de temperatura e umidade | Digital |
| LDR | Sensor de luminosidade | Analógico (ADC) |
| LED | Atuador visual (indicador de estado) | Digital (PWM) |
| Buzzer | Atuador sonoro (alarme) | Digital |

---

## 🖥️ Componentes de Software / Plataforma

| Componente | Descrição | Porta |
|---|---|---|
| **Mosquitto** | MQTT Broker para transporte de mensagens | 1883 |
| **IoT Agent MQTT** | Traduz mensagens MQTT para NGSIv2 | 4041 |
| **Orion Context Broker** | Gerenciador de contexto FIWARE (NGSIv2) | 1026 |
| **STH-Comet** | Armazenamento de séries históricas | 8666 |
| **MongoDB** | Banco de dados para contexto e histórico | 27017 |
| **Dashboard** | Visualização de dados (Grafana / Freeboard) | — |

---

## ⚙️ Funcionalidades Implementadas

- **Leitura de temperatura e umidade** via sensor DHT22, com publicação periódica no broker MQTT.
- **Leitura de luminosidade** via LDR conectado à entrada analógica do ESP32.
- **Controle remoto de LED** a partir de comandos enviados pela plataforma FIWARE.
- **Alarme sonoro com Buzzer** acionado por comandos remotos ou por regras de negócio (ex: temperatura acima do limite).
- **Armazenamento histórico** das séries temporais no STH-Comet com persistência em MongoDB.
- **Dashboard** com gráficos em tempo real de temperatura, umidade e luminosidade.

---

## 📡 Fluxo de Dados

1. O **ESP32** realiza leituras dos sensores (DHT22 e LDR) em intervalos configuráveis.
2. Os dados são publicados em **tópicos MQTT** no broker Mosquitto.
3. O **IoT Agent MQTT** consome os tópicos e converte as mensagens para o formato **NGSIv2**.
4. O **Orion Context Broker** atualiza o contexto da entidade e notifica os serviços inscritos.
5. O **STH-Comet** recebe as notificações e persiste as séries temporais no **MongoDB**.
6. O **Dashboard** consulta o Orion e o STH-Comet via API REST para exibir os dados.
7. Comandos enviados pelo dashboard percorrem o caminho inverso: Orion → IoT Agent → MQTT → ESP32 → atuador.

---

## 🚀 Como Executar

### Pré-requisitos

- Docker e Docker Compose instalados
- Arduino IDE ou PlatformIO configurado para ESP32
- Biblioteca `PubSubClient` e `DHT sensor library` instaladas na IDE

### 1. Subir a plataforma FIWARE

```bash
docker-compose up -d
```

### 2. Verificar os serviços

```bash
# Orion
curl http://localhost:1026/version

# IoT Agent
curl http://localhost:4041/iot/about
```

### 3. Provisionar o dispositivo

```bash
# Criar service group
curl -iX POST 'http://localhost:4041/iot/services' \
  -H 'Content-Type: application/json' \
  -H 'fiware-service: smart' \
  -H 'fiware-servicepath: /' \
  -d '{
    "services": [{
      "apikey": "4jggokgpepnvsb2uv4s40d59ov",
      "cbroker": "http://orion:1026",
      "entity_type": "Thing",
      "resource": "/iot/json"
    }]
  }'
```

### 4. Flash no ESP32

Abra o arquivo `firmware/main.ino` na Arduino IDE, configure as credenciais Wi-Fi e o endereço do broker, e faça o upload para o dispositivo.

---

## 📁 Estrutura do Repositório

```
CP5-EDGE-COMPUTING/
├── firmware/
│   └── main.ino               # Código do ESP32
├── docker/
│   └── docker-compose.yml     # Orion, IoT Agent, STH-Comet, MongoDB, Mosquitto
├── docs/
│   ├── arquitetura.drawio     # Diagrama de arquitetura
│   └── arquitetura.png        # Exportação do diagrama
├── dashboard/
│   └── freeboard-config.json  # Configuração do dashboard
└── README.md
```

---

## 📚 Tecnologias Utilizadas

- **C++ / Arduino** — firmware do ESP32
- **MQTT (Mosquitto)** — protocolo de mensageria IoT
- **FIWARE Orion Context Broker** — gerenciamento de contexto
- **FIWARE IoT Agent MQTT** — integração protocolo ↔ plataforma
- **FIWARE STH-Comet** — persistência de séries temporais
- **MongoDB** — banco de dados NoSQL
- **Docker / Docker Compose** — orquestração dos serviços
- **NGSIv2** — API padrão FIWARE para dados de contexto

---

## 👥 Integrantes do Grupo

| Nome | RM |
|Murilo Jeronimo Ferreira Nunes|560641|
| Vinicius Kozonoe Guaglini | 567264 |
| Bruno Santos Castilho | 5667994 |

