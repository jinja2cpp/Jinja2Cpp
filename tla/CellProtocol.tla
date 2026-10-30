---------------------------- MODULE CellProtocol ----------------------------
(***************************************************************************)
(* The borrowed-cell protocol of docs/tasks/0049 (P2) and 0331, as PR #673 *)
(* implements it at 459d8f7: src/borrowed_cells.{h,cpp}, the module call   *)
(* collector in src/cycle_collector.h and the callers in internal_value.cpp *)
(* and value_methods.cpp. See tla/README.md for the mapping and the runs.  *)
(*                                                                         *)
(* Abstractions:                                                           *)
(*  - A container's content is a number: the count of appends. A copy of   *)
(*    a list shares its item adapters (InternalValue copies share them).   *)
(*  - Memory is abstract: adapters, cells and storage generations ("gens") *)
(*    are never reused. An object is alive while a root reaches it; the GC *)
(*    action plays the reference counts and destructors right after the    *)
(*    step that dropped the last reference. Reading a dead object is a     *)
(*    use-after-free.                                                       *)
(*  - The host's data (P, which holds C, and the global G) never changes:  *)
(*    it is constant 0, and a write always goes to a cell's storage.       *)
(*  - Owned data (a module call's argument, a user callable's result) is   *)
(*    keyed by its address; an address is free again once no adapter of   *)
(*    the data lives, and its next data has a new incarnation (inc).       *)
(*  - Each FixXxx flag is one fix of #673. FALSE puts back the code as it  *)
(*    was before that fix.                                                 *)
(***************************************************************************)
EXTENDS Integers, Sequences, FiniteSets, TLC

CONSTANTS
    Threads, OwnedAddrs,
    MaxDepth, MaxLevel, MaxAd, MaxCell, MaxGen, MaxEx, MaxVal, MaxFail,
    ModuleThreads, RenderThreads,
    Actions,              \* the kinds of steps the model may take (a scope, see check.py)
    \* The fixes of #673
    FixLinkEpoch,         \* c06d969: Link sets the seen epoch to 0 before the push
    FixEnumRefetch,       \* f83b89f: CellEnumerator reads the cell's storage at each step
    FixSortRefetch,       \* 1e15c2d/89df914: sort fetches the cell's items after its key calls
    FixCallRestore,       \* 1e15c2d: a nested module call restores the outer call's id
    FixNestedGlobal,      \* 9c91ffc: a render nested in one that attached a global converts its own
    FixMarkOrder,         \* 5e37c24: the global-attached mark is cleared after the conversion
    FixSortMutableFirst,  \* 9c91ffc: sort gets MutableItems before it moves the restore values
    FixMarkerAfterCopy,   \* b3993a5: Unshare clears m_exportedIn only after the copy is in place
    FixReadOnlyExport,    \* A1: an export reads the link (Current), never Find/Link
    FixFreeze,            \* A2 (4588321 then the freeze): Finish keeps the non-global links frozen
    FixSourceKeepsId,     \* m_source keeps the data, and so its identity, alive while listed
    DropRule,             \* 459d8f7 DropUnheld: "none", "full", "nolinks", "nosource", and
                          \* "nohostcopy" (0336: no test that the host lets go of a generic container)
    FixNestedExport,      \* 0337 (finding F1 of this model): an export in a call other than the
                          \* one that marked the cell marks it shared, so every later write copies
    \* Proposed designs of the open tasks (0338 step A): the model first, then the C++
    FixCallPin,           \* 0335, 0332 (and 0337): a module call's exports read the module's cells
                          \* while the call runs, and its end pins them, by identity, to the
                          \* storages the cells then have; a pin marks the cell shared
    FixPinMarks,          \* the pin marks the cells it pins shared (without: writes in place)
    FixPinNested,         \* the pin covers the cells of nested items too (without: the top only)
    FixModuleLinksOwn,    \* 0332: an adapter's link to a module's cell is an owning reference,
                          \* as a link to a frozen cell is, so its release writes no list of links
    FixGenericOwner,      \* 0336: a generic container's adapter tells the cell its owner
    FixPinOnAttach,       \* 0340 option (a): a render's live cell holds the adapters linked to it
                          \* until Finish, so a host thread's drop is never the last reference to one
    FixLinksOwn,          \* 0340 as the C++ does it: every link but a global's owns its cell, a
                          \* render's too, so a host thread's drop is a release, never a write
    \* Failure atomicity (0338 step D): a step allocates first and commits with operations that
    \* cannot fail, so an allocation failure is one disjunct per step that changes nothing
    FixAtomicLink,        \* CellLink::Find/Link: a failed Link leaves the adapter as it was
    FixAtomicPin,         \* PinExports: a failed pin records and marks nothing (without: the cells
                          \* recorded before the failure stay pinned and marked)
    HostHandoff,          \* the host may hand what it kept to another thread while the call runs
    MaxCallId, MaxEpoch,  \* the call id and the epoch wrap past these (0341, 0342: mutant runs only)
    \* Shapes: inside a module call, keep([x]) (0332) and writes to a nested item (0332). Outside
    \* the contract before FixCallPin; a scope choice after it
    AllowModuleKeepList,
    AllowModuleNested,
    GenericArgs           \* a module call may take a generic container (0336)

Hosts == {"P", "C", "G"}
Keys == Hosts \cup OwnedAddrs
EMPTY == -1   \* the receiver of a sort while its key calls run
UNDEF == -2   \* a moved-from item
ATT == -1     \* CellLink::Attached, never an epoch
SHARED == -1  \* an m_exportedIn no call id equals (FixNestedExport)
AdIds == 1..MaxAd
CellIds == 1..MaxCell
GenIds == 1..MaxGen
ExIds == 1..MaxEx
ModReg == <<"M", 0>>
RegIds == {ModReg} \cup (Threads \X (1..MaxDepth))

VARIABLES
    epoch,     \* CellCounters::s_epoch
    live,      \* CellCounters::s_live
    ad, nAd,   \* adapters (their CellLink) of host or owned data
    cl, nCell, \* BorrowedCell
    st, nGen,  \* storage generations: a cell's m_storage, m_retired, a copy. alive: the
               \* item block (InternalValueList); acc: the accessor (the ListAdapter's
               \* Adapter object) in m_storage or m_retired, which an export does not keep
    rg,        \* BorrowedCells: the table, m_call, m_lastCall
    fr,        \* per thread: the stack of renders and module calls
    ex, nEx,   \* what the host keeps: a view of a storage ("gen"), a template list holding an adapter ("view"), the adapter itself ("raw", only without A1)
    gsnap,     \* per thread: the adapter of G in the thread's globals snapshot
    gatt,      \* per thread: RenderWorkspace::GlobalAttached
    ns,        \* an adapter a module stored in its namespace
    hkeep,     \* per owned address: the host keeps a copy of the generic container there (0336)
    lag,       \* owned addresses whose data the host let go of since the last DropUnheld: their
               \* cells may stay until the next call's end (RegistryBounded)
    lock,      \* the module's recursive mutex: the thread holding it, or "none"
    inc,       \* per owned address: the incarnation of the data there
    fails,     \* allocation failures injected so far
    dirty, dirtyBy, \* a reference was dropped: GC runs next, as the writer dirtyBy (W)
    pendDrop,  \* a module call ended: DropUnheld runs next
    wr,        \* per object: the set of <<thread, holds the module lock>> that wrote it
    flag       \* violations seen inside a step

vars == <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, fr, ex, nEx, gsnap, gatt,
          ns, hkeep, lag, lock, inc, fails, dirty, dirtyBy, pendDrop, wr, flag>>

NoAd == [idn |-> "none", inc |-> 0, held |-> FALSE, generic |-> FALSE, cell |-> 0, seen |-> 0,
         alive |-> FALSE]
NoCell == [idn |-> "none", inc |-> 0, reg |-> ModReg, stor |-> 0, retired |-> 0, source |-> 0,
           owned |-> FALSE, frozen |-> FALSE, exp |-> 0, links |-> {}, inTable |-> FALSE,
           alive |-> FALSE]
NoGen == [val |-> 0, child |-> 0, alive |-> FALSE, acc |-> FALSE]
\* kind: "gen" a view of a storage's items, "live" a module call's export that reads the head of
\* cell (FixCallPin), "view" a template list holding the adapter ad, "raw" the adapter itself.
\* pin: the call ended and the export reads only the storages pg and cg (0 for the host's data)
NoEx == [kind |-> "none", gen |-> 0, ad |-> 0, cell |-> 0, idn |-> "none", thr |-> "none",
         depth |-> 0, reg |-> ModReg, done |-> FALSE, snap |-> <<>>, pin |-> FALSE, pg |-> 0,
         cg |-> 0, alive |-> FALSE]
EmptyReg == [tbl |-> [k \in Keys |-> 0], call |-> 0, last |-> 0]
NoFlags == [uaf |-> FALSE, foreign |-> FALSE, stale |-> FALSE, undef |-> FALSE, late |-> FALSE,
            lost |-> FALSE, torn |-> FALSE]
Objs == ({"a"} \X AdIds) \cup ({"c"} \X CellIds) \cup ({"g"} \X GenIds)

NewAd(i, n, h) == [idn |-> i, inc |-> n, held |-> h, generic |-> FALSE, cell |-> 0, seen |-> 0,
                   alive |-> TRUE]

Init ==
    /\ epoch = 0 /\ live = 0
    /\ ad = [i \in AdIds |-> NoAd] /\ nAd = 0
    /\ cl = [c \in CellIds |-> NoCell] /\ nCell = 0
    /\ st = [g \in GenIds |-> NoGen] /\ nGen = 0
    /\ rg = [r \in RegIds |-> EmptyReg]
    /\ fr = [t \in Threads |-> <<>>]
    /\ ex = [e \in ExIds |-> NoEx] /\ nEx = 0
    /\ gsnap = [t \in Threads |-> 0] /\ gatt = [t \in Threads |-> FALSE]
    /\ ns = 0 /\ lock = "none"
    /\ hkeep = [o \in OwnedAddrs |-> FALSE] /\ lag = {}
    /\ inc = [o \in OwnedAddrs |-> 0]
    /\ fails = 0
    /\ dirty = FALSE /\ dirtyBy = <<"none", FALSE>> /\ pendDrop = FALSE
    /\ wr = [o \in Objs |-> {}]
    /\ flag = NoFlags

-----------------------------------------------------------------------------
(* Frames *)

Depth(t) == Len(fr[t])
Top(t) == fr[t][Depth(t)]
CurReg(t) == Top(t).reg
Running(t) == Depth(t) > 0 /\ Top(t).phase = "run"
SetTop(t, f) == [fr EXCEPT ![t][Depth(t)] = f]
Pop(t) == [fr EXCEPT ![t] = SubSeq(@, 1, Len(@) - 1)]
InModule(t) == \E i \in 1..Depth(t) : fr[t][i].kind = "mod"
\* Behaviours of at most MaxLevel states, GC steps included (TLC's BFS level). A guard of every
\* action rather than a CONSTRAINT: TLC then does not compute the last level's successors
InBound == TLCGet("level") < MaxLevel
Idle == ~dirty /\ ~pendDrop /\ InBound
\* CellCounters::NextEpoch and BeginCall skip 0: past the bound they start again at 1
NextEp(e) == IF e >= MaxEpoch THEN 1 ELSE e + 1
NextCall(k) == IF k >= MaxCallId THEN 1 ELSE k + 1
Grow(v) == IF v = EMPTY THEN 1 ELSE IF v = UNDEF THEN UNDEF ELSE v + 1
CanGrow(v) == v < MaxVal

\* A write by t: the module's state is written under its lock
W(t) == <<t, InModule(t)>>
\* (One thread: nothing to compare, so the writers are not kept and add no states)
MarkBy(w, objs, who) == IF Cardinality(Threads) = 1 THEN w
                        ELSE [o \in Objs |-> IF o \in objs THEN w[o] \cup {who} ELSE w[o]]
Mark(w, objs, t) == MarkBy(w, objs, W(t))

-----------------------------------------------------------------------------
(* What the host sees through what it keeps *)

ChildVis(a) == IF a = 0 THEN 0
               ELSE IF ad[a].cell # 0 THEN st[cl[ad[a].cell].stor].val ELSE 0
VisG(g) == <<st[g].val, ChildVis(st[g].child)>>
\* A global's adapter always exports the global's own data (0331 A1)
VisAd(a) == IF ad[a].cell # 0 /\ ~ad[a].held THEN VisG(cl[ad[a].cell].stor) ELSE <<0, 0>>
\* A pinned export reads the storages its pin recorded, never an adapter's link: pg for the data,
\* cg for its nested item (-1: the nested item through its adapter's link, FixPinNested off)
StVal(g) == IF g = 0 THEN 0 ELSE st[g].val
PinVis(e) == IF ex[e].cg = -1
             THEN IF ex[e].pg = 0 THEN <<0, 0>> ELSE VisG(ex[e].pg)
             ELSE <<StVal(ex[e].pg), StVal(ex[e].cg)>>
VisEx(e) == CASE ex[e].pin -> PinVis(e)
              [] ex[e].kind = "gen" -> VisG(ex[e].gen)
              [] ex[e].kind = "live" -> VisG(cl[ex[e].cell].stor)
              [] OTHER -> VisAd(ex[e].ad)

\* The exports of the frame at depth d of t: the host's snapshot is what it sees when the call
\* ends (Snap), before Finish runs; the call has returned to the host, which may then pass what
\* it kept to another thread, once its collector is gone (done)
Snap(t, d, done) ==
    [e \in ExIds |-> IF ex[e].alive /\ ex[e].thr = t /\ ex[e].depth = d /\ ~ex[e].done
                     THEN [ex[e] EXCEPT !.done = done,
                                        !.snap = IF @ = <<>> THEN VisEx(e) ELSE @]
                     ELSE ex[e]]

-----------------------------------------------------------------------------
(* Reachability: the reference counts *)

\* A link holds its cell: a frozen one, a module's with FixModuleLinksOwn, and with FixLinksOwn any
\* cell unless the adapter is a global's. Its release is then a reference count, not a write of
\* the cell's list of links
OwningLink(a) == LET c == ad[a].cell
                 IN cl[c].frozen \/ (FixModuleLinksOwn /\ cl[c].reg = ModReg)
                    \/ (FixLinksOwn /\ ~ad[a].held)

Succ(n) ==
    CASE n[1] = "a" -> IF ad[n[2]].cell # 0 /\ OwningLink(n[2])
                       THEN {<<"c", ad[n[2]].cell>>} ELSE {}
      \* A cell holds the accessors of its storages; an accessor holds its item block
      [] n[1] = "c" -> {<<"x", cl[n[2]].stor>>}
                       \cup (IF FixPinOnAttach /\ ~cl[n[2]].frozen /\ cl[n[2]].reg # ModReg
                             THEN {<<"a", a>> : a \in cl[n[2]].links} ELSE {})
                       \cup (IF cl[n[2]].retired # 0 THEN {<<"x", cl[n[2]].retired>>} ELSE {})
                       \cup (IF FixSourceKeepsId /\ cl[n[2]].source # 0
                             THEN {<<"a", cl[n[2]].source>>} ELSE {})
      [] n[1] = "x" -> {<<"g", n[2]>>}
      [] n[1] = "g" -> IF st[n[2]].child # 0 THEN {<<"a", st[n[2]].child>>} ELSE {}

RECURSIVE Close(_)
Close(S) == LET T == S \cup UNION {Succ(n) : n \in S} IN IF T = S THEN S ELSE Close(T)

Ads0 == UNION {UNION {fr[t][i].ads \cup (IF fr[t][i].bkind = "none" THEN {} ELSE {fr[t][i].bad}) :
                      i \in 1..Depth(t)} : t \in Threads}
LiveEx == {e \in ExIds : ex[e].alive}
RootsExcept(x) ==
    ({<<"a", a>> : a \in Ads0 \cup {gsnap[t] : t \in Threads} \cup {ns}
                     \cup {ex[e].ad : e \in {e \in LiveEx : ex[e].kind \in {"view", "raw"}}}}
       \ {<<"a", 0>>})
    \* CreateGenericList captures a copy of the storage's accessor: it shares the item block,
    \* not the accessor object in m_storage
    \cup {<<"g", ex[e].gen>> : e \in {e \in LiveEx : ex[e].kind = "gen" /\ ~ex[e].pin}}
    \* A running call's export holds the cell; a pinned one the storages it recorded
    \cup {<<"c", ex[e].cell>> : e \in {e \in LiveEx : ex[e].kind = "live" /\ ~ex[e].pin}}
    \cup ({<<"g", g>> : g \in UNION {{ex[e].pg, ex[e].cg} : e \in {e \in LiveEx : ex[e].pin}}}
           \ {<<"g", 0>>, <<"g", -1>>})
    \cup {<<"c", c>> : c \in {c \in CellIds : cl[c].alive /\ cl[c].inTable /\ c # x}}
Reach == Close(RootsExcept(0))

GC ==
    /\ dirty /\ InBound
    /\ LET R == Reach
           deadA == {a \in AdIds : ad[a].alive /\ <<"a", a>> \notin R}
           unlinked == {c \in CellIds : cl[c].alive /\ \E a \in cl[c].links \cap deadA : ~OwningLink(a)}
           cl2 == [c \in CellIds |-> [cl[c] EXCEPT !.alive = @ /\ <<"c", c>> \in R,
                                                   !.links = @ \ deadA]]
           ad2 == [a \in AdIds |-> [ad[a] EXCEPT !.alive = @ /\ <<"a", a>> \in R]]
       IN /\ cl' = cl2
          /\ ad' = ad2
          /\ st' = [g \in GenIds |-> [st[g] EXCEPT !.alive = @ /\ <<"g", g>> \in R,
                                                  !.acc = @ /\ <<"x", g>> \in R]]
          \* Unlinking a dead adapter from a live cell writes that cell's list of links
          /\ wr' = MarkBy(wr, {<<"c", c>> : c \in unlinked}, dirtyBy)
          \* An adapter that lives on, attached to a cell that went
          /\ flag' = [flag EXCEPT !.uaf = @ \/ \E a \in AdIds :
                         ad2[a].alive /\ ad2[a].cell # 0 /\ ~cl2[ad2[a].cell].alive]
    /\ dirty' = FALSE
    /\ UNCHANGED <<epoch, live, nAd, nCell, nGen, rg, fr, ex, nEx, gsnap, gatt, ns, hkeep, lag, lock, inc,
                   fails, dirtyBy, pendDrop>>

-----------------------------------------------------------------------------
(* CellLink::Get: the epoch compare, then Find and Link *)

Key(a) == ad[a].idn
WouldLink(t, a) == ad[a].seen # epoch /\ ad[a].cell = 0 /\ rg[CurReg(t)].tbl[Key(a)] # 0

GetR(t, a, f) ==
    LET A == ad[a]
        e == rg[CurReg(t)].tbl[Key(a)]
    IN CASE A.seen = epoch -> [c |-> 0, A2 |-> A, link |-> 0, failed |-> FALSE]
         [] A.cell # 0 -> [c |-> A.cell, A2 |-> A, link |-> 0, failed |-> FALSE]
         [] e = 0 -> [c |-> 0, A2 |-> [A EXCEPT !.seen = epoch], link |-> 0, failed |-> FALSE]
         [] f -> [c |-> 0, link |-> 0, failed |-> TRUE,
                  A2 |-> IF FixAtomicLink THEN A
                         ELSE [A EXCEPT !.seen = IF FixLinkEpoch THEN 0 ELSE epoch]]
         [] OTHER -> [c |-> e, A2 |-> [A EXCEPT !.cell = e, !.seen = ATT], link |-> e,
                      failed |-> FALSE]

\* The cell Get returned is of this data and, unless frozen, of the running render or call
Foreign(t, a, c) ==
    c # 0 /\ (cl[c].idn # ad[a].idn \/ cl[c].inc # ad[a].inc
              \/ (~cl[c].frozen /\ cl[c].reg # CurReg(t)))

ApplyGetAd(a, R) == [ad EXCEPT ![a] = R.A2]
ApplyGetCl(a, R) == IF R.link # 0 THEN [cl EXCEPT ![R.link].links = @ \cup {a}] ELSE cl
GetWrites(a, R) == (IF R.A2 # ad[a] THEN {<<"a", a>>} ELSE {})
                   \cup (IF R.link # 0 THEN {<<"c", R.link>>} ELSE {})
ApplyGetGatt(t, a, R) == IF R.link # 0 /\ ad[a].held /\ FixNestedGlobal
                         THEN [gatt EXCEPT ![t] = TRUE] ELSE gatt

-----------------------------------------------------------------------------
(* BorrowedCell::MutableItems and Unshare, then the write. op: "append", "touch"
   (MakeMutable on the path to a nested item) or "set" (sort writes back v) *)

ExportHolds(g) == \E e \in LiveEx : IF ex[e].pin THEN g \in {ex[e].pg, ex[e].cg}
                                    ELSE ex[e].kind = "gen" /\ ex[e].gen = g
NeedCopy(c) == cl[c].exp # 0 /\ cl[c].exp # rg[cl[c].reg].call /\ ExportHolds(cl[c].stor)
NewVal(op, old, v) == CASE op = "append" -> Grow(old) [] op \in {"touch", "drop"} -> old [] OTHER -> v
\* "drop" removes the nested item (l.pop()): the storage lets go of its adapter
NewChild(op, old) == IF op = "drop" THEN 0 ELSE old

WC(c, f, op, v, CL) ==
    LET cc == CL[c]
        g == nGen + 1
    IN CASE cc.frozen ->
              [k |-> "private", cl |-> CL, st |-> st, nGen |-> nGen, gen |-> 0]
         [] NeedCopy(c) /\ f ->
              [k |-> "fail", st |-> st, nGen |-> nGen, gen |-> 0,
               cl |-> IF FixMarkerAfterCopy THEN CL ELSE [CL EXCEPT ![c].exp = 0]]
         [] NeedCopy(c) ->
              [k |-> "copy", nGen |-> g, gen |-> g,
               cl |-> [CL EXCEPT ![c].stor = g, ![c].retired = cc.stor, ![c].exp = 0],
               st |-> [st EXCEPT ![g] = [val |-> NewVal(op, st[cc.stor].val, v),
                                         child |-> NewChild(op, st[cc.stor].child), alive |-> TRUE,
                                         acc |-> TRUE]]]
         [] OTHER ->
              [k |-> "inplace", nGen |-> nGen, gen |-> cc.stor,
               \* The host let go of what an earlier call was handed
               cl |-> IF cc.exp # 0 /\ cc.exp # rg[cc.reg].call THEN [CL EXCEPT ![c].exp = 0] ELSE CL,
               st |-> [st EXCEPT ![cc.stor].val = NewVal(op, @, v),
                                 ![cc.stor].child = NewChild(op, @)]]

WCOk(c, f, op) ==
    /\ (NeedCopy(c) /\ ~f) => nGen < MaxGen
    /\ op = "append" => CanGrow(st[cl[c].stor].val)
    /\ f => (NeedCopy(c) /\ ~cl[c].frozen /\ fails < MaxFail)

WCWrites(c, Wr) == (IF Wr.k = "private" THEN {} ELSE {<<"c", c>>})
                   \cup (IF Wr.gen # 0 THEN {<<"g", Wr.gen>>} ELSE {})

-----------------------------------------------------------------------------
(* A write through adapter a (methods::MakeMutable then the change) *)

\* The cell exists: link if needed, then write it
WriteCell(t, a, f2, op, R) ==
    LET CL1 == ApplyGetCl(a, R)
        Wr == WC(R.c, f2, op, 0, CL1)
        child == st[cl[R.c].stor].child
    IN /\ WCOk(R.c, f2, op)
       /\ cl' = Wr.cl /\ st' = Wr.st /\ nGen' = Wr.nGen
       /\ ad' = ApplyGetAd(a, R)
       /\ gatt' = ApplyGetGatt(t, a, R)
       /\ wr' = Mark(wr, GetWrites(a, R) \cup WCWrites(R.c, Wr), t)
       /\ fails' = IF Wr.k = "fail" THEN fails + 1 ELSE fails
       /\ fr' = IF op = "touch" /\ child # 0 /\ Wr.k # "fail"
                THEN SetTop(t, [Top(t) EXCEPT !.ads = @ \cup {child}]) ELSE fr
       /\ flag' = [flag EXCEPT !.foreign = @ \/ Foreign(t, a, R.c),
                               !.uaf = @ \/ ~cl[R.c].alive \/ ~st[cl[R.c].stor].acc,
                               \* Unshare could not copy: the cell must be as Get left it
                               !.torn = @ \/ (Wr.k = "fail" /\ Wr.cl # CL1)]
       /\ dirty' = (Wr.k = "copy" \/ (op = "drop" /\ Wr.k # "fail")) /\ dirtyBy' = W(t)
       /\ UNCHANGED <<epoch, live, nAd, nCell, rg, ex, nEx, gsnap, ns, hkeep, lag, lock, inc, pendDrop>>

\* CellLink::Adopt with a new cell: the copy of the data, the write applied
Adopt(t, a, op) ==
    LET r == CurReg(t)
        c == nCell + 1
        g == nGen + 1
        hasChild == ad[a].idn = "P" /\ op # "drop"
        ch == nAd + 1
    IN /\ nCell < MaxCell /\ nGen < MaxGen /\ (hasChild => nAd < MaxAd)
       /\ cl' = [cl EXCEPT ![c] = [idn |-> ad[a].idn, inc |-> ad[a].inc, reg |-> r, stor |-> g,
                                   retired |-> 0, source |-> a,
                                   \* 0336: a generic container's owner is hidden in a std::function
                                   owned |-> ad[a].idn \in OwnedAddrs /\ (~ad[a].generic \/ FixGenericOwner),
                                   frozen |-> FALSE, exp |-> 0, links |-> {a}, inTable |-> TRUE,
                                   alive |-> TRUE]]
       /\ nCell' = c
       /\ st' = [st EXCEPT ![g] = [val |-> NewVal(op, 0, 0), child |-> IF hasChild THEN ch ELSE 0,
                                   alive |-> TRUE, acc |-> TRUE]]
       /\ nGen' = g
       /\ ad' = IF hasChild THEN [ad EXCEPT ![a].cell = c, ![a].seen = ATT, ![ch] = NewAd("C", 0, FALSE)]
                ELSE [ad EXCEPT ![a].cell = c, ![a].seen = ATT]
       /\ nAd' = IF hasChild THEN ch ELSE nAd
       /\ rg' = [rg EXCEPT ![r].tbl[ad[a].idn] = c]
       /\ live' = live + 1
       /\ epoch' = NextEp(epoch)
       /\ gatt' = IF ad[a].held /\ FixNestedGlobal THEN [gatt EXCEPT ![t] = TRUE] ELSE gatt
       /\ wr' = Mark(wr, {<<"a", a>>, <<"c", c>>, <<"g", g>>}, t)
       /\ fr' = IF op = "touch" /\ hasChild
                THEN SetTop(t, [Top(t) EXCEPT !.ads = @ \cup {ch}]) ELSE fr
       /\ UNCHANGED <<ex, nEx, gsnap, ns, hkeep, lag, lock, inc, fails, flag, dirty, dirtyBy, pendDrop>>

\* The read or write was refused by an allocation failure in Link. With FixAtomicLink nothing
\* changes: the adapter keeps the epoch it saw, which the promotion of the cell moved past, so it
\* looks again. Without it the epoch it read is kept (0, or the counter without FixLinkEpoch)
LinkFailed(t, a, R) ==
    /\ ad' = ApplyGetAd(a, R)
    /\ wr' = Mark(wr, GetWrites(a, R), t)
    /\ fails' = fails + 1
    /\ flag' = [flag EXCEPT !.torn = @ \/ R.A2 # ad[a]]
    /\ UNCHANGED <<epoch, live, nAd, cl, nCell, st, nGen, rg, fr, ex, nEx, gsnap, gatt, ns, hkeep, lag, lock,
                   inc, dirty, dirtyBy, pendDrop>>

\* CellLink::Adopt ran out of memory: the table, the cell and the room for a global's link come
\* before the commit, so nothing changes (0338 step D; #673 rolled back a failed Link instead)
AdoptFailed ==
    /\ fails < MaxFail
    /\ fails' = fails + 1
    /\ UNCHANGED <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, fr, ex, nEx, gsnap, gatt, ns, hkeep, lag,
                   lock, inc, dirty, dirtyBy, pendDrop, wr, flag>>

CanUse(t, a) == Idle /\ Running(t) /\ a \in Top(t).ads /\ ad[a].alive

Write(t, a, op) ==
    /\ op \in {"touch", "drop"} => op \in Actions
    /\ CanUse(t, a)
    /\ op \in {"touch", "drop"} => (ad[a].idn = "P" /\ (Top(t).kind = "render" \/ AllowModuleNested))
    /\ op = "append" /\ ad[a].idn = "C" => (Top(t).kind = "render" \/ AllowModuleNested)
    /\ \E f1, f2 \in BOOLEAN :
        /\ f1 => (WouldLink(t, a) /\ fails < MaxFail)
        /\ ~(f1 /\ f2)
        /\ LET R == GetR(t, a, f1) IN
           CASE R.failed -> LinkFailed(t, a, R)
             [] R.c # 0 -> WriteCell(t, a, f2, op, R)
             [] rg[CurReg(t)].tbl[Key(a)] # 0 ->
                  \* Adopt finds the cell another adapter made: link to it, write it
                  WriteCell(t, a, f2, op, [c |-> rg[CurReg(t)].tbl[Key(a)], link |-> rg[CurReg(t)].tbl[Key(a)],
                                            failed |-> FALSE,
                                            A2 |-> [ad[a] EXCEPT !.cell = rg[CurReg(t)].tbl[Key(a)], !.seen = ATT]])
             [] OTHER -> IF f2 THEN AdoptFailed ELSE Adopt(t, a, op)

\* A read: what Get returns, or the host's data
Read(t, a) ==
    /\ "read" \in Actions
    /\ CanUse(t, a)
    /\ \E f \in BOOLEAN :
        /\ f => (WouldLink(t, a) /\ fails < MaxFail)
        /\ LET R == GetR(t, a, f) IN
           IF R.failed THEN LinkFailed(t, a, R)
           ELSE /\ ad' = ApplyGetAd(a, R)
                /\ cl' = ApplyGetCl(a, R)
                /\ gatt' = ApplyGetGatt(t, a, R)
                /\ wr' = Mark(wr, GetWrites(a, R), t)
                /\ flag' = [flag EXCEPT
                     !.foreign = @ \/ Foreign(t, a, R.c),
                     !.uaf = @ \/ (R.c # 0 /\ (~cl[R.c].alive \/ ~st[cl[R.c].stor].acc)),
                     \* An alias that reads the host's data while its render has a cell of it
                     !.stale = @ \/ (R.c = 0 /\ rg[CurReg(t)].tbl[Key(a)] # 0)]
                /\ UNCHANGED <<epoch, live, nAd, nCell, st, nGen, rg, fr, ex, nEx, gsnap, ns, hkeep, lag,
                               lock, inc, fails, dirty, dirtyBy, pendDrop>>

-----------------------------------------------------------------------------
(* Exports to the host *)

NewEx(t, kind, g, a) ==
    [NoEx EXCEPT !.kind = kind, !.gen = g, !.ad = a, !.cell = ad[a].cell, !.idn = ad[a].idn,
                 !.thr = t, !.depth = Depth(t), !.reg = CurReg(t), !.alive = TRUE]

\* keep(x): a host callable keeps the argument. With A1 the export reads only Current()
Export(t, a) ==
    /\ "export" \in Actions
    /\ CanUse(t, a) /\ nEx < MaxEx
    /\ IF FixReadOnlyExport
       THEN LET c == ad[a].cell
                r == CurReg(t)
                own == ~cl[c].frozen /\ cl[c].reg = r /\ rg[r].call # 0
                mark == IF cl[c].exp = 0 THEN rg[r].call
                        ELSE IF FixNestedExport /\ cl[c].exp # rg[r].call THEN SHARED
                        ELSE cl[c].exp
                \* FixCallPin: the export reads the cell until the call ends, and notes nothing
                livex == own /\ FixCallPin
                note == own /\ ~livex /\ mark # cl[c].exp
            IN /\ c # 0 /\ ~ad[a].held  \* otherwise a view of the host's data, which never changes
               /\ ex' = [ex EXCEPT ![nEx + 1] = IF livex THEN NewEx(t, "live", 0, a)
                                                ELSE NewEx(t, "gen", cl[c].stor, a)]
               \* BorrowedCell::NoteExport
               /\ cl' = IF note THEN [cl EXCEPT ![c].exp = mark] ELSE cl
               /\ wr' = IF note THEN Mark(wr, {<<"c", c>>}, t) ELSE wr
       ELSE /\ ex' = [ex EXCEPT ![nEx + 1] = NewEx(t, "raw", 0, a)]
            /\ UNCHANGED <<cl, wr>>
    /\ nEx' = nEx + 1
    /\ UNCHANGED <<epoch, live, ad, nAd, nCell, st, nGen, rg, fr, gsnap, gatt, ns, hkeep, lag, lock, inc,
                   fails, dirty, dirtyBy, pendDrop, flag>>

\* keep([x]): the host keeps a template list holding the adapter; it reads it with Current()
KeepList(t, a) ==
    /\ "keeplist" \in Actions
    /\ CanUse(t, a) /\ nEx < MaxEx
    /\ Top(t).kind = "render" \/ AllowModuleKeepList
    /\ ex' = [ex EXCEPT ![nEx + 1] = NewEx(t, "view", 0, a)]
    /\ nEx' = nEx + 1
    /\ UNCHANGED <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, fr, gsnap, gatt, ns, hkeep, lag, lock, inc,
                   fails, dirty, dirtyBy, pendDrop, wr, flag>>

\* The host lets go of what it kept: on its own thread at any time, on another after the call
Release(t, e) ==
    /\ "release" \in Actions
    /\ Idle /\ ex[e].alive
    /\ ex[e].thr = t \/ ex[e].done \/ HostHandoff
    /\ ex' = [ex EXCEPT ![e].alive = FALSE]
    /\ lag' = lag \cup ({ex[e].idn} \cap OwnedAddrs)
    /\ dirty' = TRUE /\ dirtyBy' = W(t)
    /\ UNCHANGED <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, fr, nEx, gsnap, gatt, ns, hkeep, lock,
                   inc, fails, pendDrop, wr, flag>>

-----------------------------------------------------------------------------
(* Borrowers across a callback: an enumeration and list.sort *)

NoBorrow == [bkind |-> "none", bad |-> 0, bgen |-> 0, bvals |-> 0]

EnumStart(t, a) ==
    /\ "enum" \in Actions
    /\ CanUse(t, a) /\ Top(t).bkind = "none"
    /\ LET R == GetR(t, a, FALSE) IN
       /\ R.c # 0 /\ ~cl[R.c].frozen
       /\ ad' = ApplyGetAd(a, R) /\ cl' = ApplyGetCl(a, R)
       /\ wr' = Mark(wr, GetWrites(a, R), t)
       /\ fr' = SetTop(t, [Top(t) EXCEPT !.bkind = "enum", !.bad = a, !.bgen = cl[R.c].stor])
    /\ UNCHANGED <<epoch, live, nAd, nCell, st, nGen, rg, ex, nEx, gsnap, gatt, ns, hkeep, lag, lock,
                   inc, fails, dirty, dirtyBy, pendDrop, flag>>

\* The next step of the loop, after the body's callbacks returned
EnumStep(t) ==
    /\ "enum" \in Actions
    /\ Idle /\ Running(t) /\ Top(t).bkind = "enum"
    /\ LET a == Top(t).bad
           g == IF FixEnumRefetch THEN cl[ad[a].cell].stor ELSE Top(t).bgen
       \* Before f83b89f the loop held the enumerator of the storage's accessor (cell->List()),
       \* a raw pointer to it: an export of the items does not keep that accessor alive
       IN flag' = [flag EXCEPT !.uaf = @ \/ ~st[g].acc \/ (FixEnumRefetch /\ ~cl[ad[a].cell].alive)]
    /\ fr' = SetTop(t, [Top(t) EXCEPT !.bkind = "none", !.bad = 0, !.bgen = 0])
    /\ UNCHANGED <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, ex, nEx, gsnap, gatt, ns, hkeep, lag, lock,
                   inc, fails, dirty, dirtyBy, pendDrop, wr>>

\* sort(key=...): MutableItems, move the items out, then the key calls run (nested frames)
SortStart(t, a) ==
    /\ "sort" \in Actions
    /\ CanUse(t, a) /\ Top(t).bkind = "none"
    /\ LET R == GetR(t, a, FALSE)
           c == R.c
       IN
       /\ c # 0 /\ ~cl[c].frozen /\ ~NeedCopy(c) /\ st[cl[c].stor].val >= 0
       /\ LET Wr == WC(c, FALSE, "set", EMPTY, ApplyGetCl(a, R)) IN
          /\ cl' = Wr.cl /\ st' = Wr.st
          /\ ad' = ApplyGetAd(a, R)
          /\ fr' = SetTop(t, [Top(t) EXCEPT !.bkind = "sort", !.bad = a, !.bgen = cl[c].stor,
                                            !.bvals = st[cl[c].stor].val])
          /\ wr' = Mark(wr, GetWrites(a, R) \cup WCWrites(c, Wr), t)
    /\ UNCHANGED <<epoch, live, nAd, nCell, nGen, rg, ex, nEx, gsnap, gatt, ns, hkeep, lag, lock, inc,
                   fails, dirty, dirtyBy, pendDrop, flag>>

\* The key calls returned: write the sorted items back
SortEnd(t) ==
    /\ "sort" \in Actions
    /\ Idle /\ Running(t) /\ Top(t).bkind = "sort"
    /\ LET a == Top(t).bad
           c == ad[a].cell
           v == Top(t).bvals
           fr2 == SetTop(t, [Top(t) EXCEPT !.bkind = "none", !.bad = 0, !.bgen = 0, !.bvals = 0])
       IN IF FixSortRefetch
          THEN \E f \in BOOLEAN :
                 /\ f => (NeedCopy(c) /\ fails < MaxFail)
                 \* The copy failed: the restore guard tries again, with the items or, when they
                 \* were moved before MutableItems, with what is left of them
                 /\ LET v2 == IF f /\ ~FixSortMutableFirst THEN UNDEF ELSE v
                        Wr == WC(c, FALSE, "set", v2, cl)
                    IN /\ (NeedCopy(c)) => nGen < MaxGen
                       /\ cl' = Wr.cl /\ st' = Wr.st /\ nGen' = Wr.nGen
                       /\ wr' = Mark(wr, WCWrites(c, Wr), t)
                       /\ fails' = IF f THEN fails + 1 ELSE fails
                       /\ flag' = [flag EXCEPT !.undef = @ \/ v2 = UNDEF,
                                               !.uaf = @ \/ ~cl[c].alive]
                       /\ dirty' = (Wr.k = "copy") /\ dirtyBy' = W(t)
               /\ fr' = fr2
          ELSE LET g == Top(t).bgen IN
               \* The InternalValueList& sort held across its key calls
               /\ st' = [st EXCEPT ![g].val = v]
               /\ wr' = Mark(wr, {<<"g", g>>}, t)
               /\ flag' = [flag EXCEPT !.uaf = @ \/ ~st[g].alive]
               /\ fr' = fr2
               /\ UNCHANGED <<cl, nGen, fails, dirty, dirtyBy>>
    /\ UNCHANGED <<epoch, live, ad, nAd, nCell, rg, ex, nEx, gsnap, gatt, ns, hkeep, lag, lock, inc,
                   pendDrop>>

-----------------------------------------------------------------------------
(* Renders: TemplateImpl::Render with its CycleCollector, GetGlobalsSnapshot,
   BorrowedCells::Finish. A render at depth > 1 runs from a host callback *)

CanNest(t) == Depth(t) < MaxDepth /\ IF Depth(t) = 0 THEN TRUE ELSE Top(t).phase = "run"

StartRender(t) ==
    /\ Idle /\ t \in RenderThreads /\ CanNest(t)
    /\ LET conv == gsnap[t] = 0 \/ (FixNestedGlobal /\ gatt[t]) IN
       \E f \in BOOLEAN :
        /\ f => (conv /\ fails < MaxFail)
        /\ IF f
           THEN \* The conversion of the globals failed: the render fails
                /\ gatt' = IF FixMarkOrder THEN gatt ELSE [gatt EXCEPT ![t] = FALSE]
                /\ fails' = fails + 1
                /\ UNCHANGED <<ad, nAd, fr, rg, gsnap>>
           ELSE LET a == nAd + 1
                    b == nAd + 2
                    gs == IF conv THEN nAd + 3 ELSE gsnap[t]
                    n == IF conv THEN 3 ELSE 2
                    d == Depth(t) + 1
                    \* Without A1 the host may pass on what it kept: the adapter itself
                    raws == {ex[e].ad : e \in {e \in ExIds : ex[e].alive /\ ex[e].kind = "raw"
                                                              /\ ex[e].done}}
                IN /\ nAd + n <= MaxAd
                   /\ ad' = [i \in AdIds |-> CASE i = a -> NewAd("P", 0, FALSE)
                                               [] i = b -> NewAd("P", 0, FALSE)
                                               [] i = gs /\ conv -> NewAd("G", 0, TRUE)
                                               [] OTHER -> ad[i]]
                   /\ nAd' = nAd + n
                   /\ gsnap' = [gsnap EXCEPT ![t] = gs]
                   /\ gatt' = IF conv THEN [gatt EXCEPT ![t] = FALSE] ELSE gatt
                   /\ rg' = [rg EXCEPT ![<<t, d>>] = EmptyReg]
                   /\ fr' = [fr EXCEPT ![t] = Append(@, [kind |-> "render", reg |-> <<t, d>>,
                                 outer |-> 0, call |-> 0, ads |-> {a, b, gs} \cup raws,
                                 phase |-> "run"] @@ NoBorrow)]
                   /\ UNCHANGED fails
    /\ UNCHANGED <<epoch, live, cl, nCell, st, nGen, ex, nEx, ns, hkeep, lag, lock, inc, dirty, dirtyBy,
                   pendDrop, wr, flag>>

RegCells(r) == {c \in CellIds : cl[c].alive /\ cl[c].inTable /\ cl[c].reg = r}

\* The render's scopes go first (the collector is made first and ends last), then Finish
\* drops every cell's source: the host's snapshot is what it saw until now
FinishSources(t) ==
    /\ Idle /\ Running(t) /\ Top(t).kind = "render" /\ Top(t).bkind = "none"
    /\ LET RC == RegCells(CurReg(t)) IN
       cl' = [c \in CellIds |-> IF c \in RC THEN [cl[c] EXCEPT !.source = 0] ELSE cl[c]]
    /\ ex' = Snap(t, Depth(t), FALSE)
    /\ fr' = SetTop(t, [Top(t) EXCEPT !.phase = "fin", !.ads = {}])
    /\ dirty' = TRUE /\ dirtyBy' = W(t)
    /\ UNCHANGED <<epoch, live, ad, nAd, nCell, st, nGen, rg, nEx, gsnap, gatt, ns, hkeep, lag, lock, inc,
                   fails, pendDrop, wr, flag>>

\* Then a global's links detach and the others become owning references to the frozen cell
FinishFreeze(t) ==
    /\ Idle /\ Depth(t) > 0 /\ Top(t).phase = "fin"
    /\ LET r == CurReg(t)
           RC == RegCells(r)
           detach == {a \in AdIds : \E c \in RC : a \in cl[c].links /\ (ad[a].held \/ ~FixFreeze)}
       IN /\ ad' = [a \in AdIds |-> IF a \in detach THEN [ad[a] EXCEPT !.cell = 0, !.seen = 0]
                                   ELSE ad[a]]
          /\ cl' = [c \in CellIds |-> IF c \in RC
                                      THEN [cl[c] EXCEPT !.frozen = TRUE, !.links = {},
                                                         !.inTable = FALSE]
                                      ELSE cl[c]]
          /\ wr' = Mark(wr, {<<"a", a>> : a \in detach} \cup {<<"c", c>> : c \in RC}, t)
          /\ rg' = [rg EXCEPT ![r] = EmptyReg]
          /\ live' = live - Cardinality(RC)
          /\ epoch' = IF RC # {} THEN NextEp(epoch) ELSE epoch
    /\ ex' = Snap(t, Depth(t), TRUE)
    /\ fr' = Pop(t)
    /\ dirty' = TRUE /\ dirtyBy' = W(t)
    /\ UNCHANGED <<nAd, nCell, st, nGen, nEx, gsnap, gatt, ns, hkeep, lag, lock, inc, fails,
                   pendDrop, flag>>

-----------------------------------------------------------------------------
(* Module calls: TemplateModuleState::Call under the module's recursive mutex,
   ModuleCallCollector (BeginCall, DropUnheld, EndCall). A call at depth > 1 runs
   from a host callback *)

FreeAddrs == {o \in OwnedAddrs : ~hkeep[o] /\ ~\E a \in AdIds : ad[a].alive /\ ad[a].idn = o}

\* The pin at a module call's end (FixCallPin): each export of the call records the storage of
\* every module cell it reaches, found by identity in the registry, never through a link
RegStor(k) == IF rg[ModReg].tbl[k] # 0 THEN cl[rg[ModReg].tbl[k]].stor ELSE 0
PinPg(e) == IF ex[e].kind = "live" THEN cl[ex[e].cell].stor ELSE RegStor(ex[e].idn)
PinCg(e) == LET g == PinPg(e) IN
            CASE ~FixPinNested -> -1
              [] g # 0 -> IF st[g].child # 0 THEN RegStor(ad[st[g].child].idn) ELSE 0
              [] ex[e].idn = "P" -> RegStor("C")  \* the host's P holds the host's C
              [] OTHER -> 0
PinVisOf(g, c) == IF c = -1 THEN (IF g = 0 THEN <<0, 0>> ELSE VisG(g)) ELSE <<StVal(g), StVal(c)>>
ToPin(t, d) == IF ~FixCallPin THEN {}
               ELSE {e \in LiveEx : ex[e].thr = t /\ ex[e].depth = d /\ ~ex[e].done
                                    /\ ex[e].reg = ModReg /\ ex[e].kind \in {"live", "view"}}
\* P: the exports the pin records; the others of ToPin(t, d) read nothing that changes (a
\* failed pin: the host's data, or no items for a module cell's view)
PinExOf(t, d, P) ==
    [e \in ExIds |-> CASE e \in P ->
                           [ex[e] EXCEPT !.pin = TRUE, !.pg = PinPg(e), !.cg = PinCg(e), !.cell = 0,
                                         !.done = TRUE, !.snap = PinVisOf(PinPg(e), PinCg(e))]
                       [] e \in ToPin(t, d) ->
                           [ex[e] EXCEPT !.pin = TRUE, !.pg = 0, !.cg = 0, !.cell = 0, !.done = TRUE,
                                         !.snap = <<0, 0>>]
                       [] OTHER -> Snap(t, d, TRUE)[e]]
PinEx(t, d) == PinExOf(t, d, ToPin(t, d))

StartModCall(t) ==
    /\ Idle /\ t \in ModuleThreads /\ CanNest(t)
    /\ lock \in {"none", t}
    /\ nAd + 2 <= MaxAd
    \* The argument: new data at a free address, a generic container (0336) the host may keep a
    \* copy of, or (again) the generic container the host kept, lent once more
    /\ \E o \in OwnedAddrs, gk \in (IF GenericArgs THEN BOOLEAN ELSE {FALSE}), hk \in BOOLEAN :
        LET again == hkeep[o]
            a == nAd + 1
            w == nAd + 2
            k == NextCall(rg[ModReg].last)
            n == IF again THEN inc[o] ELSE inc[o] + 1
        IN /\ IF again THEN gk /\ ~hk ELSE o \in FreeAddrs /\ (hk => gk)
           /\ ad' = [ad EXCEPT ![a] = NewAd("P", 0, FALSE),
                               ![w] = [NewAd(o, n, FALSE) EXCEPT !.generic = gk]]
           /\ inc' = [inc EXCEPT ![o] = n]
           /\ hkeep' = [hkeep EXCEPT ![o] = again \/ hk]
           /\ nAd' = nAd + 2
           /\ rg' = [rg EXCEPT ![ModReg].last = k, ![ModReg].call = k]
           /\ fr' = [fr EXCEPT ![t] = Append(@, [kind |-> "mod", reg |-> ModReg,
                         outer |-> rg[ModReg].call, call |-> k,
                         ads |-> {a, w} \cup (IF ns # 0 THEN {ns} ELSE {}),
                         phase |-> "run"] @@ NoBorrow)]
    /\ lock' = t
    /\ UNCHANGED <<epoch, live, cl, nCell, st, nGen, ex, nEx, gsnap, gatt, ns, lag, fails, dirty,
                   dirtyBy, pendDrop, wr, flag>>

