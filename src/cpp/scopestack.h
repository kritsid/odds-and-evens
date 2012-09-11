/*!
 *  Implementation is based on:
 *	Scope Stack Allocation by Andreas Fredriksson, DICE
 *  Conference: DICE Coders Day 2010 November
 */
#pragma once

#include <cstring>
#include <cstdint>
#include <new>
#include <cassert>


typedef unsigned char uint8;


template<size_t alignment> size_t aligned_size(size_t size) {
	return (size + (alignment - 1)) & ~(alignment - 1);
}


class LinearAllocator {
public:

	static const uint8 alignment = 16;

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

	LinearAllocator(const LinearAllocator&);
	LinearAllocator& operator=(const LinearAllocator&);
};



template<typename T> void destructorCall(void* ptr) {
	static_cast<T*>(ptr)->~T();
}


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

	template<typename T> T* newObject() {
		Finalizer* f = allocFinalizerAndObject(sizeof(T));
		T* result = new (objectAddress(f)) T();
		f->fn = &destructorCall<T>;
		f->chain = finalizerChain;
		finalizerChain = f;
		return result;
	}

	template<typename T, typename P> T* newObject(P p) {
		Finalizer* f = allocFinalizerAndObject(sizeof(T));
		T* result = new (objectAddress(f)) T(p);
		f->fn = &destructorCall<T>;
		f->chain = finalizerChain;
		finalizerChain = f;
		return result;
	}

	template<typename T> T* newObject(ScopeStack& scope) {
		Finalizer* f = allocFinalizerAndObject(sizeof(T));
		T* result = new (objectAddress(f)) T(scope);
		f->fn = &destructorCall<T>;
		f->chain = finalizerChain;
		finalizerChain = f;
		return result;
	}

	template<typename T, typename P> T* newObject(ScopeStack& scope, P p) {
		Finalizer* f = allocFinalizerAndObject(sizeof(T));
		T* result = new (objectAddress(f)) T(scope, p);
		f->fn = &destructorCall<T>;
		f->chain = finalizerChain;
		finalizerChain = f;
		return result;
	}

	template<typename T> T* newPOD() {
		return new (alloc.allocate(sizeof(T))) T;
	}

	template<typename T, typename P> T* newPOD(P p) {
		return new (alloc.allocate(sizeof(T))) T(p);
	}

	template<typename T> T* newPOD(ScopeStack& scope) {
		return new (alloc.allocate(sizeof(T))) T(scope);
	}

	template<typename T, typename P> T* newPOD(ScopeStack& scope, P p) {
		return new (alloc.allocate(sizeof(T))) T(scope, p);
	}

	void* allocate(size_t size) {
		return alloc.allocate(size);
	}

private:

	struct Finalizer {
		typedef void (*Function)(void*);
		Function fn;
		Finalizer* chain;
	};

	LinearAllocator& alloc;
	void* rewindPoint;
	Finalizer* finalizerChain;

	Finalizer* allocFinalizerAndObject(size_t size) {
		Finalizer* f = (Finalizer*)alloc.allocate(sizeof(Finalizer));
		alloc.allocate(size);
		return f;
	}

	void* objectAddress(const Finalizer* f) const {
		return ((uint8*)f + aligned_size<LinearAllocator::alignment>(sizeof(Finalizer)));
	}

	ScopeStack(const ScopeStack&);
	ScopeStack& operator=(const ScopeStack&);
};
