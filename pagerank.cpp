#include <iostream>
#include <cstring>
#include <string>
#include<vector>
#include<map>
#include <unordered_map>
#include<iomanip>
using namespace std;        
class AdjacencyList{
    private:
        map<string , int> index;   //key:website name, value:index
        unordered_map<int , vector<string>> graph; //key:dst, value:src website name
        int ind = 0;    //count the index
    public:
        void insertEdge(string from , string to); 
        vector<int> getOutDegree();
        void pageRank(int pow_iter);
};
void AdjacencyList::insertEdge(string from , string to){
    if (index.find(from) == index.end())
        index[from] = ind++;
    if (index.find(to) == index.end())
        index[to] = ind++;
    graph[index[to]].push_back(from);
    if (graph.find(index[from]) == graph.end())
        graph[index[from]] = {};
}
vector<int> AdjacencyList::getOutDegree(){
    vector<int> degree(ind , 0);
    for (auto it = index.begin() ; it != index.end() ; it++){
        for (int i = 0 ; i < graph[it->second].size() ; i++){
            degree.at(index[graph[it->second].at(i)])++;
        }
    }
    return degree;
}
void AdjacencyList::pageRank(int pow_iter){
    vector<int> outDegree = getOutDegree();   //get the vector of out degree
    vector<double> init_vec(ind , 1.0/ind);    //r(0)
    vector<double> rank_vec(ind , 0.0);       //init the rank
    if (pow_iter == 1){
        for (auto it = index.begin() ; it != index.end() ; it++){
            cout<<it->first<<" ";
            cout<<fixed<<showpoint;
            cout<<setprecision(2);
            cout<<(1.0/ind)<<endl;
        }
    }else{
        for (int i = 1; i < pow_iter ; i++){
            if (i!=1){
                init_vec = rank_vec;
            }
            for (auto it = index.begin(); it != index.end(); it++){
                double sum = 0;
                if (i == pow_iter-1){
                    cout<<it->first<<" ";
                }
                for (int k = 0 ; k < graph[it->second].size(); k++){
                    sum += (init_vec.at(index[graph[it->second].at(k)]) * (1.0 / outDegree.at(index[graph[it->second].at(k)])));
                }
                rank_vec.at(it->second) = sum;
				if (i == pow_iter - 1) {
					cout << fixed << showpoint;
					cout << setprecision(2);
					cout << sum << endl;
				}
            }
        }
    }
    
}
int main()
{
    int num_of_lines,num_of_po;
    //read first number of lines(num_of_lines) from cin
    cin>>num_of_lines;
    //read the number of power iterations(num_of_po) from cin
    cin>>num_of_po;
    std::string from;
    std::string to;
    AdjacencyList adj;
    //for each of the next n lines,read in the vertices from and to
    for(int i = 0; i < num_of_lines ; i++){
        //read in the source vertex
        std::cin>>from;
        //read in the destination vertex
        std::cin>>to;
        adj.insertEdge(from , to);
    }
    adj.pageRank(num_of_po);
    return 0;
}