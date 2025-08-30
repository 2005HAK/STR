#include "problemaDosFilosofos.h"

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

string Garfo::getState(){
    if(this->mtx.try_lock()){
        this->mtx.unlock();
        return "L";
    } else return "O";
}

// -----------------------
// FILOSOFO
// -----------------------

Filosofo::Filosofo(){
    cout << "Filosofo criado" << endl;
    this->running = true;
}

void Filosofo::pensando(){
    this_thread::sleep_for(chrono::milliseconds(rand() % 3000 + 500)); // usado para simular o tempo de pensamento
    this->state = StateFilosofo::COMFOME;
}

void Filosofo::comendo(){
    this_thread::sleep_for(chrono::seconds(2)); // simula o tempo de comer
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
                scoped_lock lock(this->garfoEsquerdo->mtx, this->garfoDireito->mtx);

                this->state = StateFilosofo::COMENDO;
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
    // Número de filosofos e garfos
    int n = 5;
    
    // Cria os filosofos e os garfos
    vector<Filosofo> filosofos;
    vector<unique_ptr<Garfo>> garfos;

    // Inicializa os filosofos e garfos
    for(int i = 0; i < n; i++){
        filosofos.push_back(Filosofo());
        garfos.push_back(make_unique<Garfo>());
    }

    // Configura os garfos para cada filosofo
    for(int i = 0; i < n; i++){
        filosofos[i].setGarfoDireito(garfos[i].get());
        filosofos[i].setGarfoEsquerdo(i == 0 ? garfos[n - 1].get() : garfos[i - 1].get());
    }

    // Cria as threads para cada filosofo
    vector<thread> threadsFilosofos;

    // Inicia as threads dos filosofos
    for(int i = 0; i < n; i++) threadsFilosofos.push_back(thread(&Filosofo::run, &filosofos[i]));

    // Imprime o estado dos filosofos e dos garfos
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