# Paralelização por Linha
1. Paralelizamos a execução do cálculo do next grid por linha, dividindo o número de linhas para o número de threads disponíveis executarem em paralelo.
2. Foi observado que nessa solução inicial, as entradas de teste cedidas eram tão pequenas que o overhead de gerenciamento das threads era maior do que a rapidez ganhada pelo paralelismo, causando um speedup menor que 1, então, geramos entradas de teste bem maiores.
3. Agora, com uma entrada de teste maior (life-6.in), foi observado um speedup de 3.36

# Paralelização por Coluna
1. Agora, paralelizamos o cálculo do next grid por coluna, utilizando a mesma divisão de threads que a estratégia anterior.
2. Observamos uma diminuição drástica no desempenho em comparação com a paralelização por linha, e um aumento timido comparando com a versão serial. Essa diminuição vem por conta do layout de memória da grade, armazenada em ordem row-major, conforme a indexação grid[row * cols + col]. Assim, Células de uma mesma linha ocupam posições contíguas na memória, enquanto células de uma mesma coluna estão separadas por um passo (stride) de cols elementos.
Ao dividir o trabalho por colunas, cada thread acessa a memória com esse passo grande. Cada cache line (64 bytes) carregada é aproveitada em poucos bytes, o que aumenta o tráfego de memória e os cache misses.
3. Foi anotado um speedup de 1.33

# Para testar
1. Paralelizar por coluna. Testar qtd de threads, quantas colunas para cada thread, se há uma razão calculável que sempre seja boa

2. Uma ideia é ter uma thread especializada para cada tipo de regra. Mas supondo que possa ter mais células de uma regra específica, devemos ter mais threads para essa regra. 

3. Testar a possibilidade de armazenarmos um "cache" que tem células vivas a fim de não precisarmos em toda geração calcular todas as células possíveis da matriz. Ex.: só temos algumas células no canto superior direito, não precisamos calcular o canto inferior esquerdo.


