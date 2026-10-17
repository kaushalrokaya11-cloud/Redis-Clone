#include<stdio.h>
#include<string.h>
#include<vector>
#include<iostream>
using namespace std;

int main(){
  
    vector<int> vec(10);
   int * p = vec.data();
   *p = 30;
   cout<<&vec[10]-&vec[1];
    
}