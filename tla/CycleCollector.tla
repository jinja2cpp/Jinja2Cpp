--------------------------- MODULE CycleCollector ---------------------------
(***************************************************************************)
(* The per-render cycle collector of src/cycle_collector.{h,cpp} (tasks    *)
(* 0110, 0251, 0283, 0331): NoteStore, the prune of candidates nothing     *)
(* else holds (DropUnshared), and at the render's end the walk             *)
(* (CollectOnce): Traverse from the candidates, the count of the           *)
(* references each object gets from the walk, the comparison with its      *)
(* reference count, the closure of what is held from outside, and Clear of *)
(* the rest while the collector holds it. Task 0338, step E: a spec of its *)
(* own, with an interface contract to tla/CellProtocol.tla (README,        *)
(* "Cycle collector").                                                     *)
(*                                                                         *)
(* Abstractions:                                                           *)
(*  - The heap is a multigraph of reference-counted objects (ValueObject). *)
(*    edge[o][p] is how many references o holds to p; hold[h][o] how many *)
(*    a holder outside the heap has: the render's scopes ("render") or     *)
(*    any other thread, the host included ("host"). An object's count is   *)
(*    the sum of all of them, plus the collector's own references (its     *)
(*    candidates and, while it clears, the garbage).                       *)
(*  - Releasing the last reference destroys the object and releases what   *)
(*    it held, recursively, in the same step (Settle): a destructor runs   *)
(*    on the thread that dropped the reference.                            *)
(*  - Kinds of object, by what Traverse shows and what Clear does:         *)
(*      "list"    a list, dict or namespace the template made, owning its  *)
(*                storage alone: Traverse shows every item, Clear empties  *)
(*                it (the only kind Clear changes);                        *)
(*      "cell"    a frozen BorrowedCell: Traverse shows its storage;       *)
(*      "adapter" a value of the host's data whose owning link reads a     *)
(*                frozen cell: Traverse shows the link (TraverseFrozen);   *)
(*      "opaque"  an object whose references Traverse does not show (a     *)
(*                std::function capture, storage another accessor shares). *)
(*    Objects Traverse skips entirely (strings, numbers) hold nothing and  *)
(*    are not modelled.                                                    *)
(*  - A container's storage is the object itself: a list's items are its   *)
(*    edges. Storage shared by two accessors (0348) is the opaque kind.    *)
(*  - Thread "host" is every thread but the render's. What it may do is    *)
(*    the host contract of docs/design/borrowed-cells-memory-order.md: it  *)
(*    copies and drops the references it holds at any time (H3b) and       *)
(*    writes nothing the render made. Its reads make no new reference to   *)
(*    an object of the heap (contract H6, README): HostReadsShare takes    *)
(*    that out, and the walk then empties a list the host still reaches.   *)
(*  - Out of memory is not modelled (step D of 0338): the walk's retry     *)
(*    and the note-before-store order of 0283 are in the C++ only.         *)
(***************************************************************************)
EXTENDS Integers, FiniteSets, TLC

CONSTANTS
    Objs,          \* the object identities; an identity is never reused
    Kinds,         \* the kinds a new object may have
    MaxRef,        \* at most this many references from one holder to one object
    MaxSteps,      \* steps of the render (and of the earlier one) before its end
    MaxHost,       \* steps of the host thread
    Prior,         \* TRUE: an earlier render made and published values first
    HostInCollect, \* TRUE: other threads step while the walk runs
    HostReadsShare, \* TRUE takes contract H6 out: a host read keeps a reference into the heap
    \* The design of the collector, one flag per part; FALSE takes the part out
    FixNoteStore,     \* a store into a container notes it (NoteStore)
    FixCountCands,    \* the walk counts the collector's reference to a candidate (visit)
    FixPropagate,     \* what an outside-held object reaches is held too (markReachable)
    FixHoldGarbage,   \* the garbage is held while it is emptied
    FixPruneUnique    \* the prune drops a candidate only when its count is 1

Holders == {"render", "host"}
Refs == 0..MaxRef

VARIABLES
    st,       \* [Objs -> {"none", "live", "dead"}]
    kind,     \* [Objs -> Kinds \cup {"none"}]
    pub,      \* [Objs -> BOOLEAN]: published by a completed render, read-only from then on
    edge,     \* [Objs -> [Objs -> Refs]]
    hold,     \* [Holders -> [Objs -> Refs]]
    cand,     \* the candidates: the collector holds one reference to each
    ghold,    \* the garbage the collector holds while it clears
    phase,    \* "prior", "render", then the collector: "prune", "walk", "mark",
              \* "propagate", "hold", "clear", "release", "done"
    table,    \* the walk's nodes (raw pointers in the C++)
    internal, \* [Objs -> Nat]: references the walk found to each node
    unread,   \* nodes whose count the mark has not read yet
    ext,      \* nodes found held from outside
    garbage,  \* nodes the walk found unreachable
    steps, hsteps,
    flag      \* [uaf, clearLive]: a defect seen, kept to the end of the behaviour

vars == <<st, kind, pub, edge, hold, cand, ghold, phase, table, internal, unread, ext, garbage,
          steps, hsteps, flag>>

Live == {o \in Objs : st[o] = "live"}

RECURSIVE SumOver(_, _)
SumOver(f, S) == IF S = {} THEN 0 ELSE LET x == CHOOSE x \in S : TRUE IN f[x] + SumOver(f, S \ {x})

\* The reference count, ValueObject::use_count()
CountIn(e, h, c, g, o) ==
    SumOver([p \in Objs |-> e[p][o]], Objs) + h["render"][o] + h["host"][o]
        + (IF o \in c THEN 1 ELSE 0) + (IF o \in g THEN 1 ELSE 0)
Count(o) == CountIn(edge, hold, cand, ghold, o)

\* Destroys what nobody references any more, and what that held, to a fixed point
RECURSIVE Settle(_, _, _, _, _)
Settle(s, e, h, c, g) ==
    LET z == {o \in Objs : s[o] = "live" /\ CountIn(e, h, c, g, o) = 0}
    IN IF z = {} THEN <<s, e>>
       ELSE Settle([o \in Objs |-> IF o \in z THEN "dead" ELSE s[o]],
                   [o \in Objs |-> IF o \in z THEN [p \in Objs |-> 0] ELSE e[o]], h, c, g)

\* A step that drops references: the new hold/cand/ghold, edges e; destructors run
Release(e, h, c, g) ==
    LET r == Settle(st, e, h, c, g)
    IN /\ st' = r[1] /\ edge' = r[2] /\ hold' = h /\ cand' = c /\ ghold' = g

\* What Traverse shows: the references the collector may count
Shows(o, p) == IF kind[o] = "opaque" THEN 0 ELSE edge[o][p]
Succ(o) == {p \in Objs : Shows(o, p) > 0}
RECURSIVE CloseFrom(_, _)
CloseFrom(S, Within) ==
    LET N == S \cup UNION {Succ(o) \cap Within : o \in S} IN IF N = S THEN S ELSE CloseFrom(N, Within)

\* Everything a holder reaches, through every reference, shown or not
RECURSIVE Reach(_)
Reach(S) == LET N == S \cup UNION {{p \in Objs : edge[o][p] > 0} : o \in S}
            IN IF N = S THEN S ELSE Reach(N)
HeldBy(h) == Reach({o \in Objs : hold[h][o] > 0})
\* What the host or a later read can still use: the roots a render's end leaves
LiveRoots == HeldBy("host")

SetHold(h, o, n) == [hold EXCEPT ![h][o] = n]

Init ==
    /\ st = [o \in Objs |-> "none"]
    /\ kind = [o \in Objs |-> "none"]
    /\ pub = [o \in Objs |-> FALSE]
    /\ edge = [o \in Objs |-> [p \in Objs |-> 0]]
    /\ hold = [h \in Holders |-> [o \in Objs |-> 0]]
    /\ cand = {} /\ ghold = {}
    /\ phase = IF Prior THEN "prior" ELSE "render"
    /\ table = {} /\ internal = [o \in Objs |-> 0] /\ unread = {} /\ ext = {} /\ garbage = {}
    /\ steps = 0 /\ hsteps = 0
    /\ flag = [uaf |-> FALSE, clearLive |-> FALSE]

Mutating == phase \in {"prior", "render"}
CollectorVars == <<table, internal, unread, ext, garbage>>

(***************************************************************************)
(* The render (and, with Prior, the earlier render before it)               *)
(***************************************************************************)
RStep(A) == Mutating /\ steps < MaxSteps /\ A /\ steps' = steps + 1
Held(o) == st[o] = "live" /\ hold["render"][o] > 0

\* A new value. A cell comes with its storage, an adapter with its link (MakeKept)
New(o, k) ==
    /\ st[o] = "none" /\ k \in Kinds \ {"cell", "adapter"}
    /\ st' = [st EXCEPT ![o] = "live"] /\ kind' = [kind EXCEPT ![o] = k]
    /\ hold' = SetHold("render", o, 1)
    /\ UNCHANGED <<pub, edge, cand, ghold>>

\* An object Traverse cannot see into made with a reference to v (a closure capturing v)
Capture(o, v) ==
    /\ st[o] = "none" /\ "opaque" \in Kinds /\ Held(v)
    /\ st' = [st EXCEPT ![o] = "live"] /\ kind' = [kind EXCEPT ![o] = "opaque"]
    /\ edge' = [edge EXCEPT ![o][v] = 1]
    /\ hold' = SetHold("render", o, 1)
    /\ UNCHANGED <<pub, cand, ghold>>

\* The host's data l written by the render: its cell c, frozen at Finish, and the adapter a
\* whose owning link reads it. BorrowedCells::Finish notes the cell (NoteFrozen); the model
\* notes it when it is made, which the walk cannot tell apart
MakeKept(c, a, l) ==
    /\ {"cell", "adapter"} \subseteq Kinds
    /\ c # a /\ st[c] = "none" /\ st[a] = "none" /\ Held(l) /\ kind[l] = "list" /\ ~pub[l]
    /\ st' = [st EXCEPT ![c] = "live", ![a] = "live"]
    /\ kind' = [kind EXCEPT ![c] = "cell", ![a] = "adapter"]
    /\ edge' = [edge EXCEPT ![c][l] = 1, ![a][c] = 1]
    /\ hold' = [hold EXCEPT !["render"][a] = 1, !["render"][l] = @ - 1]
    /\ cand' = IF phase = "render" THEN cand \cup {c} ELSE cand
    /\ UNCHANGED <<pub, ghold>>

\* c.append(v), ns.x = v: NoteStore(c) first (0283), then the store
Store(c, v) ==
    /\ Held(c) /\ Held(v) /\ kind[c] = "list" /\ ~pub[c] /\ edge[c][v] < MaxRef
    /\ edge' = [edge EXCEPT ![c][v] = @ + 1]
    /\ cand' = IF FixNoteStore /\ phase = "render" THEN cand \cup {c} ELSE cand
    /\ UNCHANGED <<st, kind, pub, hold, ghold>>

\* c.pop(), del ns.x
Remove(c, v) ==
    /\ Held(c) /\ kind[c] = "list" /\ ~pub[c] /\ edge[c][v] > 0
    /\ Release([edge EXCEPT ![c][v] = @ - 1], hold, cand, ghold)
    /\ UNCHANGED <<kind, pub>>

\* Reading an item or attribute into a variable: any kind, the render holds what it reads
Load(c, v) ==
    /\ Held(c) /\ edge[c][v] > 0 /\ hold["render"][v] < MaxRef
    /\ hold' = SetHold("render", v, hold["render"][v] + 1)
    /\ UNCHANGED <<st, kind, pub, edge, cand, ghold>>

\* A variable goes out of scope
Drop(o) ==
    /\ Held(o)
    /\ Release(edge, SetHold("render", o, hold["render"][o] - 1), cand, ghold)
    /\ UNCHANGED <<kind, pub>>

\* keep(x): a callback hands the host a value of the render (a kept value)
Keep(o) ==
    /\ phase = "render" /\ Held(o) /\ hold["host"][o] < MaxRef
    /\ hold' = SetHold("host", o, hold["host"][o] + 1)
    /\ UNCHANGED <<st, kind, pub, edge, cand, ghold>>

Prunable(o) == IF FixPruneUnique THEN Count(o) = 1 ELSE Count(o) <= 2

\* CycleCollector::Add, past its threshold: DropUnshared lets go of a candidate nothing else holds
PruneOne(o) ==
    /\ o \in cand /\ Prunable(o)
    /\ Release(edge, hold, cand \ {o}, ghold)
    /\ UNCHANGED <<kind, pub>>

\* The earlier render ends: what it made is published, and the values it kept are now both the
\* host's and the new render's arguments; the rest of its scopes go. Its collector ran (not
\* modelled): it noted nothing the new one walks from
Publish ==
    /\ phase = "prior" /\ steps > 0
    /\ LET kept == [o \in Objs |-> IF kind[o] = "adapter" THEN hold["render"][o] ELSE 0]
       IN Release(edge, [h \in Holders |-> kept], cand, ghold)
    /\ pub' = [o \in Objs |-> st[o] = "live"]
    /\ phase' = "render"
    /\ UNCHANGED <<kind, steps, hsteps, flag, CollectorVars>>

Render ==
    \/ \E o \in Objs, k \in Kinds : RStep(New(o, k))
    \/ \E o, v \in Objs : RStep(Capture(o, v))
    \/ \E c, a, l \in Objs : RStep(MakeKept(c, a, l))
    \/ \E c, v \in Objs : RStep(Store(c, v)) \/ RStep(Remove(c, v)) \/ RStep(Load(c, v))
    \/ \E o \in Objs : RStep(Drop(o)) \/ RStep(Keep(o)) \/ RStep(PruneOne(o))

RenderStep ==
    /\ Render
    /\ UNCHANGED <<phase, hsteps, flag, CollectorVars>>

(***************************************************************************)
(* Other threads (the host): they copy and drop the references they hold,  *)
(* at any time (H3b), the walk included                                     *)
(***************************************************************************)
HostOK == /\ hsteps < MaxHost /\ phase \notin {"prior", "done"}
          /\ (HostInCollect \/ phase = "render")
HStep(A) == HostOK /\ A /\ hsteps' = hsteps + 1

HostHeld(o) == st[o] = "live" /\ hold["host"][o] > 0

HostCopy(o) ==
    /\ HostHeld(o) /\ hold["host"][o] < MaxRef
    /\ hold' = SetHold("host", o, hold["host"][o] + 1)
    /\ UNCHANGED <<st, edge, cand, ghold>>

HostDrop(o) ==
    /\ HostHeld(o)
    /\ Release(edge, SetHold("host", o, hold["host"][o] - 1), cand, ghold)

\* H6 taken out (HostReadsShare): what a read through o would reach, if a read handed out
\* references into the heap: a kept value's link to its cell, and the items of what a
\* completed render published
RECURSIVE HostReach(_)
HostReach(S) ==
    LET N == S \cup UNION {{p \in Objs : edge[o][p] > 0} : o \in {o \in S : kind[o] = "adapter" \/ pub[o]}}
    IN IF N = S THEN S ELSE HostReach(N)

\* A host read that keeps a reference to p, reached through o: the step contract H6 rules out.
\* In the C++ a read of a kept value is a new view that keeps the kept value's owner
\* (ExportItem), and a copy shares the accessor (GenericList), so the host never gets a
\* reference to an object the walk can reach that it did not hold before
HostDerive(o, p) ==
    /\ HostReadsShare
    /\ HostHeld(o) /\ p \in HostReach({o}) \ {o} /\ hold["host"][p] < MaxRef
    /\ hold' = SetHold("host", p, hold["host"][p] + 1)
    /\ UNCHANGED <<st, edge, cand, ghold>>

HostStep ==
    /\ \/ \E o \in Objs : HStep(HostCopy(o)) \/ HStep(HostDrop(o))
       \/ \E o, p \in Objs : HStep(HostDerive(o, p))
    /\ UNCHANGED <<kind, pub, phase, steps, flag, CollectorVars>>

(***************************************************************************)
(* The collector at the render's end (~CycleCollector, Finish, Collect)     *)
(***************************************************************************)
CStep(from, to) == phase = from /\ phase' = to

\* The render's scopes are gone (the collector is made first, so it ends last)
EndScopes ==
    /\ CStep("render", "prune")
    /\ Release(edge, [hold EXCEPT !["render"] = [o \in Objs |-> 0]], cand, ghold)
    /\ UNCHANGED <<kind, pub, steps, hsteps, flag, CollectorVars>>

\* CollectOnce: `while (DropUnshared())`
Prune ==
    /\ phase = "prune"
    /\ \/ \E o \in cand : PruneOne(o) /\ phase' = phase
       \/ /\ \A o \in cand : ~Prunable(o)
          /\ phase' = "walk"
          /\ UNCHANGED <<st, edge, hold, cand, ghold>>
    /\ UNCHANGED <<kind, pub, steps, hsteps, flag, CollectorVars>>

\* visit + Traverse from the candidates: each candidate counts the collector's reference,
\* each shown reference between nodes counts once. The heap the walk reads does not change
\* under it: other threads write nothing the render made, and what the walk reaches the
\* candidates hold
Walk ==
    /\ phase = "walk"
    /\ LET T == CloseFrom(cand, Objs)
       IN /\ phase' = IF T = {} THEN "done" ELSE "mark"
          /\ table' = T
          /\ internal' = [o \in Objs |->
                IF o \in T THEN SumOver([p \in Objs |-> IF p \in T THEN Shows(p, o) ELSE 0], Objs)
                                + (IF o \in cand /\ FixCountCands THEN 1 ELSE 0)
                ELSE 0]
          /\ unread' = T
          /\ flag' = [flag EXCEPT !.uaf = @ \/ \E o \in T : st[o] # "live"]
    /\ ext' = {}
    /\ UNCHANGED <<st, kind, pub, edge, hold, cand, ghold, garbage, steps, hsteps>>

\* `node->object->use_count() > node->internalRefs`, one node at a time, in any order (the C++
\* reads them in the walk's order, one of these): other threads may step between two reads
MarkOne(o) ==
    /\ phase = "mark" /\ o \in unread
    /\ ext' = IF Count(o) > internal[o] THEN ext \cup {o} ELSE ext
    /\ unread' = unread \ {o}
    /\ phase' = IF unread = {o} THEN "propagate" ELSE phase
    /\ flag' = [flag EXCEPT !.uaf = @ \/ st[o] # "live"]
    /\ UNCHANGED <<st, kind, pub, edge, hold, cand, ghold, table, internal, garbage, steps, hsteps>>

\* markReachable from each outside-held node; the garbage is the rest
Propagate ==
    /\ CStep("propagate", "hold")
    /\ garbage' = table \ (IF FixPropagate THEN CloseFrom(ext, table) ELSE ext)
    /\ UNCHANGED <<st, kind, pub, edge, hold, cand, ghold, table, internal, unread, ext,
                   steps, hsteps, flag>>

\* `garbage.emplace_back(...)`, then `candidates.clear()`
HoldGarbage ==
    /\ CStep("hold", "clear")
    /\ LET g == IF FixHoldGarbage THEN garbage ELSE {} IN Release(edge, hold, {}, g)
    /\ UNCHANGED <<kind, pub, table, internal, unread, ext, garbage, steps, hsteps, flag>>

\* `object->Clear()`: only a list changes. Emptying a list that something live still reaches
\* takes its items from under that holder; touching a destroyed object is a use-after-free
ClearOne(o) ==
    /\ phase = "clear" /\ o \in garbage
    /\ garbage' = garbage \ {o}
    /\ phase' = IF garbage = {o} THEN "release" ELSE phase
    /\ flag' = [flag EXCEPT !.uaf = @ \/ st[o] # "live",
                            !.clearLive = @ \/ (kind[o] = "list" /\ st[o] = "live"
                                                /\ o \in LiveRoots /\ \E p \in Objs : edge[o][p] > 0)]
    /\ IF kind[o] = "list" /\ st[o] = "live"
       THEN Release([edge EXCEPT ![o] = [p \in Objs |-> 0]], hold, cand, ghold)
       ELSE UNCHANGED <<st, edge, hold, cand, ghold>>
    /\ UNCHANGED <<kind, pub, table, internal, unread, ext, steps, hsteps>>

\* The garbage is let go of: all of it goes
ReleaseGarbage ==
    /\ \/ CStep("clear", "done") /\ garbage = {}
       \/ CStep("release", "done")
    /\ Release(edge, hold, cand, {})
    /\ UNCHANGED <<kind, pub, table, internal, unread, ext, garbage, steps, hsteps, flag>>

Collector ==
    \/ EndScopes \/ Prune \/ Walk \/ (\E o \in Objs : MarkOne(o)) \/ Propagate \/ HoldGarbage
    \/ (\E o \in Objs : ClearOne(o)) \/ ReleaseGarbage

Next == RenderStep \/ HostStep \/ Publish \/ Collector

Spec == Init /\ [][Next]_vars

(***************************************************************************)
(* Invariants                                                               *)
(***************************************************************************)
\* The collector never empties a container something live still reaches (G1)
NoClearLive == ~flag.clearLive
\* The collector never reads or clears a destroyed object (G2)
NoUseAfterFree == ~flag.uaf
\* Reference counting itself: nothing live refers to a destroyed object, and every live object
\* is referenced
CountsSound ==
    \A o \in Objs : /\ st[o] = "live" => Count(o) > 0
                    /\ st[o] # "live" => (\A p \in Objs : edge[o][p] = 0) /\ hold["render"][o] = 0
                                         /\ hold["host"][o] = 0 /\ o \notin cand \cup ghold
\* After the walk, every object still alive is one the host reaches (G3): every cycle the render
\* left is gone. Holds when Traverse sees every reference and the host stands still during the
\* walk (the runs that check it)
Complete == phase = "done" => \A o \in Live : o \in LiveRoots

TypeOK ==
    /\ st \in [Objs -> {"none", "live", "dead"}]
    /\ edge \in [Objs -> [Objs -> Refs]]
    /\ cand \subseteq Objs /\ ghold \subseteq Objs
=============================================================================
