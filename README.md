
**Nume: Stan Andrei-Razvan**
**Grupă: 313CA**

## Load Balancer / tema 2

### Structuri:

* Am implementat structurile 'load_balancer' si 'server_memory'.
* Structura de 'load_balancer' contine urmatoarele:
    1. un array de servere ('server_memory **') care este hash ringul efectiv
    2. un contor care retine cate servere sunt in array, cu tot cu replici
* Structura de 'server_memory' contine urmatoarele:
    1. un hashtable
    2. id-ul sau (eticheta calculata prin formula nr_replica * 10^5 + id)
    3. id-ul server-ului original

### Implementare functii server:

* Intrucat 'server_memory' este in esenta un 'hashtable_t', functiile pentru
servere sunt aceleasi ca cele pentru un hashtable (ex. server_store -> ht_put).
Functiile de hashtable sunt cele pe care le-am implementat la laborator.
Mentionez ca functiile pentru linked list erau deja in scheletul laboratorului.
* Am considerat HMAX ca fiind 10.

### Implementare functii load_balancer:

* Am considerat hash ring-ul ca fiind un array pe care l-am sortat in
functie de hash-ul etichetei. (hash-ul pentru server_id). Array-ul este de
tipul 'server_memory' si retine toate serverele. In serverele care sunt replici
(adica cele care nu au 'server_id' == 'original_server') nu se adauga date, in
schimb se cauta server-ul original in hash_ring si se adauga datele in el.

* Implementarea se bazeaza pe consistent hashing. Astfel, functia
'loader_add_server' redistribuie datele stocate de fiecare data cand se adauga
un server nou. Distribuirea elementelor se face cautand primul server cu hash-ul
mai mare decat hash-ul obiectului care trebuie stocat, dar, pentru ca array-ul imita 
un cerc, daca nu s-a gasit un server cu hash mai mare, obiectul va fi stocat pe
serverul de pe pozitia 0. Dupa acelasi principiu functioneaza toate celelalte
functii pe care le-am implementat. De exemplu: 'loader_retrieve' cauta primul
server cu hash-ul mai mare decat hash-ul cheii si verifica daca cheia se afla
in acel server. Daca nu, inseamna ca obiectul nu exista in niciun server.

* Eliminarea unui server redistribuie obiectele care se aflau pe server-ul
sters. Acest lucru se foloseste de functia 'ht_get_all_data' pe care am implementat-o.
Functia de 'get_all_data' returneaza printr-un array de 'info_t' toate datele
care se aflau in hashtable-ul server-ului. Apoi, folosind functia 'remove_server_from_array'
se sterg server-ul si replicile sale din hash ring si se foloseste functia
'loader_store' pentru a se redistribui datele.

### Comentarii asupra temei:

* Cred ca as fi putut sa implementez tema mai bine folosind un hashtable in
load_balancer, dar am intampinat dificultati la eliberarea memoriei.
* Am invatat sa utilizez functiile de hashtable.
