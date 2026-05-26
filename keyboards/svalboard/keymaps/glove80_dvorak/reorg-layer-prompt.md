  Analyze and optimize the FUNC layer of my Svalboard keymap keyboards/svalboard/keymaps/glove80_dvorak

  Context:
  - Keyboard: Svalboard split keyboard with 4 finger wells per hand.
  - Each finger has: Center, North, South, East, West, and Double.
  - Do not use Double positions unless explicitly allowed.
  - Thumb cluster positions are: Down, Pad, Up, Nail, Knuckle, DoubleDown.
  - Each layer is activated by holding a thumb key. The activation thumb side may be off-limits on that layer because that thumb is occupied holding the
  layer.
  - For this layer, the activation thumb side is: right.
  - Therefore, do not move assignments onto right thumb keys unless explicitly allowed.
  - The opposite thumb cluster may be used and optimized.
  - Preserve any keys explicitly marked fixed/off-limits.

  Task:
  Optimize all non-transparent, non-letter key assignments on the layer, not just printable characters.

  Important philosophy:
  - Frequency must be contextual to this layer, not global text frequency.
  - Estimate what the user is doing while already holding/locking this layer.
  - Rank keys by conditional usefulness in that context.
  - Example: on a NUM layer, digits, decimal point, arithmetic operators, parentheses, equals, Backspace, Enter, and numeric editing/navigation matter
  more than globally frequent prose punctuation like comma.
  - Example: on a SYM layer, programmer symbols and paired delimiters matter more than prose punctuation frequency.
  - Use general character-frequency data only as supporting evidence, not as the primary ranking.

  Constraints:
  - Keep all letters unchanged unless this layer intentionally contains non-text commands only.
  - Keep handedness: keys currently on the right hand should remain on the right hand; keys currently on the left hand should remain on the left hand,
  unless I explicitly allow cross-hand moves.
  - for the side of the activation key: 4 finger center modifiers fixed, 4 finger south keys only permuted within south, and 4 finger
  north keys only permuted within north.
  - Do not move layer activation behavior. If a key is a layer-tap, hold behavior stays fixed; only the tap output can change if allowed.
  - Use same-class swaps to improve memorability when they do not materially hurt ergonomics.
  - Prefer paired/grouped layouts when context supports it, e.g. `()`, `[]`, `{}`, `<>`, `+-`, `*/`, navigation clusters, editing clusters.

  Ergonomic model:
  - Finger strength: index > middle > ring > pinky.
  - Direction ease: Center and South/flexion are easiest; inward lateral is next; North/extension and outward lateral are harder.
  - Thumb positions: Down/Pad are best, Knuckle is usable, Up/Nail are moderate, DoubleDown is rare-use only.
  - Use the S/A/B/C/D/F accessibility classes from `ergonomics.md` if present.

  Deliverables:
  1. A short explanation of the layer-specific context and frequency assumptions.
  2. A frequency/priority-ranked list of all movable assignments on this layer.
  3. A proposed reassignment table with columns:
     `Char/Action | Contextual Freq/Priority | Current | Class | New Position | Class | Change | Why`
  4. An ASCII-art map of the proposed layer.
  5. A short list of questionable tradeoffs or alternatives.
  6. Do not edit files unless I explicitly approve.
  
  
    - Preserve these fixed keys: <FIXED_KEYS_AND_POSITIONS>.
  - Preserve these thumb positions: <FIXED_THUMB_POSITIONS>.