#ifndef TESTS_H
#define TESTS_H

#include <stdint.h>

/**
 * @file tests.h
 * @author Paulo Vinicius & Pedro Lucas
 * @brief Suíte de testes independentes com Referência de Ouro para validação matemática.
 */

/**
 * @brief Executa a suíte de testes unitários utilizando o framework Unity.
 * * Esta função atua como o ponto central de validação do projeto, orquestrando
 * a execução dos casos de teste contra a Referência de Ouro (Golden Reference).
 * * @details
 * O fluxo de execução segue os seguintes critérios:
 * 1. Inicializa o ambiente de teste do Unity (UNITY_BEGIN).
 * 2. Realiza o isolamento de estado através do ciclo setUp/tearDown para garantir
 * a integridade das estruturas de dados estáticas (8KB).
 * 3. Valida a conformidade matemática do encoder com precisão de 10^-12 (Epsilon).
 * 4. Finaliza o framework e emite o relatório de conformidade no console (UNITY_END).
 * * @note Este procedimento é executado ANTES da demonstração real para garantir que
 * o motor matemático não sofreu regressões durante o desenvolvimento.
 */
void run_unity_tests(void);


#endif // TESTS_H
