#include "problemaDosFilosofos.h"
// -----------------------
// GARFO
// -----------------------

Garfo::Garfo(){}

void Garfo::livre(){

}

void Garfo::ocupado(){

}

// -----------------------
// FILOSOFO
// -----------------------

Filosofo::Filosofo(){
    this->running = true;
    this->run();
}

void Filosofo::pensando(){

}

void Filosofo::comendo(){

}

void Filosofo::comFome(){

}

void Filosofo::run(){
    while(running){
        switch (this->state) {
            case StateFilosofo::PENSANDO:
                pensando();
                break;
            case StateFilosofo::COMENDO:
                comendo();
                break;
            case StateFilosofo::COMFOME:
                comFome();
                break;
        }
    }
}

bool Filosofo::setGarfoEsquerdo(Garfo* garfo){
    if(garfo != nullptr){
        this->garfoEsquerdo = garfo;
        return true;
    }
    return false;
}

bool Filosofo::setGarfoDireito(Garfo* garfo){
    if(garfo != nullptr){
        this->garfoDireito = garfo;
        return true;
    }
    return false;
}

// -----------------------
// MAIN
// -----------------------

int main(){
    int n = 5;

    vector<Filosofo> filosofos;
    vector<Garfo> garfos;

    for(int i = 0; i < n; i++){
        filosofos.push_back(Filosofo());
        garfos.push_back(Garfo());
    }

    for(int i = 0; i < n; i++){
        filosofos[i].setGarfoDireito(&garfos[i]);
        filosofos[i].setGarfoEsquerdo(i == 0 ? &garfos[n -1] : &garfos[i - 1]);
    }

    return 0;
}