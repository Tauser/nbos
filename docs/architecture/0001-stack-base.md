# ADR-0001: Stack Base do NC-OS

- Status: Accepted
- Data: 2026-03-14

## Contexto
NC-OS e um produto embarcado premium com display, camera, audio, IMU, servos e armazenamento local. O sistema deve ser offline-first, evolutivo e testavel, sem acoplamento estrutural fragil.

## Decisao
Adotar **ESP-IDF** como framework principal, com **PlatformIO** como orquestrador de build e perfis de ambiente. A biblioteca grafica oficial para UI sera **LovyanGFX**.

## Justificativa
- Manutencao: APIs maduras para FreeRTOS, I2S, I2C, UART, SPI, SD e particionamento.
- Performance: controle fino de tarefas, prioridades, memoria e watchdog.
- Testabilidade: separacao clara entre dominio, interfaces e HAL, com testes host/on-target.
- Evolucao: melhor base para OTA, telemetria, cloud bridge opcional e seguranca.

## Alternativas consideradas
1. Arduino puro: onboarding rapido, mas menor controle de ciclo de vida e maior risco de divida tecnica.
2. ESP-IDF + Arduino como componente: permitido somente quando houver dependencia real de driver/lib legado.

## Consequencias
- Custo inicial de setup maior.
- Menor retrabalho estrutural no medio/longo prazo.
- Base mais previsivel para produto real e nao apenas prototipo.
