LLRFS Actions Framework
=======================

The most general interface is to use callbacks, also
known as continuation-passing style.
With callbacks, the completion of an operation can be
scheduled separately from the code that triggers the
operation and processes the result.

This is particularly relevant for LLRFS, as LLRFS talks
to block devices.
When we read from a block device, we have to wait for
the data to be delivered.
The interface to the block device thus needs to be
given a callback (continuation) which it will call once
the data has arrived.

During unit tests, we can add simple emulation that loads
data from the model and just calls the callback immediately.
However, when integrated into a kernel, we use whatever
kernel mechanisms exist to determine when the data has
arrived and register the callback somewhere, where it will
get called once the other kernel subsystems have delivered
the data from the block device.

Callback Programming Style Problems
-----------------------------------

While callbacks provide excellent flexibility of when
things get called, they do have a number of problems.

### C Stack Space

In some cases, a function that accepts a callback may
call the callback immediately.
For example, the function may determine an error
condition "early".

The problem is that by calling the callback immediately,
additional C stack space is allocated for the frame that
calls the callback.
C does not assure tail call optimization, and while some
C compilers will sometimes be able to tail-call-optimize
under some conditions, those conditions may not be
reliably written down, changes in optimization level or
optimizer code may change the conditions in which
tail-call-optimization can be done, and so on.

This can be compounded if the callback then calls another
callback-accepting function, which itself exits early
and calls the callback immediately, which then calls
another callback-accepting function, until we have
several pending C frames on the stack.

This is bad since in some contexts in which LLRFS has to
run, the C stack may be very tiny.
The Linux kernel, for example, uses a 4096-byte stack.

### Reading Backwards

Suppose we have ordinary, non-callback-using code:

```c
void do_two_things() {
	do_thing_1();
	do_thing_2();
}
```

Now suppose that we want to transform the above code so
that `do_thing_1` and `do_thing_2` are callback-accepting
functions, which of course forces `do_two_things` to also
be a callback-accepting function.
We would then have to write the code "backwards", like so:

```c
struct do_two_things_context {
	void (*callback)(void*);
	void *callback_arg;
};

static void
do_two_things_after_do_thing_1(struct do_two_things_context* ctx) {
	void (*callback)(void*) = ctx->callback;
	void *callback_arg = ctx->callback_arg;
	free(ctx);

	do_thing_2(callback, callback_arg);
}

void do_two_things(void (*callback)(void*),
		   void *callback_arg) {
	struct do_two_things_context *ctx;
	ctx = malloc(sizeof(struct do_two_things_context));

	ctx->callback = callback;
	ctx->callback_arg = callback_arg;

	do_thing_1(&do_two_things_after_do_thing_1,
		   ctx);
}
```

Aside from the large amount of boilerplate just to execute
two things in sequence, one thing we observe is that in the
code, the call to `do_thing_2` comes first, before the call
to `do_thing_1`, even though in non-callback-using-style we
would have written `do_thing_1(); do_thing_2();` in proper
order.
Thus, callback style forces coders to read backwards.

This could be fixed by forward-declaring
`do_two_things_after_do_thing_1` so that we can define it after
`do_two_things`, so that reading the code is in the correct
order, but that adds even more boilerplate to the code, further
obscuring it.

`llr_act`
---------

`libllrfs` defines a type, `llr_act`, which is used in all
APIs, both between layers of LLRFS, and between the library
and its client.

Roughly, `do_thing_1` and `do_thing_2` would, instead of being
`void` functions, return an `llr_act` instead.

Then, to write the previous example into the Actions
framework:

```c
void do_two_things() {
	do_thing_1();
	do_thing_2();
}
```

We would make `do_thing_1`, `do_thing_2`, and `do_two_things`
all return an `llr_act` instead of `void`, and `do_two_things`
would be very simply:

```c
llr_act do_two_things() {
	llr_act act = llr_act_nothing();
	llr_act_seq(&act, do_thing_1());
	llr_act_seq(&act, do_thing_2());
	return act;
}
```

This is a very tiny amount of boilerplate compared to using
callback style explicitly, and the code is no longer written
backwards.

