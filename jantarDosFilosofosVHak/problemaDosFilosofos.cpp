#include "problemaDosFilosofos.h"

string stateGarfoToString(int state){
	switch (state) {
		case 0: return "L";
		case 1: return "O";
		default: return "UNKNOWN";
	}
}

string stateFilosofoToString(int state){
	switch (state) {
		case 0: return "P";
		case 1: return "C";
		case 2: return "F";
		default: return "UNKNOWN";
	}
}

// -----------------------
// GARFO
// -----------------------

Garfo::Garfo(){
    cout << "Garfo criado" << endl;
}

void Garfo::livre(){
    this->state = StateGarfo::LIVRE;
}

void Garfo::ocupado(){
    this->state = StateGarfo::OCUPADO;
}

string Garfo::getState(){
    return stateGarfoToString(static_cast<int>(this->state));
}

// -----------------------
// FILOSOFO
// -----------------------

Filosofo::Filosofo(){
    cout << "Filosofo criado" << endl;
    this->running = true;
}

void Filosofo::pensando(){
    this_thread::sleep_for(chrono::milliseconds(rand() % 2000 + 500));
    this->state = StateFilosofo::COMFOME;
}

void Filosofo::comFome(){
    scoped_lock lock(this->garfoEsquerdo->mtx, this->garfoDireito->mtx);

    if(this->garfoDireito->getState() == "L"){
        this->garfoDireito->ocupado();
        if(this->garfoEsquerdo->getState() == "L"){
            this->garfoEsquerdo->ocupado();
            this->state = StateFilosofo::COMENDO;
        } else{
            this->garfoDireito->livre();
            // tratar o caso em que o garfo esquerdo está ocupado
        }
    }
}

void Filosofo::comendo(){
    this_thread::sleep_for(chrono::seconds(3));
    this->garfoEsquerdo->livre();
    this->garfoDireito->livre();
    this->state = StateFilosofo::PENSANDO;
}

string Filosofo::getState(){
    return stateFilosofoToString(static_cast<int>(this->state));
}

void Filosofo::run(){
    while(running){
        switch (this->state) {
            case StateFilosofo::PENSANDO:
                pensando();
                break;
            case StateFilosofo::COMFOME:
                comFome();
                break;
            case StateFilosofo::COMENDO:
                comendo();
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
    vector<unique_ptr<Garfo>> garfos;

    for(int i = 0; i < n; i++){
        filosofos.push_back(Filosofo());
        garfos.push_back(make_unique<Garfo>());
    }

    for(int i = 0; i < n; i++){
        filosofos[i].setGarfoDireito(garfos[i].get());
        filosofos[i].setGarfoEsquerdo(i == 0 ? garfos[n - 1].get() : garfos[i - 1].get());
    }

    vector<thread> threadsFilosofos;

    for(int i = 0; i < n; i++) threadsFilosofos.push_back(thread(&Filosofo::run, &filosofos[i]));

    while(true){
        string statesFilosofos;
        string statesGarfos;
        for(int i = 0; i < n; i++){
            statesFilosofos += filosofos[i].getState() + ",";
            if(i == n - 1) statesGarfos += garfos[i]->getState();
            else statesGarfos += garfos[i]->getState() + ",";
        }
        cout << statesFilosofos << statesGarfos << endl;
    }

    return 0;
}