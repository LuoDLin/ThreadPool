#include "ThreadPool.h"

#include <iostream>

using namespace std;

int main()
{
	ThreadPool pool(4, 4);
	pool.start();
	vector<future<int>> res;
	
	
	for ( int i = 0; i < 20; ++i )
	{
		res.emplace_back(pool.subTask([]( int a )
		                              {
			                              this_thread::sleep_for(a * 150ms);
			                              return a;
		                              }, i));
		cout << "push task " << i << '\n';
	}
	
	cout << "get result: ";
	for ( auto &r: res ) cout << r.get() << ' ';
	cout << '\n';
	
	pool.stop();
	
	
	return 0;
}