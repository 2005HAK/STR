#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <random>

using namespace std;

enum EstadoF { PENSANDO, FOME, COMENDO };
enum EstadoG { LIVRE, OCUPADO };

class Jantar {
    private:
        int N;
        vector<EstadoF> filosofos;
        vector<EstadoG> garfos;
        vector<condition_variable> cv;
        mutex mtx;

    public:
        Jantar(int n) : N(n), filosofos(n, PENSANDO), garfos(n, LIVRE), cv(n) {}

        void imprime_estado() {
            for (int i = 0; i < N; i++) {
                char estF = (filosofos[i] == PENSANDO ? 'P' :
                            filosofos[i] == FOME ? 'F' : 'C');
                cout << estF;
                if (i < N - 1) cout << ",";
            }
            cout << ",";
            for (int i = 0; i < N; i++) {
                char estG = (garfos[i] == LIVRE ? 'L' : 'O');
                cout << estG;
                if (i < N - 1) cout << ",";
            }
            cout << "\n";
        }

        void testa(int i) {
            int esq = i;
            int dir = (i + 1) % N;
            if (filosofos[i] == FOME &&
                filosofos[(i + N - 1) % N] != COMENDO &&
                filosofos[(i + 1) % N] != COMENDO &&
                garfos[esq] == LIVRE &&
                garfos[dir] == LIVRE) 
            {
                filosofos[i] = COMENDO;
                garfos[esq] = OCUPADO;
                garfos[dir] = OCUPADO;
                imprime_estado();
                cv[i].notify_one();
            }
        }

        void pegar_garfos(int i) {
            unique_lock<mutex> lock(mtx);
            filosofos[i] = FOME;
            imprime_estado();
            testa(i);
            while (filosofos[i] != COMENDO)
                cv[i].wait(lock);
        }

        void devolver_garfos(int i) {
            unique_lock<mutex> lock(mtx);
            filosofos[i] = PENSANDO;
            int esq = i;
            int dir = (i + 1) % N;
            garfos[esq] = LIVRE;
            garfos[dir] = LIVRE;
            imprime_estado();
            testa((i + N - 1) % N);
            testa((i + 1) % N);
        }
};

class Filosofo {
private:
    int id;
    Jantar &jantar;
    mt19937 gen;
    uniform_int_distribution<> tempo;

public:
    Filosofo(int id, Jantar &j)
        : id(id), jantar(j), gen(random_device{}()), tempo(500, 1500) {}

    void operator()() {
        while (true) {
            this_thread::sleep_for(chrono::milliseconds(tempo(gen))); // pensando ?
            jantar.pegar_garfos(id);
            this_thread::sleep_for(chrono::milliseconds(tempo(gen))); // comendo ?
            jantar.devolver_garfos(id);
        }
    }
};

int main() {
    const int N = 5;
    Jantar jantar(N);
    jantar.imprime_estado();

    vector<thread> threads;
    for (int i = 0; i < N; i++)
        threads.emplace_back(Filosofo(i, jantar));

    for (auto &t : threads)
        t.join();

    return 0;
}

