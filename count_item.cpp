//
//  count_item.cpp
//  
//
//  Created by Sheng Guan on 10/8/18.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_set>
using namespace std;

vector<string> split(string s){        //split each line to get attributes/data
    int i = 0;
    int start = 0;
    vector<string> res;
    while(s[i] != '\0'){
        if(s[i]==' '){
            res.push_back(s.substr(start, i - start));
            start = i + 1;
        }
        i++;
    }
    res.push_back(s.substr(start));
    return res;
}

int main(int argc, const char * argv[]) {
    ifstream f(argv[1], ios::in);
    if(!f.is_open())
        return 0;
    vector<vector<string> > data;
    string s;
    vector<int> res;
    while(getline(f, s)){
        data.push_back(split(s));
    }
    unordered_set<string> se;
    for(int j=0; j<data[0].size(); j++){
        for(int i=0; i<data.size(); i++){
            if(!se.count(data[i][j]))
                se.insert(data[i][j]);
        }
        res.push_back(se.size());
        se.clear();
    }
    
    for(int i=0; i<res.size(); i++)
        cout<<res[i]<<" ";
    cout<<endl;
    return 0;
}
