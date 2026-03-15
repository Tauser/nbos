# 0014 - Emotion model base (dimensional)

## Contexto
O NC-OS precisa de um modelo emocional util para face, motion, atencao e rotinas, sem virar lista de rotulos sem funcao.

## Decisao
Adotar um modelo emocional base com:
- vetor dimensional (`valence`, `arousal`, `social_engagement`)
- metadados de dinamica (`intensity`, `stability`)
- fase emocional (`baseline`, `engaged`, `alerting`, `recovering`)

Manter `tone` e `arousal` categorizados para compatibilidade incremental com partes ja existentes.

## Regras
- `vector_authoritative=true`: writer fornece vetor e o sistema deriva `tone/arousal/phase`.
- `vector_authoritative=false`: sistema converte legado (`tone/arousal/intensity`) para vetor antes de persistir.
- sinais sao normalizados (clamp) antes de entrar no `Companion State`.

## Trade-offs
- Pro: melhora semantica pratica para face/motion sem explodir complexidade.
- Pro: preserva compatibilidade com contratos legados.
- Contra: mapeamento entre categorias e dimensoes ainda e heuristico nesta fase.
- Contra: nao cobre psicologia profunda (intencional para manter arquitetura enxuta).
