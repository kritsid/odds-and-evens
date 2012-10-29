#pragma once

namespace task_graph04
{

	// graph nodes declaration:

	struct F : public tbb::task { tbb::task* execute(); };
	struct X : public tbb::task { tbb::task* execute(); };
	struct Y : public tbb::task { tbb::task* execute(); };

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
		F& f = *new (cont.allocate_child()) F();
		B& b = *new (cont.allocate_child()) B();
		C& c = *new (cont.allocate_child()) C();
		cont.set_ref_count(3); // #{ a, b, c } = 3
		cont.spawn(c);
		cont.spawn(b);
		return &f;
	}

	tbb::task* X::execute()
	{
		printf("Perform job X \n");
		return NULL;
	}

	tbb::task* Y::execute()
	{
		printf("Perform job Y \n");
		return NULL;
	}

	tbb::task* F::execute()
	{
		printf("Perform job F \n");
		A& a = *new (allocate_continuation()) A();
		X& x = *new (a.allocate_child()) X();
		Y& y = *new (a.allocate_child()) Y();
		a.set_ref_count(2);
		a.spawn(y);
		return &x;
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

} // end namespace task_graph04