Now, `do_thing_1` and `do_thing_2` are now wrappers around
functions that actually accept callbacks, like so:

```c
static void do_thing_1_core(void *_unused_,
			    void (*callback)(void*),
			    void *callback_arg) {
	if (early_out()) {
		callback(callback_arg);
		return;
	}
	register_callback(callback, callback_arg);
}
llr_act do_thing_1() {
	return LLR_ACT_NEW(&do_thing_1_core, (void*) NULL);
}
```

Thus, code that needs to use callbacks can be wrapped into
the `llr_act`, while code that needs a convenient way to
sequence actions can operate on `llr_act` objects with little
boilerplate and with code written approximately in the
"correct" order.

An `llr_act`, howevrer, represents an action that is not
yet done.
At *some* point, we have already constructed the action in
full, and need to actually cause the code inside the action
to be called with some callback.
This is done by calling `llr_act_perform`:

```c
void llr_act_perform(llr_act action,
		     void (*completionn_callback)(void* arg),
		     void (*enomem_callback)(void* arg),
		     void* arg);
```

Clients of `libllrfs` are expected to call this function with
any `llr_act` returned by a `libllrfs` API.
Normal completion of the action causes `completion_callback`
to be invoked, while out-of-memory conditions will cause
`enomem_callback` to be invoked.

`llr_act_perform` handles the sequential execution of
actions.
In particular, it provides the callback to each of the
basic component actions, and coordinates with the callback
so that if the action immediately calls the callback, the
callback will return immediately (to cut out the stack
space used) and inform `llr_act_perform` that the next
action can now be called.
This solves the limited stack space in environments like
the Linux kernel.

### Basic Actions

`llr_act_seq` composes actions and `llr_act_perform`
decomposes and executes them, but how do we have basic
actions to compose and execute in the first place?

We have already seen the `LLR_ACT_NEW` macro, which is
given a callback-accepting function and an argument to
that function, and returns it wrapped in a convenient
`llr_act` Action.

More generally, you probably want your basic actions to
be able to accept arguments.
Since actions cannot return a value, you would need to
allocate some space for them to return a value in, and
pass in a pointer to that space, and the basic action
would then put its return value there.

```c
struct basic_action_args {
	int *output;
};
static void
basic_action_core(struct basic_action_args *args,
		  void (*callback)(void*),
		  void *callback_arg) {
	int *output = args->output;
	free(args);

	*output = 42;

	callback(callback_arg);
}
llr_act basic_action(int *output) {
	struct basic_action_args *args;
	args = malloc(sizeof(struct basic_action_args));
	if (!args)
		return llr_act_enomem();
	args->output = output;

	return LLR_ACT_NEW(&basic_action_core, args);
}
```

Notice the `llr_act_enomem()`, which is used to signal
an out-of-memory condition.
Constructing actions via `LLR_ACT_NEW` may instead
return the special `llr_act_enomem()` value, if the
framework is unable to allocate memory.

In some cases, it can be convenient to package some
pure logic in an action, so that they too can be
composed with actions that need callbacks.
This can be done with `LLR_ACT_NEW_LOGIC`:

```c
struct basic_action_args {
	int *output;
};
static void
basic_action_core(struct basic_action_args *args) {
	int *output = args->output;
	free(args);

	*output = 42;
}
llr_act basic_action(int *output) {
	struct basic_action_args *args;
	args = malloc(sizeof(struct basic_action_args));
	if (!args)
		return llr_act_enomem();
	args->output = output;

	return LLR_ACT_NEW_LOGIC(&basic_action_core, args);
}
```

### Flow Control

While we can easily express sequencing with actions,
optional and repeated control flow cannot be expressed
with `llr_act_seq` alone.

We thus have a general operator for building an action
that can provide flow control called `llr_act_control`,
and build some additional operators on top.

```c
llr_act llr_act_control(llr_act (*func)(void *),
			void *func_arg);
```

The action returned by `llr_act_control` will invoke
the given `func`, and then perform whatever action
it returns.
The function will be invoked "late", i.e. if you have:

```c
	llr_act act = llr_act_nothing();
	llr_act_seq(act, do_one_thing(args));
	llr_act_seq(act, llr_act_control(&func, args));
	return act;
```

