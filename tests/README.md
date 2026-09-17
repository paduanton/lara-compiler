# Testes

Execute `make test` ou `bash tests/run_tests.sh` no Linux. O script recompila o projeto e executa
as verificações Python; `make test` também compila e executa os testes de estruturas em C.

- `valid/01` a `valid/09`: entradas da base da Etapa 2.
- `01_soma.expected` e `02_assign.expected`: resultados originais preservados byte a byte.
- Resultados `03` a `09`: expectativas do projeto, derivadas manualmente da especificação e
  do contrato de impressão TAC. Não são geradas pela execução do compilador em teste.
- Entradas/resultados `10` a `12`: chamadas e reinício de frame, índice calculado com avaliação
  do lado direito primeiro, literais e comentários. São regressões acrescentadas pelo projeto.
- `frontend/`: os seis programas válidos e seis inválidos da Etapa 1, executados com `--ast`.
- `invalid/`: sete entradas da base da Etapa 2 e duas regressões de comentário/expressão inválida.

Uma saída esperada ausente ou diferente torna a suíte TAC inválida. Todos os casos de rejeição
exigem código 1, diagnósticos e stdout vazio; crashes e timeouts não contam como rejeição correta.
Há ainda verificações de linhas após comentários, EOF em comentários, espaço em branco e
expoentes, opções inválidas e preservação da AST para o conversor Graphviz.

`test_core.c` verifica offsets diretamente na tabela, temporários entre funções, estatísticas
da AST e o contrato `TAC_STORE(result=array, arg1=index, arg2=value)`. A lista TAC duplica strings;
o teste também confere que alterar a string de origem não altera a instrução.

`make test-memory` executa Valgrind nas entradas válidas e inválidas de ambas as suítes, no
programa vazio, em comentários incompletos e no teste de estruturas. Erros de memória usam o
código 99, distinguindo-os da rejeição sintática esperada (1).
