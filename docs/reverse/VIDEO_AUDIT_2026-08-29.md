# Playable Screencast Audit — 2026-08-29

## Evidence

User gameplay capture: `Screencast from 2026-08-29 23-49-17.webm`, approximately 70 seconds of the current Linux fidelity host.

This audit distinguishes **confirmed reconstruction defects** from presentation that merely looks suspicious. Visual oddity alone is not sufficient reason to diverge from recovered executable behavior.

## Confirmed defects

### Trajectory kill-effect ownership

The playable host previously received only `trajectory_actors_destroyed` as a count. Presentation then searched for actors that changed from active to inactive and assigned explosions to the first N candidates.

That inference is invalid because natural path completion can make an unrelated trajectory actor inactive in the same logical update as a weapon kill. The result is visible as the wrong ship exploding while the actually destroyed ship appears to vanish.

**Fix:** trajectory collision results and `GameSessionTickResult` now carry bounded exact destruction events containing group index, actor index, pre-retirement X/Y, sprite dimensions, and destruction-burst count. The host spawns the effect from those exact events and no longer infers kills from inactivity transitions.

Regression coverage verifies exact identity and geometry at both collision and whole-session boundaries.

### Weapon-help overlay lifetime

The host-only startup Probe/Stinger reminder stayed visible until the first special launch. In the capture it obscures the upper playfield for most of the run.

**Fix:** the generic reminder now auto-hides after roughly five seconds unless explicitly pinned with F4. A smaller contextual DRONE-objective warning takes over only during the visible unresolved approach/hover interval and disappears when the blue Probe attaches. This changes no simulation behavior.

## Proven original behavior — do not "fix" without contrary evidence

### DRONE detonation fireworks

The large late-run explosion field looks excessive, but the recovered Win32 detonation routine requests four center-scatter plus four radial explosions on each eligible effect tick and runs a long destruction timeline. The current host therefore must not reduce this sequence solely because the capture looks chaotic. Exact original framebuffer comparison remains a Phase-6/7 validation task.

### `BAD1.JBA` failure message

The white result screen reading "Nice fireworks! But the idea is to disarm the Drones, not light 'em up!" matches the supplied original `BAD1.JBA`. It is the authentic failure result after detonating a DRONE, not a reconstruction crash or placeholder.

### Red segmented/Frisbee formations

The red bead-like formations seen in the capture correspond to recovered Frisbee sprite content. Their presence alone is not evidence of a rendering artifact.

## Still under investigation

- remaining true trajectory/AI disappearances after exact kill-event ownership is fixed;
- breakaway timing, attack cadence, and offscreen retirement during long sessions;
- Lid/Top boss motion/collision/presentation timing against reference execution;
- exact detonation explosion/debris compositor parity rather than visual plausibility;
- special-weapon target/reticle behavior under dense ordinary-enemy and boss overlap;
- mission/objective sequencing through repeated disarm/detonation outcomes;
- audio ownership and interruption under boss + DRONE + destruction overlap.

These require trace/reference evidence before behavior changes.

## Shareware versus complete game

The current playable campaign is intentionally bounded by the supplied shareware evidence. That is a **validation profile**, not the intended final scope of the project.

The recovered shareware Win32 executable proves a six-DRONE campaign structure and contains dormant registered-progression branches, including Isle/House/Night scenery transitions, additional boss dispatch slots, and the all-six-disarmed Mothership relation. However, the supplied evidence set does not contain all registered-only scenery/boss/content payloads, and the exact retail replacement for the shareware processed-count-2 termination branch is not established from a canonical full executable.

Therefore the complete-game target remains:

1. finish shareware parity so the engine has a trustworthy reference baseline;
2. obtain a lawful full/registered release and hash it as a separate canonical evidence set;
3. diff retail versus shareware executable/content behavior rather than assuming dormant shareware branches are identical;
4. reconstruct registered-only levels, enemies, bosses, scenery, audio, mission cards, progression, and ending behavior;
5. retain shareware as a selectable historical/test profile rather than a hard engine limitation.

Do not invent missing registered assets or silently remove the two-level gate before the retail evidence identifies the correct successor behavior.

## Follow-up: Probe attaches, then BAD1 appears

A subsequent playtest reported a visually successful blue-Probe attachment followed
by `BAD1.JBA`. A new end-to-end live-session regression starts from the real Probe
collision producer and advances with no destructive player input until the objective
settles. It deterministically reaches decoder completion, commits `DroneOutcome::Disarmed`,
and produces the GOOD1 interstitial. This rules out a generic Probe/decode/mission-ledger
fall-through bug.

The recovered collision ordering still allows player rapid missiles or a red Stinger
to destroy the active DRONE after Probe attachment. In the keyboard host this creates
an easy race: a missile fired immediately before attachment may remain in flight and
arrive afterward, making the successful attachment appear to have failed spontaneously.

The Linux playable host therefore now applies a host-only input safety layer on the
attachment tick: pending player rapid missiles are purged and held rapid fire is
latched off until the key is released once. This does **not** change `GameSession` or
the recovered weapon-to-DRONE rules; a deliberate subsequent fire press can still
produce the authentic detonation/BAD path. The host also exposes attached Probe
`DECODING`/`DISARMING`, Probe-knockoff feedback, and exact known BAD causes.
