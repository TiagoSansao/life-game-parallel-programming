#include <stdio.h>

int main()
{
  int numberOfLines, numberOfColumns, numberOfGens;

  scanf("%d %d %d", &numberOfLines, &numberOfColumns, &numberOfGens);

  printf("%d\n", numberOfColumns);

  int numRules;

  scanf("%d", &numRules);

  for (int j = 0; j < numRules; j++)
  {
    int bornRule, surviveRule;
    scanf("B%d/S%d", &bornRule, &surviveRule)
  }

  // ----

  int rulesMatrix[numberOfLines][numberOfColumns];

  for (int i = 0; i < numberOfLines; i++)
  {
    }

  return 0;
}