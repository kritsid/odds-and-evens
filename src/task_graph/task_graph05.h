#pragma once

namespace task_graph05
{

	// jobs that have to be done:

	void performJobA(int a);
	void performJobB(int a);
	void performJobC(int a);

	void performJobE(int a);
	void performJobF(int a);

	void performJobX(int a);
	void performJobY(int a);
	void performJobZ(int a);

	void performJobRoot(int a);
	void performJobCont(int a);
	void performJobNext(int a);


	// graph nodes declaration:

	struct F : public tbb::task
	{
		const int id;
		F(int i) : id(i) {}
		tbb::task* execute();
	};

	struct E : public tbb::task
	{
		const int id;
		E(int i) : id(i) {}
		tbb::task* execute();
	};

	struct X : public tbb::task
	{
		const int id;
		X(int i) : id(i) {}
		tbb::task* execute();
	};

	struct Y : public tbb::task
	{
		const int id;
		Y(int i) : id(i) {}
		tbb::task* execute();
	};

	struct Z : public tbb::task
	{
		const int id;
		Z(int i) : id(i) {}
		tbb::task* execute();
	};

	struct A : public tbb::task
	{
		const int id;
		A(int i) : id(i) {}
		tbb::task* execute();
	};

	struct B : public tbb::task
	{
		const int id;
		B(int i) : id(i) {}
		tbb::task* execute();
	};

	struct C : public tbb::task
	{
		const int id;
		C(int i) : id(i) {}
		tbb::task* execute();
	};


	struct Cont : public tbb::task { tbb::task* execute(); };
	struct Next : public tbb::task { tbb::task* execute(); };
	struct Root : public tbb::task { tbb::task* execute(); };


	// graph nodes implementation:

	tbb::task* Root::execute()
	{
		performJobRoot(0);
		Cont& cont = *new (allocate_continuation()) Cont();
		E& e = *new (cont.allocate_child()) E(1);
		F& f = *new (cont.allocate_child()) F(2);
		C& c = *new (cont.allocate_child()) C(3);
		cont.set_ref_count(3); // #{ e, f, c } = 3
		cont.spawn(c);
		cont.spawn(f);
		return &e;
	}

	tbb::task* X::execute()
	{
		performJobX(id);
		return NULL;
	}

	tbb::task* Y::execute()
	{
		performJobY(id);
		return NULL;
	}

	tbb::task* Z::execute()
	{
		performJobZ(id);
		return NULL;
	}

	tbb::task* E::execute()
	{
		performJobE(id);
		A& a = *new (allocate_continuation()) A(id);
		X& x = *new (a.allocate_child()) X(id);
		Y& y = *new (a.allocate_child()) Y(id);
		Z& z = *new (a.allocate_child()) Z(id);
		a.set_ref_count(3); // #{x, y, z} = 3
		a.spawn(z);
		a.spawn(y);
		return &x;
	}

	tbb::task* F::execute()
	{
		performJobF(id);
		B& b = *new (allocate_continuation()) B(id);
		X& x = *new (b.allocate_child()) X(id);
		Y& y = *new (b.allocate_child()) Y(id);
		b.set_ref_count(2); // #{x, y} = 2
		b.spawn(y);
		return &x;
	}

	tbb::task* A::execute()
	{
		performJobA(id);
		return NULL;
	}

	tbb::task* B::execute()
	{
		performJobB(id);
		return NULL;
	}

	tbb::task* C::execute()
	{
		performJobC(id);
		return NULL;
	}

	tbb::task* Cont::execute()
	{
		performJobCont(0);
		return new (allocate_continuation()) Next();
	}

	tbb::task* Next::execute()
	{
		performJobNext(0);
		return NULL;
	}


	// jobs that have to be done dummy implementation

	void performJobA(int a)    {printf("Perform job A %d\n", a);}
	void performJobB(int a)    {printf("Perform job B %d\n", a);}
	void performJobC(int a)    {printf("Perform job C %d\n", a);}

	void performJobE(int a)    {printf("Perform job E %d\n", a);}
	void performJobF(int a)    {printf("Perform job F %d\n", a);}

	void performJobX(int a)    {printf("Perform job X %d\n", a);}
	void performJobY(int a)    {printf("Perform job Y %d\n", a);}
	void performJobZ(int a)    {printf("Perform job Z %d\n", a);}

	void performJobRoot(int a) {printf("Perform job Root %d\n", a);}
	void performJobCont(int a) {printf("Perform job Cont %d\n", a);}
	void performJobNext(int a) {printf("Perform job Next %d\n", a);}

} // end namespace task_graph05