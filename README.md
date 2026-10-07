# LARA compiler

Compilador em C, Flex e Bison para a linguagem LARA. A versão atual corresponde à
Etapa 3 e gera código de três endereços (TAC) com controle de fluxo e curto-circuito.

## Compilar e testar

Ambiente: Linux, GCC, GNU Make 4.3+, Flex, Bison e Python 3. Graphviz permite
visualizar a AST; Valgrind verifica o uso de memória.

```sh
make
make test
make test-memory
```

Para usar o ambiente Docker incluído:

```sh
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm dev make test
```

## Executar

```sh
./lara < tests/stage3/valid/01_if_simples.lc
./lara < tests/stage3/valid/03_while.lc
./lara < tests/stage3/valid/06_fatorial.lc
./lara --ast < tests/stage3/valid/01_if_simples.lc
make ast-dot FILE=tests/stage3/valid/01_if_simples.lc
```

O compilador lê o programa pela entrada padrão e escreve TAC na saída padrão.
Erros vão para stderr, com código de saída 1; uma compilação válida retorna 0.
O modo `--ast` imprime a árvore, suas estatísticas e a tabela de símbolos.

## Implementação

- Declarações globais e arrays com offsets consecutivos e tamanhos int=4, float=8,
  char=1 e bool=1 bytes. Os offsets locais reiniciam por função.
- Expressões aritméticas, relacionais, atribuições escalares e escrita em arrays.
- Curto-circuito de `&&` e `||`, tanto nas condições como nos valores booleanos.
- `if/else`, `while...do`, `for`, chamadas e retornos, incluindo chamadas recursivas.
- Resolução dos destinos de salto por listas de back-patching.

A lista TAC copia as strings recebidas. Quem chama `codegen_expr()` libera o
endereço retornado; as listas de back-patching possuem apenas seus próprios nós.

## Testes

`make test` compara as saídas TAC, preserva regressões do frontend e verifica em C
os offsets e as listas de saltos. Um avaliador de TAC usado pelos testes confere
os resultados de laços, condições, recursão e chamadas que devem ser puladas.
`make test-memory` verifica programas válidos, erros de parsing e liberação das
listas de saltos. `make clean` remove os arquivos de compilação.

## Limitações

A tabela de símbolos mantém um espaço de nomes plano, sem resolução de escopos
aninhados. Tipos locais e de parâmetros sem metadados usam o tamanho padrão de
4 bytes. Leitura e atribuições compostas indexadas, acesso a campos e `return;`
sem expressão ainda não têm suporte completo. Não há validação semântica completa.
A saída é TAC; a geração de Assembly pertence à etapa seguinte.
