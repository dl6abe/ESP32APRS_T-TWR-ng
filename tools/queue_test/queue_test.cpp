// Host-native tests for the vendored cppQueue library
// (lib/Queue/src/cppQueue.cpp), built directly against the real vendored
// file (not a copy) via arduino_compat/Arduino.h - no device needed. Written
// as a regression net for the 1.9 -> 2.0 bump (Gitea issue - Queue library
// breaking-change research): exercises exactly the operations this project
// actually uses (push/pop/getCount/clean/isFull, FIFO construction) plus
// LIFO/peek/drop/peekIdx/overwrite for full API coverage, so a future bump
// can be re-verified by just re-running this instead of re-reading diffs.
//
// No test framework: each case calls check() independently and keeps going
// on failure, matching the convention in tools/aprs_test/ and
// tools/wifi_validate_test/. Run: tools/queue_test/run_queue_test.sh

#include <cstdio>
#include <cstring>
#include <cppQueue.h>

static int failures = 0;

static void check(bool condition, const char *description)
{
	if (!condition)
	{
		failures++;
		std::printf("FAIL: %s\n", description);
	}
}

struct Item
{
	int id;
	char tag;
};

static void test_fifo_order()
{
	cppQueue q(sizeof(Item), 3, FIFO);

	Item a{1, 'A'}, b{2, 'B'}, c{3, 'C'};
	check(q.push(&a), "FIFO: push 1st item succeeds");
	check(q.push(&b), "FIFO: push 2nd item succeeds");
	check(q.push(&c), "FIFO: push 3rd item succeeds");
	check(q.isFull(), "FIFO: queue reports full after 3 pushes into a 3-slot queue");
	check(q.getCount() == 3, "FIFO: getCount() == 3 after 3 pushes");

	Item out{};
	check(q.pop(&out) && out.id == 1 && out.tag == 'A', "FIFO: first pop returns oldest item (1,A)");
	check(q.pop(&out) && out.id == 2 && out.tag == 'B', "FIFO: second pop returns (2,B)");
	check(q.pop(&out) && out.id == 3 && out.tag == 'C', "FIFO: third pop returns (3,C)");
	check(q.isEmpty(), "FIFO: queue reports empty after popping everything");
	check(!q.pop(&out), "FIFO: pop on empty queue fails");
}

static void test_lifo_order()
{
	cppQueue q(sizeof(Item), 3, LIFO);

	Item a{1, 'A'}, b{2, 'B'}, c{3, 'C'};
	q.push(&a);
	q.push(&b);
	q.push(&c);

	Item out{};
	check(q.pop(&out) && out.id == 3, "LIFO: first pop returns most recently pushed item (3)");
	check(q.pop(&out) && out.id == 2, "LIFO: second pop returns (2)");
	check(q.pop(&out) && out.id == 1, "LIFO: third pop returns (1)");
}

static void test_push_refused_when_full_without_overwrite()
{
	cppQueue q(sizeof(int), 2, FIFO, false); // overwrite disabled (default)

	int v1 = 1, v2 = 2, v3 = 3;
	check(q.push(&v1), "No-overwrite: push into slot 1 succeeds");
	check(q.push(&v2), "No-overwrite: push into slot 2 succeeds");
	check(!q.push(&v3), "No-overwrite: push into full queue is refused");
	check(q.getCount() == 2, "No-overwrite: refused push does not change count");

	int out = 0;
	check(q.pop(&out) && out == 1, "No-overwrite: refused push did not corrupt existing data (oldest still 1)");
}

static void test_overwrite_when_full()
{
	cppQueue q(sizeof(int), 2, FIFO, true); // overwrite enabled

	int v1 = 1, v2 = 2, v3 = 3;
	q.push(&v1);
	q.push(&v2);
	check(q.push(&v3), "Overwrite: push into full queue succeeds (overwrites oldest)");
	check(q.getCount() == 2, "Overwrite: count stays at capacity after an overwriting push");

	int out = 0;
	check(q.pop(&out) && out == 2, "Overwrite: oldest (1) was evicted, next pop returns 2");
	check(q.pop(&out) && out == 3, "Overwrite: last pop returns the newest value (3)");
}

static void test_peek_does_not_remove()
{
	cppQueue q(sizeof(int), 3, FIFO);
	int v1 = 42;
	q.push(&v1);

	int peeked = 0;
	check(q.peek(&peeked) && peeked == 42, "Peek: returns the front item");
	check(q.getCount() == 1, "Peek: does not remove the item (count unchanged)");

	int popped = 0;
	check(q.pop(&popped) && popped == 42, "Peek: the peeked item is still poppable afterwards");
}

static void test_drop()
{
	cppQueue q(sizeof(int), 3, FIFO);
	int v1 = 1, v2 = 2;
	q.push(&v1);
	q.push(&v2);

	check(q.drop(), "Drop: succeeds while queue has items");
	check(q.getCount() == 1, "Drop: removes exactly one item");

	int out = 0;
	check(q.pop(&out) && out == 2, "Drop: dropped the oldest item, second item remains");
}

static void test_peek_idx_bounds()
{
	cppQueue q(sizeof(int), 3, FIFO);
	int v1 = 10, v2 = 20;
	q.push(&v1);
	q.push(&v2);

	int out = 0;
	check(q.peekIdx(&out, 0) && out == 10, "peekIdx: index 0 returns oldest item");
	check(q.peekIdx(&out, 1) && out == 20, "peekIdx: index 1 returns second item");
	check(!q.peekIdx(&out, 2), "peekIdx: out-of-range index is rejected");
}

static void test_clean_resets_queue()
{
	cppQueue q(sizeof(int), 2, FIFO);
	int v1 = 1, v2 = 2;
	q.push(&v1);
	q.push(&v2);
	check(q.isFull(), "Clean: queue is full before clean()");

	q.clean();
	check(q.isEmpty(), "Clean: queue reports empty right after clean()");
	check(q.getCount() == 0, "Clean: getCount() == 0 after clean()");

	int v3 = 3;
	check(q.push(&v3), "Clean: queue is usable again after clean()");
}

static void test_wraparound_after_several_cycles()
{
	// Regression check for the internal index-increment logic (_inc_idx),
	// which the 1.9 -> 2.0 bump renamed/moved but claimed not to change
	// behaviorally - push/pop past the physical buffer end several times to
	// make sure wraparound still produces the right FIFO order.
	cppQueue q(sizeof(int), 3, FIFO);

	for (int cycle = 0; cycle < 5; cycle++)
	{
		int a = cycle * 10 + 1, b = cycle * 10 + 2;
		check(q.push(&a), "Wraparound: push a succeeds");
		check(q.push(&b), "Wraparound: push b succeeds");

		int out = 0;
		check(q.pop(&out) && out == a, "Wraparound: pop returns a in FIFO order");
		check(q.pop(&out) && out == b, "Wraparound: pop returns b in FIFO order");
		check(q.isEmpty(), "Wraparound: queue empty at end of each cycle");
	}
}

int main()
{
	test_fifo_order();
	test_lifo_order();
	test_push_refused_when_full_without_overwrite();
	test_overwrite_when_full();
	test_peek_does_not_remove();
	test_drop();
	test_peek_idx_bounds();
	test_clean_resets_queue();
	test_wraparound_after_several_cycles();

	if (failures == 0)
		std::printf("All tests passed.\n");
	else
		std::printf("%d failure(s)\n", failures);

	return failures == 0 ? 0 : 1;
}
