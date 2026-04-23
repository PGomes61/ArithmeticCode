#ifndef TESTS_H
#define TESTS_H

#include <stdint.h>

/**
 * @file tests.h
 * @author Paulo Vinicius & Pedro Lucas
 * @brief Suíte de testes independentes com Referência de Ouro para validação matemática.
 */

/**
 * @brief Executa a certificação do algoritmo contra valores pré-calculados.
 * * Esta função atua como um "Juiz Independente", comparando o resultado
 * do encoder com constantes matemáticas verificadas externamente,
 * garantindo que o algoritmo não dependa apenas do seu próprio decoder
 * para ser validado.
 */
void run_certification_tests(void);

#endif // TESTS_H
