/*
 * Driver code for airballoon problem
 */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <test.h>

#define NROPES 16

/* decleration of addition functions*/
void setup(void);
void wait_and_clean(void);

/*	
*	When 0 dandelion, marigold, and Lord Flower Killer
*	threads should finish
*/
static int ropes_left = NROPES;

/*codition used for the cv in balloon and main thread */
static int threads_done = 0;

/*
*  Struct representing a rope. Each rope
*  has and associated lock, stake, and is_tied
*  value that is true if the rope is still attached
*  to the balloon 
*/
struct rope {
			volatile bool is_tied;
			volatile int  stake; 
			struct lock *rope_lk;
};

/* 
*  Array of rope objects. Indecies represent the 
*  rope hook in this context 
*/
struct rope ropes[NROPES];

/* Synchronization primitives */

/* lock to be held when accessing ropes_left */
struct lock *ropes_left_lk;
/* lock to be held when canipulating threads_done */
struct lock *threads_done_lk;
/* lock to be held by a thread when printing */
struct lock *print_lk;

/* cv's for balloon and main thread (both dependent on threads_done) */
struct cv *balloon_cv;
struct cv *airballoon_cv;

/*
*	DANDELION: picks ropes via a hook, so we simply select a random
*	number and attempt to cut the rope at that index in the ropes array.
*
*	EXIT CONDITION: ropes_left == 0
*/

static
void
dandelion(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	int rope_hook;
	bool done = false;

	kprintf("Dandelion thread starting\n");
	
	while(!done)
	{
		/* choose random hook */
		rope_hook = random() % NROPES;
		
		lock_acquire(ropes[rope_hook].rope_lk);
		if (ropes[rope_hook].is_tied == true)
		{
			lock_acquire(print_lk);
			kprintf("Dandelion severed rope %d \n", rope_hook);
			lock_release(print_lk);

			ropes[rope_hook].is_tied = false;

			lock_acquire(ropes_left_lk);
			ropes_left--;
			if(ropes_left == 0)
				done = true;
			lock_release(ropes_left_lk);
		}
		else
		{
			lock_acquire(ropes_left_lk);
			if(ropes_left == 0)
				done = true;
			lock_release(ropes_left_lk);
		}

		lock_release(ropes[rope_hook].rope_lk);

		/*we cut a rope or couldn't find our rope so we'll switch thread*/
		thread_yield();
	}

	/*we are done, need to inc threads done and wake up balloon if others done */
	lock_acquire(threads_done_lk);
	threads_done++;
	if (threads_done == 3)
		cv_signal(balloon_cv, threads_done_lk);
	lock_release(threads_done_lk);

	lock_acquire(print_lk);
	kprintf("Dandelion thread done\n");
	lock_release(print_lk);

	thread_exit();
}

/*
*	MARIGOLD: picks ropes via a stake. Picks a random stake
*	and iterates through every rope in ropes array to see if
*	there is a rope with the associate stake, and then attempts
*	to cut it.
*
*	EXIT CONDITION: ropes_left == 0
*/

static
void
marigold(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	int i = 0;
	int stake_num;
	bool done = false;

	kprintf("Marigold thread starting\n");
	while(!done)
	{
		/* choose random stake */
		stake_num = random() % NROPES;

		/* try to find rope tied to random stake */
		for(i = 0; i < NROPES; i++)
		{
			lock_acquire(ropes[i].rope_lk);

			if(ropes[i].stake == stake_num)
			{
				if (ropes[i].is_tied == true)
				{
					lock_acquire(print_lk);
					kprintf("Marigold severed rope %d on stake %d \n", i, ropes[i].stake);
					lock_release(print_lk);

					ropes[i].is_tied = false;

					lock_acquire(ropes_left_lk);
					ropes_left--;
					if(ropes_left == 0)
						done = true;
					lock_release(ropes_left_lk);
				}
				else
				{
					lock_acquire(ropes_left_lk);
					if(ropes_left == 0)
						done = true;
					lock_release(ropes_left_lk);
				}
			}
			lock_release(ropes[i].rope_lk);
		}
		/* we cut a rope or couldn't find our rope so we'll switch threads */
		thread_yield();
	}

	/* we are done, need to inc threads done and wake up balloon if others done */
	lock_acquire(threads_done_lk);
	threads_done++;
	if (threads_done == 3)
		cv_signal(balloon_cv, threads_done_lk);
	lock_release(threads_done_lk);

	lock_acquire(print_lk);
	kprintf("Marigold thread done\n");
	lock_release(print_lk);

	thread_exit();
}

/*
*	FLOWERKILLER: flowerkill finds a rope via a stake. Random number
*	is selected for from stake and to_stake. Flowerkiller checks each
*	rope to see if it has from_state associated with it. If it does
*	Flowerkiller attempts to switch the rope's stake from froms_stake
*	to to_stake.
*
*	EXIT CONDITION: ropes_left == 0
*/

