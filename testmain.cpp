#include <iostream>
#include <string>
#include <queue>
#include <iterator>
#include "ThreadPool.h"

using namespace std;

void fun1(int slp)
{
	printf("  hello, fun1 !  %d\n", std::this_thread::get_id());
	if (slp > 0) {
		printf(" ======= fun1 sleep %d  =========  %d\n", slp, std::this_thread::get_id());
		std::this_thread::sleep_for(std::chrono::milliseconds(slp));
	}
}

struct gfun {
	int operator()(int n) {
		printf("%d  hello, gfun !  %d\n", n, std::this_thread::get_id());
		return 42;
	}
};

int main() {
	SingleThreadPool executor(3);
	//std::future<void> ff = executor.submit(fun1, 0);
	//std::future<int> fg = executor.submit(gfun{}, 0);
	//std::future<std::string> fh = executor.submit([]()->std::string { std::cout << "hello, fh !  " << std::this_thread::get_id() << std::endl; return "hello,fh ret !"; });
	//ff.get();
	//cout << fg.get() << endl;
	//cout << fh.get() << endl;

	vector<std::future<void>> vecFuture;
	for (int i = 0; i < 50; i++)
	{
		vecFuture.emplace_back(executor.submit(fun1, i * 100));
	}
	int i = 0;
	for (auto& f : vecFuture) {
		f.get();
		i++;
	}
	vecFuture.clear();
	cout << i << endl;
	return 0;
}
