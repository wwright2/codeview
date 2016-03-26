#include <iostream>
#include <vector>
#include <cstdlib>
#include <string>
#include <stdexcept>

using namespace std;

template <class T>
class Stack { 
  private: 
    vector<T> elems;     // elements 

  public: 
    void push(T const&);  // push element 
    void pop();               // pop element 
    T top() const;            // return top element 
    bool empty() const{       // return true if empty.
        return elems.empty(); 
    }
	int size() { return elems.size();}
}; 

template <class T>
void Stack<T>::push (T const& elem) 
{ 
    // append copy of passed element 
    elems.push_back(elem);    
} 

template <class T>
void Stack<T>::pop () 
{ 
    if (elems.empty()) { 
        throw out_of_range("Stack<>::pop(): empty stack"); 
    }
	// remove last element 
    elems.pop_back();         
} 

template <class T>
T Stack<T>::top () const 
{ 
    if (elems.empty()) { 
        throw out_of_range("Stack<>::top(): empty stack"); 
    }
	// return copy of last element 
    return elems.back();      
} 

int main() 
{ 
    try { 
        Stack<int>         intStack;  // stack of ints 
        Stack<string> stringStack;    // stack of strings 

        // manipulate int stack 
        intStack.push(7);  cout << "Push 7\n";
		intStack.push(11); cout << "Push 11\n";
		
        cout << "count="<< intStack.size()<< ", " << intStack.top() <<endl; 
		intStack.pop();
        cout << "count="<< intStack.size()<< ", " << intStack.top() <<endl; 
		intStack.pop();
		cout << "count="<< intStack.size()<< ", " << endl;
		

        // manipulate string stack 
        stringStack.push("hello"); 
		stringStack.push("sam");
        cout << "count="<< stringStack.size()<< ", TOP=" << stringStack.top() << " *POP*" << std::endl; 
        stringStack.pop(); 
        cout << "count="<< stringStack.size()<< ", TOP=" << stringStack.top() << "  *POP* " << std::endl; 
        stringStack.pop(); 
		
		cout << "count="<< stringStack.size()<<std::endl;
		cout << "POP empty stack"<< std::endl;
        stringStack.pop(); 
    } 
    catch (exception const& ex) { 
        cerr << "Exception: " << ex.what() <<endl; 
        return -1;
    } 
} 