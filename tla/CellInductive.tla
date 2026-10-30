--------------------------- MODULE CellInductive ---------------------------
(***************************************************************************)
(* An inductive invariant of CellProtocol, for Apalache (tla/apalache.py). *)
(* IndInv holds initially, IndInv /\ Next => IndInv', and IndInv implies   *)
(* the invariants check.py gives TLC: together, they hold in every         *)
(* reachable state, at any depth, for the constants of the scope.          *)
(***************************************************************************)
EXTENDS CellApalache

Safety == SnapshotStable /\ NoCrossThreadWrite /\ NoUseAfterFree /\ NoForeignCell
          /\ RegistryBounded /\ AliasesAgree /\ NoUndefItems /\ LiveCount
          /\ ExportKeepsCallState /\ NoLostWrites

\* Probes of the encoding's cost
NextStart == \E t \in Threads : StartModCall(t)
NextEnd == \E t \in Threads : EndModCall(t)
NextGC == GC
NextDrop == DropUnheld
NextUse == \E t \in Threads : \E a \in AdIds : Write(t, a, "append") \/ Read(t, a) \/ Export(t, a)
NextSG == NextStart \/ NextUse \/ NextGC
NextSE == NextStart \/ NextUse \/ NextEnd

-----------------------------------------------------------------------------
(* Types: the shape of every variable. IndInit starts from any such state *)

Ids(n) == 0..n
\* @type: ($reg) => Bool;
RegOk(r) == r \in RegIds
\* @type: ($ad) => Bool;
AdOk(A) ==
    /\ A.idn \in Keys \cup {"none"} /\ A.inc >= 0 /\ A.held \in BOOLEAN /\ A.generic \in BOOLEAN
    /\ A.cell \in Ids(MaxCell) /\ (A.seen = ATT \/ A.seen \in 0..MaxEpoch) /\ A.alive \in BOOLEAN
\* @type: ($cell) => Bool;
CellOk(C) ==
    /\ C.idn \in Keys \cup {"none"} /\ C.inc >= 0 /\ RegOk(C.reg)
    /\ C.stor \in Ids(MaxGen) /\ C.retired \in Ids(MaxGen) /\ C.source \in Ids(MaxAd)
    /\ C.owned \in BOOLEAN /\ C.frozen \in BOOLEAN
    /\ (C.exp = SHARED \/ C.exp \in 0..MaxCallId)
    /\ C.links \subseteq AdIds /\ C.inTable \in BOOLEAN /\ C.alive \in BOOLEAN
\* @type: ($gen) => Bool;
GenOk(G) == G.val \in {EMPTY, UNDEF} \cup 0..MaxVal /\ G.child \in Ids(MaxAd)
            /\ G.alive \in BOOLEAN /\ G.acc \in BOOLEAN
\* @type: ($frame) => Bool;
FrameOk(F) ==
    /\ F.kind \in {"render", "mod"} /\ RegOk(F.reg) /\ F.outer \in 0..MaxCallId
    /\ F.call \in 0..MaxCallId /\ F.ads \subseteq AdIds /\ F.phase \in {"run", "fin"}
    /\ F.bkind \in {"none", "enum", "sort"} /\ F.bad \in Ids(MaxAd) /\ F.bgen \in Ids(MaxGen)
    /\ F.bvals \in {EMPTY, UNDEF} \cup 0..MaxVal
\* @type: ($export) => Bool;
ExOk(E) ==
    /\ E.kind \in {"none", "gen", "live", "view", "raw"} /\ E.gen \in Ids(MaxGen)
    /\ E.ad \in Ids(MaxAd) /\ E.cell \in Ids(MaxCell) /\ E.idn \in Keys \cup {"none"}
    /\ E.thr \in Threads \cup {"none"} /\ E.depth \in 0..MaxDepth /\ RegOk(E.reg)
    /\ E.done \in BOOLEAN /\ E.pin \in BOOLEAN /\ E.pg \in Ids(MaxGen) /\ E.cg \in {-1} \cup Ids(MaxGen)
    /\ E.alive \in BOOLEAN
    /\ Len(E.snap) \in {0, 2}
\* @type: ($table) => Bool;
TableOk(T) ==
    /\ DOMAIN T.tbl = Keys /\ \A k \in Keys : T.tbl[k] \in Ids(MaxCell)
    /\ T.call \in 0..MaxCallId /\ T.last \in 0..MaxCallId

TypeOK ==
    /\ epoch \in 0..MaxEpoch /\ live \in 0..MaxCell
    /\ DOMAIN ad = AdIds /\ \A a \in AdIds : AdOk(ad[a])
    /\ DOMAIN cl = CellIds /\ \A c \in CellIds : CellOk(cl[c])
    /\ DOMAIN st = GenIds /\ \A g \in GenIds : GenOk(st[g])
    /\ nAd \in Ids(MaxAd) /\ nCell \in Ids(MaxCell) /\ nGen \in Ids(MaxGen) /\ nEx \in Ids(MaxEx)
    /\ DOMAIN rg = RegIds /\ \A r \in RegIds : TableOk(rg[r])
    /\ DOMAIN fr = Threads /\ \A t \in Threads : Len(fr[t]) <= MaxDepth
                                               /\ \A i \in Frames(t) : FrameOk(fr[t][i])
    /\ DOMAIN ex = ExIds /\ \A e \in ExIds : ExOk(ex[e])
    /\ DOMAIN gsnap = Threads /\ \A t \in Threads : gsnap[t] \in Ids(MaxAd)
    /\ DOMAIN gatt = Threads /\ \A t \in Threads : gatt[t] \in BOOLEAN
    /\ ns \in Ids(MaxAd) /\ lock \in Threads \cup {"none"}
    /\ DOMAIN hkeep = OwnedAddrs /\ \A o \in OwnedAddrs : hkeep[o] \in BOOLEAN
    /\ lag \subseteq OwnedAddrs
    /\ DOMAIN inc = OwnedAddrs /\ \A o \in OwnedAddrs : inc[o] >= 0
    /\ fails \in 0..MaxFail
    /\ dirty \in BOOLEAN /\ pendDrop \in BOOLEAN
    /\ dirtyBy[1] \in Threads \cup {"none"} /\ dirtyBy[2] \in BOOLEAN
    /\ DOMAIN wr = Objs /\ \A o \in Objs : wr[o] \subseteq Threads \X BOOLEAN


IndInv == WellFormed /\ Safety

\* An invariant no state satisfies: `apalache.py sat` checks that IndInit has a state at all
NoState == FALSE

\* Any state of the right shape: Apalache's Gen picks its values, TypeOK constrains them. Gen
\* takes a literal: tla/apalache.py passes the sizes of the domains of the scope (IndInit in MCApalache): Gen(n) bounds
\* every set and function inside the value by n
IndInitOf(NAd, NCell, NGen, NReg, NFr, NEx, NThreads, NOwned, NWr) ==
    /\ epoch = Gen(1) /\ live = Gen(1) /\ ad = Gen(NAd) /\ nAd = Gen(1) /\ cl = Gen(NCell)
    /\ nCell = Gen(1) /\ st = Gen(NGen) /\ nGen = Gen(1) /\ rg = Gen(NReg)
    /\ fr = Gen(NFr) /\ ex = Gen(NEx) /\ nEx = Gen(1) /\ gsnap = Gen(NThreads)
    /\ gatt = Gen(NThreads) /\ ns = Gen(1) /\ hkeep = Gen(NOwned)
    /\ lag = Gen(NOwned) /\ lock = Gen(1)
    /\ inc = Gen(NOwned) /\ fails = Gen(1) /\ dirty = Gen(1)
    /\ dirtyBy = Gen(1) /\ pendDrop = Gen(1) /\ wr \in [Objs -> SUBSET (Threads \X BOOLEAN)] /\ flag = Gen(1)
    /\ TypeOK
    /\ IndInv


=============================================================================