Then the action returned by `do_one_thing` must first
call its callback before the `func` is ever called.
This is useful if you need to check the results of
`do_one_thing` before deciding what to do.

The above is then wrapped by the `llr_act_if` operator,
which first invokes `pred` on the given argument, then
either flows into `then` if it returns non-0 or flows
into `else` if it returns 0.

```c
llr_act llr_act_if(bool (*pred)(void *),
		  void *pred_arg,
		  llr_act then,
		  llr_act else);
```

Another control flow is `llr_act_while`, which invokes
`pred` on the given argument, then if it returns true
invokes the `func` argument and arranges itself to be
invoked again.
If `pred` returns false then this action invokes
nothing.

```c
llr_act llr_act_while(bool (*pred)(void *),
		      void *pred_arg,
		      llr_act (*func)(void *),
		      void *func_arg);
```

### Cleanup

Sometimes, we need to create long-lived objects, which
need to be cleaned up.

In particular, a sequence of actions may have an
`llr_act_control()` node, and while the initial sequence of
actions is allocated fine with sufficient free memory, but
when control enters the `llr_act_control()` node, the control
function fails to allocate memory.
In that case, some other actions may want to free resources.

This is done by constructing a cleanup node using
`llr_act_finally`:

```c
llr_act llr_act_finally(void (*cleanup)(void* cleanup_arg),
			void *cleanup_arg);
```

When a control node fails with an `llr_act_enomem()`, the 
subsequent actions are deallocated, but if any of them are
cleanup nodes, the cleanup function is called.
Thus, whether or not some earlier node in the sequence fails
due to out-of-memory, the cleanup function is always called.

### Parallel Execution

Callbacks allow for great flexibility.
For example, in LLRFS we may need to send commands to
multiple devices at once.
With callbacks, we do not need to wait for one device to
respond to the command before sending to the next, we can
just call the callback-accepting functions.
We pass in a callback which keeps track of the number of
pending requests that have not yet been responded to,
and then continue processing once all the devices have
responded.

However, writing this in explicit callback-using style
is ***PAINFUL*** and involves a lot more boilerplate
than the previous sequential example.

Now, we have already seen the `llr_act_seq` function,
which combines two actions in sequence.
The Actions framework also provides `llr_act_par` and
`llr_act_fork` functions:

```c
/* Combines b into a, such that the existing action a is
 * done in parallel with b, and the total action completes
 * when both the existing action a and the action b
 * complete.
 */
void llr_act_par(llr_act *a, llr_act b);
/* Creates an action which causes a to be started, but
 * does not wait for it to complete.
 */
llr_act llr_act_fork(llr_act a);
```

`llr_act_par` is a fork-join, while `llr_act_fork` is
just a fork (without any ability to later join).

For example, in the Grass layer to implement an `fsync` we
need to lock the global root, generate the superblock,
unlock the global root, then flush all devices (in parallel,
for speed), then write the superblock to all devices (in
parallel, too, for speed).
We could sketch it out like this:

```c
llr_act grass_fsync(struct llr_grass *grass)
{
	int i;

	superblock *sb = malloc_superblock(grass);
	llr_act act = llr_act_nothing();

	/* Generate the superblock.  */
	llr_act_seq(&act, lock_global(grass));
	llr_act_seq(&act, generate_superblock(grass, sb));
	llr_act_seq(&act, unlock_global(grass));

	/* Flush all devices.  */
	llr_act flush_acts = llr_act_nothing();
	for (i = 0; i < grass->num_devices; ++i) {
		llr_act_par(&flush_acts, device_flush(grass->devices[i]));
	}
	llr_act_seq(&act, flush_acts);

	/* Write the superblock to all devices.  */
	llr_act write_acts = llr_act_nothing();
	for (i = 0; i < grass->num_devices; ++i) {
		/* Write the superblock and flush it too.  */
		llr_act one_act = llr_act_nothing();
		llr_act_seq(&one_act, write_superblock(grass, i, sb));
		llr_act_seq(&one_act, device_flush(grass->devices[i]));

		/* Add the above sequence to the write-acts.  */
		llr_act_par(&write_acts, one_act);
	}
	/* Add the write-acts to the main sequence.  */
	llr_act_seq(&act, write_acts);

	/* Free up the superblock at the end.  */
	llr_act_seq(&act, free_superblock_act(grass, sb));

	return act;
}
```

