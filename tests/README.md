# Testes do compilador

Execute `make test` e `make test-memory` no ambiente Linux ou Docker.

- `frontend/`: 12 entradas originais da primeira etapa, preservadas integralmente.
- `valid/` e `invalid/`: testes da segunda etapa, com comparação TAC byte a byte.
- `stage3/`: seis programas válidos com condicionais, laços, curto-circuito e
  recursão, além de seis entradas inválidas.
- `test_core.c`: offsets, temporários, estatísticas da AST, representação TAC e
  propriedade/resolução das listas de saltos.
- `tac_vm.py`: avaliador restrito aos casos de teste, usado para conferir resultados
  de execução e efeitos observáveis; não é um backend do compilador.

Os dois primeiros resultados esperados da segunda etapa foram preservados.
Os demais resultados dessa etapa foram derivados manualmente das operações e
convenções TAC. Nenhum resultado esperado é gerado pelo compilador sob teste.

Os resultados da terceira etapa seguem o fluxo e os rótulos descritos nas tarefas,
com os contratos efetivos da implementação: locais de tipo desconhecido avançam
4 bytes, parâmetros usam offsets negativos e não emitem declarações locais, e
funções não recebem retornos implícitos. Os resultados de referência descreviam
outros offsets e retornos; a adaptação dessas diferenças foi explícita.

A suíte exige resultado esperado para cada entrada válida e falha em diferenças,
erros de execução ou timeouts. Também verifica destinos de salto definidos e
rótulos únicos. Casos de execução cobrem curto-circuito com efeitos observáveis,
valores booleanos, negação, laços aninhados e laços com zero iterações.
