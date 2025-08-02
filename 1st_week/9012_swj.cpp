#include <iostream>
#include <stack>
#include <string>
using namespace std;

int main(void)
{
	int a,i,j,size;
	stack<char> st;
	string str;
	cin >> a;

	for (i = 0; i < a; i++)
	{
		cin >> str;
		if (str.size() % 2) cout << "NO" << '\n';
		else
		{
			for (char temp : str)
			{
				if (st.empty() && temp == ')')
				{
					st.push(temp);
					break;
				}
				else if (temp == '(')
				{
					st.push(temp);
				}
				else if (temp == ')') st.pop();
			}
			if (st.empty()) cout << "YES" << '\n';
			else
			{
				cout << "NO" << '\n';
				size = st.size();
				for (j = 0; j < size;j++)
				{
					st.pop();
				}
			}
		}
	}
	return 0;
}