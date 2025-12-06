# Level 1 Visual Layout Guide

## Top-Down View (X-Z Plane)

```
Ship starts at (0, 0, -10), looking down -Z axis
Scroll speed: 15 units/sec
Play area: X ∈ [-50, 50], Y ∈ [-30, 30]

       -50        -25         0         25        50  (X-axis)
        │          │          │          │          │
   0 ───┼──────────┼──────────┼──────────┼──────────┼─── START
        │          │          │          │          │
 -20    │          │      ✦ S1 (center)  │          │
        │          │          │          │          │
 -30    │          │          │      o L│          │
 -35    │          │         S2 ✦       │          │
        │          │          │          │          │
 -40    │     L o  │          │          │    L o   │
 -50    │    S3 ✦  │          │          │          │
        │          │          │     L o  │          │
 -60 ───┼──────────┼──────────┼──────────┼──────────┼─── START/MID BOUNDARY
        │          │          │          │          │
 -65    │     P1💎 │          │          │          │
 -70    │     L o  │          │     L o  │          │
        │          │          │          │          │
 -80 ───┼──────────┼──────────┼──────────┼──────────┼───
        │          │          │          │          │
 -90    │          │          │        S4 ✦         │
-100    │     L o  │          │          │    L o   │
-105    │          │          │          │   P2💎   │
-110    │          │          │          │    L o   │
-115    │     L o  │          │          │          │
-120 ───┼──────────┼──────────┼──────────┼──────────┼─── MID/END BOUNDARY
-125    │   S5 ✦   │          │          │          │
-130    │     L o  │          │          │    L o   │
-140    │          │          │          │    L o   │
        │          │          │          │          │
-160    │     L o  │          │          │    L o   │
-165    │          │          P3💎       │          │
-170    │     L o  │          │          │    L o   │
-180    │        L o          │       L o           │
-190    │          │       L o         │          │
-200 ───┼──────────┼─────── 🌀 PORTAL ─┼──────────┼─── END
        │          │          │          │          │


LEGEND:
───────────────────────────────────────────────────────
✦      Energy Shard (5 total, all required)
💎      Power-Up (3 total, optional)
L o    Large Drift Asteroid (24 total, radius 2.5)
       Small asteroids not shown (49 total, radius 0.8)
🌀      Wave Portal (goal, requires 5 shards)

ZONES:
───────────────────────────────────────────────────────
z =   0 to  -60: START ZONE (safe learning)
z = -60 to -120: MID ZONE (moderate challenge)
z =-120 to -200: END ZONE (dense obstacles)
```

---

## Side View (Y-Z Plane)

```
       -30        -15         0         15        30  (Y-axis)
        │          │          │          │          │
   0 ───┼──────────┼──────────┼──────────┼──────────┼─── START
        │          │          │          │          │
 -20    │          │      ✦ S1 (y=3)    │          │
        │          │          │          │          │
 -35    │          │      S2 ✦ (y=2)    │          │
        │          │          │          │          │
 -40    │          │          │       L o (y=25)    │
 -50    │       S3 ✦ (y=-3)  │          │          │
        │          │          │          │          │
 -60 ───┼──────────┼──────────┼──────────┼──────────┼───
        │          │          │          │          │
 -75    │          │          │          │    L o   │
        │          │          │          │          │
 -90    │          │        S4 ✦ (y=5)  │          │
 -95    │    L o   │          │          │          │
-105    │          │          │          │   P2💎   │
-120 ───┼──────────┼──────────┼──────────┼──────────┼───
-125    │       S5 ✦ (y=6)   │          │          │
        │          │          │          │          │
-140    │          │          │          │    L o   │
-165    │          │       P3💎(y=-6)   │          │
-180    │        L o          │       L o           │
-200 ───┼──────────┼──────────🌀─────────┼──────────┼───
        │          │       (y=5)         │          │
```

---

## Small Asteroid Distribution (not shown on maps above)

### START ZONE (z = 0 to -40): **0 small asteroids**
- Completely clear for learning

### EARLY MID (z = -40 to -80): **12 small asteroids**
```
z=-45: (-35, 8)
z=-50: (35, -8)
z=-52: (-12, 25), (12, -25)
z=-55: (-28, -5)
z=-58: (12, -25)
z=-60: (28, 5)
z=-65: (-20, 2)
z=-68: (-40, 12)
z=-70: (20, -2)
z=-72: (40, -12)
z=-75: (-18, -8)
z=-78: (18, 8)
```
First asteroid appears at z=-45, giving 2.33 seconds reaction time

