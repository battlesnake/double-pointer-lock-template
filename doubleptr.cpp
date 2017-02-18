#if 0
(
set -eu
declare -r tmp="$(mktemp)"
g++ -std=c++14 -Wall -Wextra -o "$tmp" "$0"
valgrind --quiet "$tmp"
)
exit 0
#endif
/* Run this file with bash to compile+execute it */
#include <mutex>
#include <iostream>
#include <type_traits>

/* Demo: double-smart pointer for automatic locking of arbitrary class */

template <typename T>
class ThreadSafe
{
	T t;
	mutable std::mutex mx;
	class Proxy
	{
		std::unique_lock<std::mutex> lock;
		T& t;
	public:
		explicit Proxy(std::mutex& mx, T& t) : lock(mx), t(t) { std::cout << "  (lock)" << std::endl; }
		Proxy(Proxy&& src) : lock(std::move(src.lock)), t(src.t) { }
		~Proxy() { std::cout << "  (unlock)" << std::endl; }
		T *operator -> () { return &t; }
	};
public:
	template <typename ...Args>
	explicit ThreadSafe(Args&&... args) : t(std::forward<Args>(args)...) { }
	Proxy operator -> () { return Proxy(mx, t); }
	template <typename Func, typename R = typename std::enable_if<!std::is_void<typename std::result_of<Func(T&)>::type>::value>::type>
	auto operator () (const Func& func) {
		std::lock_guard<std::mutex> lock(mx);
		return func(t);
	}
	template <typename Func, typename R = typename std::enable_if<std::is_void<typename std::result_of<Func(T&)>::type>::value>::type>
	void operator () (const Func& func) {
		func( * operator -> () . operator -> () );
	}
};

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	/* Initialise s as a thread-safe wrapper around a std::string */
	ThreadSafe<std::string> s("potato");
	/* For doing one operation, the pointer proxy style is convenient */
	s->append("es are awesome");
	/* Function style: Passed function is executed within lock and may do multiple operations */
	s([] (auto& str) { str += "!"; });
	/* Not thread safe, as our lock object expires before the pointer is used.  Use the function style in place of this: */
	std::cout << s->c_str() << std::endl;
	return 0;
}
