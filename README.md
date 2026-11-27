# Othello Tournament AI Engine in C

> **Competitive Othello (Reversi) artificial intelligence engine in C featuring 64-bit bitboards, iterative deepening Alpha-Beta search, Zobrist transposition tables, and genetic algorithm heuristic tuning — 2nd Place (Runner-Up) in the Grenoble INP – Esisar Tournament.**

---

## 📌 Context & Methodology

- **Institution**: Grenoble INP – Esisar (`1er cycle` / 2A)
- **Course**: PXSI `PX221-Info` (C Systems & Algorithmics, prerequisite `CS221`, Dec 2025 – Jan 2026)
- **Authors**: [Jamyson Vetter](https://github.com/jamyvetter) & [Siméon Studer](https://github.com/hiiiiiiiiiboux)
- **Methodology**: Iterative **Agile SCRUM** sprints (`US1`–`US5` backlog defined by the instructor acting as Product Owner)
- **Project Overview**: [jamyvetter.net/projects/othello-ai-c/](https://jamyvetter.net/projects/othello-ai-c/)

---

## 🏛️ Architecture & Engine Highlights

1. **64-Bit Bitboard Core (`fonctions.h`, `fonctions.c`)**:
   - Dual `uint64_t` bitboards (`noir`, `blanc`) encoding the $8 \times 8$ board directly in CPU registers (replacing the baseline 2D `char[8][8]` representation).
   - Branch-minimal directional ray generation (`coups_possibles`) and disc captures across all 8 axes via bitwise shifts, wrap-around file masks (`NOT_A_FILE`, `NOT_H_FILE`), and GCC hardware intrinsics (`__builtin_popcountll`, `__builtin_ctzll`) compiled with `-O3 -march=native -flto`.
2. **Iterative Deepening Minimax & Alpha-Beta Search (`minimax_hash_temporel`)**:
   - Time-bounded depth-first Minimax search calibrated to a strict `14.1s` per-move server timeout budget via periodic node-count clock checks.
   - Alpha-Beta pruning accelerated by principal-variation / cached best-move ordering.
3. **Zobrist Hashing, Transposition Table & Opening Book**:
   - Incremental 64-bit XOR Zobrist hashing (`ZOBRIST_TABLE[64][2]`, `ZOBRIST_TOUR`).
   - $2^{20}$-entry (~1M entries) transposition table (`TT_Entry`) storing exact scores, lower/upper Alpha-Beta bounds (`TT_FLAG_EXACT`, `TT_FLAG_LOWER`, `TT_FLAG_UPPER`), and cached best moves.
   - Zobrist-keyed opening book (`opening_book`) handling early-game transpositions (`d3`, Star, and Cow lines).
4. **Evolutionary Heuristic Tuning (`algoGenetique.h`, `algoGenetique_etu.c`)**:
   - Offline genetic algorithm evolving populations of 100 candidate evaluators (`NBPOPULATION = 100`, `NBPARENTS = 40`) via tournament selection, uniform crossover, elitism, and variance-adaptive mutation.
   - Phase-interpolated evaluation blending opening/endgame linear models over 10 symmetric board-region masks (`c0`–`c9` rewarding corners and penalizing X/C-squares), legal move mobility, and disc differential.

---

## 📋 Agile SCRUM Backlog (`serveur/serveur4/US1`–`US5`)

The project progressed across five incremental User Stories integrating the Esisar referee server API (`userNetwork.h` / `libothello.so`, which arbitrates authentication, turn order, move legality, and move timeouts while delegating all board state and AI logic to the client):

| User Story | Source Files | Deliverable |
| :--- | :--- | :--- |
| **`US1`** | `deux_joueurs_local.c`, `serveur/serveur4/US1.c` | Local 2-player human match loop with move legality, 8-axis disc flipping, pass handling, and end-of-game winner detection. |
| **`US2`** | `benchmark_othello.c`, `test_unitaires.c`, `serveur/serveur4/US2.c` | Automated game-mechanics regression harness validating `(board_n, move) -> board_{n+1}` state transitions and Zobrist hashes against the `savedGames_` corpus. |
| **`US3`** | `serveur/serveur4/US3.c` | Networked human player interfacing with the referee server API (`libothello.so`), augmented with real-time AI move suggestions. |
| **`US4`** | `IA_vs_humain_local.c`, `serveur/serveur4/US4.c` | Local Human-vs-AI mode validating depth-bounded Minimax search and heuristic evaluation. |
| **`US5`** | `serveur/serveur4/US5.c` | Full autonomous tournament AI client combining bitboards, 14.1s iterative deepening Alpha-Beta, Zobrist TT, opening book, and evolved weights (**2nd place overall**). |

---

## 🚀 Quick Start

### 1. Local Modes & Evolutionary Trainer (Root)

```bash
make
./deux_joueurs_local
./IA_vs_humain_local
./ia_vs_ia_local_bitboard
./main_ia_entrainement
```

### 2. State-Transition Regression Benchmark (`US2`)

```bash
gcc -Wall -Wextra -O3 benchmark_othello.c -o benchmark_othello
./benchmark_othello savedGames_
```

### 3. Tournament AI Client (`US5` in `serveur/serveur4`)

```bash
cd serveur/serveur4
make
export LD_LIBRARY_PATH=.
./US5 <password> <server_ip> <user_id>
```