### LATE MID (z = -80 to -140): **20 small asteroids**
Moderate density, varied trajectories

### END ZONE (z = -140 to -200): **17 small asteroids**
Dense field, many targets near portal approach

---

## Timeline (at 15 units/sec scroll)

```
Time    Z-Position    Event
──────────────────────────────────────────────────────
0.00s   z = -10      Ship spawns, START ZONE begins
0.67s   z = -20      ✦ SHARD 1 collected (center)
1.67s   z = -35      ✦ SHARD 2 collected (right)
2.33s   z = -45      First small asteroid appears
2.67s   z = -50      ✦ SHARD 3 collected (left-down)
                     [Player now has 3/5 shards]
4.00s   z = -60      MID ZONE begins
4.33s   z = -65      💎 Power-up 1 (Jet Booster)
5.33s   z = -90      ✦ SHARD 4 collected (right)
7.00s   z = -105     💎 Power-up 2 (Mini Burst)
7.67s   z = -125     ✦ SHARD 5 collected (left)
                     [Portal activates! 5/5 shards]
8.00s   z = -120     END ZONE begins
11.00s  z = -165     💎 Power-up 3 (Jet Booster)
13.33s  z = -200     🌀 Fly through PORTAL to win!
```

---

## Key Design Features

### 1. First 3 Shards (z = -20, -35, -50)
- **Formation**: Center → Right → Left-down
- **Purpose**: Educational guidance
  - Shard 1: Tests if player is alive (impossible to miss)
  - Shard 2: Teaches X-axis movement (move right)
  - Shard 3: Teaches X+Y movement (diagonal)
- **Spacing**: 15 units apart = 1 second between each
- **Safety**: No obstacles nearby, center lane clear

### 2. Safe Start Zone (z = 0 to -40)
- **Zero small asteroids** for learning period
- Large asteroids only at edges (X ±38+, Y ±25+)
- Center lane (X ∈ [-15, 15]) completely clear
- First 2.5 seconds is 100% safe

### 3. Progressive Difficulty
```
Zone        Large  Small  Collectibles  Difficulty
──────────────────────────────────────────────────────
Start       8      0      3 shards      ★☆☆☆☆ (Learning)
Mid         10     32     2 shards +1PU ★★★☆☆ (Practice)
End         6      17     2 power-ups   ★★★★☆ (Challenge)
```

### 4. Fair Reaction Times
- **Small asteroids**: First at z=-45 (2.33s reaction)
- **Collectibles**: Visible from far away (glow + bob)
- **Large asteroids**: Slow drift velocities (0.5-1.6 units/sec)
- **No instant threats**: Nothing overlaps ship start position

---

## Player Experience Flow

### Act 1: Tutorial (0-4 seconds)
1. See first shard glowing ahead → fly straight → collect
2. See second shard on right → move right → collect
3. See third shard on left-down → move diagonal → collect
4. **Learn**: Movement, collection, objective (3/5 shards)

### Act 2: Practice (4-8 seconds)
5. First small asteroids appear → shoot them
6. Navigate to shard 4 on right side
7. Collect optional Jet Booster power-up (risk/reward)
8. Navigate to shard 5 on left side
9. **Portal activates!** (5/5 shards collected)

### Act 3: Challenge (8-13 seconds)
10. Dense asteroid field with many shooting targets
11. Optional Mini Burst power-up near top
12. Optional second Jet Booster in dangerous position
13. Navigate through obstacles to portal at end
14. **Fly through portal → Level complete!**

---

## Tuning Guide

### Make it Easier
```cpp
// Slower scroll speed = more reaction time
const float L1_SCROLL_SPEED = 12.0f;  // Default: 15.0f

// Move first small asteroid further away
// z = -45 → z = -60 (4 seconds instead of 2.33)
smallAsteroids.emplace_back(glm::vec3(-35.0f, 8.0f, -60.0f), ...);

// Move first 3 shards closer together
// Current: z = -20, -35, -50 (1 sec apart)
// Easier: z = -18, -25, -32 (0.5 sec apart)
```

### Make it Harder
```cpp
// Faster scroll speed = less reaction time
const float L1_SCROLL_SPEED = 20.0f;

// Add small asteroids to start zone
// Currently starts at z = -45
// Harder: start at z = -30 (instant challenge)

// Move required shards to riskier positions
// Current: safe center lanes
// Harder: near large asteroids or edges
```

---

**Status**: ✅ Level 1 placement complete and tested  
**Design Goal**: First-time players should collect 3 shards naturally while learning controls  
**Success Metric**: No deaths in first 4 seconds, clear visual path to objectives
