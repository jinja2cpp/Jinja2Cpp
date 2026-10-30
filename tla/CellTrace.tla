----------------------------- MODULE CellTrace -----------------------------
(***************************************************************************)
(* Trace validation of the borrowed-cell protocol (docs/tasks/0338 step B). *)
(* A build with JINJA2CPP_CELL_TRACE=ON logs the protocol's events          *)
(* (src/cell_trace.h); tla/check.py --trace turns a log into the module     *)
(* TraceLog (Log, OneThread) and runs TLC on this spec. A behaviour of the  *)
(* spec reads the log one event at a time and keeps the protocol's state:   *)
(* the cells, the links of the adapters, the registries and their calls,    *)
(* the values handed to the host in module calls.                           *)
(*                                                                         *)
(* Two kinds of failure:                                                    *)
(*  - no step takes the next event (TLC reports a deadlock): the code did   *)
(*    something the spec has no transition for;                             *)
(*  - an invariant fails: the step exists, but the code's decision or state *)
(*    differs from the protocol's rules (CellRules.tla) or breaks one of    *)
(*    CellProtocol's properties, as far as a trace shows it.                *)
(*                                                                         *)
(* The objects are the log's numbers: a cell, a storage generation and a    *)
(* registry are new objects each time one is made; a link (an adapter's     *)
(* CellLink) and an identity are named by their address, so an address      *)
(* used again names the same link. What only the host knows (reference      *)
(* counts, whether a value it keeps still shares items) is an input: the    *)
(* log says it, and the spec checks only that the choice was one the rules  *)
(* allow.                                                                   *)
(***************************************************************************)
EXTENDS Integers, Sequences, FiniteSets, TLC, CellRules, TraceLog

ATT == -1   \* CellLink::Attached in the log's seen epochs (UINT32_MAX)

VARIABLES
    l,       \* the next event of the log
    cl,      \* cell -> its state
    lk,      \* link -> the cell it is attached to, whether it owns it, the epoch it saw, what it looked up
    rg,      \* registry -> its table, its running calls, its thread
    exr,     \* <<cell, storage>>: a value an ended module call handed to the host shares the storage
    live,    \* the cells in a table (CellCounters::s_live)
    promoted,\* the promotions so far
    bad      \* <<property, event>>: the violations seen

vars == <<l, cl, lk, rg, exr, live, promoted, bad>>

NoLink == [cell |-> 0, own |-> FALSE, seen |-> 0, idn |-> 0, list |-> 0]
EmptyTbl == [i \in {} |-> 0]

Init ==
    /\ l = 1
    /\ cl = [c \in {} |-> 0]
    /\ lk = [a \in {} |-> NoLink]
    /\ rg = [r \in {} |-> 0]
    /\ exr = {}
    /\ live = 0
    /\ promoted = 0
    /\ bad = {}

E == Log[l]

L(a) == IF a \in DOMAIN lk THEN lk[a] ELSE NoLink
SetL(a, r) == (a :> r) @@ lk
SetC(c, r) == (c :> r) @@ cl
SetR(r, v) == (r :> v) @@ rg
IsListOf(c) == cl[c].list
Known(c) == c \in DOMAIN cl
Alive(c) == Known(c) /\ cl[c].alive
Calls(r) == rg[r].calls
RunningCall(r) == IF Calls(r) = <<>> THEN 0 ELSE Calls(r)[Len(Calls(r))].call
RunningCalls(r) == {Calls(r)[i].call : i \in 1..Len(Calls(r))}
TblCells(r) == {rg[r].tbl[i] : i \in DOMAIN rg[r].tbl} \ {0}
Lookup(r, idn, isList) == IF r = 0 \/ r \notin DOMAIN rg THEN 0 ELSE FindResult(rg[r].tbl, idn, isList, IsListOf)
AsSet(s) == {s[i] : i \in DOMAIN s}

\* The registry's state is written by its render's thread, or under the module's lock by the
\* thread whose call runs (CellProtocol's NoCrossThreadWrite)
WriterOk(r, t) ==
    IF rg[r].module THEN Calls(r) # <<>> /\ Calls(r)[Len(Calls(r))].thr = t ELSE rg[r].thr = t

\* Adds the violations whose condition holds
Bad(checks) == bad \cup {<<p, l>> : p \in {k \in DOMAIN checks : checks[k]}}
Check(checks) == bad' = Bad(checks)

Next1(more) == /\ l' = l + 1 /\ more

-----------------------------------------------------------------------------
(* Registries and module calls *)

RegNew ==
    /\ E.ev = "regnew" /\ E.reg \notin DOMAIN rg
    /\ Next1(rg' = SetR(E.reg, [tbl |-> EmptyTbl, calls |-> <<>>, last |-> 0, thr |-> E.t,
                                module |-> FALSE, alive |-> TRUE]))
    /\ UNCHANGED <<cl, lk, exr, live, promoted, bad>>

RegFree ==
    /\ E.ev = "regfree" /\ E.reg \in DOMAIN rg
    /\ Next1(rg' = [rg EXCEPT ![E.reg].alive = FALSE])
    \* Release ran Finish: the table is gone
    /\ Check([TableAgrees |-> TblCells(E.reg) # {}, CallsAgree |-> Calls(E.reg) # <<>>])
    /\ UNCHANGED <<cl, lk, exr, live, promoted>>

BeginCall ==
    /\ E.ev = "begin" /\ E.reg \in DOMAIN rg
    /\ LET r == E.reg IN
       /\ Next1(rg' = [rg EXCEPT ![r].calls = Append(@, [call |-> E.call, outer |-> E.outer, thr |-> E.t]),
                                 ![r].last = E.call, ![r].module = TRUE])
       \* A call made from a host callable in a call nests in it, on the same thread (the lock)
       /\ Check([CallsAgree |-> E.outer # RunningCall(r) \/ E.call # rg[r].last + 1,
                 NoCrossThreadWrite |-> Calls(r) # <<>> /\ ~WriterOk(r, E.t)])
    /\ UNCHANGED <<cl, lk, exr, live, promoted>>

EndCall ==
    /\ E.ev = "end" /\ E.reg \in DOMAIN rg /\ Calls(E.reg) # <<>>
    /\ LET r == E.reg
           top == Calls(r)[Len(Calls(r))]
       IN /\ Next1(rg' = [rg EXCEPT ![r].calls = SubSeq(@, 1, Len(@) - 1)])
          /\ Check([CallsAgree |-> E.call # top.call \/ E.outer # top.outer,
                    NoCrossThreadWrite |-> top.thr # E.t])
    /\ UNCHANGED <<cl, lk, exr, live, promoted>>

-----------------------------------------------------------------------------
(* Lookups: CellLink::Get, Find, Link, Unlink, LeaveSnapshot, Inherit *)

\* A read that skipped the lookup: the adapter saw the epoch it has now. It reads the host's
\* data, so the registry running must have no cell of it (CellProtocol's AliasesAgree)
Skip ==
    /\ E.ev = "skip"
    /\ LET A == L(E.a)
           consistent == IF E.a \in DOMAIN lk THEN A.seen = E.e ELSE E.e = 0
       IN Check([EpochAgrees |-> ~consistent \/ (OneThread /\ E.e = 0 /\ promoted > 0),
                 AliasesAgree |-> A.cell = 0 /\ A.idn # 0 /\ Lookup(E.cur, A.idn, A.list) # 0])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

\* A read through an attached adapter: its cell, without a lookup. Unless frozen, the cell is of
\* the registry running (CellProtocol's NoForeignCell)
Hit ==
    /\ E.ev = "hit" /\ Known(E.c)
    /\ Check([LinksAgree |-> L(E.a).cell # E.c,
              NoUseAfterFree |-> ~cl[E.c].alive,
              NoForeignCell |-> ~cl[E.c].frozen /\ cl[E.c].reg # E.cur])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

Find ==
    /\ E.ev = "find"
    /\ Check([LinksAgree |-> L(E.a).cell # 0,
              TableAgrees |-> E.hit # Lookup(E.cur, E.idn, E.list)])
    /\ Next1(lk' = SetL(E.a, [cell |-> 0, own |-> FALSE, seen |-> E.e, idn |-> E.idn, list |-> E.list]))
    /\ UNCHANGED <<cl, rg, exr, live, promoted>>

Link ==
    /\ E.ev = "link" /\ Known(E.c)
    /\ LET c == E.c IN
       /\ Check([LinksAgree |-> L(E.a).cell # 0,
                 NoUseAfterFree |-> ~cl[c].alive,
                 FrozenUnwritten |-> cl[c].frozen,
                 NoCrossThreadWrite |-> ~WriterOk(cl[c].reg, E.t)])
       /\ Next1(/\ lk' = SetL(E.a, [L(E.a) EXCEPT !.cell = c, !.own = LinkOwns(E.held = 1), !.seen = ATT])
                /\ cl' = [cl EXCEPT ![c].links = @ \cup {E.a},
                                    ![c].held = IF E.held = 1 THEN @ \cup {E.a} ELSE @])
    /\ UNCHANGED <<rg, exr, live, promoted>>

\* Link could not grow the cell's list: the adapter stays detached and looks again
LinkFail ==
    /\ E.ev = "linkfail"
    /\ Next1(lk' = SetL(E.a, [L(E.a) EXCEPT !.seen = 0]))
    /\ UNCHANGED <<cl, rg, exr, live, promoted, bad>>

\* An owning link lets go of its reference, on any thread: the count is atomic and the cell does
\* not list the link (0340). A global's link edits the live cell's list: on the cell's writer only
Unlink ==
    /\ E.ev = "unlink" /\ Known(E.c)
    /\ LET c == E.c IN
       /\ Check([LinksAgree |-> L(E.a).cell # c \/ (E.owns = 1) # L(E.a).own
                                \/ (~cl[c].frozen /\ E.a \notin cl[c].links)
                                \/ (E.owns = 0 /\ (cl[c].frozen \/ E.a \notin cl[c].held)),
                 NoUseAfterFree |-> ~cl[c].alive,
                 NoCrossThreadWrite |-> E.owns = 0 /\ ~WriterOk(cl[c].reg, E.t)])
       /\ Next1(/\ lk' = SetL(E.a, [L(E.a) EXCEPT !.cell = 0, !.own = FALSE, !.seen = 0])
                /\ cl' = [cl EXCEPT ![c].links = @ \ {E.a}, ![c].held = @ \ {E.a}])
    /\ UNCHANGED <<rg, exr, live, promoted>>

\* The snapshot of the globals let go of the block an item's adapter is in: its listed link moves
\* out of the live cell's list and becomes a reference to the cell (0346), on the cell's writer
Leave ==
    /\ E.ev = "leave" /\ Known(E.c)
    /\ LET c == E.c IN
       /\ Check([LinksAgree |-> L(E.a).cell # c \/ L(E.a).own \/ E.a \notin cl[c].held,
                 NoUseAfterFree |-> ~cl[c].alive,
                 FrozenUnwritten |-> cl[c].frozen,
                 NoCrossThreadWrite |-> ~WriterOk(cl[c].reg, E.t)])
       /\ Next1(/\ lk' = SetL(E.a, [L(E.a) EXCEPT !.own = TRUE])
                /\ cl' = [cl EXCEPT ![c].held = @ \ {E.a}])
    /\ UNCHANGED <<rg, exr, live, promoted>>

\* A copy of an adapter that reads a frozen cell reads it too
Inherit ==
    /\ E.ev = "inherit" /\ Known(E.c)
    /\ Check([LinksAgree |-> L(E.from).cell # E.c \/ ~L(E.from).own \/ L(E.a).cell # 0,
              FrozenUnwritten |-> ~cl[E.c].frozen,
              NoUseAfterFree |-> ~cl[E.c].alive])
    /\ Next1(lk' = SetL(E.a, [L(E.from) EXCEPT !.seen = ATT]))
    /\ UNCHANGED <<cl, rg, exr, live, promoted>>

\* An adapter went while attached: a dangling entry in a live cell's links, or a frozen cell's
\* reference that nothing lets go of
DieAttached ==
    /\ E.ev = "dieatt"
    /\ Check([NoDanglingLink |-> TRUE])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

-----------------------------------------------------------------------------
(* Promotion: CellLink::Adopt *)

AdoptNo ==
    /\ E.ev = "adoptno"
    /\ Check([LinksAgree |-> E.why = 1 /\ (L(E.a).cell # E.c \/ ~Known(E.c) \/ ~cl[E.c].frozen),
              TableAgrees |-> E.why = 4 /\ (~Known(E.c) \/ E.c \notin TblCells(E.cur))])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

\* A cell is made (BorrowedCell's constructor): Adopt then puts it in a table, or drops it when
\* another adapter made the data's cell first
CellNew ==
    /\ E.ev = "cellnew" /\ E.c \notin DOMAIN cl
    /\ Next1(cl' = SetC(E.c, [idn |-> 0, list |-> 0, reg |-> 0, stor |-> 0, exp |-> 0, frozen |-> FALSE,
                              links |-> {}, held |-> {}, inTable |-> FALSE, src |-> 0, owned |-> FALSE,
                              alive |-> TRUE]))
    /\ UNCHANGED <<lk, rg, exr, live, promoted, bad>>

Adopt ==
    /\ E.ev = "adopt" /\ E.reg \in DOMAIN rg
    /\ LET r == E.reg IN
       IF E.new = 1
       THEN /\ Known(E.c) /\ cl[E.c].reg = 0
            /\ Check([NoUseAfterFree |-> ~cl[E.c].alive,
                      TableAgrees |-> E.reg # E.cur \/ Lookup(r, E.idn, E.list) # 0
                                       \/ (E.idn \in DOMAIN rg[r].tbl /\ rg[r].tbl[E.idn] # 0),
                      LinksAgree |-> L(E.a).cell # 0,
                      NoCrossThreadWrite |-> ~WriterOk(r, E.t)])
            /\ Next1(/\ cl' = SetC(E.c, [idn |-> E.idn, list |-> E.list, reg |-> r, stor |-> E.g,
                                         exp |-> 0, frozen |-> FALSE, links |-> {}, held |-> {},
                                         inTable |-> TRUE, src |-> E.src, owned |-> E.owned = 1,
                                         alive |-> TRUE])
                     /\ rg' = [rg EXCEPT ![r].tbl = (E.idn :> E.c) @@ @]
                     /\ live' = live + 1
                     /\ promoted' = promoted + 1)
            /\ UNCHANGED <<lk, exr>>
       \* Another adapter of the data made the cell first: this one links to it
       ELSE /\ Check([TableAgrees |-> E.reg # E.cur \/ Lookup(r, E.idn, E.list) # E.c,
                      LinksAgree |-> L(E.a).cell # 0])
            /\ Next1(TRUE)
            /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

\* The new cell's Link failed: it leaves the table again
AdoptUndo ==
    /\ E.ev = "adoptundo" /\ Known(E.c)
    /\ LET c == E.c IN
       /\ Next1(/\ cl' = [cl EXCEPT ![c].inTable = FALSE]
                /\ rg' = [rg EXCEPT ![E.reg].tbl = [i \in DOMAIN @ |-> IF @[i] = c THEN 0 ELSE @[i]]]
                /\ live' = live - 1)
       /\ UNCHANGED <<lk, exr, promoted, bad>>

Promote ==
    /\ E.ev = "promote"
    /\ Check([LiveCount |-> OneThread /\ E.live # live])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

-----------------------------------------------------------------------------
(* Exports and writes: BorrowedCell::ExportList/Map, CallExports::Pin, MutableItems/Dict, Unshare *)

\* An export reads the cell, or, in the cell's own module call, through the call's CallExports
\* until the call ends: it marks nothing (FixCallPin)
Export ==
    /\ E.ev = "export" /\ Known(E.c)
    /\ LET c == E.c
           C == cl[c]
           own == ~C.frozen /\ C.reg = E.cur
           call == IF own THEN RunningCall(C.reg) ELSE 0
       IN Check([NoUseAfterFree |-> ~C.alive,
                 StorageAgrees |-> E.g # C.stor,
                 CallsAgree |-> E.call # call,
                 MarkRule |-> (E.live = 1) # ExportLive(C.frozen, own, call)])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

\* exr: what the values an ended call handed to the host share. The pin at the call's end records
\* the cell's storage and marks the cell (CallExports::Pin); which cells it reaches is the
\* model's to check (CellProtocol's FixPinNested), not a trace's
Pin ==
    /\ E.ev = "pin" /\ Known(E.c) /\ E.reg \in DOMAIN rg
    /\ LET c == E.c
           C == cl[c]
       IN /\ Check([NoUseAfterFree |-> ~C.alive,
                    TableAgrees |-> c \notin TblCells(E.reg),
                    FrozenUnwritten |-> C.frozen,
                    StorageAgrees |-> E.g # C.stor,
                    CallsAgree |-> E.call # RunningCall(E.reg),
                    NoCrossThreadWrite |-> ~WriterOk(E.reg, E.t)])
          /\ Next1(/\ cl' = [cl EXCEPT ![c].exp = MarkAfterPin]
                   /\ exr' = exr \cup {<<c, C.stor>>})
    /\ UNCHANGED <<lk, rg, live, promoted>>

\* A value an ended call handed to the host never changes (CellProtocol's SnapshotStable): no
\* write in place to a storage it shares
Write ==
    /\ E.ev = "write" /\ Known(E.c)
    /\ LET c == E.c
           C == cl[c]
       IN Check([NoUseAfterFree |-> ~C.alive,
                 FrozenUnwritten |-> C.frozen,
                 StorageAgrees |-> E.g # C.stor,
                 MarkRule |-> ~WritesInPlace(C.frozen, C.exp),
                 SnapshotStable |-> <<c, C.stor>> \in exr,
                 NoCrossThreadWrite |-> ~WriterOk(C.reg, E.t)])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

UnshareKind == CASE E.kind = 1 -> "frozen" [] E.kind = 3 -> "released" [] OTHER -> "copy"

Unshare ==
    /\ E.ev = "unshare" /\ Known(E.c)
    /\ LET c == E.c
           C == cl[c]
           call == IF C.frozen THEN 0 ELSE RunningCall(C.reg)
           k == UnshareKind
       IN /\ Check([NoUseAfterFree |-> ~C.alive,
                    StorageAgrees |-> E.g # C.stor,
                    CallsAgree |-> E.call # call,
                    MarkRule |-> E.exp # C.exp \/ ~NeedsUnshare(C.frozen, C.exp)
                                 \/ k \notin UnshareOutcomes(C.frozen, C.exp),
                    NoCrossThreadWrite |-> ~C.frozen /\ ~WriterOk(C.reg, E.t)])
          /\ Next1(/\ cl' = [cl EXCEPT ![c].exp = MarkAfterUnshare(k, @)]
                   /\ exr' = IF k = "released" THEN exr \ {<<c, C.stor>>} ELSE exr)
    /\ UNCHANGED <<lk, rg, live, promoted>>

\* The copy is in place: the cell's storage is a new generation
Copied ==
    /\ E.ev = "copied" /\ Known(E.c)
    /\ Next1(cl' = [cl EXCEPT ![E.c].stor = E.g])
    /\ UNCHANGED <<lk, rg, exr, live, promoted, bad>>

-----------------------------------------------------------------------------
(* The end of a module call (DropUnheld), of a render or a module (Finish), and a cell's free *)

Drop ==
    /\ E.ev = "drop" /\ Known(E.c) /\ E.reg \in DOMAIN rg
    /\ LET c == E.c
           C == cl[c]
       IN /\ Check([TableAgrees |-> c \notin TblCells(E.reg),
                    \* The code lists a global's links; the others own the cell
                    LinksAgree |-> AsSet(E.links) # C.held,
                    DropRule |-> ~MayDrop(C.owned, C.frozen, C.links, C.src),
                    NoCrossThreadWrite |-> ~WriterOk(E.reg, E.t)])
          /\ Next1(/\ cl' = [cl EXCEPT ![c].inTable = FALSE]
                   /\ rg' = [rg EXCEPT ![E.reg].tbl = [i \in DOMAIN @ |-> IF @[i] = c THEN 0 ELSE @[i]]]
                   /\ live' = live - 1)
    /\ UNCHANGED <<lk, exr, promoted>>

Dropped ==
    /\ E.ev = "dropped"
    /\ Check([LiveCount |-> OneThread /\ E.live # live])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

Freeze ==
    /\ E.ev = "freeze" /\ Known(E.c) /\ E.reg \in DOMAIN rg
    /\ LET c == E.c
           C == cl[c]
           detach == {a \in C.links : FinishDetaches(a \in C.held)}
       IN /\ Check([TableAgrees |-> c \notin TblCells(E.reg),
                    \* The code lists a global's links; the others own the cell and keep it frozen
                    LinksAgree |-> AsSet(E.held) # C.held \/ \E a \in C.links \ C.held : ~L(a).own,
                    \* A module's cells freeze when the module goes, outside its calls
                    NoCrossThreadWrite |-> IF rg[E.reg].module THEN Calls(E.reg) # <<>>
                                           ELSE rg[E.reg].thr # E.t])
          /\ Next1(/\ cl' = [cl EXCEPT ![c].frozen = TRUE, ![c].links = {}, ![c].held = {}]
                   /\ lk' = [a \in DOMAIN lk |-> IF a \in detach THEN [lk[a] EXCEPT !.cell = 0, !.seen = 0]
                                                 ELSE lk[a]])
    /\ UNCHANGED <<rg, exr, live, promoted>>

Finish ==
    /\ E.ev = "finish" /\ E.reg \in DOMAIN rg
    /\ LET r == E.reg
           RC == TblCells(r)
       IN /\ Check([TableAgrees |-> E.n # Cardinality(RC) \/ \E c \in RC : ~cl[c].frozen])
          /\ Next1(/\ cl' = [c \in DOMAIN cl |-> IF c \in RC THEN [cl[c] EXCEPT !.inTable = FALSE] ELSE cl[c]]
                   /\ rg' = [rg EXCEPT ![r].tbl = EmptyTbl]
                   /\ live' = live - Cardinality(RC))
    /\ UNCHANGED <<lk, exr, promoted>>

Finished ==
    /\ E.ev = "finished"
    /\ Check([LiveCount |-> OneThread /\ E.live # live])
    /\ Next1(TRUE)
    /\ UNCHANGED <<cl, lk, rg, exr, live, promoted>>

\* A cell goes: no table lists it, no adapter is attached to it
Free ==
    /\ E.ev = "free" /\ Known(E.c)
    /\ LET c == E.c IN
       /\ Check([NoUseAfterFree |-> ~cl[c].alive \/ cl[c].inTable \/ cl[c].links # {}
                                     \/ \E a \in DOMAIN lk : lk[a].cell = c,
                 FrozenAgrees |-> (E.frozen = 1) # cl[c].frozen])
       /\ Next1(cl' = [cl EXCEPT ![c].alive = FALSE])
    /\ UNCHANGED <<lk, rg, exr, live, promoted>>

-----------------------------------------------------------------------------

Done == l > Len(Log) /\ UNCHANGED vars

Next ==
    \/ l <= Len(Log) /\
        \/ RegNew \/ RegFree \/ BeginCall \/ EndCall
        \/ Skip \/ Hit \/ Find \/ Link \/ LinkFail \/ Unlink \/ Leave \/ Inherit \/ DieAttached
        \/ CellNew \/ AdoptNo \/ Adopt \/ AdoptUndo \/ Promote
        \/ Export \/ Pin \/ Write \/ Unshare \/ Copied
        \/ Drop \/ Dropped \/ Freeze \/ Finish \/ Finished \/ Free
    \/ Done

Spec == Init /\ [][Next]_vars

-----------------------------------------------------------------------------
(* One invariant per property, so that TLC names the one that failed *)

Holds(p) == \A x \in bad : x[1] # p

NoForeignCell == Holds("NoForeignCell")
AliasesAgree == Holds("AliasesAgree")
NoUseAfterFree == Holds("NoUseAfterFree")
NoDanglingLink == Holds("NoDanglingLink")
SnapshotStable == Holds("SnapshotStable")
FrozenUnwritten == Holds("FrozenUnwritten")
NoCrossThreadWrite == Holds("NoCrossThreadWrite")
MarkRule == Holds("MarkRule")
DropRule == Holds("DropRule")
LiveCount == Holds("LiveCount")
\* The spec's state and the code's agree
LinksAgree == Holds("LinksAgree")
TableAgrees == Holds("TableAgrees")
CallsAgree == Holds("CallsAgree")
StorageAgrees == Holds("StorageAgrees")
EpochAgrees == Holds("EpochAgrees")
FrozenAgrees == Holds("FrozenAgrees")

\* The whole log was read: checked by check.py on TLC's result (no deadlock before the end)
\* For TLC 1.8 by hand (ALIAS Alias in MC.cfg); the pinned 1.7.4 has no ALIAS
Alias == [l |-> l, ev |-> IF l > 1 /\ l - 1 <= Len(Log) THEN Log[l - 1] ELSE "-", bad |-> bad]

=============================================================================
