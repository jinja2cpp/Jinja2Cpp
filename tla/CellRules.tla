----------------------------- MODULE CellRules -----------------------------
(***************************************************************************)
(* The step rules of the borrowed-cell protocol, as operators of plain     *)
(* values: what a lookup finds, which links own their cell, how a module   *)
(* call's export reads and its pin marks, what a write does to a marked    *)
(* cell, which links the end of a render detaches and which cells          *)
(* DropUnheld may drop. CellTrace.tla checks a logged C++ trace with      *)
(* them; CellProtocol.tla states the same rules inline, in the             *)
(* actions Write/WC, Export, FinishFreeze and Unheld (see tla/README.md,   *)
(* "Trace validation", for the mapping). docs/tasks/0338 step B.           *)
(***************************************************************************)
EXTENDS Integers

\* m_shared in the log's "exp" (CellProtocol's SHARED): a module call's pin marked the cell
SHARED == -1

\* CellLink::Find: the cell a lookup in a registry's table links the adapter to, or 0. tbl maps
\* an identity to a cell, isListOf a cell to whether it holds a list
FindResult(tbl, idn, isList, isListOf(_)) ==
    IF idn \in DOMAIN tbl /\ tbl[idn] # 0 /\ isListOf(tbl[idn]) = isList THEN tbl[idn] ELSE 0

\* CellLink::Link: a link owns its cell (a reference count) unless it is a global's, which the
\* cell lists and the end of the render detaches (CellProtocol's FixModuleLinksOwn, extended to
\* a render's cells for 0340)
LinkOwns(isHeld) == ~isHeld

\* BorrowedCell::ExportList: in the cell's own module call (own, call # 0) the value reads through
\* the call's CallExports, live until the call ends (CellProtocol's FixCallPin); it marks nothing
ExportLive(frozen, own, call) == ~frozen /\ own /\ call # 0

\* BorrowedCell::MutableItems calls Unshare for a frozen or marked cell
NeedsUnshare(frozen, exp) == frozen \/ exp # 0

\* What Unshare may do (CellProtocol's WC): nothing writes a frozen cell; a marked one is written
\* either in place, as the host let go of every value that shared the items ("released"), or after
\* a copy ("copy"). Which of the two is the host's choice (OwnsItemsAlone), not the protocol's
UnshareOutcomes(frozen, exp) ==
    IF frozen THEN {"frozen"} ELSE {"released", "copy"}

MarkAfterUnshare(kind, exp) == IF kind \in {"released", "copy"} THEN 0 ELSE exp

\* A write goes in place only to an unmarked cell
WritesInPlace(frozen, exp) == ~frozen /\ exp = 0

\* CallExports::Pin, at a module call's end: the pinned cell is marked
MarkAfterPin == SHARED

\* BorrowedCells::Finish: a global's link detaches (the thread's snapshot outlives the render),
\* every other link keeps the frozen cell (CellProtocol's FinishFreeze with FixFreeze)
FinishDetaches(isHeld) == isHeld

\* BorrowedCell::IsUnheld, the part the protocol sees (the reference counts are the host's):
\* data that lives only as long as its adapters, a live cell, and no link (owning or listed) but
\* the source's
MayDrop(owned, frozen, links, source) == owned /\ ~frozen /\ links \subseteq {source}

=============================================================================