Implementation
--------------

An `llr_act` is really a structure with two pointers:

```
struct llr_act_node_s;
typedef struct llr_act_node_s llr_act_node;
typedef struct {
	llr_act_node *first;
	llr_act_node *last;
} llr_act;
```

Conceptually, the `llr_act_node`s are singly-linked lists,
denoting the order in which actions must be completed.

However, we also need to implement parallel actions.
Nodes keep track of a number of incoming edges, which
indicates the number of previous actions that need
to finish before we process this node.

In addition, we have different types of `llr_act_node`s:

* Fork nodes.
* Execution nodes.
* Control nodes (`llr_act_control`).
* Cleanup nodes (`llr_act_finally`).
* Join nodes.

Most node types have a single `next` pointer, but fork
nodes have a list of next nodes.

Execution nodes have the following states:

* Unreturned (initial state).
* Returned-in-loop.
* Loop-exited.

When an execution node has its function executed, its
state is unreturned.
If the function calls its callback immediately, it sees the
state is "unreturned" and converts it to returned-in-loop, and
returns immediately.
The outer loop will then free the node, and get its next node,
then process the next node.

If the function does not immediately call its callback,
when it returns to the outer loop, the outer loop converts
it to loop-exited, and the outer loop forgets about this
node.
If later the callback is invoked, it sees the state as
loop-exited, and it frees the node and gets its next node.
If the next node is NULL the callback just returns, as
that means there is nothing more to do.
Otherwise, it arragnes to process the next node.

The outer loop actually maintains a list of nodes it has
to process.
It is initialized with a node, which it pushes into its
list of nodes.

At each iteration of the loop, it checks if its list of
nodes is empty, and exits the loop if so.

Otherwise, it pops off one node.
It asserts that its incoming-edges counter is zero.
Then it dispatches according to type.

* If the node is a fork node, it grabs the list of
  next nodes, then iterates over them, atomic-decrementing
  their incoming-edges counters.
  If the incoming-edges counter drops to zero, it pushes
  the node into its list of nodes it is working on.
  Then it frees the fork node.
* If the node is an execution node, it checks if the node
  has a function, and calls it, giving the callback function
  and passing the node itself as the argument to the
  callback.
  If on return from that function the node has changed to
  returned-in-loop, or there was no function, it takes the
  next node and frees the
  current node.
  If the next node is null, it just continues the loop.
  Otherwise, it atomic-decrements the next node's
  incoming-edges counter and if that drops to zero, pushes
  the next node into its working list.
  If on return from the function the node is still in
  unreturned state, it switches the node to loop-exited
  and continues the loop.

When constructing a basic action, we just allocate a
new node, set it to an execution node, set its incoming-edges
counter to zero, and fills in its function and function
argument.
Then it returns an `llr_act` with both `first` and `next`
pointing to that node.

`llr_act_seq` just concatenates the inputs together:

```c
void llr_act_seq(llr_act *a, llr_act b) {
	/* Handle llr_act_nothing().  */
	if (!a->first) {
		*a = b;
		return;
	}
	if (!b.first) {
		return;
	}

	assert(a->last->type == LLR_ACT_NODE_TYPE_EXECUTION);
	atomic_increment(b.first->incoming_edges);
	a->last->u.exec.next = b.first;
	a->last = b->first;
}
```

`llr_act_par` first checks if the first argument is already
started by a fork node and ended by a join node, and if so,
adds the second argument to the fork and connects its last to
the last of the first argument, as a join node.
Otherwise, it creates a fork node and a join node and inserts
the two inputs to the fork node and the join node, then sets
the first input's `first` to the fork node and the second
input's `last` to the join node.

`llr_act_fork` constructs a fork node and a dummy join node.
The fork node indicates two next nodes, the dummy join
node and the first node of the input.
It then returns an action where the first node is the fork
node and the last node is the join node.
