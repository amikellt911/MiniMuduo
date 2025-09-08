#include<iostream>
#include<regex>

using namespace std;
int main()
{
    string s="GET /new/ssh/login?id=1&name=admin HTTP/1.1\r\n";
    //.要转义两次，正则表达式要转义一次，C++字符串里面\也是要转义
    regex re("(GET|POST) ([^?]*)(?:\\?(.*))? (HTTP/[12]\\.[01])(?:\n|\r\n)?");
    smatch res;
    if(!regex_match(s,res,re))
    {
        return -1;
    }
    for(auto i:res)
    {
        cout<<i<<endl;
    }
    return 0;
}