static
void
flowerkiller(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	bool done = false;
	int from_stake;
	int to_stake;
	int rope_number;

	lock_acquire(print_lk);
	kprintf("Lord FlowerKiller thread starting\n");
	lock_release(print_lk);

	while(!done)
	{
		/* choose randome to and from stakes */
		from_stake = random() % NROPES;
		to_stake = random() % NROPES;

		/* try to find a rope attached to from stake */
		for(rope_number = 0 ; rope_number < NROPES; rope_number++)
		{
			lock_acquire(ropes[rope_number].rope_lk);

			if(ropes[rope_number].stake == from_stake)
			{
				/* if there is a rope tied to stake, switch the stake */
				if (ropes[rope_number].is_tied == true)
				{
					lock_acquire(print_lk);
					kprintf("Lord FlowerKiller switched rope %d from stake %d to stake %d \n", rope_number, from_stake, to_stake);
					lock_release(print_lk);

					ropes[rope_number].stake = to_stake;
				}
			}

			lock_release(ropes[rope_number].rope_lk);
		}
		/* check if all the threads have been cut before trying to switch another */
		lock_acquire(ropes_left_lk);
		if(ropes_left == 0)
			done = true;
		lock_release(ropes_left_lk);

		/* we switched a rope, or no ropes on from_stake so we'll switch threads */
		thread_yield();
	}

	/* we are done, need to inc threads done and wake up balloon if others done */
	lock_acquire(threads_done_lk);
	threads_done++;
	if (threads_done == 3)
		cv_signal(balloon_cv, threads_done_lk);
	lock_release(threads_done_lk);


	lock_acquire(print_lk);
	kprintf("Lord FlowerKiller thread done\n");
	lock_release(print_lk);

	thread_exit();
}

/*
*	BALLOON: waits for lordflowerkiller, dandelion, and marigold to finish.
*	once they are all finished the  ropes have been cut, and we can signal for
*	the main thread to finish
*
*	EXIT CONDITION: threads_done == 3 (implicitaley ropes_left == 0)
*/ 

static
void
balloon(void *p, unsigned long arg)
{
	(void)p;
	(void)arg;
	
	lock_acquire(print_lk);
	kprintf("Balloon thread starting\n");
	lock_release(print_lk);

	/* wait for marigold, dandelion, and flowerkiller to finish */
	lock_acquire(threads_done_lk);
	while(threads_done != 3)
		cv_wait(balloon_cv, threads_done_lk);
	threads_done++;
	/* since we are done, wake up main as well */
	cv_signal(airballoon_cv, threads_done_lk);
	lock_release(threads_done_lk);

	lock_acquire(print_lk);
	kprintf("Balloon freed and Prince Dandelion escapes!\n");
	kprintf("Balloon thread done\n");
	lock_release(print_lk);	

	thread_exit();
}

int
airballoon(int nargs, char **args)
{	/* we are done, need to inc threads done and wake up balloon if others done */

	int err = 0;

	(void)nargs;
	(void)args;
	(void)ropes_left;

	setup();

	err = thread_fork("Marigold Thread",
			  NULL, marigold, NULL, 0);
	if(err)
		goto panic;
	
	err = thread_fork("Dandelion Thread",
			  NULL, dandelion, NULL, 0);
	if(err)
		goto panic;
	
	err = thread_fork("Lord FlowerKiller Thread",
			  NULL, flowerkiller, NULL, 0);
	if(err)
		goto panic;

	err = thread_fork("Air Balloon",
			  NULL, balloon, NULL, 0);
	if(err)
		goto panic;

	goto done;
panic:
	panic("airballoon: thread_fork failed: %s)\n",
	      strerror(err));
	
done:
	wait_and_clean();
	return 0;
}

/* initialize the data stuctures and Synch primatives we will be using */
void 
setup()
{
	int i;

	threads_done = 0;
	ropes_left = NROPES;

	ropes_left_lk = lock_create("numrope");
	print_lk = lock_create("print");
	threads_done_lk = lock_create("threads_done");

	balloon_cv = cv_create("balloon");
	airballoon_cv = cv_create("airballoon");

	for(i = 0; i < NROPES; i++)
	{
		struct rope r = {.is_tied = true, .stake=i, .rope_lk = lock_create("rope")};
		ropes[i] = r;
	}
}

/* last thing our main calls. Waits for other threads and frees memory used */
void
wait_and_clean()
{
	int i;

	/* sleep until balloon, dandelion, marigold, and flowerkiller are done */
	lock_acquire(threads_done_lk);
	while(threads_done != 4)
		cv_wait(airballoon_cv, threads_done_lk);
	lock_release(threads_done_lk);

	lock_acquire(print_lk);
	kprintf("Main thread done\n");
	lock_release(print_lk);	

	/* clean up data structures */
	lock_destroy(ropes_left_lk);
	lock_destroy(threads_done_lk);
	lock_destroy(print_lk);

	cv_destroy(balloon_cv);
	cv_destroy(airballoon_cv);

	for(i = 0; i < NROPES; i++)
	{
		lock_destroy(ropes[i].rope_lk);
	}
}