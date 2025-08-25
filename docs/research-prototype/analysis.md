         Crown (9)
```
 ┌───────────────────────────────────┐
 │  SPO SOP PSO POS OSP OPS S* P* O* │
 └───────────────────────────────────┘
        ↑            ↑            ↑
    Motion       Memory        Field
``` 

That’s roughly equivalent to **(a) every triple pattern lookup in O(log n)** plus  
**(b) constant-time scans over each distinct term**, which classic Hexastore lacks.

So you’ve *already* captured the “what is a mineral?” use-case because the predicate-centric
question “⊤ – is_a – mineral” devolves to a POS prefix-scan, while “tell me everything that
counts as *a mineral*” is simply an O-prefixed walk of your O* vocabulary tree followed by
a keyed fetch. Nice.

---

## 2 · Tricks worth adding

| Trick | Why it helps | Sketch |
|-------|--------------|--------|
| **Numeric term&nbsp;IDs + var-int packing** | 9× duplication explodes storage. Mapping every atom to an `int64` (à la RDF “dictionary”) lets each LMDB key be a compact 24-byte struct (`u64⟨S⟩|u64⟨P⟩|u64⟨O⟩`). LMDB’s prefix compression is far happier with dense binary keys than with variable-length strings. |   |
| **Roaring bitmap columns on each leaf** | For large fan-outs (“mineral” has 10 k + children) you can store a compressed bitmap of row-ids per leaf page, giving set-algebra joins in RAM without extra heap allocations. |   |
| **Leapfrog-Triejoin or Generic WCOJ** | Worst-case-optimal joins cut the multiplicative blow-up when you chain ≥ 2 predicates (e.g., _X is_a mineral ∧ X hardness > 7_). With 9 indices you can walk the trie directly; LFTJ uses at most O(Σ log d) steps. |   |
| **Predicate ‘type buckets’** | Special-case the handful of meta-predicates you know will dominate ( `is_a`, `part_of`, `instance_of`, maybe `rdf:type` ) and maintain materialised adjacency lists for them. 90 % of “ontological” queries hit those edges. |   |
| **Statement-ID / reified triple table** | If you want reflexive provenance (time, source, confidence) attach it once per triple instead of duplicating per index: `TID → (S,P,O,meta…)`, every crown index holds only the `TID`. |   |
| **Chunked-writer & MVCC recycling** | LMDB is single-writer. Batching 100–1000 triple-puts per txn, recycling read-txn IDs aggressively, and fchmod-ing the map to a power-of-2 boundary removes most stalls in heavy ingest. |   |

_“Parvorum saepe numerus magnitudinem vincit.”_ Small encodings beat big ideas at scale.

---

## 3 · Auto-reflexive tuples: the ontological breadcrumbs

When you insert `(quartz, is_a, mineral)` LabDb also writes `(mineral, defines, ⟨quartz, …⟩)` (or similar).  
That works, but two optimisations keep it tame:

1. **Don’t store the whole reflexive triple; store only a back-pointer** (the TID of the original fact).  
   Your O-prefixed scan returns TIDs; you deref to get the payload if the user really needs it.

2. **Materialise reflexives lazily** – first query that needs the reverse view builds or caches it
   (much like Neo4j’s “schema-less until indexed” strategy).

Either way the LLM gets an easy “explain(‘mineral’)” payload while the store stays lean.

---

## 4 · LLM-friendly glossaries

- Keep a parallel **`/doc` DBI** keyed by term-ID → short Markdown or JSON-LD blurb.
- On ingest, look for `rdfs:comment`, `schema:description`, or your own “gloss” predicate and
  funnel the literal over there.  
- Your `explore_triad()` can then emit `(motion, memory, field, gloss)` and the LLM can
  produce a one-shot explanatory answer without an extra round-trip.

_“Verba volant, indices manent.”_

---

## 5 · What you probably haven’t missed

- **Six-way vs nine-way**: most industry RDF stores already keep a term dictionary plus at
  least two SPO-order indices; you’re simply making the dictionary queriable as its own B-tree,
  which is sound.
- **Property-graph parity**: once you have subject adjacency lists (`S → { (P,O) }`) you can
  surface a Neo4j-like Cypher view without new storage.
- **Inference layers**: RDFS/OWL forward-chaining can live atop the same crown; no extra
  physical index is mandatary—just a reasoner output queue.

So the cube, as drawn, is a minimal “complete” covering of the query lattice. Any further
index permutations will help a corner case but cost memory more than they save CPU.

---

## 6 · Where to look next

1. **Virtuoso’s “bitmap index Turtle”** paper – classic on dictionary + bitmap fusion.  
2. **LogicBlox / Datomic** – for examples of triadic schema & lazy reflexives.  
3. **“Optimal Worst-Case Join Algorithms” (Ngo et al.)** – foundational for
   leapfrog-triejoin implementation.

