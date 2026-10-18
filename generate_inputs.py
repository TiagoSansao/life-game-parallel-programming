#!/usr/bin/env python3
"""Gera arquivos de entrada 'pesados' para o Game of Life multi-regra.

Formato de entrada (idêntico ao usado por life-*.in):
    L C G
    R
    <R regras, uma por linha>
    <L x C inteiros: matriz de identificadores de regra (0..R-1)>
    <L linhas com C caracteres: grade inicial ('x' = vivo, ' ' = morto)>
"""

import random
import sys

# Regras conhecidas (Conway, HighLife, Seeds, Day & Night)
RULES = [
    "B3/S23",
    "B36/S23",
    "B2/S",
    "B3678/S34678",
]

# Glider classico (Conway B3/S23)
GLIDER = [
    " x ",
    "  x",
    "xxx",
]

# Block (oscilador estatico)
BLOCK = [
    "xx",
    "xx",
]


def random_grid(rows, cols, density, rng):
    return [
        "".join("x" if rng.random() < density else " " for _ in range(cols))
        for _ in range(rows)
    ]


def stamp(grid, rows, cols, pattern, r, c):
    for i, line in enumerate(pattern):
        if r + i >= rows:
            break
        for j, ch in enumerate(line):
            if c + j >= cols:
                break
            if ch == "x":
                grid[r + i] = grid[r + i][: c + j] + "x" + grid[r + i][c + j + 1 :]


def generate(path, L, C, G, R, density, seed, sprinkle=True):
    rng = random.Random(seed)

    rule_matrix_lines = []
    for _ in range(L):
        row = [str(rng.randrange(R)) for _ in range(C)]
        rule_matrix_lines.append(" ".join(row))

    grid = random_grid(L, C, density, rng)

    if sprinkle:
        # Espalha gliders e blocos para manter a dinamica interessante.
        for _ in range(max(1, (L * C) // 5000)):
            r = rng.randrange(L)
            c = rng.randrange(C)
            stamp(grid, L, C, GLIDER if rng.random() < 0.7 else BLOCK, r, c)

    with open(path, "w") as f:
        f.write(f"{L} {C} {G}\n")
        f.write(f"{R}\n")
        for i in range(R):
            f.write(RULES[i % len(RULES)] + "\n")
        for line in rule_matrix_lines:
            f.write(line + "\n")
        for line in grid:
            f.write(line + "\n")


def main():
    specs = [
        # (arquivo,        L,     C,     G,      R, densidade, seed)
        ("life-5.in",     200,   200,   1000,   4, 0.35, 101),
        ("life-6.in",     500,   500,   2000,   4, 0.35, 202),
        ("life-7.in",    1000,  1000,   5000,   4, 0.35, 303),
        ("life-8.in",    2000,  2000,  10000,   4, 0.35, 404),
        ("life-9.in",    3000,  3000,  10000,   4, 0.35, 505),
    ]

    for name, L, C, G, R, density, seed in specs:
        print(f"gerando {name}: {L}x{C}, {G} geracoes, {R} regras ...",
              file=sys.stderr)
        generate(name, L, C, G, R, density, seed)
        print(f"ok: {name}", file=sys.stderr)


if __name__ == "__main__":
    main()
