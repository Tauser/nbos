# Motion Embodiment Baseline (Guards + Neutral)

## Escopo
Fechar o baseline de seguranca do subsystem de motion com:
- guards de operacao
- neutral pose consolidada
- validacao fisica minima de bancada

## Guards implementados
- `slew-rate guard`: limita salto por comando para reduzir tranco mecanico.
  - yaw: +/-140 permille por comando
  - pitch: +/-120 permille por comando
- `stale-face guard`: se sinal de face ficar obsoleto (> 1400 ms), motion volta para neutral.
- `fail-safe guard`: apos 3 falhas consecutivas de envio ao hardware, bloqueia comandos comuns e aceita apenas recovery/neutral/critical.
- `safe-mode guard`: em safe mode, reduz velocidade e hold, com recuperacao para neutral.
- `hold guard`: limita `hold_ms` maximo (900 ms) para evitar retencao longa indevida.

## Neutral pose consolidada
- Neutral e estado de recuperacao oficial do subsystem.
- Inicializacao aplica neutral por padrao.
- Guards de stale/fail-safe forcam retorno a neutral.
- Ao confirmar neutral aplicada com sucesso, fail-safe guard e liberado.

## Validacao fisica minima (bancada)
Executar nesta ordem:
1. Boot com servos conectados e sem carga externa.
2. Confirmar neutral inicial sem tranco (yaw=0, pitch=0).
3. Forcar mudanca de gaze (esquerda/direita) e observar movimento suave sem salto brusco.
4. Simular perda de sinal facial (parar updates) e confirmar retorno a neutral em ~1.4s.
5. Simular falha de comunicacao TTLinker (desconectar/erro) e confirmar bloqueio por fail-safe apos repetidas tentativas.
6. Restaurar comunicacao e executar recovery para neutral; confirmar liberacao do guard.

## Limites e riscos remanescentes
- Esses guards reduzem risco, mas nao substituem calibracao mecanica por unidade.
- Limites de yaw/pitch/speed continuam dependentes da montagem fisica real.
- Nao ha ainda deteccao de torque/corrente de servo; risco de sobrecarga mecanica permanece sem sensoriamento dedicado.
- Proxima etapa recomendada: perfil de limites por hardware real (com margem termica e de folga).

