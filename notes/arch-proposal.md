
# Goals

The high-level goals are to achieve a good balance of expressivity and
analyzability without sacrificing asymptotic scalability.
Specifically, we must be able to support the following use cases:

* Basic payments

* Interoperability with (other) blockchains and CBDCs
    * Atomic swap
        * Bilateral payments with other central banks
    * Locking up funds for use on a sidechain
    * Interoperability with third-party exchanges
        * Move funds to SPEEDEX
        * FX spot application
    * Auditability
        * Anyone can verify ledger obeys stated rules
        * Concise proofs (SPV) can trigger actions in other ledgers

* Payment channels for high frequency or small payments

* Multipart transactions (Tx2 only valid if Tx1 executed successfully)

* Auctions
    * Liquidation events (auctioning collateral)

* Scriptable policies
    * Set positive or negative interest rates
    * Tune how much banks can lever their base money

Scalability means that more hardware will allow more transactions per
second.  Should scale to multi-core from the start, and multi-node
without required any chances to the transaction format.

We also need to support organizations (e.g., a large commercial bank
or the IRS) that issue a high volume of transactions.  Such an
organization may have many machines spread across multiple data
centers, and should be able to issue a high rate of payments without
much coordination.  Keeping track of UTXOs and change is likely not
convenient for such organizations.  Rather, they must be able to issue
multiple debits concurrently on the same accounts.  However, overdraft
is expected to be rare occurrence.  Money must be conserved, but
overdrafts need not be handled efficiently---e.g., if payments from a
given account in a given block exceed the balance, it is okay for all
payments from that account to fail in the block.

# High-level approach

At a high level, we adopt two design principles:

1. Side effects come from a limited and well-defined set of (mostly)
   commutative operations (e.g., payment, set key-value pair in a map,
   etc.), and

2. Validation rules for transactions must be extensible and
   user-definable.

3. Validation scrips run on a ledger snapshot and use an explicit
   semaphore primitive to prevent replay.

The first principle helps ensure scalability by ensuring transactions
can be executed in parallel (following the scalable commutativity
rule).  It also facilitates analysis of all CBDC activity, which can
be parsed into well-defined actions without execution of any scripts.

The second principle ensures extensibility.  For instance, it allows
accounts to predicate actions on new digital signature schemes, or on
submission of proof that something occurred on a third-party
blockchain.

The third principle is the most novel.  Running validation scripts on
a ledger snapshot allows concurrent execution of all validation.
However, it also runs the risk of enabling transaction replay or
double spend.  For instance, a validation script might check a
sequence number and ensure the side effects include increasing the
sequence number.  However, since the validation script runs on a
ledger snapshot, the script could validate the same sequence number
multiple times in the same block.  To guard against this, the
validation scripts employ semaphores to protect against concurrent
access to the same sequence number by two transactions in the same
block.

Transaction contents is as follows:

* medatata (version, gas-limit, etc.)
* synchronization
* commutative side-effects (sequence numbers, payments)
* witness data (signatures, etc.)
