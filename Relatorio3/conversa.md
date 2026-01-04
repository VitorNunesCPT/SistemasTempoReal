<Pergunta><logCodigo1>[PRODUTOR] Tentando enviar para a fila...
>> [INFO] Computacao: 62 us
>> [INFO] Espera p/ Escrita: 0 ms

[CONSUMIDOR] Dado recebido e processado!
<< [INFO] Espera p/ Leitura: 7 ms
<< [INFO] Computacao: 82 us

[PRODUTOR] Tentando enviar para a fila...
>> [INFO] Computacao: 62 us
>> [INFO] Espera p/ Escrita: 0 ms

[PRODUTOR] Tentando enviar para a fila...

[CONSUMIDOR] Dado recebido e processado!
<< [INFO] Espera p/ Leitura: 0 ms
<< [INFO] Computacao: 41 us
>> [INFO] Computacao: 62 us
>> [INFO] Espera p/ Escrita: 1572 ms

[PRODUTOR] Tentando enviar para a fila...

[CONSUMIDOR] Dado recebido e processado!
<< [INFO] Espera p/ Leitura: 0 ms
<< [INFO] Computacao: 41 us
>> [INFO] Computacao: 62 us
>> [INFO] Espera p/ Escrita: 1950 ms

[CONSUMIDOR] Dado recebido e processado!
<< [INFO] Espera p/ Leitura: 0 ms
<< [INFO] Computacao: 42 us<logCodigo1><logCodigo2>[MUTEX] Recurso obtido para: Mudanca de Botao
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms
[PRODUTOR] Computacao: 62 us
[PRODUTOR] Espera Fila: 0 ms

[MUTEX] Recurso obtido para: Mudanca de Botao
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms
[PRODUTOR] Computacao: 62 us
[PRODUTOR] Espera Fila: 241 ms

[MUTEX] Recurso obtido para: Atualizando Saida
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms

[MUTEX] Recurso obtido para: Mudanca de Botao
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms
[PRODUTOR] Computacao: 62 us
[PRODUTOR] Espera Fila: 1821 ms

[MUTEX] Recurso obtido para: Atualizando Saida
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms

[MUTEX] Recurso obtido para: Mudanca de Botao
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms
[PRODUTOR] Computacao: 62 us
[PRODUTOR] Espera Fila: 1971 ms

[MUTEX] Recurso obtido para: Atualizando Saida
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms

[MUTEX] Recurso obtido para: Atualizando Saida
[AVALIAÇÃO] Tempo de espera pelo Mutex: 0 ms
<logCodigo2><Pergunta>
<Resposta>Com base nos logs que você forneceu e nos procedimentos descritos nas imagens, podemos realizar uma comparação técnica detalhada sobre o comportamento das tarefas e os riscos de **deadlock**.

---

### Tabela Comparativa de Métricas (Logs)

| Métrica | Log 1 (Apenas Queue) | Log 2 (Queue + Mutex) | Observação |
| --- | --- | --- | --- |
| **Tempo de Computação** | ~62 us (Produtor) / ~41 us (Consumidor) | ~62 us (Produtor) | O custo do código é quase idêntico. |
| **Espera p/ Escrita (Fila)** | **0 ms** a **1950 ms** | **0 ms** a **1971 ms** | A fila de tamanho 1 gera o maior gargalo. |
| **Espera pelo Mutex** | N/A | **0 ms** constante | O Mutex está sendo liberado rapidamente. |
| **Frequência de Logs** | Intercalado (Produção/Consumo) | Intercalado com logs de Posse | O Mutex adiciona uma camada de controle. |

---

### 1. Análise do Método com Queue (Log 1)

No seu primeiro log, observamos o comportamento clássico de um produtor limitado pela velocidade do consumidor:

* **Bloqueio por Fila Cheia:** O produtor registra esperas de até **1950 ms**. Isso ocorre porque a fila foi criada com **tamanho 1**.
* **Risco de Deadlock:** Como o código utiliza `portMAX_DELAY`, o produtor entra em estado **Blocked** permanentemente se o consumidor parar. No log, vemos que o produtor fica travado em "Tentando enviar para a fila..." até que o consumidor processe o item anterior.
* **O Gatilho:** Se a mensagem `[CONSUMIDOR] Dado recebido` desaparecesse, o sistema sofreria um deadlock funcional total na tarefa de botões.

### 2. Análise do Método com Mutex (Log 2)

O segundo log introduz a proteção de recurso compartilhado (Mutex) para a função `teste()`.

* **Contenção do Mutex:** O tempo de espera pelo Mutex foi de **0 ms** em todos os registros. Isso indica que, embora duas tarefas disputem o recurso, a tarefa que o detém o libera tão rápido que a outra quase não percebe o bloqueio.
* **Herança de Prioridade:** O slide menciona que o Mutex minimiza a **inversão de prioridade**. Se a tarefa de LED estivesse com o Mutex e a de Botões (prioridade maior) tentasse pegá-lo, o LED teria sua prioridade elevada temporariamente para terminar a execução e evitar o travamento do sistema.
* **Deadlock Combinado:** O risco aqui é duplo. Se o produtor pegar o Mutex e tentar enviar para a fila (`xQueueSend`) com `portMAX_DELAY` enquanto ela está cheia, e o consumidor precisar do Mutex para ler a fila, você criaria um **Deadlock Circular** (um espera pelo outro para sempre).

---

### Conclusão sobre Deadlocks

1. **Queue (Log 1):** O deadlock é **unidirecional**. O produtor é totalmente dependente do consumidor devido ao tamanho da fila e ao `portMAX_DELAY`.
2. **Mutex (Log 2):** O deadlock pode ser **bidirecional**. Embora o log mostre um sistema saudável (**0 ms de espera pelo Mutex**), o uso de `portMAX_DELAY` em seções críticas protegidas por Mutex exige cuidado extremo para não travar o recurso compartilhado permanentemente.

> **Nota Técnica:** Os tempos de espera de quase 2 segundos (**1971 ms**) sugerem que você manteve o `vTaskDelay` longo na tarefa do consumidor para testar o limite da fila. Em um cenário real, isso causaria uma percepção de "travamento" nos botões.

Gostaria que eu alterasse o código para substituir o `portMAX_DELAY` por um **timeout finito** (ex: 100ms) para demonstrar como o sistema se recupera de uma fila cheia sem entrar em deadlock?<Resposta>