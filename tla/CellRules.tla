----------------------------- MODULE CellRules -----------------------------
(***************************************************************************)
(* The step rules of the borrowed-cell protocol, as operators of plain     *)
(* values: what a lookup finds, the mark an export leaves, what a write    *)
(* does to a marked cell, which links the end of a render detaches and     *)
(* which cells DropUnheld may drop. CellTrace.tla checks a logged C++      *)
(* trace with them; CellProtocol.tla states the same rules inline, in the  *)
(* actions Write/WC, Export, FinishFreeze and Unheld (see tla/README.md,   *)
(* "Trace validation", for the mapping). docs/tasks/0338 step B.           *)
(***************************************************************************)
EXTENDS Integers

\* The m_exportedIn no call has: an export in a call other than the one that marked the cell
\* (BorrowedCell::SharedExport, #689; CellProtocol's SHARED)
SHARED == -1

\* CellLink::Find: the cell a lookup in a registry's table links the adapter to, or 0. tbl maps
\* an identity to a cell, isListOf a cell to whether it holds a list
FindResult(tbl, idn, isList, isListOf(_)) ==
    IF idn \in DOMAIN tbl /\ tbl[idn] # 0 /\ isListOf(tbl[idn]) = isList THEN tbl[idn] ELSE 0

\* BorrowedCell::NoteExport: only in the cell's own registry (own), only in a module call
\* (call # 0), never on a frozen cell
NoteApplies(frozen, own, call) == ~frozen /\ own /\ call # 0

\* The mark an export in call `call` leaves on a cell marked exp. fixNested: CellProtocol's
\* FixNestedExport (#689); without it a mark stays as the first call set it
ExportMark(exp, call, fixNested) ==
    IF exp = 0 THEN call
    ELSE IF fixNested /\ exp # call THEN SHARED
    ELSE exp

MarkAfterExport(frozen, own, exp, call, fixNested) ==
    IF NoteApplies(frozen, own, call) THEN ExportMark(exp, call, fixNested) ELSE exp

\* BorrowedCell::MutableItems calls Unshare for a frozen or marked cell
NeedsUnshare(frozen, exp) == frozen \/ exp # 0

\* What Unshare may do (CellProtocol's WC): nothing writes a frozen cell; the marking call writes
\* in place; any other call either writes in place, as the host let go of every value that shared
\* the items ("released"), or copies them first ("copy"). Which of the two is the host's choice
\* (OwnsMutableItemsAlone), not the protocol's
UnshareOutcomes(frozen, exp, call) ==
    IF frozen THEN {"frozen"} ELSE IF exp = call THEN {"own"} ELSE {"released", "copy"}

MarkAfterUnshare(kind, exp) == IF kind \in {"released", "copy"} THEN 0 ELSE exp

\* A write goes in place only to a cell that is unmarked or marked by the call that runs
WritesInPlace(frozen, exp, call) == ~frozen /\ (exp = 0 \/ exp = call)

\* BorrowedCells::Finish: a global's link detaches (the thread's snapshot outlives the render),
\* every other link keeps the frozen cell (CellProtocol's FinishFreeze with FixFreeze)
FinishDetaches(isHeld) == isHeld

\* BorrowedCell::IsUnheld, the part the protocol sees (the reference counts are the host's):
\* data that lives only as long as its adapters, a live cell, and no link but the source's
MayDrop(owned, frozen, links, source) == owned /\ ~frozen /\ links \subseteq {source}

=============================================================================
