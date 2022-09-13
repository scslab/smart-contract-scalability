# Language Spec

I've highlighted "main ideas" and "decisions" below.  The decisions are all approximately
independent.  I've included my personal opinions on them, but many of the decisions depend on
the context in which this system is deployed (and either option is viable).

## Goals

- Scalability
	- Guaranteed Parallelism
	- Commutative Semantics

- Expressivity
	- We should be able to do all the important use cases
	- We should be ideally able to do nearly everything

## State Model

- "Addresses": 32 bytes, either hash of contract script or a public key.
- Every address has an implicit map from [32 bytes] -> "Object"
	- *Minor Decision*: It doesn't particularly matter that the keys are 32 bytes - we can 
	just make them hashes of longer keys.  
	Fixed length keys makes implementation simpler.

- "Objects"
	- Int64 (balances)
	- Byte strings
	- Semaphores
	- Sequencers
	- ...

## Transaction Spec

Contents:
- replay-info                          	    ]  hashed to put in replay mechanism
- metadata (gas limit, fee)                 ]
- exec-script                               ]  
	- script address 						]
	- method                                ]
	- call-data                             ]
- witness-data
	- any reflection logs? (see later)
	- sequencer choice? (see later)
	- regular witness data

## Exec script semantics:

Script output is a list of object deltas.

This is *Idea 1* - scripts output a list of deltas, which are then merged together,
instead of requiring strict serializability.

### Deltas:
- Int64:
	- "Set-Add" : Set value to X_i, then add Y_i
		- All sets in one batch should share the same X=X_i
		- Invariant: X - \Sum_i Y_i >= min(0, X)
- Bytestring:
	- Set: Set value to X
		- All sets in one batch should share the same X

- Semaphores:
	- Value: X (int64, default: 1)
	- Deltas:
		- Set X (all sets must be same value in a batch)
		- consume/acquire: Y_i
			- Invariant: \Sum_i Y_i <= X
		- Sets do not take effect until the next batch
		- Consumes do not change the value as it would appear in the next batch
	- HW Exercise: show that Int64 Deltas can implement semaphore semantics

- Sequencers:
	- *Decision* we could just ignore this.
	- A sequencer is a ledger object.
	- API: "acquire()" -> index
	- A tx can only acquire one sequencer, at most.
	- The output of acquire is nondeterministic - chosen by the node.
	- A tx sees the output of txs that acquired that sequencer with a lower index in the ledger.
	- Choice of sequencer should be specified within the tx.
	- Details:
		- This enables things like CFMMs that require a lock on state (and otherwise could only be modified once per block).
		- Slightly tricky, but doable, to determine a restricted set of semantics/requirements that ensure 
		no conflicts when two txs on different sequencers modify the same data.
		- HW Exercise: show that pre-assembly works fine with sequencers (just assign one core per sequencer, at most).


### *Decision* Execution Script Representation

We could represent a script as either (1) a function call (as above, i.e.
an address, method, calldata) that outputs a list of deltas,
or as a function call that checks the validity of a prespecified list of deltas.

These options are equivalent, in terms of the effects that they can achieve.
A type 2 script could just run a type 1 script, and then check that the output of the type 1 script
match exactly with the prespecified list.
And a type 1 script can just output a list of deltas.

There are intermediate options, in which certain high-level deltas (i.e. event logs or other metadata)
are included in the tx, and are required to be checked by the execution script.

#### Opinion
Option 1 saves bandwidth (shorter txs) and makes life easier for users - no need to pre-execute txs locally to precompute
a list of deltas (when building the tx).
It is possible but hard to believe that validation could be slightly faster than execution.
As such, I think option 1 is preferable.

Including some information in the tx script (as in option 2) enables concurrent invocation of any reflection script (see later section).
This may matter when assembling blocks.  But I find it unlikely that in a block with 10s of thousands of txs
and only a few script invocations per tx that parallelism within a tx matters.

### *Decision* Pre-Assembly vs Filtering

Options:
1. Come to consensus on a set of txs, then filter out txs that conflict.
2. Require a block of txs to have no conflicts

Either one is viable (Filtering is arbitrarily parallelizable, in a map-reduce style).
Option 1 makes more sense with a consensus protocol like Narwhal/Tusk, or with an unstable leader (i.e. PoS/PoW).
Option 2 makes more sense with a stable leader (i.e. for a CBDC), since you save on the filtering step.

#### Opinion
Don't really care that much.  Depends on deployment context.
Pre-assembly is fairly straightforward with essentially each thread acting as a 2PC coordinator for its current tx.

## Reflection Scripts

This is *Idea 2*

It might be useful for a CBDC to require certain "authorization scripts" on txs.
If a script takes a certain action, for example, then some other script must run and return "success".

We're looking for something in between a full execution script call and an authorization token.

These scripts logically run _after_ the execution script, and can see the transaction data (witness data), but _cannot_ see 
ledger data.

*Decision*: What else can reflection script see?
	- My opinion: It should see only the witness data and any information explicitly made available for reflection
	by the execution script.

My opinion is that the right granularity for triggering these scripts is _not_ output deltas, it is something slightly higher level.
Deltas are too low-level to make sense of directly in many circumstances, I think.
It would be more natural to express scripts as acting in response to higher level events (i.e. "International Transfers" instead of 
"adjust an int64").

Scripts are registered with the ledger:  "In response to action X, reflection script Y must run".

There are, in my opinion, two natural options for actions that trigger reflection scripts: (1) function calls 
(instrumented e.g. within WASM blobs) or (2) explicit "event" logging (or (3), deltas).

*Decision* In line with the prior decision on execution script representation:
	One compromise between the two options is that the tx could include a list of "reflection" logs (or deltas).
	These could be visible to reflection scripts (thereby enabling execution of the reflection scripts concurrently with the execution
	scripts) and then the tx fails if it did not output these logs.
	From the reflection logs, a node operator can determine the list of required scripts.  If the execution script triggers
	some reflection script that is not triggered by one of the included logs, then the tx fails.






## Proposal Filtering

what happens for filtering on new ledger entries?








