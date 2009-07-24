<div class="moz-text-flowed" style="font-family: -moz-fixed">#include <algorithm>	// for generate()
#include <cstdlib>		// for rand()
#include <cassert>		// for assert()

template <class T, int N>
class IRange
{
public:
	// none virtual.
	T operator[](size_t index) const
	{
		return *(_ptr + index);
	}
	T& operator[](size_t index)
	{
		return *(_ptr + index);
	}

	IRange(T* ptr)
		: _ptr(ptr)
	{}

protected:
	T* _ptr;
};

template <class T, int BEGIN, int SIZE>
class Range
	: public IRange<T, SIZE>
{
public:
	typedef T value_type;

	Range(T *ptr)
		: IRange<T, SIZE>(ptr + BEGIN)
		, _base(ptr)
	{}

public:
	T* _base;
};

// avoid improper use.
template <class T, int BEGIN>
class Range<T, BEGIN, 0>
	: public IRange<T, 0>
{};

template <class T, int N, int STEP, template <class T, int N> class PRED, 
	template<class, int, int> class R, bool STOP = N - STEP >= 0>
struct Generator
{
	typedef R<T, N - STEP, STEP> value_type;
	typedef Generator<T, N - STEP, STEP, PRED, R> next;

	Generator(const value_type& lhs, const value_type& rhs)
		: _t(T())
	{
		// thread can be added here.
		_t = next(lhs._base, rhs._base);
		_t += PRED<T, STEP>()(lhs, rhs);
	}

	operator T() const
	{
		// thread can join here.
		return _t;
	}

private:
	T _t;
};

template <class T, int STEP, template <class, int> class PRED, 
	template<class T, int BEGIN, int STEP> class R>
struct Generator<T, 0, STEP, PRED, R, false>
{
	typedef R<T, 0, STEP> value_type;

	Generator(const value_type& lhs, const value_type& rhs)
		: _t(T())
	{}

	operator T() const
	{
		return _t;
	}

private:
	T _t;
};

// avoid improper use.
template <class T, int N, int STEP, template <class T, int N> class PRED, 
	template<class T, int BEGIN, int STEP> class R>
struct Generator<T, N, STEP, PRED, R, false>
{
	typedef R<T, 0, N> value_type;

	Generator(const value_type& lhs, const value_type& rhs)
		: _t(T())
	{
		_t = PRED<T, N>()(lhs, rhs);
	}

	operator T() const
	{
		return _t;
	}

private:
	T _t;
};


// NOTICE !!!
// now, the algorithm is indenpendent with the way how the vector is seprated.
// it just rely on the generic interface T,N. means a slice which contains T type vairable and the number is N.
template <class T, int N>
struct DotProduct
{
	T operator()(const IRange<T, N>& lhs, const IRange<T, N>& rhs) const
	{
		T tmp = T();
		for(int i = 0; i < N; ++i)
		{
			tmp += lhs[i] * rhs[i];
		}
		return tmp;
	}
};

// another algorithm.
template <class T, int N>
struct Sum
{
	T operator()(const IRange<T, N>& lhs, const IRange<T, N>& rhs) const
	{
		T tmp = T();
		for(int i = 0; i < N; ++i)
		{
			tmp += lhs[i] + rhs[i];
		}
		return tmp;
	}
};

int main()
{
	using namespace std;

	const int N = 123456;

	double *vector1 = new double[N];
	double *vector2 = new double[N];

	generate(vector1, vector1 + N, rand);
	generate(vector2, vector2 + N, rand);

	// I just make the interface simpler.
	// constructor of this class is used to create thread.
	// the operato T will be used to join the thread.
	// there are many implicit type conversion inside, but compare with virtual function call. it's faster.
	double res = Generator<double, N, 10000, DotProduct, Range>(vector1, vector2);

	double comp = 0.0;
	for(int i = 0; i < N; ++i)
		comp += vector1[i] * vector2[i];

	assert(comp == res);


	// algorithm is changed.
	res = Generator<double, N, 100, Sum, Range>(vector1, vector2);

	comp = 0.0;
	for(int i = 0; i < N; ++i)
		comp += vector1[i] + vector2[i];

	assert(comp == res);

	delete[] vector1;
	delete[] vector2;
}</div>