# Labirinto Giratório — M2

## Identificação

- Disciplina: Algoritmos e Programação II
- Trabalho: M2 — Labirinto Giratório: A Evolução
- Professor: Thiago Felski Pereira
- Equipe: Artur Ximendes, Bruno Delfino e Pedro de Moraes
- Data de desenvolvimento: 17/10/2026
- Sistema utilizado nesta compilação e nos testes: Ubuntu 24.04.5 LTS, kernel Linux 6.18.38+

A implementação C++ está centralizada em `jogo.cpp`, conforme a orientação do professor. Não há outro arquivo `.cpp` no pacote. O projeto não usa classes, `vector` ou ponteiros explícitos. O estado da partida é armazenado em `Jogo`, não em variáveis globais de estado.

## Compilação e execução

No Linux:

```bash
g++ -std=c++17 -Wall -Wextra -pedantic jogo.cpp -o jogo.exe
./jogo.exe
```

O mesmo código possui o ramo `_WIN32` para captura de teclas e limpeza de tela em Windows. O executável incluído no ZIP foi compilado no Ubuntu do ambiente acima.

## Funcionalidades preservadas

O jogo possui movimentação WASD, rotação Q/E sobre alavancas, gravidade, blocos, portas, esmagamento, vitória, derrota, R para reiniciar, ESC para salvar e retornar ao menu, soluções reproduzíveis, ranking, seleção manual e seleção aleatória de mapas.

Entradas inválidas no menu são informadas e retornam ao menu. Somente a opção Fim encerra o programa.

## Formato dos mapas

O programa procura `mapa1.csv`, `mapa2.csv` e assim por diante até o primeiro arquivo inexistente, sem fixar uma quantidade máxima no código. Cada arquivo contém:

1. uma linha com a dimensão `n`, entre 11 e 21;
2. uma linha com o título;
3. `n` linhas com `n` inteiros separados por vírgulas.

Os códigos são: `0` vazio, `1` parede, `2` jogador, `3` bloco, `4` alavanca, `5` saída, `6` porta A e `7` porta B.

A leitura rejeita dimensão, título, linhas, colunas ou códigos inválidos; quantidade diferente de exatamente um jogador; quantidade diferente de exatamente uma saída; estado inicial instável; e qualquer linha extra após a matriz. Mapas inválidos aparecem como indisponíveis com o motivo.

Os cinco mapas e suas soluções foram preservados byte a byte nesta revisão. Não foram alterados os desafios, matrizes ou arquivos `.sol`.

## Formato de `jogo_salvo.csv`

O arquivo é criado quando o jogador pressiona ESC e contém:

1. número do mapa;
2. dimensão `n`;
3. título original do mapa;
4. uma linha com `orientacao,movimentos,rotacoes,linhaJogador,colunaJogador,objetoSobJogador`;
5. `n` linhas com a matriz atual, usando valores separados por vírgulas.

A função `carregaSalvo` limita e valida os campos, a orientação, os contadores, as coordenadas, os códigos, a quantidade e a posição do jogador, o título e a ausência de conteúdo inesperado depois da matriz. Arquivo ausente ou malformado é tratado como nenhum jogo salvo.

## Formato das soluções e do ranking

Cada `mapaN.sol` contém uma única linha com teclas `W`, `A`, `S`, `D`, `Q` e `E`. A validação informa a posição exata de tecla inválida. A reprodução usa a mesma função `processaTecla` do jogo normal e não cria recordes.

`recordes.csv` usa `nome,mapa,movimentos,rotacoes`, aceita no máximo 200 registros e mostra os cinco melhores do mapa selecionado.

## Comparação M1 → M2

As assinaturas abaixo foram conferidas no código original da M1 e no `jogo.cpp` desta M2:

| M1 original | M2 atual | Motivo da alteração |
|---|---|---|
| `void moveJogador(int matriz[][TAM], int n, int &linha, int &coluna, char tecla, int &elementoAnterior, int orientacao)` | `bool processaTecla(Jogo &j, char tecla, string &mensagem)` | Centralizar todo o estado em `Jogo` e reutilizar a mesma regra nas soluções. |
| `void giraDireita(int origem[][TAM], int destino[][TAM], int n)` | `void giraMatriz(Jogo &j, bool direita)` | Atualizar matriz, jogador e orientação juntos. |
| `void giraEsquerda(int origem[][TAM], int destino[][TAM], int n)` | `void giraMatriz(Jogo &j, bool direita)` | Unificar as duas rotações sem duplicar a implementação. |
| `void aplicaGravidade(int matriz[][TAM], int n, int orientacao, bool &jogadorEsmagado)` | `void aplicaGravidade(Jogo &j, bool &esmagado)` | Usar o estado completo e permitir a validação de estabilidade. |
| `void resolveEsmagamento(int matriz[][TAM], int n, int orientacaoAnterior, int orientacaoAtual, bool &jogadorEsmagado)` | `bool processaTecla(Jogo &j, char tecla, string &mensagem)` + `aplicaGravidade` | Integrar o resultado da regra ao processamento comum de teclado e solução. |
| `void carregaMapa(int mapa[][TAM], int numeroMapa)` | `bool carregaMapa(int numero, Jogo &j, string &erro)` | Trocar mapas fixos por CSV validado e informar o motivo de indisponibilidade. |
| não existia na M1 | `bool salvaJogo(const Jogo &j)` / `bool carregaSalvo(Jogo &j)` | Adicionar a persistência exigida pela M2. |
| não existia na M1 | `int carregaRecordes(Recorde registros[], string &erro)` / `bool gravaRecorde(const Recorde &r, string &erro)` | Adicionar ranking persistente com limite de 200 registros. |

## Defeitos e limitações da M1 corrigidos na M2

Com base no projeto M1 recebido, foram corrigidos ou ampliados os seguintes pontos:

- o estado antes espalhado em matrizes e parâmetros passou a ser reunido na struct `Jogo`;
- a dimensão deixou de ficar limitada ao `TAM = 11` da M1 e passou a aceitar mapas de 11 a 21;
- os três mapas codificados em funções `criaMapa1`, `criaMapa2` e `criaMapa3` passaram a ser carregados de CSV e validados;
- foram adicionados arquivos `.sol`, validação de teclas e reprodução sem geração de recordes;
- foram adicionados salvamento/continuação com validação contra arquivos corrompidos;
- foi adicionado ranking persistente com ordenação e limite de 200 registros;
- a implementação foi centralizada em um único `.cpp`, sem manter a divisão entre arquivos de desenvolvedores;
- a M2 passou a validar jogador, saída, estabilidade, linhas extras e códigos de células.

## Decisões de projeto

- `Jogo&` é usado nas funções que alteram a partida, evitando parâmetros redundantes.
- `sobJogador` preserva a célula escondida quando o jogador está sobre alavanca ou saída.
- A descoberta de mapas termina no primeiro CSV inexistente e não depende de uma quantidade fixa.
- A seleção aleatória considera somente mapas que passam por `carregaMapa`.
- A reprodução de soluções chama `processaTecla`, evitando uma segunda implementação das regras.
- A leitura de saves valida o arquivo antes de indexar a matriz.
- A tela de jogo agora monta cada quadro em `ostringstream` e envia a saída de uma vez, reposicionando o cursor no início sem limpar a tela inteira a cada quadro. Essa é uma correção mínima para reduzir a causa identificada da cintilação: limpeza completa seguida de impressão gradual.

## Desafios extras implementados

Nenhum dos desafios extras opcionais do enunciado foi implementado nesta versão.

Os recursos obrigatórios do trabalho, como salvamento, ranking e reprodução de soluções, não são contabilizados como desafios extras.

## Testes e limitações da confirmação visual

Foi executada a compilação com `g++ -std=c++17 -Wall -Wextra -pedantic jogo.cpp -o jogo.exe` no Ubuntu 24.04.5 LTS, seguida da verificação das cinco soluções existentes. Os arquivos `mapa1.csv` a `mapa5.csv` e `mapa1.sol` a `mapa5.sol` foram comparados por hash antes e depois desta revisão e permaneceram idênticos.

A função de desenho foi executada em sessão interativa após a alteração, e a saída, os controles e as informações permaneceram presentes. Não foi possível medir a cintilação em um monitor físico nem realizar uma comparação instrumental de frames; portanto, a redução da causa no código foi confirmada por inspeção e execução no terminal, mas não é declarada como uma confirmação visual absoluta.

Não foram alteradas outras regras, menus, structs, matrizes, mapas ou soluções nesta revisão. Eventuais problemas adicionais não fazem parte desta alteração.