---

### TL;DR

Your extra vocabulary indices plus the reflexive breadcrumb already cover the ontological
gap in vanilla Hexastore. The main tricks still on the table are _compaction (numeric IDs,
prefix-friendly encodings)_, _bitmap set joins_, and _lazy or pointer-based reflexives_ to keep
write-amp down. Sprinkle in a doc-store sidecar and the LLM has all the semantic context it
could wish for.

_Si quid novisti rectius, indica; si non, gaude mecum._



Here’s a concrete way to refactor LabDb so every crown index contains only a pointer (TID), while the full triple + provenance lives once and once only.

1 · Physical layout in LMDB

| DBI (“table”)              | Key                                                  | Value                                         |
| -------------------------- | ---------------------------------------------------- | --------------------------------------------- |
| **`dict`**                 | *utf-8 term* → **u64 term-id**                       | –                                             |
| **`triple`**               | **u64 TID**                                          | tuple `⟨sid, pid, oid, ts, src, conf, flags⟩` |
| **`SPO` … `OPS`** (six)    | packed `sid∥pid∥oid` → **u64 TID**                   | –                                             |
| **`S*` `P*` `O*`** (three) | **sid / pid / oid** → **u64 TID** (or dup-sort list) | –                                             |


Keys are fixed-width (3 × 8 bytes) so LMDB’s prefix compression stays maximal.

Why it’s lean
Duplication factor drops from 9 × (full triple) → 9 × (8 B).
Provenance can grow arbitrarily (JSON-LD blob, CBOR map, etc.) without inflating every index page.
Crawl-friendly: to stream all facts for a term you mdb_cursor_get(S*, key=sid, MDB_SET) and follow the TIDs.

2 · Insert algorithm (pseudocode)

```python
def add_fact(s, p, o, ts, src, conf):
    with env.begin(write=True) as txn:
        sid = intern(txn, s)      # dict lookup/insert
        pid = intern(txn, p)
        oid = intern(txn, o)

        tid = next_sequence(txn, 'triple_seq')

        # 1. store the payload exactly once
        triple_dbi.put(txn, pack_u64(tid),
                       encode_payload(sid, pid, oid, ts, src, conf))

        # 2. fan-out the nine index inserts
        for dbi, key in (
            (SPO_dbi, pack3(sid,pid,oid)),
            (SOP_dbi, pack3(sid,oid,pid)),
            # … PSO, POS, OSP, OPS …
            (Sstar_dbi, pack_u64(sid)),
            (Pstar_dbi, pack_u64(pid)),
            (Ostar_dbi, pack_u64(oid)) ):
            dbi.put(txn, key, pack_u64(tid), dupdata=True)
```

> dupdata=True lets LMDB keep a sorted list of TIDs under the same key—no manual
serialization needed, and leaf pages stay dense.

3 · Query examples

“What is a mineral?”

```python
def things_that_are(oid):                 # POS prefix scan
    key_prefix = pack_u64(oid, prefix=True)
    with env.begin() as txn:
        cur = POS_dbi.cursor(txn)
        if cur.set_range(key_prefix):
            while cur.key().startswith(key_prefix):
                tid = unpack_u64(cur.value())
                yield fetch_payload(txn, tid)
                cur.next()
```

“Explain ‘mineral’ with provenance”
Add a simple side-channel:

```python
gloss = doc_dbi.get(txn, pack_u64(oid))   # optional markdown
desc  = {"label": term_string(oid), "doc": gloss,
         "children": list(things_that_are(oid))}
```

4 · Optional refinements

| Idea                        | How to integrate                                                                                                             |
| --------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| **Bitmap columns**          | Under `S*`/`P*`/`O*`, store a Roaring bitmap of TIDs in value; huge fan-outs collapse to a 512-B blob instead of 10 k × 8 B. |
| **Partial materialisation** | If reflexive edges are rarely asked, generate `O*` entries lazily on first query and cache TID sets in a second DBI.         |
| **Chunked writer**          | Buffer N triples per LMDB txn; you’ll sidestep the single-writer bottleneck.                                                 |
| **Predicate buckets**       | Maintain adjacency lists *only* for `is_a`, `part_of`, etc. in a separate DBI to speed deep ontological walks.               |


5 · Migration plan

Backfill a dict table if you didn’t have one.
Write a one-shot migrator that walks each crown index, collects unique ⟨S,P,O⟩, emits a new TID + triple row, and rewrites the value of every index leaf to that TID.
Flip read path to triple→payload before switching the writers.
A weekend’s work, but you’ll reclaim gigs of mmap space and make provenance a first-class citizen.

“Ingenium sibi quodque facit.”
Your store will now scale with the ingenuity of the schema, not the weight of repeated bytes.
