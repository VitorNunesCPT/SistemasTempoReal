Experimento 1: Criando e Aplicando o Conceito de
Tarefas
Valor: 2 pontos de 10
_____________________________________________________________________________
Problema: Considere um sistema de controle de distância de um robô em que além de verificação de distância em suas 4 laterais é necessário controle de velocidade das 4 rodas. Para a verificação de obstáculos são utilizados 4 sensores ultrassônicos e para movimentação, 4 motores cc. Assim são consideradas as seguintes regras:
1 – A lei de controle que delimita a tensão de atuação via PWM dos motores deve incrementar e decrementar o dutycicle, exponencialmente, a medida que o sensor indica um afastamento ou aproximação respectivamente.
2- A verificação de distância de cada sensor é considerada uma tarefa (bibliotecas são permitidas).
3 – A tarefa de controle dos 4 motores é única.
Obs: O Aluno deve definir a periodicidade de cada tarefa de acordo com a
necessidade do problema.
Objetivo: Desenvolver um sistema escalonado em executivo cíclico através de
chamadas de função.
_____________________________________________________________________________
Dispositivos necessários: Simulador wokwi
(https://wokwi.com/projects/406017473298169857)
• Arduino ou Esp 32;
• Computador;
• Sensor Ultrassônico;
• Motor DC;
• Ponte H;
Sem uso da FreeRTOS____________________________________
Tarefa 1: Defina as características de tempo real de cada tarefa, realizando testes de tempo de computação de cada tarefa;
Tarefa 2: Esquematize a execução através de um diagrama de fluxo temporal em um sistema de ciclo único maior e com um sistema de ciclos menor (Lembre de avaliar a viabilidade de cada esquema)
Tarefa 3: Desenvolva o algoritmo de executivo cíclico e verifique se a periodicidade e deadline de cada tarefa é atendido na simulação.
Tarefa 4: Desenvolva o relatório do experimento. 

Da Avaliação do relatório:
• Devem ser apresentados os códigos com resultados simulados
• Os ajustes de tempo devem estar bem descritos
• Os questionamentos devem ser bem respondidos e destalhados