# LARA compiler

Compilador em C, Flex e Bison para uma linguagem imperativa com funções, variáveis e arrays.
A versão atual gera código de três endereços (TAC), correspondente à Etapa 2.

## Compilar e testar

O ambiente de referência é Linux (Ubuntu 22.04), com GCC, GNU Make 4.3+, Flex, Bison e Python 3.
Valgrind verifica memória; Graphviz gera a visualização da AST. O contêiner inclui essas ferramentas.

```sh
docker compose -f docker/docker-compose.yml build
docker compose -f docker/docker-compose.yml run --rm dev make test
docker compose -f docker/docker-compose.yml run --rm dev make test-memory
```

Dentro de um ambiente Linux com as dependências instaladas:

```sh
make
make test
make test-memory
```

## Usar

```sh
./lara < tests/valid/01_soma.lc
./lara --ast < tests/valid/01_soma.lc
make ast-dot FILE=tests/valid/01_soma.lc
```

O modo padrão lê LARA de stdin e escreve somente TAC em stdout. Diagnósticos vão para stderr.
O código de saída é 0 no sucesso e 1 em erros léxicos/sintáticos ou de uso da linha de comando.
`--ast` imprime a AST, suas estatísticas e a tabela de símbolos. `make ast-dot` produz `ast.dot`
e, quando Graphviz está disponível, `ast.svg`.

O compilador ainda não executa os programas nem gera Assembly. Para a entrada:

```c
fun void main() { let x := 3 + 4 * 2; print x; }
```

A saída é:

```text
    beginFunc main
    local x, 0
    _t1 = 4 * 2
    _t2 = 3 + _t1
    x = _t2
    print x
    endFunc main
```

## Implementação atual

- Declarações globais escalares e arrays, com offsets consecutivos e tamanhos int=4, float=8,
  char=1 e bool=1 bytes. Os offsets locais reiniciam em cada função.
- Operadores aritméticos e relacionais, operadores unários, inicializadores locais, atribuições
  simples e compostas escalares, leitura e escrita de arrays com `:=`.
- Operações básicas de chamada, retorno e entrada/saída da infraestrutura TAC.
- Frontend com tratamento de comentários sem fechamento e liberação das estruturas em erros.

As estruturas TAC e os metadados de símbolos estão nos respectivos cabeçalhos de `src/`.
O gerador duplica os endereços ao emitir instruções; quem recebe uma string de `codegen_expr()`
ou `tac_new_temp()` deve liberá-la após o uso. A lista TAC é liberada separadamente do contexto.

## Limites desta etapa

Controle de fluxo (`if`, `while`, `for`) ainda produz um aviso e não gera os comandos internos.
Operações `&&`/`||` usam a emissão direta disponível, sem curto-circuito. Esses programas podem
ser inspecionados integralmente com `--ast`; TAC completo será desenvolvido na Etapa 3.

A tabela de símbolos usa um espaço de nomes plano. Declarações locais e parâmetros sem metadados
de tipo usam o tamanho padrão de 4 bytes; não há inferência de tipos ou resolução de escopos
aninhados. `read` e atribuições compostas indexadas, acesso a campos e `return;` sem expressão
também não fazem parte do suporte completo desta versão. Essas limitações não devem ser
interpretadas como validação semântica da linguagem.

## Testes e entregas

`make test` executa comparações TAC byte a byte, regressões do frontend e testes em C dos offsets,
nomes temporários, estatísticas e representação TAC. Consulte `tests/README.md` para a origem dos
resultados esperados. `make clean` remove os arquivos gerados.

Para gerar os pacotes da etapa atual a partir do checkout de desenvolvimento:

```sh
docker compose -f docker/docker-compose.yml run --rm dev make package
```

O empacotador confere o número em `STAGE`, inclui os fontes atuais mesmo sem commit, gera ZIP e
`.tar.gz`, recompila ambos após extração e executa os testes e a geração AST/SVG. Também executa
Valgrind no snapshot extraído do ZIP. Os artefatos e o manifesto com hashes ficam em uma nova
pasta sob `docs/releases/etapa-2/`. A pasta de trabalho local `docs/` fica excluída no
`.git/info/exclude` deste checkout e não integra os pacotes.

Os pacotes contêm o projeto compilável sem metadados Git, referências locais ou arquivos de build.
Para compilar e testar um pacote extraído, execute `make test` na pasta que contém o Makefile.
O empacotamento de uma nova entrega requer o checkout Git; a compilação dos pacotes não requer Git.
