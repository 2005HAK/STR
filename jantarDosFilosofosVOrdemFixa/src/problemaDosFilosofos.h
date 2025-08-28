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

/**
 * @brief Converte o estado do garfo para uma string
 * 
 * @param state Estado do garfo
 */
string stateGarfoToString(int state);

/**
 * @brief Converte o estado do filosofo para uma string
 * 
 * @param state Estado do filosofo
 */
string stateFilosofoToString(int state);

/**
 * @brief Enum para os estados dos garfos
 */
enum class StateGarfo{
	LIVRE,
	OCUPADO
};

/**
 * @brief Enum para os estados dos filosofos
 */
enum class StateFilosofo{
	PENSANDO,
	COMENDO,
	COMFOME
};

/**
 * @brief Classe que representa um garfo
 */
class Garfo{
	private:
		StateGarfo state = StateGarfo::LIVRE;
	
	public:
		mutex mtx;
		
		/**
		 * @brief Construtor da classe Garfo
		 */
		Garfo();

		/**
		 * @brief Define o estado do garfo como livre
		 */
		void livre();

		/**
		 * @brief Define o estado do garfo como ocupado
		 */
		void ocupado();

		/**
		 * @brief Retorna o estado do garfo como string
		 * @return Estado do garfo
		 */
		string getState();
};

/**
 * @brief Classe que representa um filosofo
 */
class Filosofo{
	private:
		StateFilosofo state = StateFilosofo::PENSANDO;
		Garfo* garfoEsquerdo;
		Garfo* garfoDireito;
		bool running = false;

	public:
		/**
		 * @brief Construtor da classe Filosofo
		 */
		Filosofo();

		/**
		 * @brief Define o estado de pensamento do filosofo
		 * 
		 * Este método simula o tempo de pensamento do filosofo e muda seu estado para COMFOME
		 */
		void pensando();

		/**
		 * @brief Define o estado de fome do filosofo
		 * 
		 * Este método tenta pegar os garfos e mudar o estado do filosofo para COMENDO
		 */
		void comFome();
		
		/**
		 * @brief Define o estado de fome do filosofo
		 * 
		 * Este método simula o tempo de comer do filosofo, libera os garfos e muda seu estado para PENSANDO
		 */
		void comendo();

		/**
		 * @brief Configura o garfo esquerdo do filosofo
		 * 
		 * @param garfo Ponteiro para o garfo esquerdo
		 */
		bool setGarfoEsquerdo(Garfo* garfo);

		/**
		 * @brief Configura o garfo direito do filosofo
		 * 
		 * @param garfo Ponteiro para o garfo direito
		 */
		bool setGarfoDireito(Garfo* garfo);

		/**
		 * @brief Retorna o estado do filosofo como string
		 * 
		 * @return Estado do filosofo
		 */
		string getState();

		/**
		 * @brief Método que executa o ciclo de vida do filosofo
		 * 
		 * Este método é executado em uma thread separada e controla o estado do filosofo
		 */
		void run();
};

#endif // PROBLEMA_DOS_FILOSOFOS_H