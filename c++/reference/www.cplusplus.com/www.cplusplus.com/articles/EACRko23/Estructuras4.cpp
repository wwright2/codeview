#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;
struct l{
    int v;
    l*next;};
void display(l*l,string&sg){
    char Result[16];
    stringstream ss;
    string f;
    if(l==NULL){cout<<"-"<<endl<<endl;sg+="*\n";}
    else if(l!=NULL){
        sprintf(Result,"%d",l->v);
        ss << Result;
        ss >> f;
        cout<<l->v<<endl;
        sg+=f;
        display(l->next,sg);}}
l* add(l*list,int s){
    l*ob=new l;ob->v=s;
    ob->next=list;return ob;}
void cambiar(l*&l1,l*&l2,l*&l3,int s,int&n,string&sg){
    char Result[16];
    stringstream ss;
    string f;
    if(s==1){int i=(n+3)%3;
            if(i==1){l3=add(l3,l1->v);l1=l1->next;
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l1 a l3\n\n";
                cout<<"Pieza "<<s<<". Cambio de l1 a l3\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}
            else if(i==2){l2=add(l2,l3->v);l3=l3->next;
                cout<<"Pieza "<<s<<". Cambio de l3 a l2\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l3 a l2\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}
            else{l1=add(l1,l2->v);l2=l2->next;
                cout<<"Pieza "<<s<<". Cambio de l2 a l1\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l2 a l1\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}}
    else{if(s%2==0){int i=(n+3)%3;
            if(i==2){l2=add(l2,l1->v);l1=l1->next;
                cout<<"Pieza "<<s<<". Cambio de l1 a l2\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l1 a l2\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}
            else if(i==1){l3=add(l3,l2->v);l2=l2->next;
                cout<<"Pieza "<<s<<". Cambio de l2 a l3\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l2 a l3\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}
            else{l1=add(l1,l3->v);l3=l3->next;
                cout<<"Pieza "<<s<<". Cambio de l3 a l1\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l3 a l1\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}}
        else{int i=(n+3)%3;
            if(i==0){l3=add(l3,l1->v);l1=l1->next;
                cout<<"Pieza "<<s<<". Cambio de l1 a l3\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l1 a l3\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}
            else if(i==1){l2=add(l2,l3->v);l3=l3->next;
                cout<<"Pieza "<<s<<". Cambio de l3 a l2\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l3 a l2\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}
            else{l1=add(l1,l2->v);l2=l2->next;
                cout<<"Pieza "<<s<<". Cambio de l2 a l1\n";
                sprintf(Result,"%d",s);
                ss << Result;
                ss >> f;
                sg+="Pieza "+f+". Cambio de l2 a l1\n\n";
                display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";}}}
            cout<<endl;}
void hanoy(l*&l1,l*&l2,l*&l3,int size,int&n1,string&sg){
    if(size==1){cambiar(l1,l2,l3,size,n1,sg);n1++;}
    else{hanoy(l1,l2,l3,size-1,n1,sg);
        cambiar(l1,l2,l3,size,n1,sg);
        hanoy(l1,l2,l3,size-1,n1,sg);}}
int main(){
    string sg="Bienvenido\n";
    l*l1=NULL;l*l2=NULL;l*l3=NULL;
    int s,n=1;int&n1=n;
    ofstream Fp("myfile.txt");
    while(true){
        cout<<"Tamano? ";cin>>s;
        if(s==0)break;
        for(int i=s;i>0;i--)l1=add(l1,i);
        sg+="Estado original de los pilares:\n\n";
        display(l1,sg);display(l2,sg);display(l3,sg);sg+="\n";
        sg+="Movimientos:\n";
        hanoy(l1,l2,l3,s,n1,sg);
        cout<<"---------------\n";
        sg+="---------------\n";
        l1=NULL;l2=NULL;l3=NULL;n=1;}
    sg+="Fin de los juegos\n";
    Fp<<sg;
    Fp.close();
    cout<<"La informacion se ha guardado en el archivo myfile.txt\n";}
