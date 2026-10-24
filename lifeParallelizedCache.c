#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#define MAX_RULE_LEN 32

typedef struct
{
  int birth[9];
  int survival[9];
} Rule;

/*
 * Estrutura para manter a cache de celulas vivas
 * e os buffers necessarios para o calculo paralelo.
 */

typedef struct
{
  int *alives;          // array de alives atuais
  int *next_alives;     // array de alives da proxima generation
  int *candidates;      // celulas que serão calculadas
  int *marked;          // flag para saber qual já foi gerada e não ocorrer mais de uma na mesma celula
  int **thread_buffers; 
  int count;            // quantidade de alives atuais
} AliveCache;

/*
 * Converte uma regra no formato B3/S23, B36/S23, etc.
 * para dois vetores booleanos:
 *
 * birth[n]    = 1 se uma célula morta nasce com n vizinhos
 * survival[n] = 1 se uma célula viva sobrevive com n vizinhos
 */

void parse_rule(const char *text, Rule *rule)
{
  int i;

  for (i = 0; i <= 8; i++)
  {
    rule->birth[i] = 0;
    rule->survival[i] = 0;
  }

  int mode = 0; /* 1 = B, 2 = S */

  for (i = 0; text[i] != '\0'; i++)
  {
    if (text[i] == 'B' || text[i] == 'b')
    {
      mode = 1;
    }
    else if (text[i] == 'S' || text[i] == 's')
    {
      mode = 2;
    }
    else if (text[i] >= '0' && text[i] <= '8')
    {
      int n = text[i] - '0';

      if (mode == 1)
      {
        rule->birth[n] = 1;
      }
      else if (mode == 2)
      {
        rule->survival[n] = 1;
      }
    }
  }
}

/*
 * Conta os vizinhos vivos da célula (row, col).
 *
 * A grade não é toroidal:
 * posições fora da matriz são simplesmente ignoradas.
 */

int count_neighbors(
    const char *grid,
    int rows,
    int cols,
    int row,
    int col)
{
  int count = 0;

  for (int dr = -1; dr <= 1; dr++)
  {
    for (int dc = -1; dc <= 1; dc++)
    {

      if (dr == 0 && dc == 0)
      {
        continue;
      }

      int nr = row + dr;
      int nc = col + dc;

      if (nr >= 0 && nr < rows &&
          nc >= 0 && nc < cols)
      {

        if (grid[nr * cols + nc] == 'x')
        {
          count++;
        }
      }
    }
  }

  return count;
}

/*
 * Calcula uma geração completa.
 *
 * current nunca é alterada durante o cálculo.
 * Todos os novos estados são escritos em next.
 *
 * Isso garante a evolução síncrona.
 *
 * Otimização: processa apenas as células vivas e suas vizinhas (candidatos),
 * paralelizado com OpenMP.
 */

void next_generation(
    char *current,
    char *next,
    AliveCache *cache,
    const int *rule_matrix,
    const Rule *rules,
    int rows,
    int cols,
    int generation)
{
  // identifica as celulas candidatas 
  int candidate_count = 0;

  for (int i = 0; i < cache->count; i++)
  {
    int pos = cache->alives[i];
    int row = pos / cols;
    int col = pos % cols;

    // examina as linhas da celula
    for (int dr = -1; dr <= 1; dr++)
    {
      int nr = row + dr;
      if (nr < 0 || nr >= rows)
        continue;

      int row_init = nr * cols;

      for (int dc = -1; dc <= 1; dc++)
      {
        int nc = col + dc;
        if (nc >= 0 && nc < cols)
        {
          int p = row_init + nc;

          // para evitar guardar a mesma celula novamente
          if (cache->marked[p] != generation)
          {
            cache->marked[p] = generation;
            cache->candidates[candidate_count++] = p;
          }
        }
      }
    }
  }

  // avalia as celulas candidatas
  int next_alive_count = 0;

  #pragma omp parallel
  {
    int tid = omp_get_thread_num();
    int *local_alives = cache->thread_buffers[tid];
    int local_count = 0;

    #pragma omp for nowait schedule(static)
    for (int i = 0; i < candidate_count; i++)
    {
      int pos = cache->candidates[i];
      int row = pos / cols;
      int col = pos % cols;

      int neighbors = count_neighbors(current, rows, cols, row, col);
      int rule_id = rule_matrix[pos];

      if (current[pos] == 'x')
      {
        /*
         * Célula atualmente viva.
         * Consulta a parte S da regra.
         */
        if (rules[rule_id].survival[neighbors])
        {
          next[pos] = 'x';
          local_alives[local_count++] = pos;
        }
      }
      else
      {
        /*
         * Célula atualmente morta.
         * Consulta a parte B da regra.
         */
        if (rules[rule_id].birth[neighbors])
        {
          next[pos] = 'x';
          local_alives[local_count++] = pos;
        }
      }
    }

    // junção das celulas vivas de cada thread no vetor global
    int offset;
    #pragma omp atomic capture
    {
      offset = next_alive_count;
      next_alive_count += local_count;
    }

    memcpy(&cache->next_alives[offset], local_alives, local_count * sizeof(int));
  }

  // limpa no buffer 'current' as células que eram vivas nesta geração
  #pragma omp parallel for schedule(static)
  for (int i = 0; i < cache->count; i++)
  {
    current[cache->alives[i]] = ' ';
  }

  // 4. Atualiza a contagem e inverte os ponteiros de vivas da cache
  cache->count = next_alive_count;

  int *temp_alives = cache->alives;
  cache->alives = cache->next_alives;
  cache->next_alives = temp_alives;
}

