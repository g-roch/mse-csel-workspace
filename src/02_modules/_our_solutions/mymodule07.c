/* skeleton.c */
#include <linux/module.h>	/* needed by all modules */
#include <linux/init.h>		/* needed for macros */
#include <linux/kernel.h>	/* needed for debugging */

#include <linux/kthread.h>	/* needed for kernel thread management */
#include <linux/wait.h>		/* needed for waitqueues handling */
#include <linux/delay.h>	/* needed for delay fonctions */


static struct task_struct* my_thread[2];

DECLARE_WAIT_QUEUE_HEAD (queue_1);
static atomic_t is_kicked;

static int skeleton_thread_1 (void* data)
{
	pr_info ("skeleton thread_1 is now active...\n");
	while(!kthread_should_stop()) {
		// La mise en sommeil est faite ici par wait_event_interruptible
		int status = wait_event_interruptible 
			(queue_1, (atomic_read(&is_kicked) != 0) 
					|| kthread_should_stop());
		if (status == -ERESTARTSYS) {
			pr_info ("skeleton thread_1 has been interrupted\n");
			break;
		}
		// À ce stade, soit is_kicked est différent de 0, soit kthread_should_stop() est vrai
		atomic_dec (&is_kicked);
		pr_info ("skeleton thread_1 has been kicked\n");
	}
	return 0;
}

static int skeleton_thread_2 (void* data)
{
	wait_queue_head_t queue;
	pr_info ("skeleton thread_2 is now active...\n");
	// Là on crée la waitqueue pour que l'autre thread puisse s'y mettre en sommeil
	init_waitqueue_head (&queue);
	
	// Tant que le thread n'est pas arrêté, on attend 5 secondes et on kick le thread_1
	while(!kthread_should_stop()) {
		ssleep(5);
/*
		int status = wait_event_interruptible_timeout 
			(queue, kthread_should_stop(), 5*HZ);
		if (status == -ERESTARTSYS) {
			pr_info ("skeleton thread_2 has been interrupted\n");
			break;
		}
*/
		pr_info ("skeleton thread_2 timout elapsed...\n");
		// Le kick consiste à 
		// - mettre is_kicked à 1
		atomic_set (&is_kicked, 1);
		// - et à réveiller le thread_1 qui est en sommeil sur la waitqueue queue_1
		wake_up_interruptible (&queue_1);
	}
	return 0;
}


static int __init skeleton_init(void)
{
	pr_info ("Linux module 07 skeleton loaded\n");

	// Initialisation de la variable atomique à 0 -> le thread 1 se mettra direct en sommeil
	atomic_set (&is_kicked, 0);

	// Lancement des threads
	my_thread[0] = kthread_run (skeleton_thread_1, 0, "s/thread/%d", 1);
	my_thread[1] = kthread_run (skeleton_thread_2, 0, "s/thread/2");

	return 0;
}

static void __exit skeleton_exit(void)
{
	// Ne pas oublier d'arrêter les threads avant de quitter le module
	kthread_stop (my_thread[1]);
	kthread_stop (my_thread[0]);

	pr_info ("Linux module skeleton unloaded\n");
}

module_init (skeleton_init);
module_exit (skeleton_exit);

MODULE_AUTHOR ("Daniel Gachet <daniel.gachet@hefr.ch>");
MODULE_DESCRIPTION ("Module skeleton");
MODULE_LICENSE ("GPL");

