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
 * @brief Converte o estado do filosofo para uma string
 * 
 * @param state Estado do filosofo
 */
string stateFilosofoToString(int state);

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
	public:
		mutex mtx;
		
		/**
		 * @brief Construtor da classe Garfo
		 */
		Garfo();

		/**
		 * @brief Retorna o estado do garfo como string
		 * @return Estado do garfo com base no estado do mutex (LOCKED="O" ou UNLOCKED="L")
		 */
		string getState();
};

/**
 * @brief Classe que representa um filosofo
 */
class Filosofo{
	private:
		bool last;                                        // Flag para indicar se é o último filosofo (para evitar deadlock)
		StateFilosofo state = StateFilosofo::PENSANDO;    // Estado do filosofo, sendo PENSANDO o inicial
		Garfo* garfoEsquerdo;							  // Ponteiro para o garfo esquerdo
		Garfo* garfoDireito;                              // Ponteiro para o garfo direito
		bool running = false;                             // Flag para controlar a execução do filosofo

	public:
		/**
		 * @brief Construtor da classe Filosofo
		 */
		Filosofo(bool last);

		/**
		 * @brief Define o estado de pensamento do filosofo
		 * 
		 * Este método simula o tempo de pensamento do filosofo e muda seu estado para COMFOME
		 * Pensa por um tempo "aleatório" entre .5 e 3.5 segundos
		 */
		void pensando();

		/**
		 * @brief Define o estado onde o filosofo esta comendo
		 * 
		 * Este método simula o tempo de comer do filosofo e muda seu estado para PENSANDO
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