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

Filosofo::Filosofo(bool last) : last(last){
    cout << "Filosofo criado" << endl;
    this->running = true;
}

void Filosofo::pensando(){
    this_thread::sleep_for(chrono::milliseconds(rand() % 3000 + 500));
    this->state = StateFilosofo::COMFOME;
}

void Filosofo::comendo(){
    this_thread::sleep_for(chrono::seconds(2));
    this->state = StateFilosofo::PENSANDO;
}

string Filosofo::getState(){
    return stateFilosofoToString(static_cast<int>(this->state));
}

void Filosofo::run(){
    while(running){
        switch(this->state){
            case StateFilosofo::PENSANDO:
                pensando();
                break;
            case StateFilosofo::COMFOME:
                if(this->last){
                    // trava o garfo esquerdo primeiro caso seja o ultimo filosofo
                    lock_guard<mutex> lock1(this->garfoEsquerdo->mtx);
                    lock_guard<mutex> lock2(this->garfoDireito->mtx);

                    this->state = StateFilosofo::COMENDO;
                    comendo();
                } else{
                    // trava o garfo direito primeiro se não for o ultimo filosofo
                    lock_guard<mutex> lock1(this->garfoDireito->mtx);
                    lock_guard<mutex> lock2(this->garfoEsquerdo->mtx);

                    this->state = StateFilosofo::COMENDO;
                    comendo();
                }
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

    // Inicializa os filosofos e os garfos
    for(int i = 0; i < n; i++){
        // OBS: a flag aqui é usada para a logica de qual garfo pegar primeiro, não para dizer pra thread que o garfo esta livre ou ocupado
        if(i == n - 1) filosofos.push_back(Filosofo(true)); // Se for o ultimo filosofo configura a flag last como true
        else filosofos.push_back(Filosofo(false)); // Se não, configura como false
        garfos.push_back(make_unique<Garfo>());
    }

    // Configura os garfos dos filosofos
    for(int i = 0; i < n; i++){
        filosofos[i].setGarfoDireito(garfos[i].get());
        filosofos[i].setGarfoEsquerdo(i == 0 ? garfos[n - 1].get() : garfos[i - 1].get());
    }

    // Cria as threads dos filosofos
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