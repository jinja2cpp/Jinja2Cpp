--------------------------- MODULE CellApalache ---------------------------
(***************************************************************************)
(* The types of CellProtocol's constants and variables, for Apalache. TLC  *)
(* never reads this module; tla/apalache.py runs it. The \* @type comments *)
(* in CellProtocol.tla use the aliases defined here.                       *)
(***************************************************************************)
EXTENDS Integers, Sequences, FiniteSets
CONSTANTS
    \* @type: Set(Str);
    Threads,
    \* @type: Set(Str);
    OwnedAddrs,
    \* @type: Int;
    MaxDepth,
    \* @type: Int;
    MaxLevel,
    \* @type: Int;
    MaxAd,
    \* @type: Int;
    MaxCell,
    \* @type: Int;
    MaxGen,
    \* @type: Int;
    MaxEx,
    \* @type: Int;
    MaxVal,
    \* @type: Int;
    MaxFail,
    \* @type: Set(Str);
    ModuleThreads,
    \* @type: Set(Str);
    RenderThreads,
    \* @type: Set(Str);
    Actions,
    \* @type: Bool;
    FixLinkEpoch,
    \* @type: Bool;
    FixEnumRefetch,
    \* @type: Bool;
    FixSortRefetch,
    \* @type: Bool;
    FixCallRestore,
    \* @type: Bool;
    FixNestedGlobal,
    \* @type: Bool;
    FixMarkOrder,
    \* @type: Bool;
    FixSortMutableFirst,
    \* @type: Bool;
    FixMarkerAfterCopy,
    \* @type: Bool;
    FixReadOnlyExport,
    \* @type: Bool;
    FixFreeze,
    \* @type: Bool;
    FixSourceKeepsId,
    \* @type: Str;
    DropRule,
    \* @type: Bool;
    FixNestedExport,
    \* @type: Bool;
    FixCallPin,
    \* @type: Bool;
    FixPinMarks,
    \* @type: Bool;
    FixPinNested,
    \* @type: Bool;
    FixModuleLinksOwn,
    \* @type: Bool;
    FixGenericOwner,
    \* @type: Bool;
    FixPinOnAttach,
    \* @type: Bool;
    FixLinksOwn,
    \* @type: Bool;
    HostHandoff,
    \* @type: Int;
    MaxCallId,
    \* @type: Int;
    MaxEpoch,
    \* @type: Bool;
    AllowModuleKeepList,
    \* @type: Bool;
    AllowModuleNested,
    \* @type: Bool;
    GenericArgs

\* @typeAlias: reg = <<Str, Int>>;
\* @typeAlias: ad = {idn: Str, inc: Int, held: Bool, generic: Bool, cell: Int, seen: Int, alive: Bool};
\* @typeAlias: cell = {idn: Str, inc: Int, reg: $reg, stor: Int, retired: Int, source: Int, owned: Bool, frozen: Bool, exp: Int, links: Set(Int), inTable: Bool, alive: Bool};
\* @typeAlias: gen = {val: Int, child: Int, alive: Bool, acc: Bool};
\* @typeAlias: table = {tbl: Str -> Int, call: Int, last: Int};
\* @typeAlias: frame = {kind: Str, reg: $reg, outer: Int, call: Int, ads: Set(Int), phase: Str, bkind: Str, bad: Int, bgen: Int, bvals: Int};
\* @typeAlias: export = {kind: Str, gen: Int, ad: Int, cell: Int, idn: Str, thr: Str, depth: Int, reg: $reg, done: Bool, snap: Seq(Int), pin: Bool, pg: Int, cg: Int, alive: Bool};
\* @typeAlias: flags = {uaf: Bool, foreign: Bool, stale: Bool, undef: Bool, late: Bool, lost: Bool};
\* @typeAlias: write = {k: Str, cl: Int -> $cell, st: Int -> $gen, nGen: Int, gen: Int};
\* @typeAlias: get = {c: Int, A2: $ad, link: Int, failed: Bool};
CellAliases == TRUE

VARIABLES
    \* @type: Int;
    epoch,
    \* @type: Int;
    live,
    \* @type: Int -> $ad;
    ad,
    \* @type: Int;
    nAd,
    \* @type: Int -> $cell;
    cl,
    \* @type: Int;
    nCell,
    \* @type: Int -> $gen;
    st,
    \* @type: Int;
    nGen,
    \* @type: $reg -> $table;
    rg,
    \* @type: Str -> Seq($frame);
    fr,
    \* @type: Int -> $export;
    ex,
    \* @type: Int;
    nEx,
    \* @type: Str -> Int;
    gsnap,
    \* @type: Str -> Bool;
    gatt,
    \* @type: Int;
    ns,
    \* @type: Str -> Bool;
    hkeep,
    \* @type: Set(Str);
    lag,
    \* @type: Str;
    lock,
    \* @type: Str -> Int;
    inc,
    \* @type: Int;
    fails,
    \* @type: Bool;
    dirty,
    \* @type: <<Str, Bool>>;
    dirtyBy,
    \* @type: Bool;
    pendDrop,
    \* @type: <<Str, Int>> -> Set(<<Str, Bool>>);
    wr,
    \* @type: $flags;
    flag

INSTANCE CellProtocol

=============================================================================
