/* ************************* 
// 백준 1158 요세푸스 문제

[풀이]

4번째 사람을 제거 하는경우.
- 1번째 사람 - 큐의 맨 뒤에 넣고 pop으로 삭제
- 2번째 사람 - 큐의 맨 뒤에 넣고 pop으로 삭제
- 3번째 사람 - 큐의 맨 뒤에 넣고 pop으로 삭제
- 4번째 사람 - 출력 후 pop으로 삭제
- 이 과정을 반복.

***************************** */
#include <iostream>
#include <queue>
using namespace std;

int main(void)
{
	queue<int> pus;
	int a, b,i;
	cin >> a >> b;

	for (i = 1; i <= a; i++) pus.push(i);
	cout << "<";
	while (1)
	{
		for (i = 0; i < b; i++)
		{
			if (i == b - 1)
			{
				cout << pus.front();
				pus.pop();
			}
			else
			{
				pus.push(pus.front());
				pus.pop();
			}
		}
		if (pus.empty()) break;
		cout << ", ";
	}
	cout << ">" << "\n";


	return 0;
}