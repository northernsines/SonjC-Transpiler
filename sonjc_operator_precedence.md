# SonjC Pratt Parser — Operator Precedence Table

Binding powers for the `rules[]` token table. Higher lbp = binds tighter = evaluated first.
Gaps of 10 left between tiers so new operators can be inserted without renumbering everything.

| Precedence (low→high) | Operators | lbp | Associativity | nud? | led? |
|---|---|---|---|---|---|
| Comma | `,` | 10 | left | — | yes |
| Assignment | `= += -= *= /= %= &= \|= ^= <<= >>=` | 20 | **right** | — | yes |
| Ternary | `?:` | 30 | right | — | yes (special, 3-arg) |
| Logical OR | `\|\|` | 40 | left | — | yes |
| Logical AND | `&&` | 50 | left | — | yes |
| Bitwise OR | `\|` | 60 | left | — | yes |
| Bitwise XOR | `^` | 70 | left | — | yes |
| Bitwise AND | `&` | 80 | left | yes (addr-of) | yes |
| Equality | `== !=` | 90 | left | — | yes |
| Relational | `< > <= >=` | 100 | left | — | yes |
| Shift | `<< >>` | 110 | left | — | yes |
| Additive | `+ -` | 120 | left | yes (unary +/-) | yes |
| Multiplicative | `* / %` | 130 | left | yes (deref for `*`) | yes |
| Cast | `(Type)` | 140 | right | yes | — |
| Unary | `! ~ ++ -- (prefix) sizeof` | 140 | right | yes | — |
| Postfix | `++ -- () [] . ->` | 150 | left | — | yes |

## Mechanics notes

- **Right-associativity** lives in the `led`, not the table. Left-assoc operators recurse with
  `parse_expr(p, rule->lbp)`; right-assoc operators (assignment, ternary, cast, unary) recurse
  with `parse_expr(p, rule->lbp - 1)`. Same lbp number either way — the `-1` is what lets a
  same-precedence operator to the right bind again instead of stopping.
- **`*` and `&` are both nud and led** at their respective tiers (deref/addr-of as prefix,
  multiply/bitwise-AND as infix). Same token, dispatch is purely positional — no special-casing
  needed in `parse_expr` itself.
- **Unary/postfix `++`/`--` share operators but never conflict** the same way — `++x` is a nud,
  `x++` is a led.
- **Cast `(Type)` is the tricky one**: `(int)x`, `(x)`, and `(x + y)` all start with `(`, and you
  can't tell which you've got until you check whether the first token inside is a known type
  name. Needs a type-name lookup (symbol table) available at parse time, not just token
  classification.
