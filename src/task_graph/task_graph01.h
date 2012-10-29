#pragma once

namespace task_graph01
{
	// graph nodes declaration:

	struct A : public tbb::task { tbb::task* execute(); };
	struct B : public tbb::task { tbb::task* execute(); };
	struct C : public tbb::task { tbb::task* execute(); };

	struct Cont : public tbb::task { tbb::task* execute(); };
	struct Root : public tbb::task { tbb::task* execute(); };


	// graph nodes implementation:

	tbb::task* Root::execute()
	{
		printf("Perform job Root \n");
		Cont& cont = *new (allocate_continuation()) Cont();
		A& a = *new (cont.allocate_child()) A();
		B& b = *new (cont.allocate_child()) B();
		C& c = *new (cont.allocate_child()) C();
		cont.set_ref_count(3); // #{ a, b, c } = 3
		cont.spawn(c);
		cont.spawn(b);
		return &a;
	}

	tbb::task* A::execute()
	{
		printf("Perform job A \n");
		return NULL;
	}

	tbb::task* B::execute()
	{
		printf("Perform job B \n");
		return NULL;
	}

	tbb::task* C::execute()
	{
		printf("Perform job C \n");
		return NULL;
	}

	tbb::task* Cont::execute()
	{
		printf("Perform job Cont \n");
		return NULL;
	}

} // end namespace task_graph01