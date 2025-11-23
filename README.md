# **LR(0) Parser Generator — README**

Este projeto implementa um **gerador de analisador sintático LR(0)** em C++.
Ele lê uma gramática, constrói os **conjuntos de itens**, monta a **tabela de parsing LR(0)** e posteriormente **analisa qualquer string** informada pelo usuário, salvando o passo a passo do processo.

---

# **📌 Funcionalidades Principais**

* Construção automática dos **conjuntos LR(0)** usando *closure* e *goto*.
* Geração da **tabela LR(0)** (ações e desvios).
* Execução completa do **processo de análise LR(0)** de uma string.
* Registro detalhado do passo a passo da análise em um arquivo `.txt`.
* Suporte a múltiplas gramáticas via arquivos externos em `grammar/<id>.txt`.
* Impressão dos estados LR(0) e da tabela de parsing.

---

# **📁 Estrutura de Pastas Esperada**

```
/project-folder
│ main.cpp
│
├── grammar/
│     ├── 1.txt
│     ├── 2.txt
│     └── N.txt
│
└── parsable_strings/
      └── (gerado automaticamente)
```

Cada arquivo `grammar/<id>.txt` deve conter **uma produção por linha**, como:

```
S->AB
A->aA
A->a
B->b
```

---

# **🛠️ Como o Programa Funciona**

A seguir está um resumo claro dos principais componentes:

---

## **1) Inserção do ponto inicial — `insert_dot()`**

Coloca o ponto `.` logo após `->`, gerando o item inicial:

```
S->AB  →  S->.AB
```

---

## **2) Montagem de chaves únicas para conjuntos — `vector_key()`**

Une um vetor de strings em uma única string com separadores.
Isso permite comparar conjuntos de itens corretamente.

---

## **3) Construção do Fecho LR(0) — `closure()`**

Dado um item com ponto antes de um não-terminal, o programa adiciona todas as produções possíveis com o ponto no início.

Exemplo:

```
A->.BC
B->.dE
C->.f
```

Esse processo continua até não surgirem novos itens.

---

## **4) Função goto — `goto_state()`**

Move o ponto `.` uma posição à direita e calcula o *closure*, formando novos estados.

---

## **5) Identificação de terminais e não-terminais**

* `get_terminals()` → retira símbolos minúsculos + `$`
* `get_non_terminals()` → extrai símbolos maiúsculos

---

## **6) Construção dos Estados LR(0)**

O programa faz:

1. Gera o estado inicial via `closure("S'->.S")`
2. Expande usando `goto` para cada item
3. Garante que estados repetidos não sejam duplicados
4. Armazena tudo em `completed_sets`

---

## **7) Construção da Tabela LR(0)**

A tabela contém:

### Ações:

* `S#` → shift
* `r#` → reduce
* `Accept` → aceitação

### Goto:

* apenas para não-terminais

A representação final é montada com `table_to_string()`.

---

## **8) Análise de String (Parsing)**

O usuário fornece a string:

```
abab
```

O programa:

1. Adiciona `$` ao final
2. Usa uma pilha para armazenar **símbolos e estados**
3. Consulta a tabela LR(0)
4. Executa **shift**, **reduce** ou **accept**
5. Guarda cada passo em `trace`

Caso aceite, o arquivo é salvo em:

```
parsable_strings/<id>/<string_comprimida>.txt
```

---

# **▶️ Como Executar**

Compile o programa (g++):

```bash
g++ -std=c++17 main.cpp -o parser
```

Execute:

```bash
./parser
```

---

# **📥 Entrada do Usuário**

### **1) ID da gramática**

O sistema pergunta:

```
Enter grammar number:
```

Se você digitar **1**, ele abrirá:

```
grammar/1.txt
```

---

### **2) String a ser analisada**

Exemplo:

```
Enter the string to be parsed:
abba
```

---

# **📤 Saída**

### O programa exibe:

✔ Conjuntos LR(0)
✔ Tabela LR
✔ Resultado da análise

Se a string for aceita:

```
A string abba e parsiavel(Aceita)!
Salvo em parsable_strings/1/a1b2.txt
```

---

# **📄 Arquivo Gerado (exemplo)**

O arquivo contém um passo a passo completo da pilha, lookahead, ação escolhida, etc.

---

# **🧠 Requisitos da Gramática**

* Não deve conter espaços extras
* Deve seguir o formato:

```
A->aB
```

* O símbolo inicial deve ser `S` (o programa cria `S'` automaticamente)

---

# **💡 Exemplos úteis**

### Gramática 1 (arquivo `grammar/1.txt`):

```
S->aA
A->b
```

### Analisando:

```
abab → Aceita
aab → Rejeita
```

---

# **📌 Observações Importantes**

* Gramáticas recursivas à esquerda podem gerar conflitos LR(0).
* O programa **não detecta conflitos explicitamente** — apenas gera a tabela e executa o parser.
* Cada string aceita gera um arquivo de log com rastreamento completo.

---

# **📜 Licença**

Uso livre para estudos, trabalhos e projetos universitários.

---