int main(void)
{

  int L, C, G;

  if (scanf("%d %d %d", &L, &C, &G) != 3)
  {
    fprintf(stderr, "Erro ao ler L, C e G.\n");
    return 1;
  }

  int R;

  if (scanf("%d", &R) != 1)
  {
    fprintf(stderr, "Erro ao ler o numero de regras.\n");
    return 1;
  }

  Rule *rules = malloc(R * sizeof(Rule));

  if (rules == NULL)
  {
    fprintf(stderr, "Erro de alocacao de memoria.\n");
    return 1;
  }

  /*
   * Leitura das regras.
   *
   * Exemplo:
   * B3/S23
   * B36/S23
   */
  for (int i = 0; i < R; i++)
  {
    char rule_text[MAX_RULE_LEN];

    if (scanf("%31s", rule_text) != 1)
    {
      fprintf(stderr, "Erro ao ler regra %d.\n", i);
      free(rules);
      return 1;
    }

    parse_rule(rule_text, &rules[i]);
  }

  int total_cells = L * C;

  int *rule_matrix = malloc(total_cells * sizeof(int));

  if (rule_matrix == NULL)
  {
    fprintf(stderr, "Erro de alocacao de memoria.\n");
    free(rules);
    return 1;
  }

  /*
   * Matriz contendo o identificador da regra
   * associada a cada posição.
   */
  for (int row = 0; row < L; row++)
  {
    for (int col = 0; col < C; col++)
    {

      int pos = row * C + col;

      if (scanf("%d", &rule_matrix[pos]) != 1)
      {
        fprintf(stderr, "Erro ao ler matriz de regras.\n");
        free(rule_matrix);
        free(rules);
        return 1;
      }

      if (rule_matrix[pos] < 0 ||
          rule_matrix[pos] >= R)
      {

        fprintf(stderr,
                "Identificador de regra invalido: %d\n",
                rule_matrix[pos]);

        free(rule_matrix);
        free(rules);
        return 1;
      }
    }
  }

  /*
   * Remove o '\n' que ficou depois da leitura
   * da matriz de inteiros.
   */
  int ch;

  while ((ch = getchar()) != '\n' && ch != EOF)
  {
  }

  char *current = malloc(total_cells * sizeof(char));
  char *next = malloc(total_cells * sizeof(char));

  // Inicialização da cache de células vivas
  AliveCache cache;
  cache.alives = malloc(total_cells * sizeof(int));
  cache.next_alives = malloc(total_cells * sizeof(int));
  cache.candidates = malloc(total_cells * sizeof(int));
  cache.marked = malloc(total_cells * sizeof(int));
  cache.count = 0;

  int max_threads = omp_get_max_threads();
  cache.thread_buffers = malloc(max_threads * sizeof(int *));
  for (int t = 0; t < max_threads; t++)
  {
    cache.thread_buffers[t] = malloc(total_cells * sizeof(int));
  }

  if (current == NULL || next == NULL ||
      cache.alives == NULL || cache.next_alives == NULL ||
      cache.candidates == NULL || cache.marked == NULL ||
      cache.thread_buffers == NULL)
  {
    fprintf(stderr, "Erro de alocacao de memoria.\n");

    free(current);
    free(next);
    free(rule_matrix);
    free(rules);

    return 1;
  }

  memset(next, ' ', total_cells * sizeof(char));
  memset(cache.marked, -1, total_cells * sizeof(int));

  /*
   * Como espaços são significativos, não podemos usar scanf("%s").
   *
   * É necessário usar fgets.
   */
  char *line = malloc((C + 3) * sizeof(char));

  if (line == NULL)
  {
    fprintf(stderr, "Erro de alocacao de memoria.\n");

    free(current);
    free(next);
    free(rule_matrix);
    free(rules);

    return 1;
  }

  for (int row = 0; row < L; row++)
  {

    if (fgets(line, C + 3, stdin) == NULL)
    {
      fprintf(stderr, "Erro ao ler a grade inicial.\n");

      free(line);
      free(current);
      free(next);
      free(rule_matrix);
      free(rules);

      return 1;
    }

    /*
     * Copia exatamente C posições.
     *
     * Caso haja alguma linha menor por algum motivo,
     * as posições faltantes são consideradas mortas.
     */
    for (int col = 0; col < C; col++)
    {
      int pos = row * C + col;

      if (line[col] == 'x' || line[col] == 'X')
      {
        current[pos] = 'x';
        cache.alives[cache.count++] = pos;
      }
      else
      {
        current[pos] = ' ';
      }
    }
  }

  free(line);

  /*
   * Executa as G gerações.
   *
   * Ao final de cada geração simplesmente trocamos
   * os ponteiros current e next.
   *
   * Assim não é necessário copiar toda a matriz.
   */
  for (int generation = 0; generation < G; generation++)
  {

    next_generation(
        current,
        next,
        &cache,
        rule_matrix,
        rules,
        L,
        C,
        generation);

    char *temp = current;
    current = next;
    next = temp;
  }

  /*
   * Imprime exatamente L linhas,
   * cada uma contendo C posições.
   */
  for (int row = 0; row < L; row++)
  {

    for (int col = 0; col < C; col++)
    {
      putchar(current[row * C + col]);
    }

    putchar('\n');
  }

  // Liberação de memória
  for (int t = 0; t < max_threads; t++)
  {
    free(cache.thread_buffers[t]);
  }
  free(cache.thread_buffers);
  free(cache.alives);
  free(cache.next_alives);
  free(cache.candidates);
  free(cache.marked);

  free(current);
  free(next);
  free(rule_matrix);
  free(rules);

  return 0;
}
