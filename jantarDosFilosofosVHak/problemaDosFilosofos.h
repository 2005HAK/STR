#ifndef PROBLEMA_DOS_FILOSOFOS_H
#define PROBLEMA_DOS_FILOSOFOS_H

#include <iostream>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <functional>
#include <memory>
#include <random>

using namespace std;

string stateGarfoToString(int state);

string stateFilosofoToString(int state);

enum class StateGarfo{
	LIVRE,
	OCUPADO
};

enum class StateFilosofo{
	PENSANDO,
	COMENDO,
	COMFOME
};

class Garfo{
	private:
		StateGarfo state = StateGarfo::LIVRE;
	
	public:
		mutex mtx;
		
		Garfo();

		void livre();

		void ocupado();

		string getState();
};

class Filosofo{
	private:
		StateFilosofo state = StateFilosofo::PENSANDO;
		Garfo* garfoEsquerdo;
		Garfo* garfoDireito;
		bool running = false;

	public:
		Filosofo();

		void pensando();

		void comendo();

		void comFome();

		bool setGarfoEsquerdo(Garfo* garfo);

		bool setGarfoDireito(Garfo* garfo);

		string getState();

		void run();
};

#endif // PROBLEMA_DOS_FILOSOFOS_H