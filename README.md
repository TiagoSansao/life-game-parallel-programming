# Execução do programa
1. Execute `make` na raiz do projeto. 
- É possível alterar o compilador, vide variável `CC`, ex.: `make CC=clang`
2. Execute o binário gerado, nome `life`: `./life`
- Para praticidade, é possível passar como entrada um dos 8 conjuntos de dados de entrada persistidos no repositório `life-{0-8}.in`. Ex.: `./life < life-8.in`
3. Para pegar o tempo de execução e calcular speedups, basta utilizar o binário utilitário `time`. Ex.: `time ./life < life-8.in`