\* The call's arguments go first (Call declares the collector first), then DropUnheld
\* and EndCall run in ~ModuleCallCollector
\* The pin may run out of memory (0338 step D): the call then fails, after the same teardown. With
\* FixAtomicPin it records nothing (P = {}); without, what it recorded before the failure stays
\* recorded and marked (P a part of ToPin), which tears the call's values: some read its end, the
\* others data nothing writes
EndModCall(t) ==
    /\ Idle /\ Running(t) /\ Top(t).kind = "mod" /\ Top(t).bkind = "none"
    /\ LET d == Depth(t)
           TP == ToPin(t, d)
       IN \E pf \in BOOLEAN, P \in SUBSET TP :
           /\ pf => (TP # {} /\ fails < MaxFail)
           /\ IF pf THEN (IF FixAtomicPin THEN P = {} ELSE P # TP) ELSE P = TP
           /\ LET late == \E e \in ExIds :
                             /\ ex[e].alive /\ ex[e].thr = t /\ ex[e].depth = d /\ ~ex[e].done
                             /\ ex[e].kind = "gen"
                             /\ LET c == rg[ModReg].tbl[ex[e].idn]
                                IN c # 0 /\ VisG(cl[c].stor) # VisG(ex[e].gen)
                   stors == UNION {{PinPg(e), PinCg(e)} : e \in P} \ {0, -1}
                   \* The pinned cells are marked shared: their next write copies (Unshare)
                   marked == IF FixPinMarks
                             THEN {c \in RegCells(ModReg) : cl[c].stor \in stors} ELSE {}
               IN /\ ex' = PinExOf(t, d, P)
                  /\ fails' = IF pf THEN fails + 1 ELSE fails
                  /\ flag' = [flag EXCEPT !.late = @ \/ late, !.torn = @ \/ (pf /\ P # {})]
                  /\ cl' = [c \in CellIds |-> IF c \in marked THEN [cl[c] EXCEPT !.exp = SHARED] ELSE cl[c]]
                  /\ wr' = Mark(wr, {<<"c", c>> : c \in marked}, t)
    /\ rg' = [rg EXCEPT ![ModReg].call = IF FixCallRestore THEN Top(t).outer ELSE @]
    /\ fr' = Pop(t)
    /\ lock' = IF \E i \in 1..(Depth(t) - 1) : fr[t][i].kind = "mod" THEN t ELSE "none"
    \* W(t) of the call, not of its caller: TemplateModuleState::Call holds its lock while the
    \* arguments die and the collector is torn down, so the GC step writes under the lock
    \* (Codex review of #688). No other thread steps before that GC: it waits for Idle
    /\ dirty' = TRUE /\ dirtyBy' = W(t)
    /\ pendDrop' = (DropRule # "none")
    /\ UNCHANGED <<epoch, live, ad, nAd, nCell, st, nGen, nEx, gsnap, gatt, ns, hkeep, lag, inc>>

\* A module stores an argument in its namespace: it lives across calls
StoreNs(t, a) ==
    /\ "ns" \in Actions
    /\ CanUse(t, a) /\ Top(t).kind = "mod" /\ ad[a].idn \in OwnedAddrs /\ ns = 0
    /\ ns' = a
    /\ UNCHANGED <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, fr, ex, nEx, gsnap, gatt, hkeep, lag,
                   lock, inc, fails, dirty, dirtyBy, pendDrop, wr, flag>>

\* BorrowedCell::IsUnheld: only the registry holds the cell, only the cell holds its source,
\* and only the source is attached
Unheld(c) ==
    LET R == Close(RootsExcept(c))
        s == cl[c].source
    IN /\ cl[c].owned /\ ~cl[c].frozen /\ s # 0
       /\ <<"c", c>> \notin R
       /\ (DropRule # "nosource") => <<"a", s>> \notin R
       /\ (DropRule # "nolinks") => cl[c].links \subseteq {s}
       \* 0336: the host keeps no copy of the generic container, so it cannot lend it again
       /\ (DropRule # "nohostcopy") => ~hkeep[cl[c].idn]

DropUnheld ==
    /\ ~dirty /\ pendDrop /\ InBound
    /\ LET cand == {c \in RegCells(ModReg) : Unheld(c)} IN
       IF cand = {}
       THEN /\ pendDrop' = FALSE
            /\ lag' = {}
            /\ UNCHANGED <<cl, rg, live, dirty, flag, fails>>
       ELSE \/ \E c \in cand :
               \* Out of the table first, then the source goes
               /\ rg' = [rg EXCEPT ![ModReg].tbl[cl[c].idn] = 0]
               /\ cl' = [cl EXCEPT ![c].inTable = FALSE, ![c].source = 0]
               /\ live' = live - 1
               /\ dirty' = TRUE
               \* A later call that is lent the same data would miss this cell's writes
               /\ flag' = [flag EXCEPT !.lost = @ \/ hkeep[cl[c].idn]]
               /\ UNCHANGED <<pendDrop, lag, fails>>
            \* The list of the cells to drop could not be allocated (0338 step D): none goes, and
            \* the cells stay until the end of the next call, as data the host let go of late
            \/ /\ fails < MaxFail
               /\ fails' = fails + 1
               /\ pendDrop' = FALSE
               /\ lag' = lag \cup {cl[c].idn : c \in cand}
               /\ UNCHANGED <<cl, rg, live, dirty, flag>>
    /\ UNCHANGED <<epoch, ad, nAd, nCell, st, nGen, fr, ex, nEx, gsnap, gatt, ns, hkeep, lock, inc,
                   dirtyBy, wr>>

\* The host lets go of its copy of a generic container (0336)
HostLetsGo(o) ==
    /\ "hostgen" \in Actions
    /\ Idle /\ hkeep[o]
    /\ hkeep' = [hkeep EXCEPT ![o] = FALSE]
    /\ lag' = lag \cup {o}
    /\ UNCHANGED <<epoch, live, ad, nAd, cl, nCell, st, nGen, rg, fr, ex, nEx, gsnap, gatt, ns,
                   lock, inc, fails, dirty, dirtyBy, pendDrop, wr, flag>>

-----------------------------------------------------------------------------

Next ==
    \/ GC
    \/ DropUnheld
    \/ \E t \in Threads :
        \/ StartRender(t) \/ StartModCall(t)
        \/ FinishSources(t) \/ FinishFreeze(t) \/ EndModCall(t)
        \/ EnumStep(t)
        \/ SortEnd(t)
        \/ \E a \in AdIds :
            \/ Write(t, a, "append")
            \/ Write(t, a, "touch")
            \/ Write(t, a, "drop")
            \/ Read(t, a)
            \/ Export(t, a)
            \/ KeepList(t, a)
            \/ StoreNs(t, a)
            \/ EnumStart(t, a)
            \/ SortStart(t, a)
        \/ "release" \in Actions /\ \E e \in ExIds : Release(t, e)
    \/ \E o \in OwnedAddrs : HostLetsGo(o)

Spec == Init /\ [][Next]_vars

-----------------------------------------------------------------------------
(* Invariants *)

Quiet == ~dirty /\ ~pendDrop

\* 1. The host's data is never written. Structural here, as in the code: no action writes
\*    host data, and every write lands in a cell's storage or the template's copy
HostNeverWritten == TRUE

\* 2. What the host keeps never changes after the call that handed it out ended
SnapshotStable ==
    Quiet => \A e \in ExIds : (ex[e].alive /\ ex[e].done) => VisEx(e) = ex[e].snap

\* 3. No object is written by two threads, unless both held the module's lock
NoCrossThreadWrite ==
    \A o \in Objs : \A p, q \in wr[o] : p[1] = q[1] \/ (p[2] /\ q[2])

\* 4. Nothing reads freed storage, and no live adapter points at a freed cell
NoUseAfterFree ==
    /\ ~flag.uaf
    /\ Quiet => \A a \in AdIds : (ad[a].alive /\ ad[a].cell # 0) => cl[ad[a].cell].alive
    /\ Quiet => \A c \in CellIds : cl[c].alive => st[cl[c].stor].acc

\* 5. A lookup never returns a cell of other data, or a live cell of another render
NoForeignCell == ~flag.foreign

\* The host still holds, or let go of since the last DropUnheld, an adapter of the data at o
\* (keep([x]) of an argument) or its copy of a generic container there: the cell stays until the
\* first call's end after the host lets go
HeldOut(o) == o \in lag \/ \E e \in LiveEx : ex[e].kind \in {"view", "raw"} /\ ex[e].idn = o

\* 6. Between module calls the registry keeps no cell of a finished call's own data
RegistryBounded ==
    (Quiet /\ \A t \in Threads : ~InModule(t))
        => Cardinality({o \in OwnedAddrs : rg[ModReg].tbl[o] # 0 /\ ~hkeep[o] /\ ~HeldOut(o)})
               <= (IF ns # 0 THEN 1 ELSE 0)

\* Extra: aliases agree (a read never misses its render's cell), no moved-from item is
\* stored, the live counter matches the tables
AliasesAgree == ~flag.stale
NoUndefItems == ~flag.undef
LiveCount == live = Cardinality({c \in CellIds : cl[c].alive /\ cl[c].inTable})

\* 0335: a module call's export keeps that call's final state (an open task before FixCallPin)
ExportKeepsCallState == ~flag.late

\* 0336: no cell is dropped while its data can be lent again (its writes would be lost)
NoLostWrites == ~flag.lost

\* 0338 step D: a step that runs out of memory changes nothing the step after it reads, but
\* the cleanup its caller runs (the teardown of a failed call, the restore guard of sort)
NoTornStep == ~flag.torn

=============================================================================
