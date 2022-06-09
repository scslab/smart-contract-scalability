There's a bunch of ways to run the sequencing.

My initial thought was "run all of the sets, then run all of the add/subs".

The issue with this is that what if the set call then fails later because of the add/subs?
some sub calls might then fail.

So ok, we could do something like : if current value = 10, set to 20, sub 10 is ok but sub 15 is bad.
Then, mid tx, the tx that sets the value should read 20 (+/- any local deltas),
and a tx that doesn't set should read 10 (+/- local deltas).

This is ok.  But for this to be useful, the subs have to be conditioned on the sets.
(i.e. the semaphore pattern of "set 1 | sub 1" doesn't work).

That conditioning makes the implementation really hard to parallelize well.
One of the nice things about the isolated deltas is that we can parallelize
not just over keys but also between deltas on keys, using something like a distributed
sorting alg.

Could require everything transacting on something to do "set X | whatever", not just "whatever"
(so they'd all share the same conditional context), but this is kind of annoying

What if we just did a set_sub operation? that seems like it solves all of our problems.