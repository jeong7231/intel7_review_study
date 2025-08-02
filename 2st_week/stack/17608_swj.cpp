#include <iostream>
#include <stack>
using namespace std;

int main(void)
{
	stack<int> a;
	int b,c,e,d=0;
	cin >> c;
	for (b=0; b<c;b++)
	{
		int d;
		cin >> d;
		a.push(d);
	}
	e = a.size();
	for (b = 0; b < e;b++)
	{
		if (b == 0)
		{
			d++;
			c = a.top();
			a.pop();
		}
		else
		{
			if (c >= a.top())
			{
				a.pop();
			}
			else if (c < a.top())
			{
				c = a.top();
				d++;
				a.pop();
			}
		}
	}
	cout << d;
	return 0;
}