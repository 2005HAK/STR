#ifndef PROBLEMA_DOS_FILOSOFOS_H
#define PROBLEMA_DOS_FILOSOFOS_H

#include <mutex>
#include <string>
#include <vector>

using namespace std;

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
		StateGarfo state;
	public:
		Garfo();

		void livre();

		void ocupado();
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

		void run();
};

#endif // PROBLEMA_DOS_FILOSOFOS_H