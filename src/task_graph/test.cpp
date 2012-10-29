#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <typeinfo>

#include "tbb/task.h"
#include "tbb/task_scheduler_init.h"

#include "task_graph01.h"
#include "task_graph02.h"
#include "task_graph03.h"
#include "task_graph04.h"
#include "task_graph05.h"


template<class RootTask> void test()
{
	printf("\nTEST %s:\n", typeid(RootTask).name());
	tbb::task_scheduler_init scheduler;
	RootTask& root = *new (tbb::task::allocate_root()) RootTask();
	tbb::task::spawn_root_and_wait(root);
	printf("Finished! \n");
}


int main()
{
	test<task_graph01::Root>();
	test<task_graph02::Root>();
	test<task_graph03::Root>();
	test<task_graph04::Root>();
	test<task_graph05::Root>();

	return 0;
}