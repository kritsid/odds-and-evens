/*!
 *  Implementation is based on:
 *	Scope Stack Allocation by Andreas Fredriksson, DICE
 *  Conference: DICE Coders Day 2010 November
 */
#pragma once

#include <new>
#include <cassert>


typedef unsigned char uint8;


template<size_t alignment> size_t aligned_size(size_t size) {
	return (size + (alignment - 1)) & ~(alignment - 1);
}

template<typename T> void destructorCall(void* ptr) {
	static_cast<T*>(ptr)->~T();
}


class LinearAllocator {
public:

	static const unsigned char alignment = 16;

	LinearAllocator(void* ptr, size_t size) 
		: current_((uint8*)ptr)
		, end_(current_ + size) { 
		memset(current_, 0, size); 
	}

	uint8* allocate(size_t size) {
		size_t asize = aligned_size<alignment>(size);
		assert(current_ + asize <= end_);
		uint8* result = current_; 
		current_ += asize;
		return result;
	}

	void rewind(void* ptr) { 
		ptrdiff_t diff = (uintptr_t)current_ - (uintptr_t)ptr;
		assert(diff >= 0);
		current_ = (uint8*)ptr;
		memset(current_, 0, diff);
	}

	void* current() { 
		return current_; 
	}

private:
	uint8* current_;
	uint8* end_;
};



struct Finalizer {
	typedef void (*Function)(void*);
	Function fn;
	Finalizer* chain;
};


class ScopeStack {
public:

	explicit ScopeStack(LinearAllocator& alloc)
		: alloc(alloc)
		, rewindPoint(alloc.current())
		, finalizerChain(0) {}

	~ScopeStack() {
		for (Finalizer* f = finalizerChain; f; f = f->chain) {
			(*f->fn)(objectAddress(f));
		}
		alloc.rewind(rewindPoint);
	}

	template<typename T> T* newObject(int n) {
		Finalizer* f = allocFinalizerAndObject(sizeof(T));
		T* result = new (objectAddress(f)) T(n);
		f->fn = &destructorCall<T>;
		f->chain = finalizerChain;
		finalizerChain = f;
		return result;
	}

	template<typename T> T* newPOD() {
		return new (alloc.allocate(sizeof(T))) T;
	}

	void* allocate(size_t size) {
		return alloc.allocate(size);
	}

private:
	LinearAllocator& alloc;
	void* rewindPoint;
	Finalizer* finalizerChain;

	Finalizer* allocFinalizerAndObject(size_t size) {
		Finalizer* f = (Finalizer*)alloc.allocate(sizeof(Finalizer));
		alloc.allocate(size);
		return f;
	}

	void* objectAddress(Finalizer* f) {
		return ((uint8*)f + aligned_size<LinearAllocator::alignment>(sizeof(Finalizer)));
	}
};
